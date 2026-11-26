#pragma once
#include "campaign_mod_options.hpp"
#include "endless_modifiers.hpp"

// Campaign-owned transient state. Never shared with Editor or Endless state.
namespace CampaignModifiers {
struct State {
    static constexpr unsigned GhostCapacity=32;
    CampaignMods::Options options{};
    Vec2 pointerDelta{},cursor{},parryVelocity{},buddyPosition{};
    std::array<Vec2,GhostCapacity> ghostVelocity{};
    std::array<unsigned,GhostCapacity> ghostStun{};
    std::array<bool,GhostCapacity> ghostInside{};
    std::array<std::array<EndlessModifiers::Afterimage,4>,GhostCapacity> trails{};
    EndlessModifiers::Missile missile{};
    double tkFraction=0;
    float rotation=0,tkPitch=.75f;
    unsigned ticks=0,parryActive=0,parryFreeze=0,buddyTicks=0,wallBlast=0;
    bool wallDeath=false;
    bool parryJustArmed=false,tkBurstRequested=false,tkAudioOn=false,buddyAlive=false,burstShot=false,shotThisTick=false,lastPrimary=false;

    bool enabled(CampaignMods::Mod mod) const { return options.enabled(mod); }
    bool any() const { return options.modifiers!=0; }
    void reset(CampaignMods::Options value,Vec2 spawn) {
        *this={};options=value;cursor=spawn;missile.position=spawn;
        buddyPosition=spawn+Vec2{32,0};buddyAlive=enabled(CampaignMods::Buddy);
    }
    // PSP adaptation: analog motion is cursor-delta input at 128 source px/tick.
    static Vec2 analogDelta(Vec2 stick) {
        const float magnitude=length(stick);
        return magnitude>.18f?stick*((std::min(magnitude,1.f)-.18f)/(.82f*magnitude)*128):Vec2{};
    }
    void aim(Vec2 stick) {
        pointerDelta=analogDelta(stick);
        // A free world cursor for G/teleport; no clamp, just like the original.
        if (enabled(CampaignMods::God)) cursor=cursor+pointerDelta*(1.f/8);
    }
    bool primaryPressed(Game& game) {
        if (!enabled(CampaignMods::Telekinesis) || !enabled(CampaignMods::Burst) || parryFreeze) return false;
        (void)game;tkBurstRequested=true;burstShot=true;return true;
    }
    void secondaryPressed() {
        if (enabled(CampaignMods::Parry) && !parryActive) { parryActive=33;parryJustArmed=true; }
    }
    bool teleport(Game& game) const {
        if (!enabled(CampaignMods::God) || game.won) return false;
        game.position=cursor;return true;
    }
    bool shoot(Game& game,const Course& course,Vec2 drag,bool drift=false) {
        burstShot=false;
        if (game.won || parryFreeze) return false;
        if (enabled(CampaignMods::Telekinesis)) return false;
        const bool burst=enabled(CampaignMods::Burst) && game.moving();
        if (game.moving() && !burst && !drift) return false;
        if (burst && length(game.velocity)<100) { game.velocity={};return false; }
        const float power=length(drag);
        if (!burst && power<course.minDrag) return false;
        if (!enabled(CampaignMods::God) && power>course.maxDrag) drag=drag*(course.maxDrag/power);
        game.velocity=drag*course.dragMultiplier;
        ++game.strokes;burstShot=burst;shotThisTick=true;return true;
    }
    void beforeStep(Game& game,bool airborne=false) {
        game.modifierKinesis=enabled(CampaignMods::Telekinesis) && !airborne;
        game.modifierSticky=enabled(CampaignMods::Sticky);
        game.modifierParryFrozen=false;
        game.modifierDizzyAngle=0;
        if (!any()) return;
        ++ticks;
        if (parryActive && !parryJustArmed) --parryActive;
        parryJustArmed=false;
        if (parryFreeze) {
            game.velocity={};
            if (--parryFreeze) { game.modifierParryFrozen=true;return; }
            game.velocity=parryVelocity;
            // PParry.exit immediately dispatches the restored moving/TK state.
        }
        if (airborne || shotThisTick) return;
        if (game.modifierKinesis) {
            game.velocity=game.velocity*.99f+pointerDelta*.25f;
            if (tkBurstRequested) { game.velocity=game.velocity*-1;tkBurstRequested=false; }
            tkFraction+=length(pointerDelta)/3400.0;
            while (tkFraction>=1) { ++game.strokes;tkFraction-=1; }
            const float pitch=.75f+2.25f*length(pointerDelta)/128;
            tkPitch=pitch>tkPitch?pitch:tkPitch+(.75f-tkPitch)*.05f;
            pointerDelta={};
        }
        if (enabled(CampaignMods::Dizzy) && (game.modifierKinesis || length(game.velocity)>20)) {
            constexpr float angle=1.309f/60;
            game.modifierDizzyAngle=angle;
            rotation+=angle;
        }
    }
    void afterStep(Game&) { shotThisTick=false; }
    bool parryContact(Game& game,unsigned index,bool freezeGhost=true) {
        if (!enabled(CampaignMods::Parry) || parryActive<=23 || index>=GhostCapacity) return false;
        parryActive=0;parryVelocity=game.velocity;game.velocity={};parryFreeze=16;ghostStun[index]=freezeGhost?16:0;return true;
    }
    void stepBuddy(Vec2 player) {
        if (!buddyAlive) return;
        ++buddyTicks;
        const Vec2 offset=player-buddyPosition;
        const float distance=length(offset);
        const Vec2 direction=distance>0?offset*(1/distance):Vec2{};
        const float weight=(distance-32)/(384-32);
        const Vec2 velocity=direction*(1024*weight)+Vec2{-direction.y,direction.x}*(96*(1-weight));
        buddyPosition=buddyPosition+velocity*(1.f/60);
    }
    bool buddyTouch(Vec2 ghost,float radius) {
        if (!buddyAlive) return false;
        const float reach=radius+8*std::min(buddyTicks/60.f,1.f);
        if (dot(ghost-buddyPosition,ghost-buddyPosition)>reach*reach) return false;
        buddyAlive=false;return true;
    }
    unsigned stepMissile(Vec2 player,float radius,bool grounded) {
        return enabled(CampaignMods::Missiles)?missile.step(player,radius,grounded):0;
    }
    void recordGhost(unsigned index,Vec2 position,Vec2 velocity) {
        if (!enabled(CampaignMods::Panic) || index>=GhostCapacity || ticks%3) return;
        const float speed=length(velocity);
        if (speed<20) return;
        const float weight=std::clamp((speed-20)/130,0.f,1.f);
        trails[index][ticks/3%4]={position,ticks,static_cast<unsigned>(12*weight+.5f),static_cast<unsigned>(191*weight+.5f)};
    }
};
}
