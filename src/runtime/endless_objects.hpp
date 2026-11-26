#pragma once
#include "actor_pronouns054.hpp"

#include "endless_geometry.hpp"
#include "key_doors.hpp"

namespace EndlessObjects {

using EndlessGeometry::Item;
using EndlessGeometry::Kind;
using EndlessGeometry::Level;

enum class Enemy : uint8_t { None, Skull, Chuckle, Car };

struct Actor {
    Enemy kind=Enemy::None;
    Vec2 velocity{};
    unsigned owner=UINT32_MAX,points=0,bornAt=0,defeatedAt=0;
    ActorPronouns054::Tag label{};
};

struct Spawner {
    Vec2 center{},velocity{};
    Enemy enemy=Enemy::None;
    unsigned first=0,period=0,limit=0,live=0,lastSpawn=0,itemIndex=0;

    unsigned frame(unsigned tick) const {
        constexpr unsigned frames[]={1,2,3,2,1,0};
        const unsigned index=lastSpawn?(tick-lastSpawn)/4:std::size(frames);
        return index<std::size(frames)?frames[index]:0;
    }
};

inline unsigned score(unsigned par,unsigned strokes,unsigned onTime,unsigned tick,unsigned bonus) {
    const int strokeBonus=std::max(0,5000+500*(static_cast<int>(par)-static_cast<int>(strokes)));
    const int timeNumerator=60*2500+100*(static_cast<int>(onTime)-static_cast<int>(tick));
    return strokeBonus+std::max(0,timeNumerator/60)+bonus;
}

struct State {
    static constexpr unsigned EnemyCapacity=32,CarCapacity=16,SpawnerCapacity=3,MaxItems=256;
    static constexpr unsigned Absent=255;
    CircleCollider enemies[EnemyCapacity]{};
    SolidBlock cars[CarCapacity]{};
    Actor enemyActors[EnemyCapacity]{},carActors[CarCapacity]{};
    Spawner spawners[SpawnerCapacity]{};
    unsigned enemyCount=0,carCount=0,spawnerCount=0;
    KeyDoorRun keys;
    KeyDoorStage keyStage{};
    uint8_t keyIndex[MaxItems]{},doorIndex[MaxItems]{};
    unsigned coinMultiplier=0,blockMultiplier=0;
    float despawn=0;

    void restart(const Level& stage,unsigned hole,float bounds) {
        *this={};
        despawn=bounds+128;
        std::fill_n(keyIndex,MaxItems,Absent);
        std::fill_n(doorIndex,MaxItems,Absent);
        for (unsigned index=0;index<stage.count && index<MaxItems;++index) {
            const auto& item=stage.items[index];
            const Vec2 at{item.x,item.y};
            switch (item.kind) {
            case Kind::Key:
                if (keyStage.keyCount<MaxRoundKeys) {
                    keyIndex[index]=keyStage.keyCount;
                    keyStage.keys[keyStage.keyCount++]={at,item.color};
                }
                break;
            case Kind::Door:
                if (keyStage.doorCount<MaxKeyDoors) {
                    const unsigned door=keyStage.doorCount++;
                    doorIndex[index]=door;
                    keyStage.doors[door]={at,{item.a,item.b}};
                    keyStage.doorColors[door]=item.color;
                }
                break;
            case Kind::Chuckle:
                add(Enemy::Chuckle,at,{},UINT32_MAX,25,0,item.a);
                break;
            case Kind::Skull: {
                const float direction=item.arg/1000.0f;
                add(Enemy::Skull,at,{std::cos(direction)*item.b,std::sin(direction)*item.b},UINT32_MAX,25,0,item.a);
                break;
            }
            case Kind::Spawner:
                if (spawnerCount<SpawnerCapacity) {
                    // E_03 and E_07 spawn target-seeking Chuckles. E_06 has
                    // northbound cars and E_09 launches diagonal skulls.
                    const Enemy kind=hole==6?Enemy::Car:hole==9?Enemy::Skull:Enemy::Chuckle;
                    constexpr float diagonal=80.0f*0.70710678118f;
                    const Vec2 velocity=kind==Enemy::Car?Vec2{0,-120}:
                        kind==Enemy::Skull?Vec2{item.x<0?diagonal:-diagonal,-diagonal}:Vec2{};
                    spawners[spawnerCount++]={at,velocity,kind,static_cast<unsigned>(item.a),
                        static_cast<unsigned>(item.b),static_cast<unsigned>(item.arg),0,0,index};
                }
                break;
            default: break;
            }
        }
        keys.restart(keyStage);
    }

    bool pickUp(const Level& stage,unsigned item,unsigned& bonus) {
        const unsigned key=keyIndex[item];
        if (key==Absent || !keys.collect(keyStage,keyStage.keys[key].position,0,0,1u<<key)) return false;
        bonus+=stage.items[item].arg;
        return true;
    }

