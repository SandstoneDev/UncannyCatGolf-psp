#pragma once

struct CosmicEndingClock053 {
    static constexpr unsigned IntroMs=23000, CountMs=2250;
    unsigned introMs=0, resultMs=0, visualMs=0;
    bool fast=false, seen=false;
    bool results() const { return introMs>=IntroMs; }
    bool countVisible() const { return results() && resultMs>=CountMs; }
};

CosmicEndingClock053 cosmicEnding053;
bool cosmicEndingFrameReady053=false;

namespace CosmicEndingArt053 {
constexpr unsigned PoseCount=1381, SpriteCount=36, ConfettiCount=59;
constexpr unsigned TextureSide=512, AtlasBytes=TextureSide*TextureSide*4;
constexpr unsigned ConfettiOffset=CosmicEarthboundOffset045+CosmicEarthboundBytes045;
constexpr unsigned ConfettiCapacity=2*1024*1024-ConfettiOffset;
constexpr float Scale=17.0f/32;
constexpr float Left=ViewX+136;

struct Pose {
    int16_t catX,catY,ghostX,ghostY,heartY;
    uint8_t heartAlpha,flags;
    enum : unsigned { Flip=1, Ghost=2, Cured=4, Poof=8, Heart=16 };
};
static_assert(sizeof(Pose)==12 && AtlasBytes<=GoalFlagArt::ScratchBytes);

struct Glass {
    uint16_t x,y,w,h;
    unsigned bytes;
};
static_assert(sizeof(Glass)==12);
constexpr unsigned GlassCount=3, GlassPitch[GlassCount]={208,208,272}, GlassRows[GlassCount]={128,24,48};
constexpr unsigned GlassOffset[GlassCount]={0,GlassPitch[0]*GlassRows[0],
                                           GlassPitch[0]*GlassRows[0]+GlassPitch[1]*GlassRows[1]};
constexpr unsigned GlassBytes=GlassOffset[2]+GlassPitch[2]*GlassRows[2];
constexpr unsigned GlassStart=(PoseCount*sizeof(Pose)+63)&~63u;
static_assert(GlassStart+GlassBytes<512*272*2);
static_assert(GlassOffset[1]%64==0 && GlassOffset[2]%64==0);

std::string atlasPath,glassPath,posePath,earthPath;
ParkPatchBank<TextureSide,TextureSide,SpriteCount> atlas;
LiveResultArt confetti;
LiveResultArt::Slot confettiFrame;
Glass glass[GlassCount]{};
Pose* poses=nullptr;
uint8_t* glassPixels=nullptr;
bool prepared=false;
alignas(64) uint32_t palette[8]{};

bool readPoses(uint8_t* workspace) {
    const auto file=openAssetFile(posePath.c_str());
    uint32_t header[2];
    if (!file || std::fread(header,sizeof header,1,file.get())!=1 ||
        header[0]!=0x54454355 || header[1]!=PoseCount ||
        std::fread(workspace,sizeof(Pose),PoseCount,file.get())!=PoseCount ||
        std::fgetc(file.get())!=EOF || std::ferror(file.get())) return false;
    for (unsigned i=0;i<PoseCount;++i) {
        Pose pose;
        std::memcpy(&pose,workspace+i*sizeof pose,sizeof pose);
        if ((pose.flags&Pose::Poof) && (pose.flags>>4)>=10) return false;
    }
    poses=reinterpret_cast<Pose*>(workspace);
    return true;
}

bool readGlass(uint8_t* workspace) {
    const auto file=openAssetFile(glassPath.c_str());
    uint32_t header[2];
    if (!file || std::fread(header,sizeof header,1,file.get())!=1 ||
        header[0]!=0x47454355 || header[1]!=GlassCount) return false;
    for (unsigned i=0;i<GlassCount;++i) {
        auto& patch=glass[i];
        auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage);
        if (std::fread(&patch,sizeof patch,1,file.get())!=1 || !patch.w || !patch.h ||
            patch.x+patch.w>272 || patch.y+patch.h>272 ||
            ((patch.w+15u)&~15u)!=GlassPitch[i] || ((patch.h+7u)&~7u)!=GlassRows[i] ||
            !patch.bytes || patch.bytes>sizeof fallbackPage ||
            std::fread(scratch,patch.bytes,1,file.get())!=1) return false;
        unsigned long packed=patch.bytes, bytes=GlassPitch[i]*GlassRows[i];
        if (uncompress2(workspace+GlassOffset[i],&bytes,scratch,&packed)!=Z_OK ||
            packed!=patch.bytes || bytes!=GlassPitch[i]*GlassRows[i]) return false;
    }
    if (std::fgetc(file.get())!=EOF || std::ferror(file.get())) return false;
    sceKernelDcacheWritebackRange(workspace,GlassBytes);
    glassPixels=workspace;
    return true;
}

