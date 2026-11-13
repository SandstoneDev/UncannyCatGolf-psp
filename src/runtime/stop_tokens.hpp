#pragma once

#include "game.hpp"
#include <pspkernel.h>

constexpr unsigned StopTokenStages=19, MaxStopTokens=32, MaxStopTokenGhosts=7;

struct StopTokenStage {
    uint32_t count,ghostCount;
    Vec2 positions[MaxStopTokens], ghosts[MaxStopTokenGhosts];
    uint32_t colors[MaxStopTokens];
};
static_assert(sizeof(StopTokenStage)==448);

struct StopTokenAssets {
    StopTokenStage stages[StopTokenStages]{};
    float radius=0;
    alignas(64) uint32_t sprite[32*32]{};

    bool open(const char* path) {
        auto* file=std::fopen(path,"rb");
        if (!file) return false;
        uint32_t magic=0;
        const bool complete=std::fread(&magic,sizeof magic,1,file)==1 && magic==0x54534355 &&
                            std::fread(&radius,sizeof radius,1,file)==1 &&
                            std::fread(stages,sizeof stages,1,file)==1 &&
                            std::fread(sprite,sizeof sprite,1,file)==1 &&
                            std::fgetc(file)==EOF && !std::ferror(file);
        std::fclose(file);
        if (!complete || !std::isfinite(radius) || radius<=0 || radius>32) return false;
        for (const auto& stage:stages) {
            if (stage.count>MaxStopTokens || stage.ghostCount>std::size(stage.ghosts)) return false;
            for (unsigned i=0;i<stage.count;++i)
                if (!std::isfinite(stage.positions[i].x) || !std::isfinite(stage.positions[i].y)) return false;
            for (unsigned i=0;i<stage.ghostCount;++i)
                if (!std::isfinite(stage.ghosts[i].x) || !std::isfinite(stage.ghosts[i].y)) return false;
        }
        sceKernelDcacheWritebackRange(sprite,sizeof sprite);
        return true;
    }
};

inline constexpr unsigned MaxMotionTokens=64;

struct MotionTokenStage {
    unsigned count,ghostCount;
    Vec2 positions[MaxMotionTokens],ghosts[MaxStopTokenGhosts];
    uint32_t colors[MaxMotionTokens];
};

struct MotionTokenView {
    unsigned count=0;
    const Vec2* positions=nullptr;
    const uint32_t* colors=nullptr;

    constexpr MotionTokenView()=default;
    constexpr MotionTokenView(const StopTokenStage& stage) : count(stage.count), positions(stage.positions), colors(stage.colors) {}
    constexpr MotionTokenView(const MotionTokenStage& stage) : count(stage.count), positions(stage.positions), colors(stage.colors) {}
    explicit constexpr operator bool() const { return count!=0; }
};

struct StopTokenRun {
    uint64_t collected=0;
    unsigned pickedAt[MaxMotionTokens]{};

    bool has(unsigned token) const { return collected&(uint64_t{1}<<token); }

    bool step(MotionTokenView stage,Game& game,float radius,unsigned tick,float speed=0) {
        bool pickedUp=false;
        for (unsigned i=0;i<stage.count;++i) {
            if (has(i)) continue;
            const Vec2 offset=stage.positions[i]-game.position;
            if (dot(offset,offset)>radius*radius) continue;
            collected|=uint64_t{1}<<i;
            pickedAt[i]=tick;
            const float currentSpeed=length(game.velocity);
            game.velocity=currentSpeed>0 ? game.velocity*(speed/currentSpeed) : Vec2{};
            pickedUp=true;
        }
        return pickedUp;
    }
};
