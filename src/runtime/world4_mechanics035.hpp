#pragma once

#include "baddie_run.hpp"
#include "chuckle_run.hpp"
#include "golf.hpp"
#include "jump_pad.hpp"
#include "key_doors.hpp"
#include "nuke.hpp"
#include "stop_tokens.hpp"

inline constexpr SolidBlock FrontierSwitchBlocks408[]{
    {{464,272},{16,16}}, {{464,304},{16,16}}, {{464,240},{16,16}},
    {{464,208},{16,16}}, {{-80,48},{16,16}}, {{-80,80},{16,16}},
};

inline constexpr StopTokenStage FrontierStop408{
    4,0,{{-192,-48},{-232,-240},{96,-96},{96,208}}, {},
    {0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu},
};

inline constexpr StopTokenStage FrontierStop409{
    9,0,{{-288,-144},{192,-192},{-128,-352},{0,64},{320,0},
         {224,224},{32,-96},{-288,32},{320,224}}, {},
    {0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,
     0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu},
};

inline constexpr Vec2 FrontierNukes408[]{{-384,16},{-208,-176},{32,-32},{416,416},{416,32}};
inline constexpr Vec2 FrontierNukes409[]{{-288,224}};
static_assert(std::size(FrontierNukes408)<=NukeRun::Capacity);

inline constexpr ChuckleSpawner FrontierChuckleSpawners408[]{
    {{-384,-416},10,20,30}, {{-384,432},10,10,30}, {{-288,384},10,10,30},
    {{-192,384},10,10,30}, {{0,-384},10,10,30}, {{112,-400},10,10,30},
};
inline constexpr float FrontierChuckleDespawn408=640.0f;

inline constexpr BaddieStage FrontierCars409{
    0,1,512.0f,0xff808080u,{{{-448,288},{200,0},1,10,100}},
};

inline constexpr JumpPad FrontierJump409[]{{{256,-336},2.79253f,0.0f}};
inline constexpr uint32_t FrontierKeyColor409=0xff8080ffu;
inline constexpr KeyDoorStage FrontierKeys409{
    1,1,{{{96,-352},FrontierKeyColor409}},{{{-224,224},{32,32}}},{FrontierKeyColor409},{},
};

inline constexpr Vec2 FrontierPrimaryGhost409{96,-32};
inline constexpr Vec2 FrontierExtraGhosts409[]{
    {-224,-352},{-32,-224},{-320,256},{320,328},{288,160},{-480,96},{-480,-280},
    {448,192},{448,-64},{-224,0},{-640,288},{-736,-544},{672,-224},{832,128},
    {-96,-800},{448,-736},{-864,-32},{64,96},{-96,352},{-368,-368},{-320,640},
    {64,768},{448,960},{0,1408},{1728,0},{0,-1728},{-1856,0},
};

inline void stepFrontierCars409(BaddieRun& run,const TileField& walls,unsigned tick) {
    const unsigned openedAt=run.doorOpenedAt[0];
    run.step(walls,tick);
    // Playing Open again preserves its current frame until the clip finishes.
    if (openedAt && tick-openedAt<ChuckleRun::SpawnerOpenTicks) run.doorOpenedAt[0]=openedAt;
}

inline void clearFrontierCars409(BaddieRun& run) {
    for (unsigned i=0;i<run.count;++i) if (run.alive(i)) run.remove(i);
    run.count=0;
}

inline bool stepFrontierGhosts409(GhostMotion& primary,GhostMotion* extra,unsigned extraCount,Vec2 target,
                                  const CircleCollider* additionalBody=nullptr,uint32_t active=UINT32_MAX) {
    constexpr float BodyRadius=16.2788f, CatchRadius=27.9655f;
    constexpr unsigned MaxGhosts=1+std::size(FrontierExtraGhosts409);
    static_assert(MaxGhosts<=32);
    const unsigned count=1+std::min(extraCount,MaxGhosts-1);
    CircleCollider bodies[MaxGhosts];
    const auto ghostAt=[&](unsigned i) -> GhostMotion& { return i?extra[i-1]:primary; };
    for (unsigned i=0;i<count;++i) bodies[i]={ghostAt(i).position,BodyRadius};

    bool caught=false;
    for (unsigned i=0;i<count;++i) {
        if (!(active&(1u<<i))) continue;
        auto& ghost=ghostAt(i);
        ghost.chase(target);
        Vec2 position=bodies[i].center;
        Vec2 motion=ghost.position-position;
        // Ghosts occupy physics layer 4 and slide along the other ghost bodies.
        for (unsigned slide=0;slide<4 && dot(motion,motion)>1.0e-8f;++slide) {
            WallContact contact;
            for (unsigned j=0;j<count;++j) {
                if (i==j || !(active&(1u<<j))) continue;
                const Vec2 gap=position-bodies[j].center;
                const float range=2*BodyRadius+1.0f;
                if (std::abs(gap.x)>range || std::abs(gap.y)>range) continue;
                sweepCircle(bodies[j],position,motion,BodyRadius,contact);
            }
            if (additionalBody) sweepCircle(*additionalBody,position,motion,BodyRadius,contact);
            position=position+motion*std::min(contact.time,1.0f);
            if (contact.time>1.0f) break;
            ghost.velocity=ghost.velocity-contact.normal*dot(ghost.velocity,contact.normal);
            motion=(motion-contact.normal*dot(motion,contact.normal))*(1.0f-contact.time);
        }
        bodies[i].center=ghost.position=position;
        const Vec2 gap=target-position;
        caught|=dot(gap,gap)<=CatchRadius*CatchRadius;
    }
    return caught;
}
