#pragma once

#include <array>
#include <limits>
#include "game.hpp"
#include "meowls_schedule.hpp"

namespace MeowlsSim {

constexpr unsigned WorldCount=13, PointsPerWorld=15000, IntroTicks=210, PlayerDeathTicks=24;
// ceil(84331 frames / 735 audio samples per physics tick), including the faster Meowling cue.
constexpr unsigned BoomTicks=115, BabyBoomTicks=77;
enum class Phase { Ready, Playing, Exploding, Death };
enum class Kind { Caholic, Officer, Head, Egg, Gator, Baby, Bottle, Pellet, Life };
enum class Cue { Flap,Throw,Break,Hurt,Death,Slurp,Mini,Warning,Rotate,EggBurst,Bite,Gator,Location,Win,Boom,BabyBoom,Shatter,
                 StopRotate,StopWarning };

struct Box { Vec2 at,half; };

inline bool overlap(Box a,Box b) {
    return std::abs(a.at.x-b.at.x)<=a.half.x+b.half.x && std::abs(a.at.y-b.at.y)<=a.half.y+b.half.y;
}
inline bool touches(Box box,Vec2 point,float radius) {
    const float x=std::max(0.f,std::abs(point.x-box.at.x)-box.half.x);
    const float y=std::max(0.f,std::abs(point.y-box.at.y)-box.half.y);
    return x*x+y*y<=radius*radius;
}
inline Vec2 velocity(Vec2 direction,float speed) {
    const float distance=length(direction);
    return distance>0.0001f?direction*(speed/distance):Vec2{};
}

struct Actor {
    Kind kind{};
    bool live=false,falling=false,bitten=false,ready=false;
    unsigned age=0,death=0,timer=0,source=0;
    Vec2 pos{},aim{};
    float originY=0,speed=0,angle=0,steer=0;
};

struct Trail {
    Kind kind{};
    Vec2 pos{},drift{};
    float angle=0,scale=1;
    unsigned frame=0,life=0,maxLife=0;
};

struct Game {
    Phase phase=Phase::Ready;
    Vec2 player{320,320};
    float health=100,ySpeed=0,tilt=0,hitTint=0,treeX=896,treeY=0;
    bool treeHit=false,weary=false,shotLive=false,deathExit=false;
    unsigned score=0,difficulty=0,ticks=0,flightTicks=0,explodeTicks=0,deathAlpha=0;
    unsigned flash=0,regionTicks=UINT32_MAX,phoneTicks=UINT32_MAX;
    std::array<unsigned,5> timers{};
    std::array<Actor,144> actors{};
    std::array<Trail,96> trails{};
    unsigned trailSlot=0;
    uint32_t seed=1,cues=0;
    unsigned actorSerial=0,shots=0,bodyHits=0;

    unsigned region() const { return difficulty%WorldCount; }
    unsigned laps() const { return difficulty/WorldCount; }
    float scroll() const { return flightTicks/3.f; }
    float introScroll() const { return std::min(IntroTicks,flightTicks)*(768.f/IntroTicks); }
    Box body() const { return {{player.x,player.y-12},{8,20}}; }
    void signal(Cue cue) { cues|=1u<<static_cast<unsigned>(cue); }
    bool signaled(Cue cue) const { return (cues&(1u<<static_cast<unsigned>(cue)))!=0; }

    uint32_t nextRandom() {
        seed^=seed<<13;
        seed^=seed>>17;
        return seed^=seed<<5;
    }
    float range(float first,float last) { return first+(last-first)*(nextRandom()>>8)*(1.f/16777216.f); }

    void reset(uint32_t randomSeed) {
        phase=Phase::Ready;
        player={320,320};
        health=100;
        ySpeed=tilt=hitTint=0;
        treeX=896;
        seed=randomSeed?randomSeed:1;
        treeY=range(96,416);
        treeHit=weary=shotLive=deathExit=false;
        score=difficulty=ticks=flightTicks=explodeTicks=deathAlpha=flash=0;
        bodyHits=actorSerial=shots=trailSlot=cues=0;
        regionTicks=phoneTicks=UINT32_MAX;
        timers.fill(0);
        actors.fill({});
        trails.fill({});
    }

    Actor* spawn(Kind kind,Vec2 pos) {
        for (auto& actor:actors) if (!actor.live) {
            actor={};
            actor.kind=kind;
            actor.pos=pos;
            actor.originY=pos.y;
            actor.live=true;
            actor.source=++actorSerial;
            return &actor;
        }
        return nullptr;
    }

