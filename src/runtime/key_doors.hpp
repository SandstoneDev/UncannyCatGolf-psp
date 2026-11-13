#pragma once

#include "game.hpp"
#include "visual.hpp"

constexpr unsigned KeyDoorGolfStageCount=5, KeyDoorStageCount=KeyDoorGolfStageCount+9;
constexpr unsigned LegacyKeyDoorCapacity=16, MaxRoundKeys=32, MaxKeyDoors=32;
constexpr unsigned KeyDoorAtlasWidth=256, KeyDoorAtlasHeight=256, KeyFrames=72;

struct RoundKeySpawn {
    Vec2 position;
    uint32_t color;
};

template<unsigned KeyCapacity,unsigned DoorCapacity>
struct KeyDoorStageStorage {
    uint32_t keyCount,doorCount;
    RoundKeySpawn keys[KeyCapacity];
    SolidBlock doors[DoorCapacity];
    uint32_t doorColors[DoorCapacity];
    Patch doorArt[DoorCapacity];
};

using KeyDoorStage=KeyDoorStageStorage<MaxRoundKeys,MaxKeyDoors>;
using LegacyKeyDoorStage=KeyDoorStageStorage<LegacyKeyDoorCapacity,LegacyKeyDoorCapacity>;
static_assert(sizeof(LegacyKeyDoorStage)==712);

struct KeyDoorData {
    KeyDoorStage stages[KeyDoorStageCount];
};

inline KeyDoorStage expandKeyDoorStage(const LegacyKeyDoorStage& source) {
    KeyDoorStage stage{};
    stage.keyCount=source.keyCount;
    stage.doorCount=source.doorCount;
    std::copy_n(source.keys,source.keyCount,stage.keys);
    std::copy_n(source.doors,source.doorCount,stage.doors);
    std::copy_n(source.doorColors,source.doorCount,stage.doorColors);
    std::copy_n(source.doorArt,source.doorCount,stage.doorArt);
    return stage;
}

inline bool readKeyDoorData(std::FILE* file,KeyDoorData& data) {
    uint32_t magic=0;
    if (std::fread(&magic,sizeof magic,1,file)!=1 || magic!=0x444b4355) return false;
    for (auto& stage:data.stages) {
        LegacyKeyDoorStage source;
        if (std::fread(&source,sizeof source,1,file)!=1 ||
            source.keyCount>LegacyKeyDoorCapacity || source.doorCount>LegacyKeyDoorCapacity) return false;
        stage=expandKeyDoorStage(source);
    }
    return true;
}

struct KeyDoorRun {
    static constexpr unsigned DoorBonus=200;
    struct KeyMotion { Vec2 position,velocity; float acceleration=-0.2f; unsigned pickedAt=0; };
    KeyMotion motion[MaxRoundKeys]{};
    uint8_t inventory[MaxRoundKeys]{};
    unsigned held=0;
    uint32_t collected=0,available=0,opened=0;
    unsigned openedAt[MaxKeyDoors]{},bonus=0;
    float lockAlpha[MaxKeyDoors]{};

    void restart(const KeyDoorStage& stage) {
        *this={};
        for (unsigned i=0;i<stage.keyCount;++i) motion[i].position=stage.keys[i].position;
        for (unsigned i=0;i<stage.doorCount;++i) lockAlpha[i]=1.0f;
    }

    bool collect(const KeyDoorStage& stage,Vec2 player,float playerRadius,unsigned tick,uint32_t mask=UINT32_MAX) {
        bool pickedUp=false;
        const float range=16.0f+playerRadius;
        for (unsigned i=0;i<stage.keyCount;++i) {
            const uint32_t bit=1u<<i;
            if (!(mask&bit) || (collected&bit)) continue;
            const Vec2 offset=stage.keys[i].position-player;
            if (dot(offset,offset)>range*range) continue;
            collected|=bit;
            available|=bit;
            inventory[held++]=i;
            motion[i].pickedAt=tick;
            pickedUp=true;
        }
        return pickedUp;
    }

    bool open(const KeyDoorStage& stage,unsigned door,unsigned tick,unsigned score=DoorBonus) {
        if (opened&(1u<<door)) return false;
        for (unsigned i=0;i<held;++i) {
            const unsigned key=inventory[i];
            if (stage.keys[key].color!=stage.doorColors[door]) continue;
            std::move(inventory+i+1,inventory+held,inventory+i);
            --held;
            available&=~(1u<<key);
            opened|=1u<<door;
            openedAt[door]=tick;
            bonus+=score;
            return true;
        }
        return false;
    }

    void step(const KeyDoorStage& stage,Vec2 player,unsigned tick) {
        if (held) {
            const float angle=tick*(2.5f/60.0f);
            const float spacing=6.283185307f/held;
            for (unsigned i=0;i<held;++i) {
                auto& key=motion[inventory[i]];
                const float direction=angle+spacing*i;
                const Vec2 target=player+Vec2{48.0f*std::cos(direction),48.0f*std::sin(direction)};
                key.velocity=(key.velocity+(target-key.position)*key.acceleration)*0.35f;
                key.acceleration=std::min(5.0f,key.acceleration+0.01f);
                key.position=key.position+key.velocity*0.75f;
            }
        }
        for (unsigned i=0;i<stage.doorCount;++i) {
            bool matching=false;
            for (unsigned j=0;j<held;++j) matching|=stage.keys[inventory[j]].color==stage.doorColors[i];
            lockAlpha[i]+=((matching?0.0f:1.0f)-lockAlpha[i])*0.1f;
        }
    }
};
