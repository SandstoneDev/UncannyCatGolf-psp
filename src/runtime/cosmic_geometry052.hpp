#pragma once

#include "baddie_run.hpp"
#include "chuckle_run.hpp"
#include "golf.hpp"
#include "jump_pad.hpp"
#include "key_doors.hpp"
#include "portal.hpp"
#include "smiley_ring.hpp"
#include "stop_tokens.hpp"

inline constexpr Vec2 CosmicFakeGoals614[]={
    {-64.0f, -320.0f},
    {-256.0f, -64.0f},
    {192.0f, -288.0f},
    {-48.0f, -96.0f},
    {-320.0f, 192.0f},
    {-128.0f, 64.0f},
    {64.0f, 320.0f},
    {-256.0f, 320.0f},
    {160.0f, 64.0f},
    {-32.0f, 224.0f},
    {256.0f, 192.0f},
    {320.0f, -128.0f},
    {192.0f, -48.0f},
};
inline constexpr float CosmicFakeGoalRadius614=16.0f;
inline constexpr unsigned CosmicFakeGoalScore614=500;
inline constexpr Vec2 CosmicNukes614[]={
    {192.0f, 320.0f},
    {-128.0f, 184.0f},
    {48.0f, -128.0f},
};
inline constexpr ChuckleSpawner CosmicChuckles614[]={
    {{112.0f, -320.0f}, 300, 300, 10},
};
inline constexpr float CosmicChuckleDespawn614=512.0f;
inline constexpr CourseArea CosmicWater614{4, {{-384.0f, -384.0f}, {384.0f, -384.0f}, {384.0f, 384.0f}, {-384.0f, 384.0f}}};
inline constexpr StopTokenStage CosmicStop614{1, 0, {{320.0f, 16.0f}}, {}, {0xffffffffu}};
inline constexpr MotionTokenStage CosmicSpeed614{4, 0, {{0.0f, 0.0f}, {-224.0f, 32.0f}, {-160.0f, -288.0f}, {216.0f, 256.0f}}, {}, {0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu}};
inline constexpr StopTokenStage CosmicStop615{2, 0, {{-176.0f, -152.0f}, {-80.0f, -1704.0f}}, {}, {0xffffffffu, 0xffffffffu}};
inline constexpr MotionTokenStage CosmicSpeed615{1, 0, {{0.0f, 0.0f}}, {}, {0xffffffffu}};
inline constexpr PortalNode CosmicPortalNodes615[]={
    {{136.0f, -920.0f}, 1u},
    {{-128.0f, -968.0f}, PortalNode::NoTarget},
    {{-64.0f, -1536.0f}, 3u},
    {{-80.0f, -1704.0f}, PortalNode::NoTarget},
};
inline constexpr PortalStage CosmicPortals615{CosmicPortalNodes615, std::size(CosmicPortalNodes615)};
inline constexpr uint32_t CosmicPortalColors615[]={
    0xffffffffu,
    0xffffffffu,
    0xffff005eu,
    0xffff005eu,
};
inline constexpr JumpPad CosmicJump615[]={
    {{-232.0f, -1656.0f}, 3.14159f, 0.0f},
    {{-200.0f, -1816.0f}, 3.14159f, 0.0f},
    {{-168.0f, -1816.0f}, 3.14159f, 0.0f},
    {{-136.0f, -1816.0f}, 3.14159f, 0.0f},
    {{-104.0f, -1816.0f}, 3.14159f, 0.0f},
    {{-72.0f, -1816.0f}, 3.14159f, 0.0f},
    {{-40.0f, -1816.0f}, 3.14159f, 0.0f},
    {{-8.0f, -1816.0f}, 3.14159f, 0.0f},
    {{24.0f, -1816.0f}, 3.14159f, 0.0f},
    {{-232.0f, -1688.0f}, 3.14159f, 0.0f},
    {{-232.0f, -1720.0f}, 3.14159f, 0.0f},
    {{-232.0f, -1752.0f}, 3.14159f, 0.0f},
    {{-232.0f, -1784.0f}, 3.14159f, 0.0f},
    {{-232.0f, -1816.0f}, 3.14159f, 0.0f},
    {{-232.0f, -1848.0f}, 3.14159f, 0.0f},
    {{-232.0f, -1880.0f}, 3.14159f, 0.0f},
    {{-232.0f, -1912.0f}, 3.14159f, 0.0f},
    {{-232.0f, -1944.0f}, 3.14159f, 0.0f},
    {{-232.0f, -1976.0f}, 3.14159f, 0.0f},
    {{184.0f, -1656.0f}, 0.0f, 0.0f},
    {{184.0f, -1688.0f}, 0.0f, 0.0f},
    {{184.0f, -1720.0f}, 0.0f, 0.0f},
    {{184.0f, -1752.0f}, 0.0f, 0.0f},
    {{184.0f, -1784.0f}, 0.0f, 0.0f},
    {{184.0f, -1816.0f}, 0.0f, 0.0f},
    {{184.0f, -1848.0f}, 0.0f, 0.0f},
    {{184.0f, -1880.0f}, 0.0f, 0.0f},
    {{184.0f, -1912.0f}, 0.0f, 0.0f},
    {{184.0f, -1944.0f}, 0.0f, 0.0f},
    {{184.0f, -1976.0f}, 0.0f, 0.0f},
};
inline constexpr BaddieStage CosmicCars615{0, 4, 384.0f, 0xffffffffu, {
    {{-216.0f, -280.0f}, {100.0f, 0.0f}, 1, 60, 100},
    {{216.0f, -280.0f}, {-100.0f, 0.0f}, 1, 55, 100},
    {{232.0f, -608.0f}, {-400.0f, 0.0f}, 1, 2, 15},
    {{136.0f, -664.0f}, {0.0f, -200.0f}, 60, 60, 100},
}};
inline constexpr bool CosmicCarIgnoreY615[]={
    false,
    false,
    true,
    true,
};
inline constexpr float CosmicCarRotation615[]={
    0.0f,
    180.0f,
    180.0f,
    -90.0f,
};
inline constexpr KeyDoorStage CosmicKeys615{2, 2, {{{0.0f, -360.0f}, 0xffff0071u}, {{-64.0f, -1456.0f}, 0xff8ceeffu}}, {{{0.0f, -416.0f}, {192.0f, 16.0f}}, {{-64.0f, -1488.0f}, {176.0f, 16.0f}}}, {0xffff0071u, 0xff8ceeffu}, {}};
inline constexpr SmileyStage CosmicKeyRing615{{64.0f, -1240.0f}, 24.0f, 16.0f, 1, 240, 0, 110, 83, 0, {{-64.0f, -1456.0f}}};
inline constexpr uint32_t CosmicKeyRingTint615=0xff8ceeffu;
inline constexpr float CosmicCameraFloor615=-1824.0f;
