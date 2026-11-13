#pragma once

static_assert(CosmicGloss049Phases==CosmicGloss048Phases);
static_assert(CosmicTailOffset046+CosmicPortals608Width*CosmicPortals608Height*4<=MetronomeOffset048);
LiveResultArt cosmicGloss049[2];
ParkPatchBank<CosmicPortals608Width,CosmicPortals608Height,CosmicPortals608Count> cosmicPortalArt608;

bool openCosmicAssets049(const char* executable) {
    constexpr unsigned Scratch=sizeof fallbackPage-HorseMeter039Width*HorseMeter039Height;
    const auto path=[executable](const char* name) { return assetPath(executable,name); };
    return cosmicGloss049[0].open(path("cosmic-gloss-6_08.ucr"),CosmicGloss608Count,Scratch) &&
           cosmicGloss049[1].open(path("cosmic-gloss-6_09.ucr"),CosmicGloss609Count,Scratch) &&
           cosmicField046[6].open(path("cosmic-field-6_08.ucr"),CosmicField608Count,Scratch) &&
           cosmicField046[7].open(path("cosmic-field-6_09.ucr"),CosmicField609Count,Scratch) &&
           cosmicFlag046[6].open(path("cosmic-flag-6_08.ucr"),CosmicFlag608Count,Scratch) &&
           cosmicFlag046[7].open(path("cosmic-flag-6_09.ucr"),CosmicFlag609Count,Scratch);
}

void resetRhythmArt049() {
    for (auto& field:cosmicGloss049) field.closeStream();
    cosmicPortalArt608.unload();
}

bool loadRhythmArt049(const char* executable,unsigned index) {
    return index!=CosmicEighthCourse || loadVramAtlas045(cosmicPortalArt608,
        assetPath(executable,"cosmic-portals-6_08.pak"),CosmicTailOffset046);
}
