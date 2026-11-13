#pragma once

#include "key_doors.hpp"
#include "stop_tokens.hpp"

inline constexpr KeyDoorStage FrontierKeys414{
    22,22,
    {
        {{-224,-32},0xffffe278u},
        {{224,-32},0xffffe278u},
        {{-160,176},0xffffe278u},
        {{-128,0},0xffffe278u},
        {{232,88},0xffffe278u},
        {{168,208},0xffffe278u},
        {{352,64},0xffffe278u},
        {{296,-8},0xffffe278u},
        {{-296,24},0xffffe278u},
        {{-296,160},0xffffe278u},
        {{-200,216},0xffffe278u},
        {{-96,248},0xffffe278u},
        {{248,176},0xffffe278u},
        {{-272,72},0xffffe278u},
        {{176,16},0xffffe278u},
        {{-232,152},0xffffe278u},
        {{336,104},0xffffe278u},
        {{-192,64},0xffffe278u},
        {{120,136},0xffffe278u},
        {{-96,112},0xffffe278u},
        {{88,24},0xffffe278u},
        {{-344,64},0xffffe278u},
    },
    {
        {{-128,-224},{32,32}},
        {{0,-224},{32,32}},
        {{128,-224},{32,32}},
        {{-64,-160},{32,32}},
        {{64,-160},{32,32}},
        {{-128,-96},{32,32}},
        {{0,-96},{32,32}},
        {{128,-96},{32,32}},
        {{-64,-240},{32,16}},
        {{-64,-208},{32,16}},
        {{-128,-176},{32,16}},
        {{-128,-144},{32,16}},
        {{-64,-112},{32,16}},
        {{-64,-80},{32,16}},
        {{0,-176},{32,16}},
        {{0,-144},{32,16}},
        {{64,-240},{32,16}},
        {{64,-208},{32,16}},
        {{64,-112},{32,16}},
        {{64,-80},{32,16}},
        {{128,-176},{32,16}},
        {{128,-144},{32,16}},
    },
    {
        0xffffe278u,0xffffe278u,0xffffe278u,0xffffe278u,0xffffe278u,0xffffe278u,
        0xffffe278u,0xffffe278u,0xffffe278u,0xffffe278u,0xffffe278u,0xffffe278u,
        0xffffe278u,0xffffe278u,0xffffe278u,0xffffe278u,0xffffe278u,0xffffe278u,
        0xffffe278u,0xffffe278u,0xffffe278u,0xffffe278u,
    },
    {},
};

inline constexpr KeyDoorStage FrontierKeys415{
    4,4,
    {
        {{64,-328},0xffffb3b3u},
        {{0,288},0xffc773ffu},
        {{304,192},0xff67cc5cu},
        {{352,-288},0xff73f3ffu},
    },
    {
        {{112,0},{48,16}},
        {{112,-192},{48,16}},
        {{-80,192},{48,16}},
        {{-240,192},{48,16}},
    },
    {
        0xffffb3b3u,0xff73f3ffu,0xff67cc5cu,0xffc773ffu,
    },
    {},
};

inline constexpr StopTokenStage FrontierStop415{
    1,0,{{112,152}},{},{0xffffffffu},
};
inline constexpr float FrontierStopRadius415=10.05f;
inline constexpr Vec2 FrontierCarCenter415{-352,232};
inline constexpr float FrontierBaddieDespawn415=512.0f;
