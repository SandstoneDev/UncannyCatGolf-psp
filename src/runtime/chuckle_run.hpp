#pragma once

#include "game.hpp"

#include <iterator>

enum class ChuckleCourse { None, Coaster, Mart, Finale };

struct ChuckleSpawner {
    Vec2 center;
    unsigned countdown,interval,limit;
};

struct ChuckleRun {
    static constexpr unsigned Capacity=32, SwarmCapacity=180, Bonus=25;
    static constexpr unsigned SpawnerFrameTicks=4, SpawnerOpenTicks=6*SpawnerFrameTicks;
    static constexpr float Radius=16, Acceleration=4, MaxSpeed=200, DespawnDistance=384;
    static constexpr Vec2 SpawnerCenter{272,0};

    CircleCollider bodies[SwarmCapacity]{};
    Vec2 velocities[SwarmCapacity]{};
    unsigned defeatedAt[SwarmCapacity]{};
    Vec2 defeatedPosition[SwarmCapacity]{};
    unsigned count=0,bonus=0,defeatedCount=0;

    void restart(bool enabled) { restart(enabled?ChuckleCourse::Coaster:ChuckleCourse::None); }

    void restart(ChuckleCourse next) {
        if (next==ChuckleCourse::Coaster) {
            constexpr Vec2 coaster[]={{-144,-80},{-80,-80}};
            restart(coaster,std::size(coaster));
        } else if (next==ChuckleCourse::Mart) {
            constexpr Vec2 mart[]={
                {-224,224},{-224,128},{-176,128},{-128,128},{-80,128},
                {-224,32},{-160,32},{-96,32},{-32,32},
                {-224,-64},{-144,-64},{-64,-64},{16,-64},
            };
            restart(mart,std::size(mart));
        } else {
            const bool finale=next==ChuckleCourse::Finale;
            restart(nullptr,0,finale?448.0f:DespawnDistance);
            spawnEnabled=finale;
        }
    }

    void restart(const Vec2* positions,unsigned size,float despawnDistance=DespawnDistance) {
        *this={};
        despawn=despawnDistance;
        count=positions?std::min(size,Capacity):0;
        for (unsigned i=0;i<count;++i) bodies[i]={positions[i],Radius};
    }

    void startSpawners(const ChuckleSpawner* spawners,unsigned size,float despawnDistance,bool swarm=false) {
        restart(nullptr,0,despawnDistance);
        swarmEnabled=swarm;
        sources=spawners;
        sourceCount=std::min(size,MaxSpawners);
        for (unsigned i=0;i<sourceCount;++i) nextSourceSpawn[i]=sources[i].countdown;
    }

    void clearSpawned() {
        std::fill_n(bodies,count,CircleCollider{});
        std::fill_n(defeatedAt,count,0u);
        count=0;
    }

    bool spawnerVisible() const { return spawnEnabled || sourceCount; }
    unsigned spawners() const { return sourceCount; }
    const ChuckleSpawner& spawner(unsigned index) const { return sources[index]; }
    unsigned points() const { return spawnerVisible()?0:Bonus; }
    unsigned actorTick(unsigned index,unsigned tick) const { return tick-bornAt[index]; }

    unsigned spawnerFrame(unsigned tick,unsigned source=MaxSpawners) const {
        const unsigned opened=source<MaxSpawners?sourceOpenedAt[source]:lastSpawnAt;
        if (!opened) return 0;
        constexpr unsigned frames[]={1,2,3,2,1,0};
        const unsigned frame=(tick-opened)/SpawnerFrameTicks;
        return frame<std::size(frames)?frames[frame]:0;
    }

