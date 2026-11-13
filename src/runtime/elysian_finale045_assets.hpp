#pragma once

ElysianEnding045 endingArt045;
LiveResultArt mineBlastArt518,secretArt518,secretLabels518,cosmicFieldArt045,cosmicFlagArt045;
LiveResultArt::Slot mineBlastSlots518[3],secretSlot518,secretLabelSlots518[2],cosmicFieldSlot045,cosmicFlagSlot045;
unsigned mineBlastIndices518[3]{};
constexpr unsigned SecretSecondLabelBytes518=104320;
ParkPatchBank<ElysianMines518Width,ElysianMines518Height,ElysianMines518Count> mineArt518;
ParkPatchBank<ElysianFinale518Width,ElysianFinale518Height,ElysianFinale518Count> teeArt518;
ParkPatchBank<ElysianHorseDefeat518Width,ElysianHorseDefeat518Height,ElysianHorseDefeat518Count> horseDefeatArt518;
ParkPatchBank<CosmicGateProps045Width,CosmicGateProps045Height,CosmicGateProps045Count> cosmicGateArt045;
ParkPatchBank<CosmicGateLabel045Width,CosmicGateLabel045Height,CosmicGateLabel045Count> cosmicGateLabel045;
alignas(64) uint32_t cosmicPalette045[256]{};

constexpr unsigned CosmicEarthboundOffset045=FrontierLightVramOffset;
constexpr unsigned CosmicEarthboundBytes045=CosmicEarthbound045Width*CosmicEarthbound045Height*4;
constexpr unsigned CosmicFieldOffset045=CosmicEarthboundOffset045+CosmicEarthboundBytes045;
constexpr unsigned CosmicFlagOffset045=CosmicFieldOffset045+CosmicField045VramBytes;
constexpr unsigned CosmicGateOffset045=(CosmicFlagOffset045+CosmicFlag045VramBytes+63)&~63u;
constexpr unsigned CosmicGateBytes045=CosmicGatePattern045Width*CosmicGatePattern045Height;
constexpr unsigned CosmicGateArtOffset045=CosmicGateOffset045+CosmicGateBytes045;
constexpr unsigned CosmicGateLabelOffset045=CosmicGateArtOffset045+CosmicGateProps045Width*CosmicGateProps045Height*4;
static_assert(CosmicGateLabelOffset045+CosmicGateLabel045Width*CosmicGateLabel045Height*4<=2*1024*1024);

constexpr unsigned HorseDefeatOffset518=FrontierLightVramOffset;
constexpr unsigned TeeOffset518=HorseDefeatOffset518+ElysianHorseDefeat518Width*ElysianHorseDefeat518Height*4;
constexpr unsigned MineOffset518=TeeOffset518+ElysianFinale518Width*ElysianFinale518Height*4;
constexpr unsigned BlastOffset518=MineOffset518+ElysianMines518Width*ElysianMines518Height*4;
static_assert(ElysianSecretLabels518VramBytes+SecretSecondLabelBytes518<=3*ElysianMineBlasts518VramBytes+
              ElysianMines518Width*ElysianMines518Height*4);
constexpr unsigned SecretOffset518=BlastOffset518+3*ElysianMineBlasts518VramBytes;
static_assert(SecretOffset518+ElysianSecret518VramBytes<=2*1024*1024);

uint8_t* vram045(unsigned offset) {
    return static_cast<uint8_t*>(sceGeEdramGetAddr())+offset;
}

class WaterArt518 {
public:
    bool open(const std::string& path) {
        if (path==name) return true;
        close();
        auto next=openAssetFile(path.c_str());
        if (!next || std::fseek(next.get(),0,SEEK_END) ||
            std::ftell(next.get())!=static_cast<long>(ElysianWater518Count*Bytes)) return false;
        name=path;
        stream=std::move(next);
        return true;
    }
    void close() { stream.reset(); name.clear(); previous=ElysianWater518Count; }
    const uint8_t* frame(unsigned index) {
        unsigned generation=0;
        auto* pixels=clips.compressedTextureWorkspace<WaterOffset+Bytes>(generation)+WaterOffset;
        if (!stream) return nullptr;
        if (previous!=index || tag!=generation) {
            if (std::fseek(stream.get(),index*Bytes,SEEK_SET) || std::fread(pixels,Bytes,1,stream.get())!=1)
                return nullptr;
            sceKernelDcacheWritebackRange(pixels,Bytes);
            previous=index;
            tag=generation;
        }
        return pixels;
    }
private:
    static constexpr unsigned Bytes=ElysianWater518Width*ElysianWater518Height*4, WaterOffset=176*1024;
    AssetFile stream;
    std::string name;
    unsigned previous=ElysianWater518Count,tag=0;
} waterArt518;

std::string neutralPath518;
bool neutralReady518=false;

bool prepareNeutral518() {
    if (neutralReady518) return true;
    const auto file=openAssetFile(neutralPath518.c_str());
    if (!file) return false;
    if (std::fread(levelSelectArt.base,sizeof levelSelectArt.base,1,file.get())!=1) return false;
    sceKernelDcacheWritebackRange(levelSelectArt.base,sizeof levelSelectArt.base);
    levelSelectArt.invalidateBase();
    neutralReady518=true;
    return true;
}

