#pragma once
#include "phone_pronouns054.hpp"
#include <cstdint>

namespace ActorPronouns054 {
// One byte per live pool slot. Each call to ready belongs to an actual spawn,
// including a spawn that reuses a slot at the same tick as its predecessor.
struct Tag { uint8_t value=0; };
enum class Kind { Ghost,Horse,Baddie,Dog,GoalFace };
constexpr int offset(Kind kind) {
    return kind==Kind::Horse ? -64 : kind==Kind::GoalFace ? -48 : -36;
}
struct Session {
    bool enabled=false;
    uint32_t state=0;
    void toggle(uint32_t seed) {
        if (!state) state=seed ? seed : 0x7a63b9e5u;
        enabled=!enabled;
    }

    unsigned random(unsigned maximum) {
        // xorshift32 never emits zero. Rejection keeps each inclusive draw uniform.
        const uint32_t n=maximum+1,limit=UINT32_MAX-UINT32_MAX%n;
        do {state^=state<<13;state^=state>>17;state^=state<<5;} while(state>limit);
        return (state-1)%n;
    }
    Tag ready(Kind kind=Kind::Baddie) {
        if (!enabled) return {};
        const auto label=PhonePronouns054::draw([this](unsigned maximum){return random(maximum);},
            kind!=Kind::GoalFace);
        return {static_cast<uint8_t>(1+static_cast<unsigned>(label))};
    }
};
inline Session session;
}
