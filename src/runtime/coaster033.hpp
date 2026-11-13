#pragma once

#include "jump_pad.hpp"

enum class CoasterPhase033 : uint8_t { Boarding, Starting, Riding, Leaving, Slowing, Ending, Station };
enum class CoasterObjectKind033 : uint8_t { None, Coin, Ghost };
enum class CoasterDeath033 : uint8_t { None, Ghost, WallStuck, OutOfBounds, Backrooms };

struct CoasterObject033 {
    CoasterObjectKind033 kind=CoasterObjectKind033::None;
    uint8_t lane=0;
    bool collected=false, grazing=false, touching=false;
    float x=0, age=0;
    uint32_t tint=0xffffffff;
    unsigned ticks=0, popupScore=0;
    float popupAge=0;
};

struct CoasterEvents033 {
    bool started=false, phaseChanged=false, caught=false, launched=false;
    bool bounced=false, pushed=false, grazed=false, parried=false, windStarted=false;
    bool ghostWarning=false, coinThought=false;
    int coinWarning=-1;
    unsigned coins=0, score=0, elapsedTicks=0;
    float coinWarningPitch=1, ghostWarningPitch=1, collectPitch=1;
};

namespace coaster033 {

struct Spawn { unsigned delay; int lane; CoasterObjectKind033 kind; };
inline constexpr Spawn Spawns[]={
#include "coaster_spawns033.inc"
};

inline constexpr Vec2 Trigger{-168,84};
inline constexpr Vec2 TriggerHalf{64,44};
inline constexpr SolidBlock CartWalls[]={
    {{-176,0},{104,8}}, {{-76,80},{4,72}},
    {{-268,80},{12,72}}, {{-176,160},{104,8}},
};
inline constexpr SolidBlock AntiJump[]={
    {{0,-164},{320,156}}, {{0,244},{320,76}},
    {{124,80},{196,88}}, {{-300,80},{20,88}},
};
struct OneWay { Vec2 face, normal; };
inline constexpr OneWay StationFences[]={
    {{-28,80},{-1,0}}, {{-212,80},{1,0}},
    {{-116,0},{0,1}}, {{20,160},{0,-1}},
};

inline float mix(float a,float b,float weight) { return a+(b-a)*weight; }

inline bool overlap(Vec2 a,Vec2 b,float radius) {
    const auto offset=a-b;
    return dot(offset,offset)<=radius*radius;
}

inline uint32_t hashInteger(uint64_t value) {
    // Variant::hash for integer frames in the source's six-frame RNG window.
    value=(~value)+(value<<18);
    value^=value>>31;
    value*=21;
    value^=value>>11;
    value+=value<<6;
    return static_cast<uint32_t>(value^(value>>22));
}

struct Random {
    static constexpr uint64_t Increment=(1442695040888963407ULL<<1)|1;
    uint64_t state=0;

    uint32_t next() {
        const uint64_t old=state;
        state=old*6364136223846793005ULL+Increment;
        const auto bits=static_cast<uint32_t>(((old>>18)^old)>>27);
        const auto rotation=static_cast<unsigned>(old>>59);
        return (bits>>rotation)|(bits<<((32-rotation)&31));
    }

    void seed(uint64_t seed) {
        state=0;
        next();
        state+=seed;
        next();
    }

    float unit() { return float(next()>>8)*(1.0f/16777216.0f); }

    float pitch(float limit) { return mix(1/limit,limit,unit()); }

    uint32_t coinTint() {
        const float hue=unit()*6;
        const unsigned sector=static_cast<unsigned>(hue);
        const float fraction=hue-sector;
        const float rising=mix(0.5f,1.0f,fraction), falling=mix(1.0f,0.5f,fraction);
        const Vec2 rg[]={{1,rising},{falling,1},{0.5f,1},{0.5f,falling},{rising,0.5f},{1,0.5f}};
        const float b[]={0.5f,0.5f,rising,1,1,falling};
        return 0xff000000u|(uint32_t(b[sector]*255)<<16)|
            (uint32_t(rg[sector].y*255)<<8)|uint32_t(rg[sector].x*255);
    }
};

} // namespace coaster033

