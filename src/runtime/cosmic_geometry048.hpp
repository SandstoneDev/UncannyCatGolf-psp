#pragma once

#include "game.hpp"
#include "key_doors.hpp"

inline constexpr SolidBlock CosmicSwitches607[]={
    {{-352.0f, 288.0f}, {16.0f, 16.0f}},
    {{352.0f, -48.0f}, {16.0f, 16.0f}},
};
inline constexpr uint32_t CosmicKeyColor607=0xffff00ffu;
inline constexpr KeyDoorStage CosmicKeys607{
    1, 1, {{{192.0f, 200.0f}, CosmicKeyColor607}}, {{{192.0f, -120.0f}, {144.0f, 24.0f}}}, {CosmicKeyColor607}, {},
};
inline constexpr unsigned CosmicDoorScore607=200;
inline constexpr Vec2 CosmicGridOffset606={8.0f, 0.0f};
inline constexpr unsigned CosmicRhythmBpm048=135;
inline constexpr unsigned CosmicRhythmTriggerBeats048=2;
inline constexpr float CosmicRhythmLoopOffset048=14.22f;
inline constexpr Vec2 CosmicSewer607[]={
    {-336.0f, -144.0f},
    {0.0f, -144.0f},
    {0.0f, -336.0f},
    {-336.0f, -336.0f},
};
inline constexpr uint32_t CosmicGhostTints606[]={
    0xfffca3ffu,
    0xfffca3ffu,
};
inline constexpr uint32_t CosmicGhostTints607[]={
    0xfffca3ffu,
    0xfffcb3ffu,
    0xfffcb3ffu,
    0xfffcb3ffu,
};