bool prepare() {
    if (prepared) return true;
    unsigned generation=0;
    auto* workspace=clips.compressedTextureWorkspace<GlassStart+GlassBytes>(generation);
    fallbackPageIndex=ScreenPages;
    horseMeter039.invalidateShared();
    // All other course art has been released. Borrow both existing RAM workspaces.
    prepared=readPoses(workspace) && readGlass(workspace+GlassStart) &&
             loadVramRaw045(earthPath,CosmicEarthboundOffset045,CosmicEarthboundBytes045) &&
             atlas.loadDirect(atlasPath,reinterpret_cast<uint32_t*>(goalFlags.scratch()),false);
    return prepared;
}

uint32_t color(float hue,float saturation,float value) {
    const float h=hue*6;
    const float c=value*saturation;
    const float x=c*(1-std::abs(std::fmod(h,2.0f)-1));
    const float m=value-c;
    const float variants[6][3]={{c,x,0},{x,c,0},{0,c,x},{0,x,c},{x,0,c},{c,0,x}};
    const auto& rgb=variants[std::min(5u,static_cast<unsigned>(h))];
    return GU_RGBA(static_cast<unsigned>((m+rgb[0])*255+0.5f),
                   static_cast<unsigned>((m+rgb[1])*255+0.5f),
                   static_cast<unsigned>((m+rgb[2])*255+0.5f),255);
}

void drawGlass(unsigned i,unsigned visualMs) {
    const auto& sprite=glass[i];
    const auto extent=[](unsigned size) {
        unsigned p=1;
        while (p<size) p*=2;
        return p;
    };
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_T8,0,0,GU_TRUE);
    sceGuClutMode(GU_PSM_8888,0,255,0);
    sceGuClutLoad(1,palette);
    sceGuTexImage(0,extent(sprite.w),extent(sprite.h),GlassPitch[i],glassPixels+GlassOffset[i]);
    sceGuTexFlush();
    constexpr unsigned Step=16;
    const unsigned cols=(sprite.w+Step-1)/Step, rows=(sprite.h+Step-1)/Step;
    const unsigned count=2*(cols+1);
    auto* mesh=static_cast<TextureVertex*>(sceGuGetMemory(rows*count*sizeof(TextureVertex)));
    for (unsigned row=0;row<rows;++row) {
        auto* strip=mesh+row*count;
        for (unsigned col=0;col<=cols;++col) {
            const float u=std::min<unsigned>(sprite.w,col*Step);
            for (unsigned edge=0;edge<2;++edge) {
                const float v=std::min<unsigned>(sprite.h,(row+edge)*Step);
                const float x=sprite.x+u,y=sprite.y+v;
                const float wave=visualMs/500.0f+10*(136+x)/408+4*y/272;
                const auto alpha=static_cast<unsigned>(255*(0.61f+0.2f*std::cos(wave)));
                strip[2*col+edge]={u,v,GU_RGBA(255,255,255,alpha),Left+x,y,0};
            }
        }
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       count,nullptr,strip);
    }
}

void setAtlas() { setTexture(GU_PSM_8888,TextureSide,TextureSide,atlas.texture()); }

void sprite(unsigned id,float x=0,float y=0,uint32_t tint=White,bool flip=false) {
    const auto& p=atlas.patch(id);
    patch(p,Left+x+(flip?-p.x-p.width:p.x),y+p.y,1,tint,flip);
}

void drawStage(unsigned visualMs) {
    const unsigned tick=visualMs*60/1000;
    drawCosmicEarthbound045(tick,0,1,0,1);
    const float hue=(tick+180)%360/360.0f;
    palette[1]=color(hue,1-0.698039f,1);
    palette[2]=color(hue,1-0.282353f,1);
    palette[3]=color(hue,1,0.872685f);
    palette[4]=palette[2];
    sceKernelDcacheWritebackRange(palette,sizeof palette);
    setAtlas();
    sprite(0);
    drawGlass(0,visualMs);
    drawGlass(1,visualMs);
    rectangle(Left,0,272,272,GU_RGBA(0,0,0,140));
    drawGlass(2,visualMs);
}

void drawCanny(float x,float y,bool ghost=false,bool flip=false) {
    sprite(ghost?13:12,(x-256)*Scale,y*Scale,White,flip);
}
}

bool openCosmicEnding053(const char* executable) {
    using namespace CosmicEndingArt053;
    atlasPath=assetPath(executable,"cosmic-ending053-art.pak");
    glassPath=assetPath(executable,"cosmic-ending053-glass.bin");
    posePath=assetPath(executable,"cosmic-ending053-track.bin");
    earthPath=assetPath(executable,"cosmic-earthbound-6_01.bin");
    for (const auto* path:{&atlasPath,&glassPath,&posePath,&earthPath})
        if (!openAssetFile(path->c_str())) return false;
    return confetti.open(assetPath(executable,"cosmic-ending053-confetti.ucr"),ConfettiCount,sizeof fallbackPage);
}

void resetCosmicEnding053() {
    using namespace CosmicEndingArt053;
    confetti.closeStream();
    confettiFrame={};
    atlas.unload();
    prepared=false;
    cosmicEndingFrameReady053=false;
    cosmicEnding053={};
}

