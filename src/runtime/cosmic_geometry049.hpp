#pragma once

#include "game.hpp"
#include "portal.hpp"
#include "stop_tokens.hpp"

inline constexpr Vec2 CosmicGhosts608[]={
    {-144.0f, -400.0f},
    {320.0f, -128.0f},
    {152.0f, 0.0f},
    {24.0f, 304.0f},
    {-192.0f, 416.0f},
    {176.0f, -400.0f},
    {-32.0f, 128.0f},
    {-64.0f, -128.0f},
    {360.0f, 80.0f},
    {256.0f, 400.0f},
};
inline constexpr uint32_t CosmicGhostTints608[]={
    0xfffca3ffu,
    0xfffcb3ffu,
    0xfffcb3ffu,
    0xfffcb3ffu,
    0xfffcb3ffu,
    0xfffcb3ffu,
    0xfffcb3ffu,
    0xfffcb3ffu,
    0xfffcb3ffu,
    0xfffcb3ffu,
};
inline constexpr uint32_t CosmicGhostMasks608[]={
    0x1fu,
    0x3e0u,
    0x3ffu,
};
inline constexpr PortalNode CosmicPortalNodes608[]={
    {{384.0f, -400.0f}, 1u},
    {{-64.0f, -264.0f}, PortalNode::NoTarget},
    {{320.0f, 392.0f}, 3u},
    {{-384.0f, -224.0f}, PortalNode::NoTarget},
};
inline constexpr PortalStage CosmicPortals608{CosmicPortalNodes608, std::size(CosmicPortalNodes608)};
inline constexpr StopTokenStage CosmicStop608{1, 0, {{128.0f, -264.0f}}, {}, {0xffffffffu}};
inline constexpr float CosmicStopRadius608=10.05f;
inline constexpr Vec2 CosmicSand608[]={
    {-448.0f, 448.0f},
    {-448.0f, 232.0f},
    {-408.0f, 248.0f},
    {-336.0f, 232.0f},
    {-296.0f, 240.0f},
    {-280.0f, 256.0f},
    {-264.0f, 296.0f},
    {-264.0f, 344.0f},
    {-248.0f, 368.0f},
    {-224.0f, 376.0f},
    {-200.0f, 360.0f},
    {-192.0f, 328.0f},
    {-192.0f, 448.0f},
};
inline constexpr float CosmicSandVelocityFactor608=0.85f;
inline constexpr uint32_t CosmicCoinMasks609[]={
    0x7u,
    0x78u,
    0x7fu,
};
inline constexpr uint32_t CosmicCoinTints609[]={
    0xffff00ffu,
    0xffff00ffu,
    0xffff00ffu,
    0xffffff00u,
    0xffffff00u,
    0xffffff00u,
    0xffffff00u,
};