    void step(const TileField& walls,Vec2 target,
              const SolidBlock* blocks=nullptr,unsigned blockCount=0,BlockMask broken={},
              const SolidBlock* doors=nullptr,unsigned doorCount=0,uint32_t opened=0,
              const SwitchRun* switches=nullptr,const TileField* gate=nullptr,
              const CircleCollider* skulls=nullptr,unsigned skullCount=0,
              const CircleCollider* dogs=nullptr,unsigned dogCount=0,
              WallContact (*otherWalls)(Vec2,Vec2,float)=nullptr) {
        const auto sweepSolids=[&](Vec2 center,Vec2 motion,float radius) {
            auto contact=walls.sweep(center,motion,radius);
            if (otherWalls) {
                const auto other=otherWalls(center,motion,radius);
                if (other.time<contact.time) contact=other;
            }
            for (unsigned j=0;j<blockCount;++j) {
                if (!broken.has(j)) TileField::sweepBlock(blocks[j],center,motion,radius,contact);
            }
            for (unsigned j=0;j<doorCount;++j) {
                if (!(opened&(1u<<j))) TileField::sweepBlock(doors[j],center,motion,radius,contact);
            }
            if (switches) switches->sweep(center,motion,radius,contact);
            if (gate) {
                const auto door=gate->sweep(center,motion,radius);
                if (door.time<contact.time) contact=door;
            }
            for (unsigned j=0;j<dogCount;++j) sweepCircle(dogs[j],center,motion,radius,contact);
            for (unsigned j=0;j<skullCount;++j) sweepCircle(skulls[j],center,motion,radius,contact);
            return contact;
        };
        for (unsigned i=0;i<count;++i) {
            auto& body=bodies[i];
            if (body.radius<=0) continue;
            if (std::max(std::abs(body.center.x),std::abs(body.center.y))>despawn) {
                body.radius=0;
                continue;
            }
            if (spawnEnabled || swarmEnabled) {
                const Vec2 correction=peerCorrection(i);
                if (dot(correction,correction)>0) {
                    const auto contact=sweepSolids(body.center,correction,body.radius);
                    body.center=body.center+correction*std::min(1.0f,contact.time);
                }
            }
            auto& velocity=velocities[i];
            const Vec2 offset=target-body.center;
            const float distance=length(offset);
            if (distance>0) velocity=velocity+offset*(Acceleration/distance);
            const float speed=length(velocity);
            if (speed>MaxSpeed) velocity=velocity*(MaxSpeed/speed);
            const Vec2 motion=velocity*(1.0f/60.0f);
            auto contact=sweepSolids(body.center,motion,body.radius);
            for (unsigned j=0;j<count;++j) {
                if (j==i) continue;
                if (swarmEnabled) {
                    const Vec2 gap=body.center-bodies[j].center;
                    constexpr float reach=2*Radius+MaxSpeed/60.0f;
                    if (std::abs(gap.x)>reach || std::abs(gap.y)>reach) continue;
                }
                sweepCircle(bodies[j],body.center,motion,body.radius,contact);
            }
            body.center=body.center+motion*std::min(contact.time,1.0f);
            if (contact.time<=1.0f)
                velocity=velocity-contact.normal*(2.0f*dot(velocity,contact.normal));
        }
        if (spawnerVisible()) ++spawnTick;
        if (spawnEnabled && spawnTick>=nextSpawnAt) spawn();
        for (unsigned source=0;source<sourceCount;++source) {
            if (spawnTick<nextSourceSpawn[source]) continue;
            const auto& config=sources[source];
            unsigned live=0;
            for (unsigned i=0;i<count;++i) live+=owner[i]==source && bodies[i].radius>0;
            if (live>=config.limit) continue;
            const unsigned slots=swarmEnabled?SwarmCapacity:SpawnerSlots;
            for (unsigned i=0;i<slots;++i) {
                if (bodies[i].radius>0) continue;
                bodies[i]={config.center,Radius};
                velocities[i]={};
                defeatedAt[i]=0;
                owner[i]=source;
                bornAt[i]=spawnTick;
                unsigned& openedAt=sourceOpenedAt[source];
                if (!swarmEnabled || !openedAt || spawnTick-openedAt>=SpawnerOpenTicks)
                    openedAt=spawnTick;
                count=std::max(count,i+1);
                nextSourceSpawn[source]=spawnTick+config.interval;
                break;
            }
        }
    }

    bool defeat(unsigned index,unsigned tick) {
        if (index>=count || bodies[index].radius<=0) return false;
        bodies[index].radius=0;
        velocities[index]={};
        defeatedAt[index]=tick;
        defeatedPosition[index]=bodies[index].center;
        bonus+=points();
        ++defeatedCount;
        return true;
    }

private:
    static constexpr unsigned FirstSpawnTick=10, SpawnInterval=60, SpawnCapacity=20, MaxSpawners=6, SpawnerSlots=22;
    const ChuckleSpawner* sources=nullptr;
    unsigned sourceCount=0;
    unsigned nextSourceSpawn[MaxSpawners]{},sourceOpenedAt[MaxSpawners]{};
    uint8_t owner[SwarmCapacity]{};
    bool spawnEnabled=false,swarmEnabled=false;
    float despawn=DespawnDistance;
    unsigned spawnTick=0,nextSpawnAt=FirstSpawnTick,lastSpawnAt=0;
    unsigned bornAt[SwarmCapacity]{};

    Vec2 peerCorrection(unsigned index) const {
        // The spawner may place a body on top of a surviving Chuckle.
        const auto& body=bodies[index];
        Vec2 correction{};
        for (unsigned j=0;j<count;++j) {
            if (j==index || bodies[j].radius<=0) continue;
            const float clearance=body.radius+bodies[j].radius;
            const Vec2 delta=body.center+correction-bodies[j].center;
            const float squared=dot(delta,delta);
            if (squared>=clearance*clearance) continue;
            const float distance=std::sqrt(squared);
            const Vec2 normal=distance>0.001f?delta*(1.0f/distance):Vec2{index<j?1.0f:-1.0f,0};
            correction=correction+normal*((clearance-distance)*0.4f);
        }
        return correction;
    }

    void spawn() {
        for (unsigned i=0;i<SpawnCapacity;++i) {
            if (bodies[i].radius>0) continue;
            bodies[i]={SpawnerCenter,Radius};
            velocities[i]={};
            bornAt[i]=spawnTick;
            count=std::max(count,i+1);
            lastSpawnAt=spawnTick;
            nextSpawnAt=spawnTick+SpawnInterval;
            return;
        }
    }
};
