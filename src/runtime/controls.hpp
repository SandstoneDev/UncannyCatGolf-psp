#pragma once

#include "game.hpp"
#include "campaign_modifiers.hpp"
#include "shot_settings.hpp"

enum class ControlEvent { None, Shot, Restart };

class Controls {
public:
    Vec2 drag{};
    bool aiming = false;

    // Round supplies its fixed-step clock: render calls do not age the buffer.
    void setPhysicsFrame(unsigned frame) { physicsFrame=frame; }
    // Optional real spawn gate. Main currently has no timed PSpawn input gate.
    void setSpawning(bool active) { spawning=active; }
    // Drop transient shots and require a physical release after held UI input.
    void cancel(bool held=false) {
        aiming=bufferAiming=buffered=false;
        drag=rawDrag={};
        lastCross=held;
        blocked=held;
        motionActive=false;
    }

    // Call at EACH fixed-step idle transition, before another physics step.
    // Returns the same event as a direct shot so callers retain shot side effects.
    ControlEvent consumeBuffered(Game& game,const Course& course,unsigned frame,
                                 bool allowDrift=false,CampaignModifiers::State* modifiers=nullptr) {
        // Observe every fixed step, even without a pending shot: input can be
        // absent for several ticks while a new moving state begins.
        if (spawning || game.moving()) {
            if (!motionActive) { motionActive=true;motionFrame=frame; }
            return ControlEvent::None;
        }
        motionActive=false;
        if (blocked) return ControlEvent::None;
        // PIdle.enter gives a still-held click priority over a pending release.
        // This must happen inside batched physics steps, before the next render.
        if (lastCross && bufferAiming) { aiming=true;bufferAiming=false;return ControlEvent::None; }
        if (!buffered || lastCross) return ControlEvent::None;
        const Vec2 shot=bufferedDrag;
        buffered=false; // Invalid/weak/expired buffers are also consumed only once.
        if (game.won || unsigned(frame-bufferedFrame)>=12) return ControlEvent::None;
        if (modifiers?modifiers->shoot(game,course,shot,allowDrift):game.hit(shot,allowDrift)) {
            motionActive=true;motionFrame=frame+1;
            return ControlEvent::Shot;
        }
        return ControlEvent::None;
    }

    ControlEvent update(Game& game, const Course& course, Vec2 stick, bool cross, bool triangle,bool allowDrift=false,CampaignModifiers::State* modifiers=nullptr,const ShotSettings& settings={}) {
        const bool pressed = cross && !lastCross;
        const bool released = !cross && lastCross;
        const bool restart = triangle && !lastTriangle;
        lastCross = cross;
        lastTriangle = triangle;

        if (restart) {
            game.restart();
            cancel(cross);
            return ControlEvent::Restart;
        }
        if (blocked) { if (!cross) blocked=false;return ControlEvent::None; }
        if (game.won) { cancel(cross);return ControlEvent::None; }
        const bool moving=game.moving() || spawning;
        if (moving && !motionActive) { motionActive=true;motionFrame=physicsFrame; }
        if (!moving) motionActive=false;
        if (pressed) buffered=false; // player.gd clears the old buffer on every new click.
        if (!settings.buffer) buffered=false;
        const bool burst=modifiers && modifiers->enabled(CampaignMods::Burst);
        const bool god=modifiers && modifiers->enabled(CampaignMods::God);
        if (!moving && bufferAiming && cross) { aiming=true;bufferAiming=false; }
        if (pressed && !spawning && (!game.moving() || allowDrift || burst)) {
            aiming = true;
            drag=rawDrag={};
        } else if (pressed && settings.buffer && moving) {
            bufferAiming=true;
            drag=rawDrag={};
        }
        if ((aiming || bufferAiming) && cross) {
            constexpr float deadzone = 0.18f;
            const float magnitude = length(stick);
            if (magnitude > deadzone) {
                const float power = (std::min(magnitude, 1.0f) - deadzone) / (1.0f - deadzone);
                const Vec2 target=stick * (course.maxDrag * power / magnitude);
                rawDrag=god?rawDrag+target*(1.f/8):target;
            } else if (!god) rawDrag = {};
            drag=rawDrag;
            // Keep the accepted default float operation sequence unchanged.
            if (settings.sensitivity!=1.f) drag=drag*settings.sensitivity;
            if (settings.invert) drag=drag*-1.f;
            if (!god && settings.sensitivity!=1.f) {
                const float power=length(drag);
                if (power>course.maxDrag) drag=drag*(course.maxDrag/power);
            }
        }
        if (released && (aiming || bufferAiming)) {
            if (settings.buffer && moving && unsigned(physicsFrame-motionFrame)>0 &&
                unsigned(physicsFrame-motionFrame)<0x80000000u) {
                bufferedDrag=drag;bufferedFrame=physicsFrame;buffered=true;
            }
            bufferAiming=false;
            if (aiming) {
                aiming=false;
                if (modifiers?modifiers->shoot(game,course,drag,allowDrift):game.hit(drag,allowDrift)) {
                    // A successful immediate/Burst shot owns this release.
                    buffered=false;
                    if (!moving) { motionActive=true;motionFrame=physicsFrame+1; }
                    return ControlEvent::Shot;
                }
            }
        }
        if (!moving && !cross) return consumeBuffered(game,course,physicsFrame,allowDrift,modifiers);
        return ControlEvent::None;
    }

private:
    Vec2 rawDrag{},bufferedDrag{};
    unsigned physicsFrame=0,bufferedFrame=0,motionFrame=0;
    bool lastCross = false;
    bool lastTriangle = false;
    bool bufferAiming=false,buffered=false,spawning=false,blocked=false,motionActive=false;
};

class HoldToSkip {
public:
    static constexpr unsigned RequiredTicks=31;

    bool update(bool held) {
        if (!held) {
            ticks=0;
            blocked=false;
        } else if (!blocked && ++ticks==RequiredTicks) {
            blocked=true;
            return true;
        }
        return false;
    }

    void cancel() { ticks=0; blocked=true; }
    unsigned progress() const { return blocked?0:ticks; }

private:
    unsigned ticks=0;
    bool blocked=false;
};
