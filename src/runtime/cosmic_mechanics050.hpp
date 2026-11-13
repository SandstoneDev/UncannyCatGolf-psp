#pragma once

#include "baddie_run.hpp"
#include "jump_pad.hpp"
#include "key_doors.hpp"
#include "portal.hpp"
#include "smiley_ring.hpp"

inline constexpr uint32_t CosmicShade610=0xff333333u;
inline constexpr uint32_t CosmicShade611=0xff693737u;
inline constexpr uint32_t CosmicGreen610=0xff00ff26u;
inline constexpr unsigned CosmicDoorScore610=200;
inline constexpr KeyDoorStage CosmicKeys610{
    1,1,{{{160,-160},CosmicGreen610}},{{{-160,160},{32,32}}},{CosmicGreen610},{},
};
inline constexpr Vec2 CosmicNukes610[]={{160,-32}};
inline constexpr BaddieStage CosmicCars610{0,1,320,CosmicShade610,{
    {{-256,32},{160,0},10,40,100},
}};
inline constexpr BaddieStage CosmicSkulls610{1,2,320,CosmicShade610,{
    {{-160,-160},{125,0},10,30,3},
    {{160,160},{-125,0},10,30,3},
}};
inline constexpr PortalNode CosmicPortalNodes611[]={{{-32,128},1},{{280,-344},0}};
inline constexpr PortalStage CosmicPortals611{CosmicPortalNodes611,std::size(CosmicPortalNodes611)};
inline constexpr JumpPad CosmicJump611[]={{ {280,-344},1.5708f,0 }};
inline constexpr SmileyStage CosmicRing611{
    {-112,-216},24,10,5,900,2000,110,83,0,
    {{0,272},{-32,96},{280,-104},{280,96},{-224,72}},
};
