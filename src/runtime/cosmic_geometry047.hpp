#pragma once

#include "game.hpp"
#include "smiley_ring.hpp"
#include "stop_tokens.hpp"

inline constexpr SolidBlock CosmicSwitches604[]={
    {{16.0f, -256.0f}, {16.0f, 16.0f}},
    {{48.0f, -256.0f}, {16.0f, 16.0f}},
};
inline constexpr MotionTokenStage CosmicSpeed605{20, 0, {{-224.0f, -384.0f}, {-176.0f, -384.0f}, {-128.0f, -384.0f}, {-80.0f, -384.0f}, {-32.0f, -384.0f}, {16.0f, -384.0f}, {64.0f, -384.0f}, {112.0f, -384.0f}, {160.0f, -384.0f}, {208.0f, -384.0f}, {256.0f, -384.0f}, {304.0f, -384.0f}, {352.0f, -384.0f}, {-224.0f, 384.0f}, {-128.0f, 384.0f}, {208.0f, 384.0f}, {304.0f, 384.0f}, {640.0f, -32.0f}, {592.0f, -152.0f}, {608.0f, -56.0f}}, {}, {0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu}};
inline constexpr StopTokenStage CosmicStop605{2, 0, {{432.0f, 0.0f}, {-320.0f, 384.0f}}, {}, {0xffffffffu, 0xffffffffu}};
inline constexpr SmileyStage CosmicRings605[]={
    {{352.0f, 0.0f}, 24.0f, 10.0f, 14, 1800, 2000, 110, 83, 0,
     {{192.0f, 0.0f}, {-64.0f, -112.0f}, {-64.0f, 112.0f}, {-32.0f, -128.0f}, {-32.0f, -96.0f}, {-32.0f, 96.0f}, {-32.0f, 128.0f}, {224.0f, -16.0f}, {224.0f, 16.0f}, {-352.0f, 224.0f}, {-352.0f, 96.0f}, {-352.0f, -32.0f}, {-352.0f, -160.0f}, {0.0f, 384.0f}}},
};
inline constexpr uint32_t CosmicRingTint605=0xff8ceeffu;
inline constexpr uint32_t CosmicRingCoinTint605=0xff8ceeffu;
inline constexpr bool CosmicRingBell605=true;

inline constexpr unsigned CosmicWindPeriod605=270;
inline constexpr Vec2 CosmicWindVelocity605(unsigned tick) {
    tick%=CosmicWindPeriod605;
    if (tick<120) return {0.0f, 0.0f};
    if (tick<150) return {-5.0f, 0.0f};
    if (tick<180) return {-25.0f, 0.0f};
    if (tick<188) return {-5.0f, 0.0f};
    return {0.0f, 0.0f};
}
