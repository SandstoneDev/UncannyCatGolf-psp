#pragma once

#include "game.hpp"

struct DogSpawn {
    Vec2 center,velocity;
    float radius,graphicScale,winnerLerp,winnerShrink;
    bool redirectable;
};

struct DogTarget {
    Vec2 center,radii;

    bool touches(CircleCollider dog) const {
        const Vec2 offset=dog.center-center;
        const float distanceSquared=dot(offset,offset);
        if (radii.x==radii.y) {
            const float sum=dog.radius+radii.x;
            return distanceSquared<=sum*sum;
        }
        // Godot projects both circles onto the line between their centers.
        if (distanceSquared==0) return true;
        const float padding=0.05f*(radii.x+radii.y+2*dog.radius);
        if (std::abs(offset.x)>radii.x+dog.radius+padding ||
            std::abs(offset.y)>radii.y+dog.radius+padding) return false;
        const float distance=std::sqrt(distanceSquared);
        const Vec2 scaled{radii.x*offset.x,radii.y*offset.y};
        return distanceSquared<=length(scaled)+dog.radius*distance;
    }
};

struct DogCourseStage {
    static constexpr unsigned MaxDogs=3, MaxGoals=2;
    unsigned dogCount,goalCount;
    DogSpawn dogs[MaxDogs];
    DogTarget goals[MaxGoals];
};

struct DogObstacles {
    const SolidBlock* doors=nullptr;
    unsigned doorCount=0;
    uint32_t openedDoors=0;
    const CircleCollider* skulls=nullptr;
    unsigned skullCount=0;
    const CircleCollider* chuckles=nullptr;
    unsigned chuckleCount=0;
};

struct MultiDogUpdate {
    // Bounce/switch bits identify dogs, win bits identify dog goals.
    unsigned bounced=0,switched=0,won=0;
    uint32_t skullHits=0,chuckleHits=0;
};

class MultiDogRun {
public:
    CircleCollider bodies[DogCourseStage::MaxDogs]{};
    Vec2 velocities[DogCourseStage::MaxDogs]{};
    float graphicScales[DogCourseStage::MaxDogs]{};
    int dogGoals[DogCourseStage::MaxDogs]{};
    unsigned openMask=0;

    void restart(const DogCourseStage& stage) {
        *this={};
        std::fill(std::begin(dogGoals),std::end(dogGoals),-1);
        for (unsigned i=0;i<stage.dogCount;++i) {
            const auto& dog=stage.dogs[i];
            bodies[i]={dog.center,dog.radius};
            velocities[i]=dog.velocity;
            graphicScales[i]=dog.graphicScale;
        }
    }

    // Gate table order follows the masks 01, 10, 11.
    const TileField* activeGate(const DogCourseStage& stage,const TileField* const gates[3]) const {
        const unsigned closed=((1u<<stage.goalCount)-1u)&~openMask;
        return gates && closed?gates[closed-1]:nullptr;
    }

