#pragma once

#include "game.hpp"

enum class ControlEvent { None, Shot, Restart };

class Controls {
public:
    Vec2 drag{};
    bool aiming = false;

    ControlEvent update(Game& game, const Course& course, Vec2 stick, bool cross, bool triangle,bool allowDrift=false) {
        const bool pressed = cross && !lastCross;
        const bool released = !cross && lastCross;
        const bool restart = triangle && !lastTriangle;
        lastCross = cross;
        lastTriangle = triangle;

        if (restart) {
            game.restart();
            aiming = false;
            drag = {};
            return ControlEvent::Restart;
        }
        if (pressed && (!game.moving() || allowDrift) && !game.won) {
            aiming = true;
            drag = {};
        }
        if (aiming && cross) {
            constexpr float deadzone = 0.18f;
            const float magnitude = length(stick);
            if (magnitude > deadzone) {
                const float power = (std::min(magnitude, 1.0f) - deadzone) / (1.0f - deadzone);
                drag = stick * (course.maxDrag * power / magnitude);
            } else drag = {};
        }
        if (aiming && released) {
            aiming = false;
            if (game.hit(drag,allowDrift)) return ControlEvent::Shot;
        }
        return ControlEvent::None;
    }

private:
    bool lastCross = false;
    bool lastTriangle = false;
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
