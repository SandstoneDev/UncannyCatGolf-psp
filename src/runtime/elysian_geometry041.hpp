#pragma once

#include "baddie_run.hpp"
#include "elysian_horse039.hpp"
#include "jump_pad.hpp"
#include "stop_tokens.hpp"

// Godot instances exported from w5/5_10.tscn and w5/5_11.tscn.

inline constexpr HorseCourse039 ElysianHorse510{
    384.0f, {256.0f, 0.0f}, {-1.0f, 1.0f}, 20, 150.0f, 300, {{
        {{-112.0f, -320.0f}, -1, true},
        {{-56.0f, -320.0f}, -1, true},
        {{0.0f, -320.0f}, -1, true},
        {{56.0f, -320.0f}, -1, true},
        {{112.0f, -320.0f}, -1, true},
        {{320.0f, -112.0f}, -1, true},
        {{320.0f, -56.0f}, -1, true},
        {{320.0f, 0.0f}, -1, true},
        {{320.0f, 56.0f}, -1, true},
        {{320.0f, 112.0f}, -1, true},
        {{112.0f, 320.0f}, -1, true},
        {{56.0f, 320.0f}, -1, true},
        {{0.0f, 320.0f}, -1, true},
        {{-56.0f, 320.0f}, -1, true},
        {{-112.0f, 320.0f}, -1, true},
        {{-320.0f, 112.0f}, -1, true},
        {{-320.0f, 56.0f}, -1, true},
        {{-320.0f, 0.0f}, -1, true},
        {{-320.0f, -56.0f}, -1, true},
        {{-320.0f, -112.0f}, -1, true},
    }}
};

inline constexpr HorseCourse039 ElysianHorse511{
    512.0f, {0.0f, 0.0f}, {1.0f, -0.3f}, 5, 150.0f, 300, {{
        {{288.0f, -448.0f}, -1, true},
        {{416.0f, 448.0f}, -1, true},
        {{-416.0f, 352.0f}, -1, true},
        {{-256.0f, -288.0f}, -1, true},
        {{0.0f, 0.0f}, -1, true},
    }}
};

inline constexpr StopTokenStage ElysianStop510{4, 0, {
        {-320.0f, -320.0f},
        {320.0f, -320.0f},
        {320.0f, 320.0f},
        {-320.0f, 320.0f},
    }, {},
    {0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu}
};

inline constexpr MotionTokenStage ElysianSpeed510{36, 0, {
        {-32.0f, -32.0f},
        {-96.0f, -96.0f},
        {-96.0f, -160.0f},
        {-32.0f, -160.0f},
        {-32.0f, -96.0f},
        {-96.0f, -32.0f},
        {-160.0f, -160.0f},
        {-160.0f, -96.0f},
        {-160.0f, -32.0f},
        {160.0f, -32.0f},
        {96.0f, -96.0f},
        {96.0f, -160.0f},
        {160.0f, -160.0f},
        {160.0f, -96.0f},
        {96.0f, -32.0f},
        {32.0f, -160.0f},
        {32.0f, -96.0f},
        {32.0f, -32.0f},
        {-32.0f, 160.0f},
        {-96.0f, 96.0f},
        {-96.0f, 32.0f},
        {-32.0f, 32.0f},
        {-32.0f, 96.0f},
        {-96.0f, 160.0f},
        {-160.0f, 32.0f},
        {-160.0f, 96.0f},
        {-160.0f, 160.0f},
        {160.0f, 160.0f},
        {96.0f, 96.0f},
        {96.0f, 32.0f},
        {160.0f, 32.0f},
        {160.0f, 96.0f},
        {96.0f, 160.0f},
        {32.0f, 32.0f},
        {32.0f, 96.0f},
        {32.0f, 160.0f},
    }, {},
    {0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu}
};

inline constexpr JumpStage ElysianJump511{5, {
    {{-288.0f, -448.0f}, 0.0f, 0.0f},
    {{-192.0f, 0.0f}, 0.0f, 0.0f},
    {{0.0f, -192.0f}, 1.5708f, 0.0f},
    {{192.0f, 0.0f}, 3.14159f, 0.0f},
    {{0.0f, 192.0f}, 4.71239f, 0.0f},
}};

inline constexpr BaddieStage ElysianCars511{0, 2, 640.0f, 0xffffe2d9u, {
    {{-576.0f, 128.0f}, {200.0f, 0.0f}, 60, 60, 100},
    {{576.0f, -128.0f}, {-200.0f, 0.0f}, 60, 60, 100},
}};