bool loadVramRaw045(const std::string& path,unsigned offset,unsigned bytes) {
    if (static_cast<unsigned>(sceGeEdramGetSize())<offset+bytes) return false;
    const auto file=openAssetFile(path.c_str());
    auto* target=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(vram045(offset))|0x40000000u);
    if (!file) return false;
    alignas(64) uint8_t chunk[4096];
    while (bytes) {
        const unsigned size=std::min<unsigned>(bytes,sizeof chunk);
        if (std::fread(chunk,size,1,file.get())!=1) return false;
        std::memcpy(target,chunk,size);
        target+=size;
        bytes-=size;
    }
    return std::fgetc(file.get())==EOF && !std::ferror(file.get());
}

template<unsigned Width,unsigned Height,unsigned Count>
bool loadVramAtlas045(ParkPatchBank<Width,Height,Count>& bank,const std::string& path,unsigned offset) {
    constexpr unsigned Bytes=Width*Height*4;
    if (static_cast<unsigned>(sceGeEdramGetSize())<offset+Bytes) return false;
    auto* dest=vram045(offset);
    if constexpr (Bytes<=512*272*2) {
        unsigned generation=0;
        auto* scratch=clips.textureWorkspace<Bytes>(generation);
        return bank.load(path,nullptr,reinterpret_cast<uint32_t*>(dest),reinterpret_cast<uint32_t*>(scratch)) &&
               bank.size()==Count;
    } else {
        return bank.loadDirect(path,reinterpret_cast<uint32_t*>(dest)) && bank.size()==Count;
    }
}

const uint8_t* cosmicEarthboundPixels045() { return vram045(CosmicEarthboundOffset045); }

bool openFinaleAssets045(const char* executable) {
    constexpr unsigned ResultScratchBytes=sizeof fallbackPage-HorseMeter039Width*HorseMeter039Height;
    const auto path=[executable](const char* file) { return assetPath(executable,file); };
    const auto ending=path("ending-045.uce");
    return endingArt045.open(ending,ending) &&
           mineBlastArt518.open(path("elysian-mine-blasts-5_18.ucr"),ElysianMineBlasts518Count,ResultScratchBytes) &&
           secretArt518.open(path("elysian-secret-5_18.ucr"),ElysianSecret518Count,ResultScratchBytes) &&
           secretLabels518.open(path("elysian-secret-labels-5_18.ucr"),ElysianSecretLabels518Count,ResultScratchBytes) &&
           cosmicFieldArt045.open(path("cosmic-field-6_01.ucr"),CosmicField045Count,ResultScratchBytes) &&
           cosmicFlagArt045.open(path("cosmic-flag-6_01.ucr"),CosmicFlag045Count,ResultScratchBytes);
}

bool loadFinaleCourse045(const char* executable,unsigned index) {
    const auto path=[executable](const char* file) { return assetPath(executable,file); };
    if (index!=Elysian518Course) {
        waterArt518.close();
        mineArt518.unload();
        teeArt518.unload();
        horseDefeatArt518.unload();
        for (auto& slot:mineBlastSlots518) slot={};
        secretSlot518={};
        mineBlastArt518.closeStream();
        secretArt518.closeStream();
        secretLabels518.closeStream();
        for (auto& slot:secretLabelSlots518) slot={};
    }
    if (index!=CosmicFirstCourse) {
        cosmicFieldSlot045=cosmicFlagSlot045={};
        cosmicGateArt045.unload();
        cosmicGateLabel045.unload();
        cosmicFieldArt045.closeStream();
        cosmicFlagArt045.closeStream();
    }
    if (index==Elysian518Course) {
        mineArt518.unload();
        teeArt518.unload();
        horseDefeatArt518.unload();
        neutralPath518=path("elysian-neutral-5_18.bin");
        neutralReady518=false;
        return waterArt518.open(path("elysian-water-5_18.pak")) &&
               loadVramAtlas045(mineArt518,path("elysian-mines-5_18.pak"),MineOffset518) &&
               loadVramAtlas045(teeArt518,path("elysian-finale-5_18.pak"),TeeOffset518) &&
               loadVramAtlas045(horseDefeatArt518,path("elysian-horse-defeat-5_18.pak"),HorseDefeatOffset518);
    }
    if (index==CosmicFirstCourse) {
        return loadVramRaw045(path("cosmic-earthbound-6_01.bin"),CosmicEarthboundOffset045,CosmicEarthboundBytes045) &&
               loadVramRaw045(path("cosmic-gate-pattern-6_01.bin"),CosmicGateOffset045,CosmicGateBytes045) &&
               loadVramAtlas045(cosmicGateArt045,path("cosmic-gate-props-6_01.pak"),CosmicGateArtOffset045) &&
               loadVramAtlas045(cosmicGateLabel045,path("cosmic-gate-label-6_01.pak"),CosmicGateLabelOffset045);
    }
    return true;
}
