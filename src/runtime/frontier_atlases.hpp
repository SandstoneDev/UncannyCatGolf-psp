#pragma once

#include "snow_switch_patches.hpp"

constexpr unsigned FrontierSwitchAtlasWidth=SnowSwitchAtlasSize[0];
constexpr unsigned FrontierSwitchAtlasHeight=SnowSwitchAtlasSize[1];
constexpr auto& FrontierSwitchPatchCounts=SnowSwitchPatchCounts;
constexpr unsigned FrontierSwitchPatchCount=FrontierSwitchPatchCounts[0]+FrontierSwitchPatchCounts[1]+
                                            FrontierSwitchPatchCounts[2]+FrontierSwitchPatchCounts[3];
constexpr unsigned FrontierPortalAtlasWidth=128,FrontierPortalAtlasHeight=64;
