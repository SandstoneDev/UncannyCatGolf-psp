#pragma once

#include "bunker_jumpy.hpp"
#include "world_transition.hpp"
#include "endless_run.hpp"

namespace EndlessModifiers {

enum Event : unsigned { MissileWarning=1, MissileBlast=2, MissileHit=4 };

constexpr unsigned blacklist(unsigned hole) {
    using namespace EndlessRules;
    switch (hole) {
    case GlowstickFirstCourse+3: return Jumpy;                           // 2_04
    case GlowstickFirstCourse+8:                                         // 2_09
    case ChuckleParkFirstCourse+7: case ChuckleParkFirstCourse+12:         // 3_08, 3_13
    case FinalFrontierFirstCourse+8: case FinalFrontierFirstCourse+9:     // 4_09, 4_10
    case FinalFrontierFirstCourse+12: return Panic;                       // 4_13
    case GlowstickFirstCourse+10: case GlowstickFirstCourse+12:           // 2_11, 2_13
    case ChuckleParkFirstCourse+10: case First+8: return Pharaoh;         // 3_11, E_08
    case GlowstickFirstCourse+14: return Jumpy|Pharaoh;                   // 2_15
    case GlowstickFirstCourse+15: case First+3: return Panic|Pharaoh;      // 2_16, E_03
    case FinalFrontierFirstCourse+11: return Jumpy|Panic|Pharaoh;          // 4_12
    default: return 0;
    }
}

inline bool jumpInFlight(const BunkerJumpy& jump) { return jump.flightTicks || jump.landing; }

inline float jumpScale(const BunkerJumpy& jump) {
    if (!jumpInFlight(jump)) return 1.0f;
    static constexpr auto scales=[] {
        std::array<float,BunkerJumpy::FlightTicks+1> values{};
        constexpr float base=JumpRun::BaseScale;
        float scale=base;
        values[0]=1.0f;
        for (unsigned age=1;age<values.size();++age) {
            scale+=(0.15f-scale)*(age<=BunkerJumpy::FlightTicks/2?0.13f:-0.13f);
            values[age]=std::max(base,scale)/base;
        }
        return values;
    }();
    return scales[BunkerJumpy::FlightTicks-jump.flightTicks];
}

struct Missile {
    static constexpr unsigned Interval=240,TrackAt=180,LockAt=60,HitboxTicks=5,BlastTicks=44;
    static constexpr float Radius=40;
    Vec2 position{};
    unsigned timer=Interval,hitbox=0,blast=0,animation=0;

    unsigned step(Vec2 player,float playerRadius,bool grounded) {
        unsigned events=0;
        if (blast && ++blast>BlastTicks) blast=0;
        if (timer>TrackAt) {
            --timer;
        } else if (timer>LockAt) {
            position=position+(player-position)*.2f;
            ++animation;
            --timer;
        } else if (timer) {
            if (timer==LockAt) events=MissileWarning;
            animation+=2;
            --timer;
        } else {
            timer=Interval;
            hitbox=HitboxTicks;
            blast=1;
            events=MissileBlast;
        }
        if (hitbox) {
            --hitbox;
            const Vec2 offset=player-position;
            const float radius=Radius+playerRadius;
            if (grounded && dot(offset,offset)<=radius*radius) {
                hitbox=0;
                events|=MissileHit;
            }
        }
        return events;
    }
};

struct Bird {
    Vec2 position{},velocity{};
    unsigned wingTime=0,wingTick=0,frame=0,hue=0,blast=0;
    bool live=false;

    void step() {
        if (!live) {
            if (blast && ++blast>34) blast=0;
            return;
        }
        if (wingTick>wingTime) {
            frame^=1;
            wingTick=0;
        } else ++wingTick;

        // The HUD cage's solid edges are at 256, 768, 0 and 512. The
        // CharacterBody collision box is about 16 x 32 source pixels.
        constexpr float Left=264,Right=760,Top=15.99f,Bottom=496.01f;
        const Vec2 motion=velocity*(1.0f/60);
        const Vec2 next=position+motion;
        const float xTime=motion.x<0 && next.x<Left?(Left-position.x)/motion.x:
                          motion.x>0 && next.x>Right?(Right-position.x)/motion.x:2.0f;
        const float yTime=motion.y<0 && next.y<Top?(Top-position.y)/motion.y:
                          motion.y>0 && next.y>Bottom?(Bottom-position.y)/motion.y:2.0f;
        position=position+motion*std::clamp(std::min(xTime,yTime),0.0f,1.0f);
        position.x=std::clamp(position.x,Left,Right);
        position.y=std::clamp(position.y,Top,Bottom);
        if (xTime<=1.0f && xTime<=yTime) velocity.x=-velocity.x;
        if (yTime<=1.0f && yTime<=xTime) velocity.y=-velocity.y;
    }