    void trail(Kind kind,Vec2 pos,float angle,unsigned frame,unsigned life,float scale,Vec2 drift={}) {
        trails[trailSlot]={kind,pos,drift,angle,scale,frame,life,life};
        trailSlot=(trailSlot+1)%trails.size();
    }

    void hurt(float amount) {
        if (phase==Phase::Exploding || phase==Phase::Death) return;
        health-=amount;
        ++bodyHits;
        hitTint=1;
        trail(Kind::Life,player,0,1,24,1,{-.8f,-.5f});
        if (health<=0) {
            phase=Phase::Exploding;
            explodeTicks=0;
            signal(Cue::Death);
        } else {
            signal(Cue::Hurt);
        }
    }

    void heal() {
        health+=1;
        signal(Cue::Slurp);
    }

    Box hitBox(const Actor& e) const {
        switch (e.kind) {
        case Kind::Head: return {e.pos+Vec2{28,0},{20,8}};
        case Kind::Baby: return {e.pos+Vec2{0,-6},{4,10}};
        case Kind::Gator: return {e.pos+Vec2{0,-24},{40,24}};
        default: return {e.pos+Vec2{0,-12},{8,20}};
        }
    }
    bool hostile(const Actor& e) const {
        if (!e.live || e.death) return false;
        switch (e.kind) {
        case Kind::Caholic: case Kind::Officer: case Kind::Baby: return true;
        case Kind::Head: return e.age>120;
        case Kind::Gator: return !e.ready;
        default: return false;
        }
    }

    void beginGator(Actor& actor) {
        actor.ready=true;
        actor.timer=0;
        actor.speed=10;
    }

    void explode(Actor& actor) {
        actor.death=1;
        signal(actor.kind==Kind::Baby?Cue::BabyBoom:Cue::Boom);
        if (actor.kind==Kind::Head) signal(Cue::StopWarning);
        trail(Kind::Life,actor.pos,0,1,24,1,{-.8f,-.5f});
    }

    void kill(Actor& actor) {
        if (actor.kind==Kind::Head) {
            actor.speed=.2f;
            trail(Kind::Life,actor.pos,0,1,24,1,{-.8f,-.5f});
        } else if (actor.kind==Kind::Gator) {
            beginGator(actor);
        } else {
            explode(actor);
        }
    }

    void spawnLife(Vec2 pos) {
        if (auto* life=spawn(Kind::Life,pos)) {
            life->aim={range(1,5),range(-3,1)};
        }
    }

    void hatch(Actor& egg) {
        std::array<float,5> positions{608,640,672,704,736};
        for (unsigned count=positions.size();count>1;--count)
            std::swap(positions[count-1],positions[nextRandom()%count]);
        for (unsigned index=0;index<positions.size();++index) {
            if (auto* baby=spawn(Kind::Baby,egg.pos)) {
                baby->aim={positions[index],range(32,352)};
                baby->timer=30*(index+1);
            }
        }
        signal(Cue::StopRotate);
        signal(Cue::EggBurst);
    }

    void spawnWave(unsigned id) {
        switch (id) {
        case 0: spawn(Kind::Caholic,{792,range(96,416)}); break;
        case 1: spawn(Kind::Officer,{792,range(96,416)}); break;
        case 2:
            if (auto* head=spawn(Kind::Head,{768,treeY+range(-80,80)})) {
                head->speed=5;
                signal(Cue::Warning);
            }
            break;
        case 3:
            if (spawn(Kind::Egg,{704,-48})) signal(Cue::Rotate);
            break;
        case 4: spawn(Kind::Gator,{832,512}); break;
        }
    }

    void advanceStage() {
        ++difficulty;
        timers.fill(0);
        regionTicks=0;
        if (difficulty==WorldCount) {
            phoneTicks=0;
            signal(Cue::Win);
        }
        weary=difficulty>=39;
    }

