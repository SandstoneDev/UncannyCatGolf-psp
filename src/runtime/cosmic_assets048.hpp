#pragma once

constexpr unsigned CosmicWaterOffset048=CosmicTailOffset046;
constexpr unsigned CosmicPlantsOffset048=CosmicWaterOffset048+CosmicWater607VramBytes;
constexpr unsigned CosmicSwitchOffset048=CosmicPlantsOffset048+CosmicPlants607VramBytes;
constexpr unsigned MetronomeOffset048=CosmicSwitchOffset048+ElysianSwitchFaces514Width*ElysianSwitchFaces514Height*4;
static_assert(MetronomeOffset048+CosmicMetronome048VramBytes+CosmicMetronome048LabelBytes<=2*1024*1024);

inline unsigned cosmicGlossPhase048(unsigned tick) {
    constexpr float Pi=3.14159265358979323846f;
    return static_cast<unsigned>(tick*(CosmicGloss048Phases/(60.0f*Pi)))%CosmicGloss048Phases;
}

LiveResultArt cosmicGloss048[3];
LiveResultArt cosmicWater607,cosmicPlants607,cosmicMetronome048;
LiveResultArt::Slot cosmicWaterSlot048,cosmicPlantsSlot048,cosmicMetronomeSlot048,cosmicBeatLabelSlot048;
ParkPatchBank<CosmicDoor607Width,CosmicDoor607Height,CosmicDoor607Count> cosmicDoor607;
std::string cosmicDoorPath048,cosmicFallbackPrefix048;
unsigned cosmicDoorGeneration048=0;

bool openCosmicAssets048(const char* executable) {
    constexpr unsigned Scratch=sizeof fallbackPage-HorseMeter039Width*HorseMeter039Height;
    const auto path=[executable](const char* name) { return assetPath(executable,name); };
    return cosmicGloss048[0].open(path("cosmic-gloss-6_06.ucr"),CosmicGloss048Count,Scratch) &&
           cosmicGloss048[1].open(path("cosmic-gloss-6_07-switch0.ucr"),CosmicGloss048Count,Scratch) &&
           cosmicGloss048[2].open(path("cosmic-gloss-6_07-switch1.ucr"),CosmicGloss048Count,Scratch) &&
           cosmicField046[4].open(path("cosmic-field-6_06.ucr"),CosmicField606Count,Scratch) &&
           cosmicField046[5].open(path("cosmic-field-6_07.ucr"),CosmicField607Count,Scratch) &&
           cosmicFlag046[4].open(path("cosmic-flag-6_06.ucr"),CosmicFlag606Count,Scratch) &&
           cosmicFlag046[5].open(path("cosmic-flag-6_07.ucr"),CosmicFlag607Count,Scratch) &&
           cosmicWater607.open(path("cosmic-water-6_07.ucr"),CosmicWater607Count,Scratch) &&
           cosmicPlants607.open(path("cosmic-plants-6_07.ucr"),CosmicPlants607Count,Scratch) &&
           cosmicMetronome048.open(path("cosmic-metronome-048.ucr"),CosmicMetronome048Count,Scratch);
}

void resetRhythmArt048() {
    for (auto& field:cosmicGloss048) field.closeStream();
    cosmicWaterSlot048=cosmicPlantsSlot048=cosmicMetronomeSlot048=cosmicBeatLabelSlot048={};
    cosmicDoor607.unload();
}

bool loadRhythmArt048(const char* executable,unsigned index) {
    const auto prefix=std::string("cosmic-fallback-")+CourseIds[index]+"-state";
    cosmicFallbackPrefix048=assetPath(executable,prefix.c_str());
    if (index!=CosmicSeventhCourse) return true;
    cosmicDoorPath048=assetPath(executable,"cosmic-door-6_07.pak");
    return loadVramAtlas045(cosmicSwitchFaces046,assetPath(executable,"elysian-switch-faces-5_14.pak"),CosmicSwitchOffset048);
}

uint32_t cosmicShade048(unsigned index) {
    if (index==CosmicEleventhCourse) return CosmicShade611;
    const unsigned shade=usesPolyrhythm049(index)?51:index==CosmicSixthCourse?156:178;
    return GU_RGBA(shade,shade,shade,255);
}
