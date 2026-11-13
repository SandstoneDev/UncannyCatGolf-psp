#pragma once

LiveResultArt cosmicWater614,santaCourseArt,santaGlassArt,santaStuckArt,santaBackdropArt,santaKeyArt[2];
LiveResultArt::Slot cosmicWaterSlot614,santaKeySlot,santaStuckSlot;
LiveResultArt::Slot santaCourseSlots[3],santaGlassSlots[3],santaBackdropSlots[3];
ParkPatchBank<CosmicGoals614Width,CosmicGoals614Height,CosmicGoals614Count> cosmicGoals614;
ParkPatchBank<CosmicProps615Width,CosmicProps615Height,CosmicProps615Count> cosmicProps615;
ParkPatchBank<CosmicSanta615Width,CosmicSanta615Height,CosmicSanta615Count> cosmicSanta615;

constexpr unsigned SantaChunkBytes=147968;
constexpr unsigned SantaRamBytes=6*SantaChunkBytes;
constexpr unsigned SantaPropBytes=CosmicProps615Width*CosmicProps615Height*4;
constexpr unsigned SantaSpriteBytes=CosmicSanta615Width*CosmicSanta615Height*4;
constexpr unsigned SantaBackdropRamOffset=SantaRamBytes;
constexpr unsigned SantaStuckOffset=SantaBackdropRamOffset+CosmicParallax615VramBytes;
unsigned santaSpriteGeneration=0;
std::string santaSpritePath;
constexpr unsigned SantaPropOffset=CosmicFieldOffset045;
constexpr unsigned SantaKeyOffset=SantaPropOffset+SantaPropBytes;
constexpr unsigned SantaBackdropOffset=SantaKeyOffset+CosmicKey615VramBytes;

static_assert(SantaStuckOffset+CosmicStuck615VramBytes<=GoalFlagArt::ScratchBytes);
static_assert(SantaBackdropOffset+CosmicParallax615VramBytes<=2*1024*1024);
static_assert(SantaSpriteBytes<=FrameBytes);
static_assert(CosmicWater614VramBytes<=CosmicFieldBytes046);
static_assert(CosmicGoals614Width*CosmicGoals614Height*4<=CosmicEarthboundBytes045);

bool openCosmicAssets052(const char* executable) {
    constexpr unsigned Scratch=sizeof fallbackPage-HorseMeter039Width*HorseMeter039Height;
    const auto path=[executable](const char* name) { return assetPath(executable,name); };
    return cosmicWater614.open(path("cosmic-water-6_14.ucr"),16,Scratch) &&
           santaCourseArt.open(path("cosmic-course-6_15.ucr"),10,Scratch) &&
           santaGlassArt.open(path("cosmic-glass-6_15.ucr"),10,Scratch) &&
           santaStuckArt.open(path("cosmic-stuck-6_15.ucr"),1,Scratch) &&
           santaBackdropArt.open(path("cosmic-parallax-6_15.ucr"),CosmicParallax615Count,Scratch) &&
           santaKeyArt[0].open(path("cosmic-key-6_15-a.ucr"),128,Scratch) &&
           santaKeyArt[1].open(path("cosmic-key-6_15-b.ucr"),121,Scratch);
}

void resetCosmicArt052() {
    cosmicWaterSlot614=santaKeySlot=santaStuckSlot={};
    for (auto* bank:{&santaCourseArt,&santaGlassArt,&santaStuckArt,&santaBackdropArt,&cosmicWater614,&santaKeyArt[0],&santaKeyArt[1]})
        bank->closeStream();
    for (auto* slots:{santaCourseSlots,santaGlassSlots,santaBackdropSlots})
        std::fill_n(slots,3,LiveResultArt::Slot{});
    cosmicGoals614.unload();
    cosmicProps615.unload();
    cosmicSanta615.unload();
}

bool prepareSantaSprite052() {
    unsigned generation=0;
    auto* sprite=reinterpret_cast<uint32_t*>(clips.compressedTextureWorkspace<SantaSpriteBytes>(generation));
    if (cosmicSanta615.texture() && generation==santaSpriteGeneration) return true;
    cosmicSanta615.unload();
    santaSpriteGeneration=generation;
    return cosmicSanta615.loadDirect(santaSpritePath,sprite,false);
}

bool loadCosmicArt052(const char* executable,unsigned index) {
    const auto path=[executable](const char* name) { return assetPath(executable,name); };
    if (index==CosmicFourteenthCourse)
        return loadVramAtlas045(cosmicGoals614,path("cosmic-goals-6_14.pak"),CosmicTailOffset046);
    santaSpritePath=path("cosmic-santa-6_15.pak");
    return loadVramRaw045(path("cosmic-earthbound-6_15.bin"),CosmicEarthboundOffset045,CosmicEarthboundBytes045) &&
           loadVramAtlas045(cosmicProps615,path("cosmic-props-6_15.pak"),SantaPropOffset) &&
           prepareSantaSprite052();
}
