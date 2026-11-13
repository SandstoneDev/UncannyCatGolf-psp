#pragma once

WallContact sweepCosmic053(Vec2 position,Vec2 motion,float radius) {
    return lateWalls053.sweep(position,motion,radius);
}

void stopLastHurrahSounds053() {
    audio.stop(Sound::HurrahSiren);
    audio.stop(Sound::HurrahUfo);
}

bool loadCosmicCollisions053(const char* executable,unsigned index) {
    if (index==CosmicEighteenthCourse) return true;
    static_assert(LateCosmicWalls053::Bytes<=128*1024);
    const auto name=[&](const char* suffix) { return assetPath(executable,suffix); };
    if (index==CosmicSixteenthCourse) {
        const auto first=name("6_16-r1.bin"),second=name("6_16-r2.bin");
        const char* paths[]={first.c_str(),second.c_str()};
        return lateWalls053.load(goalFlags.scratch(),paths,std::size(paths));
    }
    const auto trap=name("6_17-trap.bin"),dog=name("6_17-dog.bin"),
               first=name("6_17-r1.bin"),second=name("6_17-r2.bin");
    const char* paths[]={trap.c_str(),dog.c_str(),first.c_str(),second.c_str()};
    return lateWalls053.load(goalFlags.scratch(),paths,std::size(paths));
}

LiveResultArt cosmicLayers053[3];
LiveResultArt::Slot cosmicSlots053[12];
ParkPatchBank<CosmicProps616Width,CosmicProps616Height,CosmicProps616Count> cosmicProps616;
ParkPatchBank<CosmicProps617Width,CosmicProps617Height,CosmicProps617Count> cosmicProps617;
ParkPatchBank<CosmicProps618Width,CosmicProps618Height,CosmicProps618Count> cosmicProps618;
ParkPatchBank<CosmicTrap617Width,CosmicTrap617Height,CosmicTrap617Count> cosmicTrap617;
constexpr unsigned CosmicPropOffset053=CosmicFieldOffset045;
constexpr unsigned CosmicRainbowOffset053=CosmicEarthboundOffset045+CosmicSkyBytes613;
constexpr unsigned CosmicTrapOffset053=CosmicEarthboundOffset045+CosmicSkyBytes613;
constexpr unsigned CosmicTrapBytes053=CosmicTrap617Width*CosmicTrap617Height*4;
constexpr unsigned CosmicLayersRamOffset053=128*1024;
constexpr unsigned CosmicLayersVramOffset053=CosmicPropOffset053+CosmicProps617Width*CosmicProps617Height*4;
static_assert(CosmicLayersVramOffset053+CosmicLayers617VramBytes<=2*1024*1024);
constexpr unsigned CosmicRingOffset053=CosmicTrapOffset053+CosmicTrapBytes053;
static_assert(CosmicRingOffset053+ParkArt::Side*ParkArt::Side*4<=CosmicPropOffset053);
static_assert(CosmicLayersVramOffset053+CosmicLayers617VramBytes<=MetronomeOffset048);
static_assert(8*sizeof(TileField)<=CosmicLayersRamOffset053);
static_assert(CosmicLayersRamOffset053+CosmicLayers617SlotBytes[1]+CosmicLayers617SlotBytes[2]+
              63488+CosmicLayers617SlotBytes[7]+1536+1536+1920+1920<=GoalFlagArt::ScratchBytes);
ParkPatchBank<CosmicUfo617Width,CosmicUfo617Height,CosmicUfo617Count> cosmicUfo617;
ParkPatchBank<CosmicLights053Width,CosmicLights053Height,CosmicLights053Count> cosmicLights053;
constexpr unsigned CosmicUfoRamOffset053=640*1024;
constexpr unsigned CosmicLightRamOffset053=CosmicUfoRamOffset053+CosmicUfo617Width*CosmicUfo617Height*4;
static_assert(CosmicLightRamOffset053+CosmicLights053Width*CosmicLights053Height*4<=GoalFlagArt::ScratchBytes);

bool openCosmicAssets053(const char* executable) {
    const auto path=[executable](const char* name) { return assetPath(executable,name); };
    constexpr unsigned Scratch=sizeof fallbackPage-HorseMeter039Width*HorseMeter039Height;
    return cosmicLayers053[0].open(path("cosmic-layers-6_16.ucr"),4,Scratch) &&
           cosmicLayers053[1].open(path("cosmic-layers-6_17.ucr"),12,Scratch) &&
           cosmicLayers053[2].open(path("cosmic-layers-6_18.ucr"),1,Scratch);
}

void resetCosmicArt053() {
    for (auto& slot:cosmicSlots053) slot={};
    for (auto& bank:cosmicLayers053) bank.closeStream();
    cosmicProps616.unload();
    cosmicProps617.unload();
    cosmicProps618.unload();
    cosmicTrap617.unload();
    cosmicUfo617.unload();
    cosmicLights053.unload();
}

bool loadCosmicArt053(const char* executable,unsigned index) {
    const char* cloud=index==CosmicSeventeenthCourse?"cosmic-sky-6_17.pak":"cosmic-sky-6_13.pak";
    const auto file=openAssetFile(assetPath(executable,cloud).c_str());
    auto* sky=vram045(CosmicEarthboundOffset045);
    if (!file || std::fread(cosmicSkyPalette613,sizeof cosmicSkyPalette613,1,file.get())!=1 ||
        std::fread(sky,CosmicSkyBytes613,1,file.get())!=1 || std::fgetc(file.get())!=EOF ||
        std::ferror(file.get())) return false;
    sceKernelDcacheWritebackRange(cosmicSkyPalette613,sizeof cosmicSkyPalette613);
    sceKernelDcacheWritebackRange(sky,CosmicSkyBytes613);
    const auto path=[&](const char* name) { return assetPath(executable,name); };
    auto* scratch=goalFlags.scratch();
    if (index==CosmicSixteenthCourse)
        return loadVramAtlas045(cosmicProps616,path("cosmic-props-6_16.pak"),CosmicPropOffset053);
    if (index==CosmicSeventeenthCourse)
        return loadVramAtlas045(cosmicProps617,path("cosmic-props-6_17.pak"),CosmicPropOffset053) &&
               loadVramAtlas045(cosmicTrap617,path("cosmic-trap-6_17.pak"),CosmicTrapOffset053) &&
               frontierDogArt.openShared(path("frontier-dog-4_07.pak")) &&
               cosmicUfo617.loadDirect(path("cosmic-ufo-6_17.pak"),
                                      reinterpret_cast<uint32_t*>(scratch+CosmicUfoRamOffset053),false) &&
               cosmicLights053.loadDirect(path("cosmic-lights-053.pak"),
                                         reinterpret_cast<uint32_t*>(scratch+CosmicLightRamOffset053),false);
    return loadVramAtlas045(cosmicProps618,path("cosmic-props-6_18.pak"),CosmicPropOffset053) &&
           loadVramRaw045(path("cosmic-rainbow-6_18.bin"),CosmicRainbowOffset053,512*512);
}
