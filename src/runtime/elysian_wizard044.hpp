#pragma once

#include "game.hpp"

enum class WizardSpell516 { None, Wind, Mirror, Blind, Death };

struct WizardRun516 {
    static constexpr unsigned CastTicks=42, SpellIconTicks=48, CastSoundTick=10;
    WizardSpell516 spell=WizardSpell516::None;
    unsigned age=0, castAge=CastTicks, idleTick=0, windTick=0, launched=0;
    float castWandY=0;
    bool shotMotion=false;
    float mistSeconds=0;

    bool mirrored() const { return spell==WizardSpell516::Mirror; }
    bool blind() const { return spell==WizardSpell516::Blind || dead(); }
    bool dead() const { return spell==WizardSpell516::Death; }
    bool changedHat() const { return launched>=4; }
    bool drifting() const { return spell==WizardSpell516::Wind && !shotMotion; }
    float mistTime() const { return mistSeconds; }

    Vec2 input(Vec2 aim) const {
        if (mirrored()) aim.x=-aim.x;
        return aim;
    }

    void prepare(unsigned mask) {
        for (;mask;mask&=mask-1) ++launched;
    }

    WizardSpell516 impact(unsigned health) {
        constexpr WizardSpell516 spells[]={WizardSpell516::Death,WizardSpell516::Blind,
                                           WizardSpell516::Mirror,WizardSpell516::Wind};
        if (health>=std::size(spells)) return WizardSpell516::None;
        spell=spells[health];
        age=0;
        if (spell==WizardSpell516::Wind) windTick=0;
        if (dead()) castAge=CastTicks;
        else if (castAge==CastTicks) {
            castWandY=idleWandY();
            castAge=0;
        }
        return spell;
    }

    float idleWandY() const {
        const unsigned phase=idleTick%60;
        const float t=std::min(phase,60-phase)/30.0f;
        return 16*(t<0.5f?2*t*t:1-2*(1-t)*(1-t));
    }

    Vec2 acceleration() const {
        if (spell!=WizardSpell516::Wind) return {};
        const unsigned phase=windTick%300;
        return {phase<120?0.0f:phase<150 || phase>=270?-5.0f:-25.0f,0};
    }

    float pitch() const { return 0.8f+0.4f*gust(); }
    float volumeDb() const { return -10.0f+10.0f*gust(); }

    void beginMove() { shotMotion=true; }
    void afterStep(const Game& game) {
        if (!game.moving()) shotMotion=false;
    }

    bool advance() {
        mistSeconds+=mistSpeed()/60.0f;
        if (spell==WizardSpell516::Wind) ++windTick;
        if (spell!=WizardSpell516::None) ++age;
        if (castAge<CastTicks) {
            ++castAge;
            if (castAge==CastTicks) idleTick=0;
            return castAge==CastSoundTick;
        }
        ++idleTick;
        return false;
    }

private:
    float gust() const {
        if (spell!=WizardSpell516::Wind) return 0;
        const unsigned phase=windTick%300;
        if (phase<120 || phase>=270) return 0;
        return phase<150?(phase-120)/30.0f:(270-phase)/120.0f;
    }

    float mistSpeed() const {
        if (spell!=WizardSpell516::Wind) return 0.1f;
        const unsigned phase=windTick%300;
        if (phase<120) return 0.1f;
        if (phase<150) return 0.1f+0.6f*(phase-120)/30.0f;
        if (phase<270) return 0.7f;
        return 0.7f-0.6f*(phase-270)/30.0f;
    }
};
