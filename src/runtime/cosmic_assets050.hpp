#pragma once

LiveResultArt cosmicGloss050[3],cosmicWater611,cosmicWaterGloss611[2];
LiveResultArt::Slot cosmicWaterGlossSlot611;
ParkPatchBank<CosmicDoor610Width,CosmicDoor610Height,CosmicDoor610Count> cosmicDoor610;
ParkPatchBank<CosmicPortals611Width,CosmicPortals611Height,CosmicPortals611Count> cosmicPortalArt611;
constexpr unsigned CosmicPortalOffset611=CosmicTailOffset046+CosmicWater611VramBytes+CosmicWaterGloss611VramBytes;
static_assert(CosmicPortalOffset611+CosmicPortals611Width*CosmicPortals611Height*4<=MetronomeOffset048);
constexpr unsigned CosmicDoorOffset610=CosmicTailOffset046+CosmicFlag610VramBytes;
static_assert(CosmicFlag611VramBytes<=CosmicFlagBytes046);
static_assert(CosmicDoorOffset610+CosmicDoor610Width*CosmicDoor610Height*4<=MetronomeOffset048);

bool openCosmicAssets050(const char* executable) {
    constexpr unsigned Scratch=sizeof fallbackPage-HorseMeter039Width*HorseMeter039Height;
    const auto path=[executable](const char* name) { return assetPath(executable,name); };
    return cosmicGloss050[0].open(path("cosmic-gloss-6_10.ucr"),4*CosmicGloss048Phases,Scratch) &&
           cosmicGloss050[1].open(path("cosmic-gloss-6_11-low.ucr"),4*CosmicGloss048Phases,Scratch) &&
           cosmicGloss050[2].open(path("cosmic-gloss-6_11-high.ucr"),4*CosmicGloss048Phases,Scratch) &&
           cosmicField046[8].open(path("cosmic-field-6_10.ucr"),4,Scratch) &&
           cosmicField046[9].open(path("cosmic-field-6_11.ucr"),8,Scratch) &&
           cosmicFlag046[8].open(path("cosmic-flag-6_10.ucr"),4,Scratch) &&
           cosmicFlag046[9].open(path("cosmic-flag-6_11.ucr"),4,Scratch) &&
           cosmicWater611.open(path("cosmic-water-6_11.ucr"),128,Scratch) &&
           cosmicWaterGloss611[0].open(path("cosmic-water-gloss-6_11-low.ucr"),CosmicWaterGloss611Count,Scratch) &&
           cosmicWaterGloss611[1].open(path("cosmic-water-gloss-6_11-high.ucr"),CosmicWaterGloss611Count,Scratch);
}

void resetRhythmArt050() {
    for (auto& field:cosmicGloss050) field.closeStream();
    cosmicWater611.closeStream();
    for (auto& field:cosmicWaterGloss611) field.closeStream();
    cosmicWaterGlossSlot611={};
    cosmicDoor610.unload();
    cosmicPortalArt611.unload();
}

bool loadRhythmArt050(const char* executable,unsigned index) {
    if (index==CosmicTenthCourse)
        return loadVramAtlas045(cosmicDoor610,assetPath(executable,"cosmic-door-6_10.pak"),CosmicDoorOffset610);
    if (index==CosmicEleventhCourse)
        return loadVramAtlas045(cosmicPortalArt611,assetPath(executable,"cosmic-portals-6_11.pak"),CosmicPortalOffset611);
    return true;
}