class CoasterRun033 {
public:
    static constexpr float Dt=1.0f/60.0f;
    static constexpr float SpawnX=1200, ScreenEdge=320, KillEdge=-400;
    static constexpr float ObjectSpeed=-800;
    static constexpr float StartEndX=1156, StationEndX=-44;
    static constexpr float ExitThreshold=800, FrontierThreshold=600, EndThreshold=1768;
    static constexpr float LoopLength=579.2176f;

    CoasterPhase033 phase=CoasterPhase033::Boarding;
    CoasterDeath033 death=CoasterDeath033::None;
    float phaseTime=0, rideTime=0, pathProgress=0, pathSpeed=0;
    float startX=0, endX=StartEndX, startWallX=0, safetySignX=0, trackX=0, trackSpeed=0;
    float cloudAlpha=0;
    float cartPitch=0.2f, cartVolumeDb=-80, musicPitch=1, musicVolume=1, windVolumeDb=-80;
    bool skipping=false;
    unsigned bonus=0, coinMultiplier=0, collectedCoins=0, spawnedCoins=0, spawnedGhosts=0;
    JumpRun jump{};
    std::array<CoasterObject033,16> objects{};
    std::array<float,4> coinWarnings{}, ghostWarnings{};

    void restart(bool hard=false,bool easy=false) {
        *this=CoasterRun033{};
        requiredCoins=hard?50:25;
        easyMode=easy;
    }

    static float laneY(unsigned lane) {
        constexpr float Y[]={24,61.333f,98.667f,136};
        return Y[lane];
    }

    unsigned remainingCoins() const { return requiredCoins-collectedCoins; }
    bool cartClosed() const { return phase!=CoasterPhase033::Boarding && phase!=CoasterPhase033::Station; }
    bool scrollTiles() const { return rideTicks>=104; }
    bool exitPath() const { return phase>=CoasterPhase033::Leaving; }
    bool frontierVisible() const { return exitPath() && pathProgress>=FrontierThreshold; }
    bool stairsFront() const { return phase!=CoasterPhase033::Station; }
    bool parryFrozen() const { return parryTimer!=0; }
    Vec2 goalCenter() const { return {endX+148,-128}; }
    Vec2 padCenter() const { return {endX+148,264}; }
    float playbackMusicPitch() const { return musicPitch*(skipping?2:1); }

    CoasterEvents033 step(Game& game,const Course& course,const TileField& fixed,
                          const TileField& start,const TileField& end,bool skipHeld=false,bool parry=false) {
        CoasterEvents033 event;
        skipping=skipHeld && canSkip();
        const unsigned ticks=skipping?4:1;
        for (unsigned i=0;i<ticks && !game.won && death==CoasterDeath033::None;++i) {
            if (i && !canSkip()) break;
            stepOne(game,course,fixed,start,end,parry && i==0,event);
            ++event.elapsedTicks;
        }
        skipping=skipHeld && canSkip() && death==CoasterDeath033::None && !game.won;
        return event;
    }

private:
    unsigned requiredCoins=25;
    unsigned frame=0, phaseTicks=0, rideTicks=0, previousStrokes=0;
    unsigned spawnIndex=0, nextSpawn=coaster033::Spawns[0].delay;
    unsigned parryActive=0, parryTimer=0, wallTicks=0;
    bool easyMode=false;
    Vec2 parryVelocity{};
    coaster033::Random spawnRng{}, effectsRng{};

    bool canSkip() const {
        return phase==CoasterPhase033::Leaving || phase==CoasterPhase033::Slowing;
    }

    void setPhase(CoasterPhase033 next,CoasterEvents033& event) {
        phase=next;
        phaseTicks=0;
        phaseTime=0;
        event.phaseChanged=true;
        if (next==CoasterPhase033::Slowing) event.windStarted=true;
    }

    void begin(CoasterEvents033& event) {
        setPhase(CoasterPhase033::Starting,event);
        const auto seed=coaster033::hashInteger(frame/6);
        spawnRng.seed(seed);
        effectsRng.seed(seed^0x3e588u);
        event.started=true;
    }

