#pragma once

#include "game.hpp"

struct BaddieSpawner {
    Vec2 center,velocity;
    uint32_t firstTick,period,limit;
};

struct BaddieStage {
    static constexpr unsigned MaxSpawners=4;
    uint32_t skull,spawnerCount;
    float despawnDistance;
    uint32_t shade;
    BaddieSpawner spawners[MaxSpawners];
};
static_assert(sizeof(BaddieStage)==128);

struct BaddieRun {
    static constexpr unsigned Capacity=128, LegacyCapacity=90, DefaultCapacity=32, DoorBonus=1000;
    CircleCollider skulls[Capacity]{};
    SolidBlock cars[Capacity]{};
    Vec2 velocities[Capacity]{};
    uint8_t owners[Capacity]{};
    unsigned bornAt[Capacity]{},defeatedAt[Capacity]{},doorRemovedAt[BaddieStage::MaxSpawners]{},doorOpenedAt[BaddieStage::MaxSpawners]{};
    unsigned countdowns[BaddieStage::MaxSpawners]{},liveCount[BaddieStage::MaxSpawners]{},count=0,bonus=0,defeatedCount=0;
    const BaddieStage* stage=nullptr;
    unsigned activeCapacity=DefaultCapacity;

    void restart(const BaddieStage* source,unsigned ticksBeforeActivation=0,unsigned capacity=DefaultCapacity) {
        *this={};
        stage=source;
        if (!source) return;
        activeCapacity=std::min(capacity,Capacity);
        for (unsigned i=0;i<source->spawnerCount;++i) {
            const unsigned firstTick=source->spawners[i].firstTick;
            countdowns[i]=firstTick-std::min(firstTick,ticksBeforeActivation);
        }
    }

    bool alive(unsigned i) const { return skulls[i].radius>0 || cars[i].halfSize.x>0; }
    Vec2 center(unsigned i) const { return stage->skull ? skulls[i].center : cars[i].center; }

    void remove(unsigned i) {
        skulls[i].radius=0;
        cars[i].halfSize={};
        --liveCount[owners[i]];
    }

    void defeat(unsigned i,unsigned tick) {
        remove(i);
        defeatedAt[i]=tick;
        ++defeatedCount;
    }

    void step(const TileField& walls,unsigned tick,
              const CircleCollider* otherSkulls=nullptr,unsigned otherCount=0,const SolidBlock* slotDoor=nullptr,
              const SolidBlock* blocks=nullptr,unsigned blockCount=0,BlockMask broken={},
              const SwitchRun* switches=nullptr,unsigned ignoreYSpawnerMask=0) {
        if (!stage) return;
        for (unsigned i=0;i<count;++i) {
            if (!alive(i)) continue;
            const Vec2 position=center(i);
            const float distance=ignoreYSpawnerMask&(1u<<owners[i])?std::abs(position.x):
                                 std::max(std::abs(position.x),std::abs(position.y));
            if (distance>stage->despawnDistance) {
                remove(i);
                continue;
            }
            const Vec2 motion=velocities[i]*(1.0f/60.0f);
            if (!stage->skull) {
                cars[i].center=position+motion;
                continue;
            }
            auto contact=walls.sweep(position,motion,skulls[i].radius);
            for (unsigned j=0;j<count;++j)
                if (i!=j) sweepCircle(skulls[j],position,motion,skulls[i].radius,contact);
            if (switches) switches->sweep(position,motion,skulls[i].radius,contact);
            if (slotDoor) TileField::sweepBlock(*slotDoor,position,motion,skulls[i].radius,contact);
            for (unsigned j=0;j<blockCount;++j)
                if (!broken.has(j)) TileField::sweepBlock(blocks[j],position,motion,skulls[i].radius,contact);
            for (unsigned j=0;j<otherCount;++j)
                sweepCircle(otherSkulls[j],position,motion,skulls[i].radius,contact);
            skulls[i].center=position+motion*std::min(contact.time,1.0f);
            if (contact.time<=1.0f)
                velocities[i]=velocities[i]-contact.normal*(2.0f*dot(velocities[i],contact.normal));
        }
        for (unsigned i=0;i<stage->spawnerCount;++i) {
            const auto& spawner=stage->spawners[i];
            if (doorRemovedAt[i]) continue;
            if (countdowns[i]) --countdowns[i];
            if (countdowns[i] || liveCount[i]>=spawner.limit) continue;
            unsigned slot=0;
            while (slot<count && (alive(slot) || (defeatedAt[slot] && tick-defeatedAt[slot]<30))) ++slot;
            if (slot==activeCapacity) continue;
            count=std::max(count,slot+1);
            owners[slot]=i;
            bornAt[slot]=tick;
            velocities[slot]=spawner.velocity;
            defeatedAt[slot]=0;
            if (stage->skull) skulls[slot]={spawner.center,16};
            else cars[slot]={spawner.center,{8,8}};
            ++liveCount[i];
            countdowns[i]=spawner.period;
            doorOpenedAt[i]=tick;
        }
    }

    bool collect(Vec2 ball,float radius,unsigned tick) {
        if (!stage) return false;
        bool pickedUp=false;
        for (unsigned i=0;i<stage->spawnerCount;++i) {
            if (doorRemovedAt[i]) continue;
            const Vec2 delta=ball-stage->spawners[i].center;
            const Vec2 distance{std::max(0.0f,std::abs(delta.x)-16.0f),std::max(0.0f,std::abs(delta.y)-16.0f)};
            if (dot(distance,distance)>radius*radius) continue;
            doorRemovedAt[i]=tick;
            bonus+=DoorBonus;
            pickedUp=true;
        }
        return pickedUp;
    }

    unsigned doorFrame(unsigned i,unsigned tick) const {
        constexpr unsigned animation[]={1,2,3,2,1,0};
        if (!doorOpenedAt[i]) return 0;
        const unsigned frame=(tick-doorOpenedAt[i])/4;
        return frame<std::size(animation)?animation[frame]:0;
    }
};
