#pragma once
#include "attract_data.hpp"
#include "game.hpp"

namespace Attract {
inline Vec2 direction(const Shot& shot) {
    // The original Vector2 uses float32 decimals. Double division then casting
    // preserves those original values, including .65, 1.1 and negative values.
    return {static_cast<float>(shot.dx100/100.0),static_cast<float>(shot.dy100/100.0)};
}
inline constexpr unsigned course(const Recording& data) {
    return data.world?5+(data.world-1)*18+data.level:data.level;
}
inline unsigned eligibleCount(unsigned world) {
    world=std::max(1u,world);
    unsigned count=0;
    for (const auto& data:recordings) count+=data.world<=world;
    return count;
}
template<class Random> unsigned choose(unsigned world,unsigned previous,Random random) {
    // Catalog is ordered by world; source pick_random rejects only the same resource.
    const unsigned count=eligibleCount(world);
    unsigned selected;
    do { selected=random()%count; } while (selected==previous);
    return selected;
}
struct Timer {
    unsigned value=0;
    bool tick(unsigned limit) {
        if (value>limit) {value=0;return true;}
        ++value;return false;
    }
};
struct Event { bool pickup=false,shot=false; };
class Playback {
public:
    void begin(unsigned selected) { index=selected;phase=Phase::Spawn;scale=.003f;wait=0; }
    bool spawning() const {return phase==Phase::Spawn;}
    float renderScale() const {return scale/.064f;}
    unsigned idleTicks() const {return wait;}
    void winTick() {if (scale>.001f) scale-=.001f;}
    Event tick(Game& game,bool airborne,bool terminal) {
        Event event{};
        if (terminal) return event;
        if (phase==Phase::Spawn) {
            scale+=.001f;
            if (scale<.064f) return event;
            scale=.064f;phase=Phase::Idle;event.pickup=true;
            // transition_to calls the new state's update in this same callback.
        } else if (phase==Phase::Moving && !airborne && length(game.velocity)<=20.0f) {
            game.velocity={};phase=Phase::Idle;wait=0;event.pickup=true;
        }
        if (phase!=Phase::Idle) return event;
        const auto& data=recordings[index];
        if (game.strokes>=data.count) return event;
        const auto& shot=shots[data.first+game.strokes];
        if (wait>shot.wait) {
            const auto vector=direction(shot);
            const float magnitude=length(vector);
            game.velocity={vector.x/magnitude*shot.power,vector.y/magnitude*shot.power};
            ++game.strokes;
            wait=0;phase=Phase::Moving;event.shot=true;
            // Caller steps existing PMoving friction/collision immediately.
        } else ++wait;
        return event;
    }
private:
    enum class Phase {Spawn,Idle,Moving};
    Phase phase=Phase::Spawn;
    unsigned index=0,wait=0;
    float scale=.003f;
};
} // namespace Attract