    bool bash(Vec2 cursor) {
        const Vec2 offset=position-cursor;
        if (!live || std::abs(offset.x)>20 || std::abs(offset.y)>32) return false;
        live=false;
        blast=1;
        return true;
    }
};

struct BeeMotion {
    Vec2 position{},velocity{};
    float acceleration=0,limit=0,animationSpeed=0;

    void step(Vec2 cursor) {
        const Vec2 offset=cursor-position;
        const float distanceSquared=dot(offset,offset);
        if (distanceSquared>1.0e-6f) velocity=velocity+offset*(acceleration/std::sqrt(distanceSquared));
        const float speedSquared=dot(velocity,velocity);
        if (speedSquared>limit*limit) velocity=velocity*(limit/std::sqrt(speedSquared));
        position=position+velocity*(1.0f/60);
    }
};

struct Afterimage {
    Vec2 position{};
    unsigned created=0,duration=0,alpha=0;

    unsigned opacity(unsigned now) const {
        const unsigned age=now-created;
        return age<duration?alpha*(duration-age)/duration:0;
    }
};

struct State {
    static constexpr unsigned BirdCount=25,BeeCount=20,GhostCount=16,AfterimageCount=4;
    static constexpr float ViewWidth=768,ViewHeight=512,SandDamping=0.85f;

    unsigned mods=0,ticks=0;
    uint32_t random=1;
    Vec2 cursor{512,256};
    BunkerJumpy uniqueJumpy;
    Missile missile;
    std::array<Bird,BirdCount> birds{};
    std::array<BeeMotion,BeeCount> bees{};
    std::array<std::array<Afterimage,AfterimageCount>,GhostCount> trails{};

    bool enabled(unsigned bit) const { return (mods&bit)!=0; }

    void reset(unsigned mask,uint32_t seed,Vec2 spawn) {
        *this={};
        mods=mask;
        random=seed?seed:1;
        uniqueJumpy.enabled=enabled(EndlessRules::Jumpy);
        missile.position=spawn;
        if (enabled(EndlessRules::Bort)) {
            for (auto& bird:birds) {
                constexpr float HalfPi=1.5707963268f;
                const float angle=range(.261f,1.309f)+draw(4)*HalfPi;
                const float speed=100+draw(601);
                bird.velocity={std::cos(angle)*speed,std::sin(angle)*speed};
                bird.position={static_cast<float>(320+draw(385)),static_cast<float>(64+draw(385))};
                bird.wingTime=3+draw(8);
                bird.hue=draw(8);
                bird.live=true;
            }
        }
        if (enabled(EndlessRules::Bees))
            for (auto& bee:bees) {
                bee.position={range(0,ViewWidth),range(0,ViewHeight)};
                bee.acceleration=range(20,50);
                bee.limit=range(100,1000);
                bee.animationSpeed=range(.5f,2.5f);
            }
    }

    float damping(bool authoredSand) const {
        return enabled(EndlessRules::Pharaoh) && !authoredSand?SandDamping:1.0f;
    }

    void moveCursor(Vec2 stick,uint64_t microseconds) {
        if (!(mods&(EndlessRules::Bort|EndlessRules::Bees))) return;
        constexpr float Deadzone=.18f;
        const float magnitude=length(stick);
        if (magnitude<=Deadzone) return;
        const float strength=(std::min(magnitude,1.0f)-Deadzone)/(1.0f-Deadzone);
        const float distance=strength*(440.0f/1000000.0f)*microseconds;
        cursor=cursor+stick*(distance/magnitude);
        cursor.x=std::clamp(cursor.x,0.0f,ViewWidth);
        cursor.y=std::clamp(cursor.y,0.0f,ViewHeight);
    }

    unsigned bash() {
        if (!enabled(EndlessRules::Bort)) return 0;
        unsigned count=0;
        for (auto& bird:birds) count+=bird.bash(cursor);
        return count;
    }

    unsigned tick(Vec2 player,float radius,bool grounded) {
        ++ticks;
        if (enabled(EndlessRules::Bort)) for (auto& bird:birds) bird.step();
        if (enabled(EndlessRules::Bees)) for (auto& bee:bees) bee.step(cursor);
        return enabled(EndlessRules::Missiles)?missile.step(player,radius,grounded):0;
    }

    void recordGhost(unsigned index,Vec2 position,Vec2 velocity) {
        if (!enabled(EndlessRules::Panic) || index>=trails.size() || ticks%3) return;
        const float speedSquared=dot(velocity,velocity);
        if (speedSquared<400) return;
        const float weight=std::clamp((std::sqrt(speedSquared)-20)/130,0.0f,1.0f);
        trails[index][ticks/3%AfterimageCount]={position,ticks,
            static_cast<unsigned>(12*weight+.5f),static_cast<unsigned>(191*weight+.5f)};
    }

private:
    unsigned draw(unsigned limit) {
        random^=random<<13;
        random^=random>>17;
        random^=random<<5;
        return random%limit;
    }

    float range(float min,float max) {
        return min+(max-min)*(draw(65536)*(1.0f/65535));
    }
};

} // namespace EndlessModifiers
