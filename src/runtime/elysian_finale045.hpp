#pragma once

#include "elysian_geometry045.hpp"
#include "jump_pad.hpp"

enum class FinalePhase518 { Hunt, Defeat, Results, Drop, TeeReady, Attract, Ending };
enum class HuntLoss518 { None, Mine, Water, Horse, OutOfBounds, Backrooms };

struct HuntEvents518 {
    HorseEvents039 horse;
    HuntLoss518 loss=HuntLoss518::None;
    bool mineHit=false,launch=false,sand=false;
};

struct HorseFinale518Run {
    static constexpr unsigned DefeatTicks=720,DropTicks=30,CameraTicks=60,TeeTicks=1170;
    static constexpr unsigned KeyDelayTicks=2400,KeyAwardTick=4268;
    FinalePhase518 phase=FinalePhase518::Hunt;
    unsigned spent=0,defeatTicks=0,dropTicks=0,attractTicks=0,cameraTicks=0,keyTicks=0;
    unsigned mineAt[std::size(ElysianMines518)]{};
    bool cameraEntered=false,keyWaiting=false,fast=false;
    Vec2 attractPosition{},attractVelocity{};

    void restart(bool keyAvailable=false) {
        *this={};
        keyWaiting=keyAvailable;
    }
    bool canEnterEnding() const { return phase==FinalePhase518::Ending; }
    bool aftermath() const { return phase!=FinalePhase518::Hunt && phase!=FinalePhase518::Defeat; }
    bool usesTee() const {
        return phase==FinalePhase518::Drop || phase==FinalePhase518::TeeReady || phase==FinalePhase518::Attract;
    }
    bool advanceDefeat() {
        if (++defeatTicks<DefeatTicks) return false;
        phase=FinalePhase518::Results;
        return true;
    }
    void beginTee(Vec2) {
        phase=FinalePhase518::Drop;
        dropTicks=0;
    }
    bool advanceDrop() {
        if (++dropTicks<DropTicks) return false;
        phase=FinalePhase518::TeeReady;
        return true;
    }
    void beginAttract(Vec2 position={},Vec2 velocity={}) {
        phase=FinalePhase518::Attract;
        attractPosition=position;
        attractVelocity=velocity;
    }
    bool advanceAttract(bool accelerate) {
        fast=accelerate;
        const unsigned before=attractTicks;
        attractTicks=std::min(TeeTicks,attractTicks+(fast?4u:1u));
        attractPosition=attractPosition+attractVelocity*(std::min(30u,attractTicks)-std::min(30u,before))*(1.0f/60.0f);
        if (attractTicks<TeeTicks) return false;
        phase=FinalePhase518::Ending;
        return true;
    }

    static bool overlaps(Vec2 player,float radius,const SolidBlock& box) {
        const Vec2 delta=player-box.center;
        const Vec2 excess{std::max(0.0f,std::abs(delta.x)-box.halfSize.x),
                          std::max(0.0f,std::abs(delta.y)-box.halfSize.y)};
        return dot(excess,excess)<=radius*radius;
    }

    HuntEvents518 stepHunt(Game& game,const Course& course,const TileField& horseWalls,
                           HorseRun039& horse,JumpRun& jump,unsigned tick) {
        HuntEvents518 event;
        if (jump.outside(game.position,course)) {
            event.loss=jump.airborne() || jump.deathBuffer?HuntLoss518::OutOfBounds:HuntLoss518::Backrooms;
            return event;
        }
        if (jump.advance()) {
            jump.move(game);
        } else {
            event.sand=touchesElysianSand518(game.position,course.playerRadius);
            if (event.sand) game.velocity=game.velocity*0.85f;
            game.step(nullptr,0,{},nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,nullptr,nullptr,
                      nullptr,1,{},false,sweepElysianRocks518);
            if (collectMine(game.position,course.playerRadius,tick)) {
                event.loss=HuntLoss518::Mine;
                return event;
            }
            if (touchesElysianWater518(game.position,course.playerRadius)) {
                event.loss=HuntLoss518::Water;
                return event;
            }
            event.launch=jump.tryLaunch(ElysianJump518,game,course);
        }
        // Area contacts from this tick observe the new horse position.
        // Check the horse body before resolving its separate player-catching area.
        event.horse=horse.step(game,course.playerRadius,horseWalls,false,false);
        for (unsigned i=0;i<std::size(ElysianMines518) && horse.health;++i) {
            if (!consumeMine(i,horse.position,HorseRun039::WallRadius,tick)) continue;
            horse.damage(game,true);
            event.mineHit=true;
            if (!horse.health) {
                phase=FinalePhase518::Defeat;
                return event;
            }
        }
        if (!jump.airborne() && horse.touchesPlayer(game.position,course.playerRadius))
            event.loss=HuntLoss518::Horse;
        return event;
    }

private:
    bool collectMine(Vec2 position,float radius,unsigned tick) {
        for (unsigned i=0;i<std::size(ElysianMines518);++i)
            if (consumeMine(i,position,radius,tick)) return true;
        return false;
    }
    bool consumeMine(unsigned i,Vec2 position,float radius,unsigned tick) {
        const Vec2 delta=position-(ElysianMines518[i]+ElysianMineDetectorOffset518);
        const float reach=radius+ElysianMineRadius518;
        if ((spent&(1u<<i)) || dot(delta,delta)>reach*reach) return false;
        spent|=1u<<i;
        mineAt[i]=tick;
        return true;
    }
};