    void advanceTimeline(CoasterEvents033& event) {
        using coaster033::mix;
        if (phase==CoasterPhase033::Boarding || phase==CoasterPhase033::Station) return;
        ++phaseTicks;
        phaseTime=phaseTicks*Dt;
        ++rideTicks;
        rideTime=rideTicks*Dt;
        const float oldTime=(phaseTicks-1)*Dt;
        const float t=phaseTime;

        if (phase>=CoasterPhase033::Riding) {
            startX-=800*Dt;
            startWallX-=640*Dt;
            safetySignX-=1000*Dt;
        }

        switch (phase) {
        case CoasterPhase033::Starting: {
            const float u=t/3, prev=oldTime/3;
            const float advance=u*u*u-prev*prev*prev;
            startX=-1152*u*u*u;
            startWallX=-640*u*u*u;
            safetySignX=-1000*u*u*u;
            trackX-=800*advance;
            trackSpeed=-800*u*u;
            pathProgress+=10*advance;
            pathSpeed=10*u*u;
            if (phaseTicks==180) setPhase(CoasterPhase033::Riding,event);
            break;
        }
        case CoasterPhase033::Riding:
            trackX-=800*Dt;
            pathProgress+=10*Dt;
            if (pathProgress>LoopLength) pathProgress-=LoopLength;
            break;
        case CoasterPhase033::Leaving: {
            const float u=std::min(t/2,1.0f), average=(u+std::min(oldTime/2,1.0f))*0.5f;
            trackSpeed=mix(-800,-2400,u);
            trackX+=mix(-800,-2400,average)*Dt;
            pathSpeed=mix(10,80,u);
            pathProgress+=mix(10,80,average)*Dt;
            musicPitch=mix(1,1.5f,u);
            cartPitch=mix(1.1f,2.5f,u);
            if (pathProgress>=ExitThreshold) setPhase(CoasterPhase033::Slowing,event);
            break;
        }
        case CoasterPhase033::Slowing: {
            const float u=std::min(t/3,1.0f), average=(u+std::min(oldTime/3,1.0f))*0.5f;
            trackSpeed=mix(-2400,-1200,u);
            trackX+=mix(-2400,-1200,average)*Dt;
            pathSpeed=mix(100,30,u);
            pathProgress+=mix(100,30,average)*Dt;
            musicVolume=1-u;
            musicPitch=mix(1.5f,0.5f,u);
            windVolumeDb=mix(-80,-15,u);
            cartPitch=mix(2.5f,0.7f,u);
            cartVolumeDb=mix(-6,-16,std::min(t/2.5f,1.0f));
            cloudAlpha=u;
            if (pathProgress>=EndThreshold) setPhase(CoasterPhase033::Ending,event);
            break;
        }
        case CoasterPhase033::Ending: {
            const float u=1-t/3, prev=1-oldTime/3;
            const float advance=prev*prev*prev-u*u*u;
            const float newEndX=StationEndX+1200*u*u*u;
            trackX+=newEndX-endX;
            endX=newEndX;
            trackSpeed=-1200*u*u;
            pathSpeed=30*u*u;
            pathProgress+=30*advance;
            cartPitch=mix(0.7f,0.01f,t/3);
            cartVolumeDb=mix(-16,-80,std::clamp((t-2.25f)/0.75f,0.0f,1.0f));
            if (phaseTicks==180) setPhase(CoasterPhase033::Station,event);
            break;
        }
        case CoasterPhase033::Boarding:
        case CoasterPhase033::Station:
            break;
        }

        if (rideTicks<=216 && phase<=CoasterPhase033::Riding) {
            cartVolumeDb=mix(-80,-6,std::min(rideTime/0.6f,1.0f));
            cartPitch=mix(0.2f,1.1f,std::clamp((rideTime-0.6f)/3,0.0f,1.0f));
        }
    }

    void spawn(CoasterEvents033& event) {
        if (phase!=CoasterPhase033::Starting && phase!=CoasterPhase033::Riding) return;
        while (nextSpawn<=rideTicks) {
            const auto& entry=coaster033::Spawns[spawnIndex];
            const auto lane=static_cast<uint8_t>(entry.lane<0?spawnRng.next()&3u:entry.lane);
            spawnIndex=(spawnIndex+1)%std::size(coaster033::Spawns);
            nextSpawn+=coaster033::Spawns[spawnIndex].delay;
            if (entry.kind==CoasterObjectKind033::Ghost && easyMode) continue;
            auto it=std::find_if(objects.begin(),objects.end(),[](const auto& object) {
                return object.kind==CoasterObjectKind033::None;
            });
            if (it==objects.end()) continue;
            *it={};
            it->kind=entry.kind;
            it->lane=lane;
            it->x=SpawnX;
            if (entry.kind==CoasterObjectKind033::Ghost) {
                ++spawnedGhosts;
                event.ghostWarning=true;
                event.ghostWarningPitch=effectsRng.pitch(1.2f);
            } else {
                ++spawnedCoins;
                it->tint=effectsRng.coinTint();
                event.coinWarning=effectsRng.next()%3;
                event.coinWarningPitch=effectsRng.pitch(1.4f);
            }
        }
    }

