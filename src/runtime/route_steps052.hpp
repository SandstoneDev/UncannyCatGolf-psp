#pragma once

struct CosmicStroke052 { unsigned tick; int x,y; };

inline constexpr CosmicStroke052 CosmicRoute614[]={{1,85,39},{104,-80,-90}};
inline constexpr unsigned CosmicWinTick614=118,CosmicWinBounces614=0;

inline constexpr CosmicStroke052 CosmicRoute615[]={
    {0,-125,-104},{185,-59,-69},{319,54,-119},{535,48,-63},{651,-117,-5},
    {769,-126,-64},{797,-27,-88},{814,-80,-67},{841,7,-81},{857,6,-78},
    {872,-10,-127},{891,-26,-118},{899,17,61},{976,125,40},
};
inline constexpr unsigned CosmicWinTick615=1071,CosmicWinBounces615=73;
inline constexpr unsigned CosmicRingStartTick615=706,CosmicRingPassTick615=876;



// PPSSPP's MIPS float results diverge from the host replay near the fifth block.
inline constexpr CosmicStroke052 CosmicPspRoute615[]={
    {0,-125,-104},{185,-59,-69},{319,54,-119},{535,48,-63},{651,-117,-5},
    {769,-126,-64},{797,-27,-88},{814,-80,-67},{841,7,-81},{857,6,-78},
    {872,-38,-26},{882,-69,-93},{900,-15,-57},{913,-13,-125},{926,47,-4},{970,112,-2},
};
inline constexpr unsigned CosmicPspRingPass615=889,CosmicPspUpperPortal615=920;
inline constexpr unsigned CosmicPspWinTick615=1017,CosmicPspWinBounces615=68;