    void advanceActors();
    void collide();
    void step(bool flap,bool fire,Vec2 aim);
};

inline void Game::advanceActors() {
    const auto serial=actorSerial;
    for (auto& e:actors) {
        if (!e.live || e.source>serial) continue;
        ++e.age;
        if (e.death) {
            ++e.death;
            if (e.kind==Kind::Head) {
                e.pos.x-=e.speed;
                e.speed+=.1f;
                if (e.pos.x<-272) e.live=false;
                else if (e.death%BoomTicks==0) signal(Cue::Boom);
            } else if (e.death>=(e.kind==Kind::Baby?BabyBoomTicks:BoomTicks)) e.live=false;
            continue;
        }
        switch (e.kind) {
        case Kind::Caholic: case Kind::Officer: {
            const bool officer=e.kind==Kind::Officer;
            e.pos.x-=officer?1.f:3.f;
            e.pos.y=e.originY+std::sin(e.age*((officer?2.f:5.f)/60))*96.f;
            if (e.pos.x<0) { e.live=false; break; }
            if (officer && e.age%121==0) {
                if (auto* pellet=spawn(Kind::Pellet,e.pos)) {
                    pellet->aim=velocity(player-e.pos,325);
                    pellet->timer=e.source;
                    signal(Cue::Mini);
                }
            }
            break;
        }
        case Kind::Head:
            if (e.age<=120) break;
            if (e.speed>2) {
                const Vec2 drift={range(-10,10),range(-10,10)};
                trail(Kind::Head,e.pos,0,e.age/2%2,14,range(.8f,1.1f),drift);
            }
            e.pos.x-=e.speed;
            e.speed+=.1f;
            if (e.pos.x<-272) e.live=false;
            break;
        case Kind::Egg: {
            const float fall=std::min(e.age/90.f,1.f);
            e.pos.y=-48+512*fall*fall;
            if (e.age==90) hatch(e);
            if (e.age>=111) e.live=false;
            break;
        }
        case Kind::Gator:
            if (!e.ready) {
                e.pos.x-=1.5f;
                if (e.pos.x<320) beginGator(e);
            } else if (++e.timer==24) {
                signal(Cue::Gator);
            } else if (e.timer>24) {
                if (e.falling) {
                    e.steer+=.25f;
                    e.pos=e.pos+Vec2{2,e.steer};
                    e.angle+=.004363323f;
                } else {
                    e.pos.y-=e.speed;
                    e.speed+=(10-e.speed)*.2f;
                }
                if (e.pos.y<-256 || e.pos.y>640) e.live=false;
            }
            break;
        case Kind::Baby:
            if (!e.ready && e.age>=e.timer) {
                e.aim=velocity(player-e.pos,450.f);
                e.ready=true;
            }
            if (e.ready) e.pos=e.pos+e.aim*(1.f/60);
            else e.pos=e.pos+(e.aim-e.pos)*.1f;
            if (e.pos.x<0) e.live=false;
            break;
        case Kind::Bottle: case Kind::Pellet:
            trail(e.kind,e.pos,e.angle,0,8,1);
            e.pos=e.pos+e.aim*(1.f/60);
            e.angle+=.3926990817f;
            if (e.pos.x<232 || e.pos.x>792 || e.pos.y<-24 || e.pos.y>536) {
                e.live=false;
                if (e.kind==Kind::Bottle) { shotLive=false; signal(Cue::Break); }
            }
            break;
        case Kind::Life:
            e.pos=e.pos+e.aim;
            e.speed+=.01f;
            e.aim.x=std::clamp(e.aim.x,-10.f,10.f)+(player.x<e.pos.x?-e.speed:e.speed);
            e.aim.y=std::clamp(e.aim.y,-10.f,10.f)+(player.y<e.pos.y?-e.speed:e.speed);
            if (dot(player-e.pos,player-e.pos)<32.f*32.f) {
                heal();
                e.live=false;
            } else if (e.age>=180) e.live=false;
            break;
        }
    }
}

inline void Game::collide() {
    const auto ship=body();
    const Box upper{{treeX,treeY-288},{24,224}},lower{{treeX,treeY+288},{24,224}};
    if (phase==Phase::Playing && !treeHit && (overlap(ship,upper)||overlap(ship,lower))) {
        treeHit=true;
        hurt(15);
    }
    for (auto& e:actors) {
        if (!e.live || e.death) continue;
        if (e.kind==Kind::Head && hostile(e) &&
            (overlap(hitBox(e),upper)||overlap(hitBox(e),lower))) explode(e);
        if (e.kind==Kind::Gator && e.ready && e.timer>=24 && !e.falling) {
            const Box mouth{e.pos+Vec2{4,128},{28,112}};
            if (phase==Phase::Playing && !e.bitten && overlap(ship,mouth)) {
                e.bitten=true;
                e.speed=2;
                signal(Cue::Bite);
                hurt(25);
            }
            for (auto& prey:actors) {
                if (!prey.live || prey.death || prey.kind==Kind::Gator) continue;
                if ((prey.kind==Kind::Bottle || prey.kind==Kind::Pellet) &&
                    touches(mouth,prey.pos,16)) {
                    prey.live=false;
                    if (prey.kind==Kind::Bottle) {
                        shotLive=false;
                        e.falling=true;
                        e.steer=-2;
                    } else {
                        e.speed=2;
                    }
                } else if (hostile(prey) && overlap(hitBox(prey),mouth)) {
                    e.speed=2;
                    prey.live=false;
                    trail(Kind::Life,prey.pos,0,1,24,1);
                    signal(Cue::Bite);
                }
            }
        }
        if (phase==Phase::Playing && hostile(e) && e.kind!=Kind::Gator && overlap(ship,hitBox(e))) {
            hurt(10);
            explode(e);
        }
        if (e.kind!=Kind::Bottle && e.kind!=Kind::Pellet) continue;
        const bool bottle=e.kind==Kind::Bottle;
        if (!bottle && phase==Phase::Playing && touches(ship,e.pos,16)) {
            e.live=false;
            hurt(10);
            continue;
        }
        for (auto& target:actors) {
            if (!hostile(target) || (!bottle && target.source==e.timer) ||
                !touches(hitBox(target),e.pos,16)) continue;
            kill(target);
            e.live=false;
            if (bottle) {
                shotLive=false;
                signal(Cue::Shatter);
                trail(Kind::Bottle,e.pos,0,1,24,1);
                spawnLife(e.pos);
            }
            break;
        }
    }
}

inline void Game::step(bool flap,bool fire,Vec2 aim) {
    cues=0;
    ++ticks;
    if (phase==Phase::Death) {
        deathExit|=flap;
        if (deathExit) {
            if (deathAlpha>5) deathAlpha-=5;
            else reset(nextRandom());
        } else {
            deathAlpha=deathAlpha<255?deathAlpha+5:250;
        }
        return;
    }
    if (health>100) health=100;
    else health+=weary?-.02f:.02f;
    tilt*=.95f;
    hitTint=std::max(0.f,hitTint-.1f);
    if (flap && phase!=Phase::Exploding) {
        phase=Phase::Playing;
        ySpeed=-260;
        tilt+=.2617993878f;
        signal(Cue::Flap);
    }
    if (phase==Phase::Ready) return;
    if (fire && !shotLive) {
        if (auto* shot=spawn(Kind::Bottle,player)) {
            shot->aim=velocity(aim-player,650);
            shotLive=true;
            ++shots;
            signal(Cue::Throw);
        }
    }
    ++flightTicks;
    ySpeed+=10;
    player.y+=ySpeed/60;
    if (phase==Phase::Playing && (player.y<32 || player.y>504)) {
        player.y=std::clamp(player.y,32.f,504.f);
        ySpeed=-ySpeed;
        hurt(5);
    }
    treeX-=2;
    if (treeX<232) {
        treeX=792;
        treeY=range(96,416);
        treeHit=false;
    }
    const auto& schedule=Schedule[std::min<std::size_t>(difficulty,std::size(Schedule)-1)];
    for (unsigned i=0;i<timers.size();++i) if (schedule[i]) {
        if (timers[i]>schedule[i]) { spawnWave(i); timers[i]=0; }
        ++timers[i];
    }
    score+=5;
    const unsigned target=PointsPerWorld*(difficulty+1);
    flash=score>=target-200?std::min(100u,flash+4):(flash>=4?flash-4:0);
    if (regionTicks!=UINT32_MAX) ++regionTicks;
    if (phoneTicks!=UINT32_MAX) ++phoneTicks;
    if (score>=target) advanceStage();
    if (regionTicks==22) signal(Cue::Location);
    if (phoneTicks==42 || phoneTicks==84) signal(Cue::Win);
    for (auto& image:trails) {
        if (image.life) { --image.life; image.pos=image.pos+image.drift; }
    }
    advanceActors();
    collide();
    if (phase==Phase::Exploding && ++explodeTicks>=PlayerDeathTicks) {
        phase=Phase::Death;
        deathAlpha=0;
        deathExit=false;
    }
}

} // namespace MeowlsSim
