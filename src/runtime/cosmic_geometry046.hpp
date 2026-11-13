#pragma once

#include "game.hpp"
#include "jump_pad.hpp"
#include "key_doors.hpp"
#include "stop_tokens.hpp"

inline constexpr JumpStage CosmicJump602{1, {{{256.0f, 208.0f}, 3.14159f, 0.0f}}};
inline constexpr MotionTokenStage CosmicSpeed602{1, 0, {{-272.0f, 208.0f}}, {}, {0xffffffffu}};
inline constexpr JumpStage CosmicJump603{1, {{{-384.0f, 16.0f}, 1.5708f, 0.0f}}};
inline constexpr MotionTokenStage CosmicSpeed603{2, 0, {{-160.0f, -400.0f}, {-224.0f, 32.0f}}, {}, {0xffffffffu, 0xffffffffu}};
inline constexpr StopTokenStage CosmicStop603{3, 0, {{-16.0f, -144.0f}, {-176.0f, 384.0f}, {-224.0f, 128.0f}}, {}, {0xffffffffu, 0xffffffffu, 0xffffffffu}};
inline constexpr uint32_t CosmicKeyColor602=0xff95ff80u;
inline constexpr KeyDoorStage CosmicKeys602{
    1, 1, {{{32.0f, 0.0f}, CosmicKeyColor602}}, {{{256.0f, -32.0f}, {64.0f, 32.0f}}}, {CosmicKeyColor602}, {},
};
inline constexpr Vec2 CosmicChuckles602[]={
    {-80.0f, 144.0f},
    {-112.0f, 112.0f},
    {-144.0f, 144.0f},
    {-112.0f, 304.0f},
    {-144.0f, 272.0f},
    {-48.0f, 112.0f},
    {-16.0f, 144.0f},
    {16.0f, 112.0f},
    {48.0f, 144.0f},
    {80.0f, 112.0f},
    {112.0f, 144.0f},
    {144.0f, 112.0f},
    {144.0f, 304.0f},
    {112.0f, 272.0f},
    {80.0f, 304.0f},
    {48.0f, 272.0f},
    {16.0f, 304.0f},
    {-16.0f, 272.0f},
    {-48.0f, 304.0f},
    {-80.0f, 272.0f},
};
inline constexpr SolidBlock CosmicSwitches603[]={
    {{-144.0f, 48.0f}, {16.0f, 16.0f}},
    {{-144.0f, 16.0f}, {16.0f, 16.0f}},
};
