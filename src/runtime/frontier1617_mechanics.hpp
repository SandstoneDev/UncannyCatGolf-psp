#pragma once

#include "world4_mechanics035.hpp"
#include "portal.hpp"

inline constexpr BaddieStage FrontierCars416{
    0,2,448.0f,0xffffffffu,
    {{{-384,160},{150,0},80,120,100},{{-224,-384},{0,150},20,120,100}},
};
inline constexpr ChuckleSpawner FrontierChuckleSpawners416[]{{{272,-192},10,120,18}};
inline constexpr Vec2 FrontierNukes416[]{{-224,256},{160,-160},{80,288}};
inline constexpr Vec2 FrontierSkullSpawner416{48,-32};
inline constexpr float FrontierDespawn416=448.0f;

struct FrontierSkullRun416 {
    static constexpr unsigned Capacity=18, FirstTick=10, Period=120;
    static constexpr float Speed=125.0f, Radius=16.0f;
    CircleCollider bodies[Capacity]{};
    Vec2 velocities[Capacity]{};
    unsigned defeatedAt[Capacity]{},count=0,openedAt=0,defeatedCount=0;

    void restart() { *this={}; }

    void clearSpawned() {
        std::fill_n(bodies,count,CircleCollider{});
        std::fill_n(defeatedAt,count,0u);
        count=0;
    }

    bool defeat(unsigned index,unsigned tick) {
        if (index>=count || bodies[index].radius<=0) return false;
        bodies[index].radius=0;
        defeatedAt[index]=tick;
        ++defeatedCount;
        return true;
    }

    unsigned doorFrame(unsigned tick) const {
        constexpr unsigned frames[]{1,2,3,2,1,0};
        if (!openedAt) return 0;
        const unsigned frame=(tick-openedAt)/ChuckleRun::SpawnerFrameTicks;
        return frame<std::size(frames)?frames[frame]:0;
    }

    void step(const TileField& walls,unsigned tick,const SolidBlock* blocks,unsigned blockCount,
              BlockMask broken,const SolidBlock* cars,unsigned carCount,
              const CircleCollider* chuckles,unsigned chuckleCount) {
        for (unsigned i=0;i<count;++i) {
            auto& body=bodies[i];
            if (body.radius<=0) continue;
            if (std::max(std::abs(body.center.x),std::abs(body.center.y))>FrontierDespawn416) {
                body.radius=0;
                continue;
            }
            const Vec2 motion=velocities[i]*(1.0f/60.0f);
            auto contact=walls.sweep(body.center,motion,body.radius);
            for (unsigned j=0;j<count;++j)
                if (j!=i) sweepCircle(bodies[j],body.center,motion,body.radius,contact);
            for (unsigned j=0;j<chuckleCount;++j)
                sweepCircle(chuckles[j],body.center,motion,body.radius,contact);
            for (unsigned j=0;j<blockCount;++j)
                if (!broken.has(j)) TileField::sweepBlock(blocks[j],body.center,motion,body.radius,contact);
            for (unsigned j=0;j<carCount;++j)
                if (cars[j].halfSize.x>0) TileField::sweepBlock(cars[j],body.center,motion,body.radius,contact);
            body.center=body.center+motion*std::min(contact.time,1.0f);
            if (contact.time<=1.0f)
                velocities[i]=velocities[i]-contact.normal*(2.0f*dot(velocities[i],contact.normal));
        }
        if (tick<nextSpawn) return;
        for (unsigned i=0;i<Capacity;++i) {
            if (bodies[i].radius>0) continue;
            constexpr float diagonal=-Speed*0.7071067812f;
            bodies[i]={FrontierSkullSpawner416,Radius};
            velocities[i]={diagonal,diagonal};
            defeatedAt[i]=0;
            count=std::max(count,i+1);
            nextSpawn=tick+Period;
            openedAt=tick;
            break;
        }
    }

private:
    unsigned nextSpawn=FirstTick;
};

inline constexpr SolidBlock FrontierSwitchBlocks417[]{
    {{336,-240},{16,16}},{{336,-208},{16,16}},{{336,-272},{16,16}},{{336,-176},{16,16}},
    {{336,208},{16,16}},{{336,240},{16,16}},{{336,272},{16,16}},{{336,176},{16,16}},
};
inline constexpr PortalNode FrontierPortals417[]{{{-408,224},1},{{432,208},PortalNode::NoTarget}};
inline constexpr StopTokenStage FrontierStop417{1,0,{{64,64}},{},{0xffffffffu}};
inline constexpr Vec2 FrontierExtraGhosts417[]{{64,-224},{-160,-288}};
inline constexpr Vec2 FrontierFakeGoalCenter417{-352,416};
inline constexpr Vec2 FrontierRealGoalCenter417{-528,-448};
inline constexpr Vec2 FrontierRealGoalRadii417{16,64};
inline constexpr Vec2 FrontierFinalGhostStart417{-616,424};
inline constexpr Vec2 FrontierKeyHint417{448,-480};
inline constexpr SolidBlock FrontierGlassZone417{{316,-224},{20,40}};
inline constexpr SolidBlock FrontierUfoTrigger417{{472,-504},{8,8}};
inline constexpr float FrontierFinalGhostBodyRadius417=88.0f;
inline constexpr float FrontierFinalGhostHitRadius417=72.0f;

