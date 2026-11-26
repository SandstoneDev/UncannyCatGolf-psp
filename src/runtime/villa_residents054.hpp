#pragma once

#include "villa_costumes054.hpp"
#include <cmath>

namespace VillaCostumes {

struct ResidentOffset {
    float x = 0;
    float y = 0;
    float rotation = 0;
};

class ResidentsMotion {
public:
    void reset(VillaUnlocks::Flags flags, uint32_t seed) {
        *this = {};
        for (const Id id : ResidentOrder) {
            auto& state = motions[index(id)];
            state.random = mix(seed + index(id));
            if (!available(id, flags)) continue;
            const Row& resident = row(id);
            if (resident.behavior == Behavior::WalkAndJump && resident.speedX > 0) {
                state.active = true;
                startWalk(resident, state);
            } else if (resident.behavior == Behavior::Ghost) {
                state.active = true;
                state.ghostSeed = random01(state) * 1000000.0f;
            } else if (resident.behavior == Behavior::Fight && available(resident.opponent, flags)) {
                auto& opponent = motions[index(resident.opponent)];
                if (!state.active && !opponent.active && pairCount < 3) {
                    state.active = opponent.active = true;
                    fights[pairCount].attacker = id;
                    startFight(fights[pairCount++]);
                }
            }
        }
        advance(0);
    }

    void advance(float seconds) {
        if (!std::isfinite(seconds) || seconds < 0) return;
        const float dt = std::min(seconds, 1.0f);
        for (unsigned i = 0; i < Count; ++i) {
            auto& state = motions[i];
            if (!state.active) continue;
            const Row& resident = Rows[i];
            if (resident.behavior == Behavior::WalkAndJump) {
                updateWalk(resident, state, dt);
            } else if (resident.behavior == Behavior::Ghost) {
                state.elapsed += dt;
                state.pose.x = smoothNoise(state.ghostSeed + state.elapsed * resident.speedX) * resident.moveX * 0.5f;
                state.pose.y = smoothNoise(state.ghostSeed - state.elapsed * resident.speedY) * resident.moveY * 0.5f;
            }
        }
        for (unsigned i = 0; i < pairCount; ++i) updateFight(fights[i], dt);
    }

    ResidentOffset offset(Id id) const {
        return validPreference(index(id)) ? motions[index(id)].pose : ResidentOffset{};
    }

private:
    struct Motion {
        ResidentOffset pose{}, from{}, to{};
        float elapsed = 0, walk = 0, rise = 0, fall = 0, ghostSeed = 0;
        uint32_t random = 0;
        bool active = false;
    };

    struct Fight {
        Id attacker = Id::Count;
        float elapsed = 0, attackTime = 0, returnTime = 0;
        ResidentOffset reach{}, push{};
    };

    Motion motions[Count]{};
    Fight fights[3]{};
    unsigned pairCount = 0;

    static constexpr unsigned index(Id id) { return static_cast<unsigned>(id); }

    static uint32_t mix(uint32_t x) {
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        return x ^ (x >> 16);
    }

    static float random01(Motion& state) {
        state.random += 0x9e3779b9u;
        return (mix(state.random) >> 8) * (1.0f / 16777216.0f);
    }

    static float lattice(int x) {
        return (mix(static_cast<uint32_t>(x)) >> 8) * (2.0f / 16777216.0f) - 1.0f;
    }

    // Godot uses FastNoiseLite SimplexSmooth. Smooth value noise keeps the same
    // normalized bounds and per-axis speeds without adding an engine dependency.
    static float smoothNoise(float x) {
        x *= 0.01f;
        const int n = static_cast<int>(std::floor(x));
        const float t = x - n;
        const float eased = t * t * t * (t * (t * 6 - 15) + 10);
        return lerp(lattice(n), lattice(n + 1), eased);
    }

    static float lerp(float a, float b, float t) { return a + (b - a) * t; }

    static ResidentOffset scale(ResidentOffset value, float factor) {
        return {value.x * factor, value.y * factor, 0};
    }

    static float ratio(float elapsed, float duration) {
        return duration > 0 ? std::clamp(elapsed / duration, 0.0f, 1.0f) : 1.0f;
    }

    static float cubicOut(float t) {
        const float remaining = 1 - t;
        return 1 - remaining * remaining * remaining;
    }

    static float cubicInOut(float t) {
        if (t < 0.5f) return 4 * t * t * t;
        const float remaining = 1 - t;
        return 1 - 4 * remaining * remaining * remaining;
    }

    static float backIn(float t) { return t * t * (2.70158f * t - 1.70158f); }

