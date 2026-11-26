#pragma once

#include "jump_pad.hpp"

struct BunkerJumpy {
    static constexpr float LaunchDistance = 1024.0f;
    static constexpr unsigned FlightTicks = JumpRun::FlightTicks;

    enum class Event { Grounded, Takeoff, Airborne, WallStuck, OutOfBounds };

    Vec2 lastGroundPosition{};
    float charge = 0;
    unsigned flightTicks = 0;
    bool enabled = false;
    bool landing = false;

    Event step(Game& game, const Course& course, const TileField& field, float surfaceDamping) {
        if (!enabled) return Event::Grounded;
        if (landing) {
            landing = false;
            // The PC restores the ground collision mask on landing.
            if (length(field.recoverCircle(game.position, 0.1f)) > 0.05f) return Event::WallStuck;
        }
        if (flightTicks) {
            advanceFlight(game);
            return JumpRun{}.outside(game.position, course) ? Event::OutOfBounds : Event::Airborne;
        }
        const float speed = length(game.velocity) * surfaceDamping;
        if (speed <= course.stopSpeed) return Event::Grounded;
        charge += length(game.position - lastGroundPosition);
        lastGroundPosition = game.position;
        if (charge < LaunchDistance) return Event::Grounded;
        charge -= LaunchDistance;
        const float loss = game.modifierKinesis?0:course.maxDrag * std::pow(course.frictionCoeff / speed, course.frictionPow);
        game.velocity = game.velocity * (surfaceDamping * (1.0f - loss));
        flightTicks = FlightTicks;
        advanceFlight(game);
        return JumpRun{}.outside(game.position, course) ? Event::OutOfBounds : Event::Takeoff;
    }

private:
    void advanceFlight(Game& game) {
        game.position = game.position + game.velocity * (1.0f / 60.0f);
        landing = --flightTicks == 0;
    }
};