    void advanceObjects() {
        for (auto& object:objects) {
            if (object.kind==CoasterObjectKind033::None) continue;
            object.age=++object.ticks*Dt;
            if (object.collected) object.popupAge+=Dt;
            object.x=SpawnX+ObjectSpeed*object.age;
            if (object.x<=KillEdge || object.popupAge>=50*Dt) object={};
        }
    }

    void warnings() {
        unsigned coins=0, ghosts=0;
        for (const auto& object:objects) {
            if (object.x<=ScreenEdge) continue;
            if (object.kind==CoasterObjectKind033::Coin) coins|=1u<<object.lane;
            if (object.kind==CoasterObjectKind033::Ghost) ghosts|=1u<<object.lane;
        }
        for (unsigned i=0;i<4;++i) {
            coinWarnings[i]=coins&(1u<<i) ? (coinWarnings[i]>0?coaster033::mix(coinWarnings[i],1,0.1f):1.5f) : 0;
            ghostWarnings[i]=ghosts&(1u<<i) ? (ghostWarnings[i]>0?coaster033::mix(ghostWarnings[i],1,0.1f):1.5f) : 0;
        }
    }

    void kill(Game& game,CoasterDeath033 reason,CoasterEvents033& event) {
        death=reason;
        game.velocity={};
        event.caught=true;
    }

    void contactObjects(Game& game,const Course& course,CoasterEvents033& event) {
        for (auto& object:objects) {
            const Vec2 center{object.x,laneY(object.lane)};
            if (object.kind==CoasterObjectKind033::Coin && !object.collected && !jump.airborne() &&
                coaster033::overlap(center,game.position,course.playerRadius+11.1803f)) {
                object.collected=true;
                object.popupScore=50+5*coinMultiplier;
                bonus+=object.popupScore;
                ++event.coins;
                event.score+=object.popupScore;
                event.collectPitch=effectsRng.pitch(1.3f);
                if (coinMultiplier<=9) ++coinMultiplier;
                event.coinThought|=coinMultiplier>=6;
                if (collectedCoins<requiredCoins && ++collectedCoins==requiredCoins)
                    setPhase(CoasterPhase033::Leaving,event);
            }
            if (object.kind!=CoasterObjectKind033::Ghost) continue;
            const bool graze=!jump.airborne() &&
                coaster033::overlap(center,game.position,course.playerRadius+24);
            event.grazed|=object.grazing && !graze;
            object.grazing=graze;
            const bool touch=!jump.airborne() &&
                coaster033::overlap(center,game.position,course.playerRadius+4);
            if (touch && !object.touching) {
                if (parryActive>23) {
                    parryVelocity=game.velocity;
                    game.velocity={};
                    parryActive=0;
                    parryTimer=16;
                    event.parried=true;
                } else {
                    kill(game,CoasterDeath033::Ghost,event);
                    return;
                }
            }
            object.touching=touch;
        }
    }

    void fenceSweep(Vec2 position,Vec2 motion,float radius,WallContact& contact) const {
        for (const auto& fence:coaster033::StationFences) {
            const Vec2 offset=position-fence.face-Vec2{endX,0};
            const float speed=dot(motion,fence.normal);
            if (speed>=0) continue;
            const float distance=dot(offset,fence.normal)-radius;
            if (distance<-0.1f) continue;
            const float t=std::max(0.0f,-distance/speed);
            if (t>1 || t>=contact.time) continue;
            const Vec2 tangent{fence.normal.y,-fence.normal.x};
            if (std::abs(dot(offset+motion*t,tangent))<=112+radius)
                contact={t,fence.normal};
        }
    }