enum FrontierFinaleEvent417 : unsigned {
    FrontierFakeGoal417=1u<<0,
    FrontierUfoLaunched417=1u<<1,
    FrontierKeyGranted417=1u<<2,
    FrontierRealGoal417=1u<<3,
};

inline bool touchesFrontierBox417(Vec2 point,float radius,const SolidBlock& block) {
    const Vec2 delta=point-block.center;
    const Vec2 distance{std::max(std::abs(delta.x)-block.halfSize.x,0.0f),
                        std::max(std::abs(delta.y)-block.halfSize.y,0.0f)};
    return dot(distance,distance)<=radius*radius;
}

inline bool touchesFrontierRealGoal417(Vec2 point,float radius) {
    const Vec2 delta=point-FrontierRealGoalCenter417;
    const Vec2 offset{std::abs(delta.x),std::abs(delta.y)};
    constexpr auto axes=FrontierRealGoalRadii417;
    if (offset.x>axes.x+radius || offset.y>axes.y+radius) return false;
    const Vec2 normalized{offset.x/axes.x,offset.y/axes.y};
    if (dot(normalized,normalized)<=1.0f) return true;

    // The closest point of an ellipse to an exterior point has one nonnegative Lagrange multiplier.
    constexpr Vec2 axesSquared{axes.x*axes.x,axes.y*axes.y};
    const auto pointAt=[&](float multiplier) {
        return Vec2{axesSquared.x*offset.x/(axesSquared.x+multiplier),
                    axesSquared.y*offset.y/(axesSquared.y+multiplier)};
    };
    float lo=0.0f,hi=axes.y*length(offset);
    for (unsigned i=0;i<16;++i) {
        const float mid=(lo+hi)*0.5f;
        const Vec2 closest=pointAt(mid);
        const Vec2 test{closest.x/axes.x,closest.y/axes.y};
        if (dot(test,test)>1.0f) lo=mid;
        else hi=mid;
    }
    const Vec2 remaining=offset-pointAt(hi);
    return dot(remaining,remaining)<=radius*radius;
}

struct FrontierFinaleRun417 {
    static constexpr unsigned FakeGoalBonus=500, UfoTravelTicks=120;
    GhostMotion finalGhost{FrontierFinalGhostStart417,{}};
    unsigned revealedAt=0,ufoAt=0,keyAt=0;
    bool finalGhostActive=false,glassInside=false,glassConsumed=false,keyAvailable=false;

    void restart(bool available) {
        *this={};
        keyAvailable=available;
    }

    unsigned bonus() const { return revealedAt?FakeGoalBonus:0; }
    const TileField* gate(const TileField& field) const { return revealedAt?nullptr:&field; }

    Vec2 ufoPosition(unsigned tick) const {
        const unsigned age=ufoAt && tick>ufoAt?tick-ufoAt:0;
        const float fraction=std::min(age,UfoTravelTicks)/static_cast<float>(UfoTravelTicks);
        return {-352,592.0f-200.0f*fraction};
    }

    unsigned step(Game& game,const Course& scene,unsigned tick,bool chillMode) {
        unsigned events=0;
        const float playerRadius=scene.playerRadius;
        const bool inGlass=!glassConsumed && touchesFrontierBox417(game.position,playerRadius,FrontierGlassZone417);
        if (glassInside && !inGlass) glassConsumed=true;
        glassInside=inGlass;

        if (!revealedAt) {
            const Vec2 distance=game.position-FrontierFakeGoalCenter417;
            const float radius=playerRadius+16.0f;
            if (dot(distance,distance)<=radius*radius) {
                revealedAt=tick;
                finalGhostActive=!chillMode;
                game.velocity={};
                events|=FrontierFakeGoal417;
            }
        }
        if (keyAvailable && !keyAt) {
            if (!ufoAt && touchesFrontierBox417(game.position,playerRadius,FrontierUfoTrigger417)) {
                ufoAt=tick;
                events|=FrontierUfoLaunched417;
            }
            const SolidBlock keyTrigger{ufoPosition(tick)+Vec2{0,20},{24,28}};
            if (touchesFrontierBox417(game.position,playerRadius,keyTrigger)) {
                keyAt=tick;
                events|=FrontierKeyGranted417;
            }
        }
        if (!game.won && touchesFrontierRealGoal417(game.position,playerRadius)) {
            game.won=true;
            game.velocity={};
            events|=FrontierRealGoal417;
        }
        return events;
    }
};

inline bool stepFrontierGhosts417(GhostMotion& primary,GhostMotion* extras,unsigned extraCount,
                                 FrontierFinaleRun417& finale,Vec2 target,float playerRadius) {
    auto& finalGhost=finale.finalGhost;
    const CircleCollider body{finalGhost.position,FrontierFinalGhostBodyRadius417};
    const bool caughtByPeer=stepFrontierGhosts409(primary,extras,extraCount,target,&body);
    if (finale.finalGhostActive) {
        const Vec2 direction=target-finalGhost.position;
        const float distance=length(direction);
        if (distance>0.001f) finalGhost.velocity=finalGhost.velocity+direction*(0.7f/distance);
        const float speed=length(finalGhost.velocity);
        if (speed>80.0f) finalGhost.velocity=finalGhost.velocity*(80.0f/speed);
        finalGhost.position=finalGhost.position+finalGhost.velocity*(1.0f/60.0f);
    }
    const Vec2 gap=target-finalGhost.position;
    const float radius=playerRadius+FrontierFinalGhostHitRadius417;
    return caughtByPeer || dot(gap,gap)<=radius*radius;
}