void startCosmicEnding053(bool seen) {
    resetCosmicEnding053();
    cosmicEnding053.seen=seen;
    audio.muteMusic();
}

bool updateCosmicEnding053(unsigned deltaMs,uint32_t pressed,uint32_t held) {
    auto& state=cosmicEnding053;
    constexpr unsigned VisualPeriodMs=354000; // Common period of 6s hue, 59 confetti frames and 50ms prism frames.
    state.visualMs=(state.visualMs+deltaMs)%VisualPeriodMs;
    if (state.results()) {
        state.resultMs=std::min(CosmicEndingClock053::CountMs,state.resultMs+deltaMs);
        return state.countVisible() && (pressed&PSP_CTRL_CROSS);
    }
    state.fast=held&PSP_CTRL_TRIANGLE;
    const unsigned previous=state.introMs;
    if (previous>=183 && (pressed&PSP_CTRL_START)) state.introMs=CosmicEndingClock053::IntroMs;
    else {
        state.introMs=std::min(CosmicEndingClock053::IntroMs,previous+deltaMs*(state.fast?4u:1u));
        const auto cue=[previous,&state](unsigned ms) { return previous<ms && state.introMs>=ms; };
        if (cue(6500) || cue(12000) || cue(12600)) audio.playTuned(Sound::Shot,0,1.65f);
        if (cue(18800)) audio.play(Sound::CosmicEndingKiss);
        if (cue(19274)) audio.playTuned(Sound::CosmicEndingSparkle,-6,1);
        if (cue(21800)) audio.play(Sound::StopToken);
    }
    if (state.results()) {
        state.fast=false;
        audio.music(Sound::CosmicEndingMusic);
    }
    return false;
}

void drawCosmicEnding053(unsigned prisms,const Round* round=nullptr) {
    using namespace CosmicEndingArt053;
    const auto& state=cosmicEnding053;
    cosmicEndingFrameReady053=prepare();
    if (cosmicEndingFrameReady053 && state.results()) {
        const unsigned phase=(state.visualMs*25/1000)%ConfettiCount;
        cosmicEndingFrameReady053=confetti.prepare(phase,confettiFrame,vram045(ConfettiOffset),
                                                 ConfettiCapacity,reinterpret_cast<uint8_t*>(fallbackPage),
                                                 sizeof fallbackPage);
        confetti.closeStream();
    }
    beginFrame();
    if (!cosmicEndingFrameReady053) {
        endFrame();
        return;
    }
    // The overlay replaces only the playfield; the playthrough status remains visible.
    drawFull(gameplayPage);
    if (round) {
        drawLiveStats(*round,gameplayPage,false);
        setTexture(GU_PSM_5650,VisualWidth,HudHeight,hud);
        drawClock(round->totalTicks*5/3,6,true);
        drawTv(thoughts.frame(round->tvTick()+state.visualMs*60/1000));
    }
    sceGuScissor(Left,0,Left+272,272);
    drawStage(state.visualMs);
    setAtlas();
    const unsigned languageOffset=russian()?5:0;
    if (!state.results()) {
        const unsigned tick=std::min(PoseCount-1,state.introMs*60/1000);
        const auto& pose=poses[tick];
        drawCanny(pose.catX/16.0f,pose.catY/16.0f);
        if (pose.flags&Pose::Ghost) drawCanny(pose.ghostX/16.0f,pose.ghostY/16.0f,true,pose.flags&Pose::Flip);
        if (pose.flags&Pose::Cured) drawCanny(592,272);
        if (pose.flags&Pose::Poof) sprite(14+(pose.flags>>4),(520-256)*Scale,208*Scale);
        else if (pose.flags&Pose::Heart)
            sprite(24+state.visualMs/200%2,(576-256)*Scale,pose.heartY/16.0f*Scale,
                   GU_RGBA(255,255,255,pose.heartAlpha));
        if (state.fast) sprite(5+languageOffset);
        else if (state.seen && state.visualMs<2000) {
            float fade=std::min(state.visualMs,2000u-state.visualMs)/1000.0f;
            fade=fade*fade*fade;
            sprite(6+languageOffset,0,0,GU_RGBA(255,255,255,static_cast<unsigned>(255*fade)));
        }
    } else {
        drawCanny(552,272);
        drawCanny(592,272);
        drawLiveResultSprite(confettiFrame);
        setAtlas();
        sprite(1);
        if (state.countVisible()) {
            sprite(26+state.visualMs/50%10,(480-256)*Scale,336*Scale);
            sprite(2+languageOffset+(prisms>=565));
            sprite(4+languageOffset);
            char count[16];
            std::snprintf(count,sizeof count,"%u / 565",prisms);
            localeText(count,305,172,0.78f,White,true);
            localeText("×",151,253,0.7f,GU_RGBA(31,24,32,255),true);
        }
    }
    endFrame();
}