    void move(Game& game,const Course& course,const TileField& fixed,const TileField& start,
              const TileField& end,CoasterEvents033& event) {
        const float radius=course.playerRadius;
        const bool airborne=jump.advance();
        if (parryTimer && --parryTimer==0) game.velocity=parryVelocity;
        if (!airborne && !parryTimer) {
            const float speed=length(game.velocity);
            if (speed<=course.stopSpeed) game.velocity={};
            else game.velocity=game.velocity*(1-course.maxDrag*std::pow(course.frictionCoeff/speed,course.frictionPow));
        }
        const Vec2 motion=game.velocity*Dt;
        WallContact contact;
        if (airborne) {
            if (cartClosed())
                for (const auto& block:coaster033::AntiJump)
                    TileField::sweepBlock(block,game.position,motion,radius,contact);
        } else {
            const Vec2 startOffset{startX,0}, endOffset{endX,0};
            Vec2 position=game.position;
            position=position+fixed.recoverCircle(position,radius);
            position=position+start.recoverCircle(position-startOffset,radius);
            position=position+end.recoverCircle(position-endOffset,radius);
            if (cartClosed()) {
                // The starter's rectangle sits wholly inside the cabin. Its leading
                // edge can activate while the player's circle straddles the new wall.
                position.x=std::clamp(position.x,-256+radius,-80-radius);
                position.y=std::clamp(position.y,8+radius,152-radius);
            }
            game.pushed=position.x!=game.position.x || position.y!=game.position.y;
            event.pushed|=game.pushed && !game.moving();
            game.position=position;
            contact=fixed.sweep(position,motion,radius);
            const auto startHit=start.sweep(position-startOffset,motion,radius);
            if (startHit.time<contact.time) contact=startHit;
            const auto endHit=end.sweep(position-endOffset,motion,radius);
            if (endHit.time<contact.time) contact=endHit;
            if (cartClosed())
                for (const auto& block:coaster033::CartWalls)
                    TileField::sweepBlock(block,position,motion,radius,contact);
            fenceSweep(position,motion,radius,contact);
        }
        game.position=game.position+motion*std::min(1.0f,contact.time);
        if (contact.time<=1) {
            game.velocity=game.velocity-contact.normal*(2*dot(game.velocity,contact.normal));
            ++game.bounces;
            event.bounced=true;
        }
    }

    void stepOne(Game& game,const Course& course,const TileField& fixed,const TileField& start,
                 const TileField& end,bool parry,CoasterEvents033& event) {
        ++frame;
        if (previousStrokes!=game.strokes) {
            coinMultiplier=0;
            previousStrokes=game.strokes;
        }
        if (parryActive) --parryActive;
        if (parry && !parryActive && !parryTimer) parryActive=33;
        advanceTimeline(event);
        advanceObjects();
        spawn(event);
        warnings();
        game.pushed=game.passiveBounce=false;
        move(game,course,fixed,start,end,event);
        if (jump.outside(game.position,course)) {
            kill(game,jump.airborne() || jump.deathBuffer?CoasterDeath033::OutOfBounds:CoasterDeath033::Backrooms,event);
            return;
        }
        const bool embedded=!jump.airborne() && (length(fixed.recoverCircle(game.position,0))>0 ||
            length(start.recoverCircle(game.position-Vec2{startX,0},0))>0 ||
            length(end.recoverCircle(game.position-Vec2{endX,0},0))>0);
        wallTicks=embedded?wallTicks+1:0;
        if (wallTicks>=9) {
            kill(game,CoasterDeath033::WallStuck,event);
            return;
        }
        if (!jump.airborne() && phase==CoasterPhase033::Boarding) {
            const auto local=game.position-coaster033::Trigger;
            const auto half=coaster033::TriggerHalf;
            const Vec2 closest{std::clamp(local.x,-half.x,half.x),std::clamp(local.y,-half.y,half.y)};
            if (coaster033::overlap(local,closest,course.playerRadius)) begin(event);
        }
        contactObjects(game,course,event);
        if (death!=CoasterDeath033::None || jump.airborne()) return;
        if (coaster033::overlap(game.position,goalCenter(),course.playerRadius+course.goalRadius)) {
            game.won=true;
            game.position=goalCenter();
            game.velocity={};
            return;
        }
        const JumpStage pad{1,{{padCenter(),-1.5708f,875}}};
        event.launched|=jump.tryLaunch(pad,game,course);
    }
};