    bool open(const Level& stage,const Item& item,unsigned tick,unsigned& bonus) {
        const unsigned door=doorIndex[&item-stage.items];
        if (door==Absent || !keys.open(keyStage,door,tick,item.arg)) return false;
        bonus+=item.arg;
        return true;
    }

    void updateKeys(Vec2 player,unsigned tick) { keys.step(keyStage,player,tick); }

    void coin(const Item& item,unsigned& bonus) {
        bonus+=item.arg+5*coinMultiplier;
        coinMultiplier=std::min(10u,coinMultiplier+1);
    }

    void breakBlock(const Item& item,unsigned& bonus) { bonus+=item.arg*++blockMultiplier; }

    Enemy defeat(unsigned impact,unsigned tick,unsigned& bonus) {
        if (impact>=enemyCount+carCount) return Enemy::None;
        const bool car=impact>=enemyCount;
        const unsigned index=car?impact-enemyCount:impact;
        auto& actor=car?carActors[index]:enemyActors[index];
        if (car?cars[index].halfSize.x<=0:enemies[index].radius<=0) return Enemy::None;
        bonus+=actor.points;
        actor.defeatedAt=tick;
        remove(car,index);
        return actor.kind;
    }

    void move(const TileField& walls,Vec2 player,const SolidBlock* blocks,unsigned blockCount,
              BlockMask broken,unsigned tick) {
        for (unsigned i=0;i<enemyCount;++i) {
            auto& body=enemies[i];
            if (body.radius<=0) continue;
            if (outside(body.center)) { remove(false,i); continue; }
            auto& actor=enemyActors[i];
            const bool chasing=actor.kind==Enemy::Chuckle;
            if (chasing) {
                const Vec2 delta=player-body.center;
                const float distance=length(delta);
                if (distance>0) actor.velocity=actor.velocity+delta*(4.0f/distance);
                const float speed2=dot(actor.velocity,actor.velocity);
                if (speed2>200.0f*200.0f) actor.velocity=actor.velocity*(200.0f/std::sqrt(speed2));
            }
            const Vec2 motion=actor.velocity*(1.0f/60.0f);
            auto contact=walls.sweep(body.center,motion,body.radius);
            for (unsigned j=0;j<blockCount;++j)
                if (!broken.has(j)) TileField::sweepBlock(blocks[j],body.center,motion,body.radius,contact);
            for (unsigned j=0;j<enemyCount;++j)
                if (i!=j && (!chasing || enemyActors[j].kind==Enemy::Chuckle))
                    sweepCircle(enemies[j],body.center,motion,body.radius,contact);
            if (!chasing) {
                for (unsigned j=0;j<carCount;++j)
                    if (cars[j].halfSize.x>0) TileField::sweepBlock(cars[j],body.center,motion,body.radius,contact);
            }
            body.center=body.center+motion*std::min(contact.time,1.0f);
            if (contact.time<=1.0f)
                actor.velocity=actor.velocity-contact.normal*(2.0f*dot(actor.velocity,contact.normal));
        }
        for (unsigned i=0;i<carCount;++i) {
            auto& car=cars[i];
            if (car.halfSize.x<=0) continue;
            if (outside(car.center)) { remove(true,i); continue; }
            car.center=car.center+carActors[i].velocity*(1.0f/60.0f);
        }
        for (unsigned i=0;i<spawnerCount;++i) {
            auto& source=spawners[i];
            const unsigned next=source.lastSpawn?source.lastSpawn+source.period:source.first;
            if (tick>=next && source.live<source.limit && add(source.enemy,source.center,source.velocity,i,0,tick,16)) {
                source.lastSpawn=tick;
                ++source.live;
            }
        }
    }

private:
    bool outside(Vec2 p) const { return std::max(std::abs(p.x),std::abs(p.y))>despawn; }

    void remove(bool car,unsigned index) {
        if (car) cars[index].halfSize={};
        else enemies[index].radius=0;
        const unsigned owner=(car?carActors[index]:enemyActors[index]).owner;
        if (owner<spawnerCount) --spawners[owner].live;
    }

    bool add(Enemy kind,Vec2 at,Vec2 velocity,unsigned owner,unsigned points,unsigned tick,float radius) {
        const bool car=kind==Enemy::Car;
        const unsigned capacity=car?CarCapacity:EnemyCapacity;
        auto& count=car?carCount:enemyCount;
        auto* actors=car?carActors:enemyActors;
        unsigned slot=0;
        for (;slot<count;++slot) {
            const auto defeated=actors[slot].defeatedAt;
            if ((car?cars[slot].halfSize.x:enemies[slot].radius)<=0 && (!defeated || tick-defeated>=50)) break;
        }
        if (slot>=capacity) return false;
        count=std::max(count,slot+1);
        if (car) cars[slot]={at,{8,8}};
        else enemies[slot]={at,radius};
        actors[slot]={kind,velocity,owner,points,tick,0,ActorPronouns054::session.ready(ActorPronouns054::Kind::Baddie)};
        return true;
    }
};

} // namespace EndlessObjects