    MultiDogUpdate step(const DogCourseStage& stage,const TileField& walls,
                        const TileField* const gates[3],SwitchRun* switches,
                        const DogObstacles& obstacles={}) {
        MultiDogUpdate result;
        for (unsigned i=0;i<stage.dogCount;++i) {
            auto& body=bodies[i];
            auto& velocity=velocities[i];
            const auto& data=stage.dogs[i];
            if (dogGoals[i]>=0) {
                const Vec2 goal=stage.goals[dogGoals[i]].center;
                body.center=body.center+(goal-body.center)*data.winnerLerp;
                if (graphicScales[i]>0.001f) graphicScales[i]-=data.winnerShrink;
                continue;
            }

            const TileField* gate=activeGate(stage,gates);
            const TileField* active=switches?switches->activeTiles():nullptr;
            if (active!=previousSwitchTiles[i]) {
                previousSwitchTiles[i]=active;
                recovering[i]=active!=nullptr;
            }
            if (recovering[i]) recover(i,*active,walls,gate);
            const Vec2 motion=velocity*(1.0f/60.0f);
            WallContact contact=walls.sweep(body.center,motion,body.radius);
            if (gate) {
                const auto door=gate->sweep(body.center,motion,body.radius);
                if (door.time<contact.time) contact=door;
            }
            for (unsigned j=0;j<obstacles.doorCount;++j) {
                if (!(obstacles.openedDoors&(1u<<j)))
                    TileField::sweepBlock(obstacles.doors[j],body.center,motion,body.radius,contact);
            }
            const int block=switches?switches->sweep(body.center,motion,body.radius,contact):-1;
            const float solidTime=contact.time;
            int peer=-1;
            for (unsigned j=0;j<stage.dogCount;++j) {
                if (j==i) continue;
                const float previous=contact.time;
                sweepCircle(bodies[j],body.center,motion,body.radius,contact);
                if (contact.time<previous) peer=static_cast<int>(j);
            }
            const int skull=sweepBaddies(obstacles.skulls,obstacles.skullCount,result.skullHits,
                                         body,motion,contact);
            const int chuckle=sweepBaddies(obstacles.chuckles,obstacles.chuckleCount,result.chuckleHits,
                                            body,motion,contact);
            body.center=body.center+motion*std::min(contact.time,1.0f);
            if (contact.time<=1.0f) {
                velocity=velocity-contact.normal*(2.0f*dot(velocity,contact.normal));
                result.bounced|=1u<<i;
                if (chuckle>=0) result.chuckleHits|=1u<<chuckle;
                else if (skull>=0) result.skullHits|=1u<<skull;
                else if (peer>=0) struckByPlayer(stage,static_cast<unsigned>(peer),body.center);
                else if (block>=0 && contact.time==solidTime) {
                    switches->strike(static_cast<unsigned>(block));
                    result.switched|=1u<<i;
                }
            }

            for (unsigned j=0;j<stage.goalCount;++j) {
                const unsigned bit=1u<<j;
                if (openMask&bit || !stage.goals[j].touches(body)) continue;
                dogGoals[i]=static_cast<int>(j);
                body.radius=0;
                velocity={};
                openMask|=bit;
                result.won|=bit;
                break;
            }
        }
        return result;
    }

    void struckByPlayer(const DogCourseStage& stage,unsigned index,Vec2 player) {
        if (index>=stage.dogCount || !stage.dogs[index].redirectable || dogGoals[index]>=0) return;
        const Vec2 away=bodies[index].center-player;
        constexpr float QuarterPi=0.7853981633974483f, Diagonal=0.7071067811865475f;
        constexpr Vec2 directions[]{
            {1,0}, {Diagonal,Diagonal}, {0,1}, {-Diagonal,Diagonal},
            {-1,0}, {-Diagonal,-Diagonal}, {0,-1}, {Diagonal,-Diagonal},
        };
        const int turn=static_cast<int>(std::floor(std::atan2(away.y,away.x)/QuarterPi+0.5f));
        velocities[index]=directions[(turn+8)%8]*length(velocities[index]);
    }

private:
    const TileField* previousSwitchTiles[DogCourseStage::MaxDogs]{};
    bool recovering[DogCourseStage::MaxDogs]{};

    static int sweepBaddies(const CircleCollider* others,unsigned count,uint32_t removed,
                            const CircleCollider& body,Vec2 motion,WallContact& contact) {
        int hit=-1;
        for (unsigned i=0;i<count;++i) {
            if (removed&(1u<<i)) continue;
            const float previous=contact.time;
            sweepCircle(others[i],body.center,motion,body.radius,contact);
            if (contact.time<previous) hit=static_cast<int>(i);
        }
        return hit;
    }

    void recover(unsigned index,const TileField& active,const TileField& walls,const TileField* gate) {
        auto& body=bodies[index];
        constexpr float Padding=0.08f;
        const float safeRadius=body.radius+Padding;
        for (unsigned attempt=0;attempt<4;++attempt) {
            Vec2 shift=active.recoverCircle(body.center,safeRadius);
            if (dot(shift,shift)<1.0e-8f) break;
            shift=walls.recoverCircle(body.center,safeRadius,shift);
            if (gate) shift=gate->recoverCircle(body.center,safeRadius,shift);
            body.center=body.center+shift;
        }
        const Vec2 overlap=active.recoverCircle(body.center,body.radius);
        recovering[index]=dot(overlap,overlap)>1.0e-8f;
    }
};
