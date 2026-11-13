#pragma once

LiveResultArt cosmicLightField612,cosmicGlass613,cosmicPlants613;
LiveResultArt::Slot cosmicLightSlot612,cosmicGlassSlot613,cosmicPlantsSlot613;
ParkPatchBank<CosmicMovingGoal613Width,CosmicMovingGoal613Height,CosmicMovingGoal613Count> cosmicMovingGoal613;
alignas(64) uint32_t cosmicSkyPalette613[256];
constexpr unsigned CosmicSkyBytes613=512*512;
const char* cosmicExecutable051=nullptr;

constexpr unsigned CosmicGoalScratchOffset613=CosmicField613VramBytes;
constexpr unsigned CosmicGoalBytes613=CosmicMovingGoal613Width*CosmicMovingGoal613Height*4;
static_assert(CosmicGoalScratchOffset613%64==0 && CosmicGoalScratchOffset613+CosmicGoalBytes613<=GoalFlagArt::ScratchBytes);
constexpr unsigned CosmicPlantsOffset613=CosmicEarthboundOffset045+CosmicSkyBytes613;
constexpr unsigned CosmicGlassOffset613=CosmicFieldOffset045;
static_assert(CosmicPlantsOffset613+CosmicPlants613VramBytes<=CosmicFieldOffset045);
static_assert(CosmicGloss613VramBytes<=CosmicFieldBytes046);
static_assert(CosmicField612VramBytes<=CosmicFieldBytes046 && CosmicField613VramBytes<=CosmicFieldBytes046);
static_assert(CosmicFlag612VramBytes<=CosmicFlagBytes046);
static_assert(CosmicLightField612VramBytes<=GoalFlagArt::ScratchBytes);

bool openCosmicAssets051(const char* executable) {
    cosmicExecutable051=executable;
    constexpr unsigned Scratch=sizeof fallbackPage-HorseMeter039Width*HorseMeter039Height;
    const auto path=[executable](const char* name) { return assetPath(executable,name); };
    return cosmicField046[10].open(path("cosmic-field-6_12.ucr"),4,Scratch) &&
           cosmicFlag046[10].open(path("cosmic-flag-6_12.ucr"),4,Scratch) &&
           cosmicLightField612.open(path("cosmic-light-field-6_12.ucr"),4,Scratch) &&
           cosmicField046[11].open(path("cosmic-field-6_13.ucr"),1,Scratch) &&
           cosmicGlass613.open(path("cosmic-gloss-6_13.ucr"),CosmicGloss613Count,Scratch) &&
           cosmicPlants613.open(path("cosmic-plants-6_13.ucr"),CosmicPlants613Count,Scratch);
}

void resetCosmicArt051() {
    cosmicLightSlot612=cosmicGlassSlot613=cosmicPlantsSlot613={};
    cosmicLightField612.closeStream();
    cosmicGlass613.closeStream();
    cosmicPlants613.closeStream();
    cosmicMovingGoal613.unload();

}

bool loadCosmicArt051(const char* executable,unsigned index) {
    const auto path=[executable](const char* name) { return assetPath(executable,name); };
    cosmicFallback046=nullptr;
    if (index==CosmicTwelfthCourse)
        return loadVramRaw045(path("cosmic-earthbound-6_09.bin"),CosmicEarthboundOffset045,CosmicEarthboundBytes045) &&
               loadVramAtlas045(cosmicSwitchFaces046,path("elysian-switch-faces-5_14.pak"),CosmicSwitchFacesOffset046);
    const auto file=openAssetFile(path("cosmic-sky-6_13.pak").c_str());
    auto* sky=vram045(CosmicEarthboundOffset045);
    if (!file || std::fread(cosmicSkyPalette613,sizeof cosmicSkyPalette613,1,file.get())!=1 ||
        std::fread(sky,CosmicSkyBytes613,1,file.get())!=1 || std::fgetc(file.get())!=EOF ||
        std::ferror(file.get())) return false;
    sceKernelDcacheWritebackRange(cosmicSkyPalette613,sizeof cosmicSkyPalette613);
    sceKernelDcacheWritebackRange(sky,CosmicSkyBytes613);
    auto* goalPixels=reinterpret_cast<uint32_t*>(goalFlags.scratch()+CosmicGoalScratchOffset613);
    return cosmicMovingGoal613.loadDirect(path("cosmic-moving-goal-6_13.pak"),goalPixels,false);
}
