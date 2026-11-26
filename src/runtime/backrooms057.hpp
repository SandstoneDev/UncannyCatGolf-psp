#pragma once
// level_hud.tscn Backrooms: original clean background ROI + selected player.
// Prepared before beginFrame. Immutable own storage never aliases fallbackPage.
namespace Backrooms057 {
struct CleanPatch {
    alignas(64) uint16_t pixels[64*64]{};
    bool attempted=false,ready=false;
    bool load(const char* path) {
        if(attempted)return ready;
        attempted=true;
        const auto file=openAssetFile(path);
        uint32_t magic=0;
        if(!file || std::fread(&magic,sizeof magic,1,file.get())!=1 || magic!=0x37424355 ||
           std::fread(pixels,sizeof pixels,1,file.get())!=1 ||
           std::fgetc(file.get())!=EOF || std::ferror(file.get()))return false;
        sceKernelDcacheWritebackRange(pixels,sizeof pixels);
        ready=true;return true;
    }
};
inline CleanPatch clean;
inline std::string path;
inline bool prepared=false;
inline void prepare(bool active) {
    prepared=active && Costumes054::screenActorReady056() && clean.load(path.c_str());
}
inline void draw() {
    if(!prepared || !Costumes054::screenActorReady056())return;
    setTexture(GU_PSM_5650,64,64,clean.pixels);
    patch({0,0,48,48,0,0},ViewX+308,188);
    Costumes054::drawScreenActor056(false,ViewX+331.5f,212.5f,2.f,GU_RGBA(255,249,166,255));
}
}
