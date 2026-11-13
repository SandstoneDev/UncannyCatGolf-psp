#pragma once

#include "baddie_run.hpp"
#include "chuckle_run.hpp"
#include "jump_pad.hpp"
#include "key_doors.hpp"
#include "park_late031.hpp"
#include "smiley_ring.hpp"
#include "stop_tokens.hpp"

inline constexpr unsigned CosmicRanks616[]={
    1,
    6500,
    8500,
    10500,
    12000,
};
inline constexpr JumpPad CosmicJump616[]={
    {{96.0f, -280.0f}, 1.0472f, 0.0f},
    {{160.0f, 32.0f}, 1.91986f, 0.0f},
};
inline constexpr MotionTokenStage CosmicSpeed616{2, 0, {{32.0f, -280.0f}, {160.0f, -24.0f}}, {}, {0xffffffffu, 0xffffffffu}};
inline constexpr KeyDoorStage CosmicKeys616{1, 1, {{{208.0f, -304.0f}, 0xffffff00u}}, {{{112.0f, 192.0f}, {16.0f, 64.0f}}}, {0xffffff00u}, {}};
inline constexpr ChuckleSpawner CosmicChuckles616[]={
    {{-336.0f, 64.0f}, 500, 1, 1},
};
inline constexpr float CosmicChuckleDespawn616=512.0f;
inline constexpr unsigned CosmicRanks617[]={
    1,
    10000,
    14000,
    18000,
    22500,
};
inline constexpr JumpPad CosmicJump617[]={
    {{152.0f, 192.0f}, 1.5708f, 0.0f},
};
inline constexpr MotionTokenStage CosmicSpeed617{12, 0, {{-144.0f, 304.0f}, {-144.0f, 256.0f}, {-144.0f, 208.0f}, {-144.0f, 160.0f}, {-144.0f, 112.0f}, {-144.0f, 64.0f}, {-144.0f, 16.0f}, {-144.0f, -32.0f}, {-144.0f, -80.0f}, {-144.0f, -128.0f}, {-144.0f, -176.0f}, {152.0f, 240.0f}}, {}, {0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu}};
inline constexpr StopTokenStage CosmicStop617{2, 0, {{-144.0f, -256.0f}, {152.0f, 576.0f}}, {}, {0xffffffffu, 0xffffffffu}};
inline constexpr KeyDoorStage CosmicKeys617{4, 5, {{{-384.0f, -128.0f}, 0xffffd480u}, {{512.0f, -528.0f}, 0xffffd480u}, {{-256.0f, -160.0f}, 0xffffd480u}, {{-256.0f, -96.0f}, 0xff95ff80u}}, {{{-384.0f, -48.0f}, {64.0f, 16.0f}}, {{512.0f, -432.0f}, {64.0f, 16.0f}}, {{-304.0f, -128.0f}, {16.0f, 64.0f}}, {{-464.0f, 32.0f}, {16.0f, 64.0f}}, {{-464.0f, -128.0f}, {16.0f, 64.0f}}}, {0xffffd480u, 0xffffd480u, 0xffffd480u, 0xffffd480u, 0xff95ff80u}, {}};
inline constexpr ChuckleSpawner CosmicChuckles617[]={
    {{320.0f, 304.0f}, 60, 180, 18},
};
inline constexpr float CosmicChuckleDespawn617=768.0f;
inline constexpr BaddieStage CosmicCars617{0, 2, 768.0f, 0xffffffffu, {
    {{-32.0f, -704.0f}, {0.0f, 200.0f}, 35, 150, 100},
    {{32.0f, 704.0f}, {0.0f, -200.0f}, 35, 150, 100},
}};
inline constexpr float CosmicCarRotation617[]={
    90.0f,
    270.0f,
};
inline constexpr SolidBlock CosmicSwitches617[]={
    {{336.0f, -240.0f}, {16.0f, 16.0f}},
    {{368.0f, -240.0f}, {16.0f, 16.0f}},
    {{272.0f, -176.0f}, {16.0f, 16.0f}},
    {{272.0f, -144.0f}, {16.0f, 16.0f}},
    {{336.0f, -80.0f}, {16.0f, 16.0f}},
    {{368.0f, -80.0f}, {16.0f, 16.0f}},
    {{432.0f, -144.0f}, {16.0f, 16.0f}},
    {{432.0f, -176.0f}, {16.0f, 16.0f}},
};
inline constexpr Vec2 CosmicNukes617[]={
    {272.0f, 160.0f},
};
inline constexpr DogGolfScene CosmicDog617{{336.0f, 160.0f}, {150.0f, 0.0f}, {544.0f, 384.0f}, 15.8f, 16.0f, 0.064f, 0.05f, 0.001f, true};
inline constexpr SmileyStage CosmicRing617{{352.0f, -344.0f}, 24.0f, 10.0f, 7, 1200, 5000, 110, 83, 0, {{168.0f, -344.0f}, {536.0f, -344.0f}, {536.0f, -160.0f}, {536.0f, 24.0f}, {352.0f, 24.0f}, {168.0f, 24.0f}, {168.0f, -160.0f}}};
inline constexpr uint32_t CosmicRingTint617=0xff8ceeffu;
inline constexpr bool CosmicRingBell617=true;
inline constexpr Vec2 CosmicFakeGoal617={-448.0f, 544.0f};
inline constexpr float CosmicFakeGoalRadius617=16.0f;
inline constexpr unsigned CosmicFakeGoalScore617=500;
inline constexpr uint32_t CosmicTrapTint617=0xff333340u;
inline constexpr Vec2 CosmicFinalGhost617={-769.0f, 530.0f};
inline constexpr float CosmicFinalGhostScale617=2.5f;
inline constexpr Vec2 CosmicGoalScale617={1.0f, 4.0f};
inline constexpr Vec2 CosmicMetronome617={-32.0f, 264.0f};
inline constexpr unsigned CosmicTriggerEvery617=2;
inline constexpr unsigned CosmicRanks618[]={
    1,
    7000,
    7500,
    8000,
    8300,
};
inline constexpr float CosmicRotationRates616[]={
    0.1f,
    -0.1f,
};
