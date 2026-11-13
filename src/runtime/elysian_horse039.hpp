#pragma once

#include "game.hpp"

enum class HorsePhase039 { Chase, Flight, Finished, Lost, Defeat };
enum class HorseGoalPhase039 { Hidden, Ready, Homing, Gone };

struct HorseGoalSpec039 {
    Vec2 position{};
    int next=-1;
    bool active=false;
};

struct HorseCourse039 {
    float bounds;
    Vec2 position,direction;
    unsigned health;
    float baseSpeed=150;
    unsigned attackTime=300;
    static constexpr unsigned GoalCapacity=24;
    static_assert(GoalCapacity<=32);
    std::array<HorseGoalSpec039,GoalCapacity> goals{};
    unsigned targetDelay=20;
};

struct HorseGoal039 {
    Vec2 position{},velocity{};
    HorseGoalPhase039 phase=HorseGoalPhase039::Hidden;
    unsigned launchedAt=0;
    float widthScale=0.376471f,heightScale=0.367816f,rotation=0;

    bool active() const {
        return phase==HorseGoalPhase039::Ready || phase==HorseGoalPhase039::Homing;
    }
};

struct HorseEvents039 {
    unsigned launched=0,hit=0;
    bool chargeUp=false,charge=false,bounce=false,caught=false,burst=false;
};

struct HorseTrail039 {
    Vec2 position{},scale{};
    unsigned tick=0;
};

struct HorseExplosion039 {
    Vec2 position{};
    unsigned tick=0;
    bool quiet=false;
};

struct HorseRun039 {
    static constexpr unsigned ChargeTime=60;
    static constexpr float GoalSpeed=800,GoalAcceleration=10;
    static constexpr float WallRadius=64,GoalBodyRadius=10,GoalAreaRadius=16,CatchHalfSize=37;

    std::array<HorseGoal039,HorseCourse039::GoalCapacity> goals{};
    const HorseCourse039* config=nullptr;
    std::array<HorseTrail039,20> trails{};
    std::array<HorseExplosion039,8> explosions{};
    unsigned trailIndex=0,explosionIndex=0,age=0;
    Vec2 position{},velocity{},direction{},scale{0.25f,0.25f};
    unsigned health=0,maximumHealth=0,attackTimer=0,lastHitAt=0,healthBeforeHit=0;
    bool targeted=false;
    float bounds=0,zoomPulse=1,shake=0,saturation=0;
    unsigned randomState=0x87a21d3u;
    HorsePhase039 phase=HorsePhase039::Chase;

    bool flying() const { return phase==HorsePhase039::Flight; }

    static Vec2 normalized(Vec2 vector) {
        const float magnitude=length(vector);
        return magnitude>0 ? vector*(1.0f/magnitude) : Vec2{};
    }

    void restart(const HorseCourse039& course) {
        *this={};
        config=&course;
        bounds=course.bounds;
        position=course.position;
        direction=course.direction;
        attackTimer=course.attackTime;
        for (unsigned index=0;index<goals.size();++index) {
            const auto& goal=course.goals[index];
            goals[index].position=goal.position;
            if (goal.active) goals[index].phase=HorseGoalPhase039::Ready;
        }
        healthBeforeHit=health=maximumHealth=course.health;
        velocity=normalized(direction)*course.baseSpeed;
    }

    void lose() {
        phase=HorsePhase039::Lost;
        velocity={};
        for (auto& goal:goals) goal.phase=HorseGoalPhase039::Gone;
    }