    static float bounceOut(float t) {
        constexpr float span = 2.75f;
        constexpr float factor = 7.5625f;
        if (t < 1 / span) return factor * t * t;
        if (t < 2 / span) {
            t -= 1.5f / span;
            return factor * t * t + 0.75f;
        }
        if (t < 2.5f / span) {
            t -= 2.25f / span;
            return factor * t * t + 0.9375f;
        }
        t -= 2.625f / span;
        return factor * t * t + 0.984375f;
    }

    static void startWalk(const Row& resident, Motion& state) {
        state.from = state.to;
        state.to.x = (random01(state) - 0.5f) * resident.moveX;
        state.to.y = (random01(state) - 0.5f) * resident.moveY;
        state.walk = std::hypot(state.to.x - state.from.x, state.to.y - state.from.y) / resident.speedX;
        state.fall = resident.jumpTime * 0.5f;
        state.rise = random01(state) < resident.jumpChance ? state.fall : 0.0f;
    }

    static void updateWalk(const Row& resident, Motion& state, float dt) {
        state.elapsed += dt;
        float cycleLength = state.walk + state.rise + state.fall + resident.wait;
        while (cycleLength > 0 && state.elapsed >= cycleLength) {
            state.elapsed -= cycleLength;
            // In the original the downward tween also runs when the rise was skipped.
            state.to.y += state.rise == 0 && state.fall > 0 ? resident.jumpHeight : 0;
            startWalk(resident, state);
            cycleLength = state.walk + state.rise + state.fall + resident.wait;
        }
        const float t = state.elapsed;
        if (t < state.walk) {
            constexpr float pi = 3.14159265f;
            const float path = (1.0f - std::cos(pi * ratio(t, state.walk))) * 0.5f;
            const float cycle = t * (resident.speedX / 32.0f);
            const float tilt = -0.0872665f * std::sin(2 * pi * cycle);
            const float bob = -4.0f + 4.0f * std::cos(4 * pi * cycle - pi);
            state.pose = {lerp(state.from.x, state.to.x, path), lerp(state.from.y, state.to.y, path) + bob, tilt};
        } else if (t < state.walk + state.rise) {
            state.pose = {state.to.x, state.to.y - resident.jumpHeight * cubicOut(ratio(t - state.walk, state.rise)), 0};
        } else {
            const float start = state.rise > 0 ? state.to.y - resident.jumpHeight : state.to.y;
            const float landing = state.fall > 0 ? resident.jumpHeight * bounceOut(ratio(t - state.walk - state.rise, state.fall)) : 0;
            state.pose = {state.to.x, start + landing, 0};
        }
    }

    void startFight(Fight& fight) {
        const Row& attacker = row(fight.attacker);
        const Row& defender = row(attacker.opponent);
        const float x = static_cast<float>(defender.x - attacker.x);
        const float y = static_cast<float>(defender.y - attacker.y);
        const float distance = std::hypot(x, y);
        const float power = lerp(attacker.attackPowerMin, attacker.attackPowerMax, random01(motions[index(fight.attacker)]));
        fight.reach = {x, y, 0};
        fight.push = distance > 0 ? scale(fight.reach, power / distance) : ResidentOffset{};
        fight.attackTime = attacker.attackSpeed > 0 ? distance / attacker.attackSpeed : 0;
        fight.returnTime = std::max(2 * fight.attackTime, 0.5f);
    }

    void updateFight(Fight& fight, float dt) {
        fight.elapsed += dt;
        float total = fight.attackTime + fight.returnTime + 1.0f;
        while (fight.elapsed >= total) {
            fight.elapsed -= total;
            fight.attacker = row(fight.attacker).opponent;
            startFight(fight);
            total = fight.attackTime + fight.returnTime + 1.0f;
        }
        auto& attacker = motions[index(fight.attacker)].pose;
        auto& defender = motions[index(row(fight.attacker).opponent)].pose;
        if (fight.elapsed < fight.attackTime) {
            attacker = scale(fight.reach, backIn(ratio(fight.elapsed, fight.attackTime)));
            defender = {};
            return;
        }
        const float retreat = fight.elapsed - fight.attackTime;
        attacker = scale(fight.reach, 1.0f - cubicInOut(ratio(retreat, fight.attackTime * 2.0f)));
        const float knockback = retreat < fight.returnTime ? cubicOut(ratio(retreat, 0.5f)) :
                                1.0f - ratio(retreat - fight.returnTime, 1.0f);
        defender = scale(fight.push, knockback);
    }
};

} // namespace VillaCostumes