    HorseEvents039 step(Game& player,float radius,const TileField& horseWalls,bool lift,bool playerGrounded=true) {
        HorseEvents039 result;
        if (!config || phase==HorsePhase039::Lost || phase==HorsePhase039::Finished || phase==HorsePhase039::Defeat) return result;
        ++age;
        zoomPulse+=(1-zoomPulse)*0.2f;
        if (age%((attackTimer<ChargeTime && phase==HorsePhase039::Chase)?1u:8u)==0) {
            trails[trailIndex]={position,scale,age};
            trailIndex=(trailIndex+1)%trails.size();
        }
        shake*=0.9f;
        if (flying()) {
            if (age%8==0) {
                shake+=250;
                addExplosion(position+Vec2{randomOffset(),randomOffset()},true);
                result.burst=true;
            }
            if (lift) velocity.y=-600;
            velocity.y-=2;
            if (position.y < -bounds-128) {
                phase=HorsePhase039::Finished;
                player.won=true;
            } else {
                position=position+velocity*(1.0f/60.0f);
            }
            return result;
        }

        for (auto& goal:goals) {
            if (goal.phase!=HorseGoalPhase039::Homing) continue;
            const WallContact contact=move(goal.position,goal.velocity,GoalBodyRadius,horseWalls);
            if (dot(goal.velocity,goal.velocity)>0) goal.rotation=std::atan2(goal.velocity.y,goal.velocity.x);
            goal.widthScale+=0.002f;
            goal.heightScale-=0.001f;
            goal.velocity=goal.velocity+normalized(position-goal.position)*GoalAcceleration;
            const float speed=length(goal.velocity);
            goal.velocity=goal.velocity*(0.99f*(speed>GoalSpeed?GoalSpeed/speed:1.0f));
            if (contact.time<=1) {
                goal.widthScale=0.376f;
                goal.heightScale=0.368f;
                reflect(goal.velocity,contact.normal);
            }
        }

        if (attackTimer<=config->targetDelay && !targeted) {
            direction=player.position-position;
            targeted=true;
        }
        if (attackTimer) {
            --attackTimer;
            if (attackTimer<ChargeTime) {
                velocity=velocity*0.97f;
                scale.x+=(0.3f-scale.x)*0.03f;
                scale.y+=(0.2f-scale.y)*0.03f;
                saturation=std::min(1.0f,saturation+0.005f);
                shake+=20;
            } else {
                const float speed=length(velocity);
                if (speed>config->baseSpeed)
                    velocity=velocity*((speed+(config->baseSpeed-speed)*0.03f)/speed);
                result.chargeUp=attackTimer==ChargeTime;
            }
        } else {
            targeted=false;
            velocity=normalized(direction)*(config->baseSpeed*3);
            attackTimer=config->attackTime;
            scale={0.25f,0.25f};
            saturation=0;
            result.charge=true;
        }
        const WallContact wall=move(position,velocity,WallRadius,horseWalls);
        if (wall.time<=1) {
            reflect(velocity,wall.normal);
            result.bounce=true;
        }

        for (unsigned id=0;id<goals.size();++id) {
            auto& goal=goals[id];
            if (playerGrounded && goal.phase==HorseGoalPhase039::Ready &&
                overlapsCircle(player.position,goal.position,radius)) {
                goal.phase=HorseGoalPhase039::Homing;
                goal.launchedAt=age;
                result.launched|=1u<<id;
            }
            if (goal.phase!=HorseGoalPhase039::Homing ||
                !overlapsCircle(position,goal.position,WallRadius)) continue;
            goal.phase=HorseGoalPhase039::Gone;
            const int next=config->goals[id].next;
            if (next>=0) goals[static_cast<unsigned>(next)].phase=HorseGoalPhase039::Ready;
            result.hit|=1u<<id;
            if (damage(player)) return result;
        }
        if (playerGrounded && touchesPlayer(player.position,radius)) {
            lose();
            player.velocity={};
            result.caught=true;
        }
        return result;
    }

    bool touchesPlayer(Vec2 point,float radius) const {
        const Vec2 offset=point-position;
        const Vec2 closest{std::max(std::abs(offset.x)-CatchHalfSize,0.0f),
                           std::max(std::abs(offset.y)-CatchHalfSize,0.0f)};
        return dot(closest,closest)<=radius*radius;
    }

    bool damage(Game& player,bool finalDeath=false) {
        addExplosion(position,false);
        lastHitAt=age;
        healthBeforeHit=health;
        if (--health) {
            shake+=450;
            zoomPulse=1.2f;
            return false;
        }
        for (auto& target:goals) target.phase=HorseGoalPhase039::Gone;
        player.velocity={};
        velocity={};
        phase=finalDeath?HorsePhase039::Defeat:HorsePhase039::Flight;
        scale={0.25f,0.25f};
        saturation=0;
        shake+=1000;
        zoomPulse=2;
        return true;
    }

    void defeatBurst() {
        shake+=250;
        addExplosion(position+Vec2{randomOffset(),randomOffset()},true);
    }

private:
    static bool overlapsCircle(Vec2 a,Vec2 b,float radius) {
        const Vec2 delta=a-b;
        const float reach=GoalAreaRadius+radius;
        return dot(delta,delta)<=reach*reach;
    }

    static WallContact move(Vec2& center,Vec2 velocity,float radius,const TileField& walls) {
        const Vec2 motion=velocity*(1.0f/60.0f);
        const auto contact=walls.active()?walls.sweep(center,motion,radius):WallContact{};
        center=center+motion*std::min(1.0f,contact.time);
        return contact;
    }

    static void reflect(Vec2& velocity,Vec2 normal) {
        velocity=velocity-normal*(2*dot(velocity,normal));
    }

    float randomOffset() {
        randomState=randomState*1664525u+1013904223u;
        return static_cast<float>(randomState>>16)*(188.0f/65535.0f)-94;
    }

    void addExplosion(Vec2 center,bool quiet) {
        explosions[explosionIndex]={center,age,quiet};
        explosionIndex=(explosionIndex+1)%explosions.size();
    }
};
