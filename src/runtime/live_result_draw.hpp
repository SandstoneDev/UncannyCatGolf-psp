#pragma once

#include "live_result_art.hpp"
#include "live_result_slide.inc"

LiveResultArt liveResultArt,commonResultArt,elysianOutOfBoundsArt;
LiveResultArt::Slot liveResultLarge,liveResultClear,liveResultConfetti;
bool liveResultPrepared=false;

constexpr unsigned ResultScratchOffset=HorseMeter039Width*HorseMeter039Height;
constexpr unsigned ResultScratchBytes=sizeof fallbackPage-ResultScratchOffset;
static_assert(ResultScratchOffset<sizeof fallbackPage);

bool authoredConfetti040() {
    return currentCourse!=TheWallCourse &&
           (currentCourse<HorseBroadcastCourse || currentCourse>=ElysianFieldsCourse+18);
}

bool prepareLiveResult(bool won,bool ghost,unsigned elapsed,bool common,unsigned offset=LiveResultVramOffset,
                       uint8_t* residentPixels=nullptr) {
    static_assert(LiveResultVramOffset%64==0);
    if (static_cast<unsigned>(sceGeEdramGetSize())<offset+(authoredConfetti040()?ResultVramBytes:ResultLargeBytes+ResultClearBytes))
        return false;
    auto* vram=static_cast<uint8_t*>(sceGeEdramGetAddr())+offset;
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    fallbackPageIndex=ScreenPages;
    auto& art=common?commonResultArt:liveResultArt;
    auto& other=common?liveResultArt:commonResultArt;
    other.closeStream();
    if (!art.prepare(won?0:ghost?4:3,liveResultLarge,vram,ResultLargeBytes,scratch,ResultScratchBytes))
        return false;
    if (!won) return true;
    if (!art.prepare(russian()?2:1,liveResultClear,vram+ResultLargeBytes,ResultClearBytes,scratch,ResultScratchBytes))
        return false;
    if (!authoredConfetti040()) return true;
    const unsigned first=common?3:5;
    const unsigned confetti=first+(elapsed/40)%59;
    return art.prepare(confetti,liveResultConfetti,residentPixels?residentPixels:vram+ResultLargeBytes+ResultClearBytes,
                       ResultConfettiBytes,scratch,ResultScratchBytes,residentPixels!=nullptr);
}

bool prepareElysianOutOfBounds041() {
    if (static_cast<unsigned>(sceGeEdramGetSize())<LiveResultVramOffset+ResultLargeBytes) return false;
    auto* vram=static_cast<uint8_t*>(sceGeEdramGetAddr())+LiveResultVramOffset;
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    fallbackPageIndex=ScreenPages;
    liveResultArt.closeStream();
    commonResultArt.closeStream();
    return elysianOutOfBoundsArt.prepare(0,liveResultLarge,vram,ResultLargeBytes,scratch,ResultScratchBytes);
}

bool prepareHorseScenery(LiveResultArt& art,unsigned state) {
    if (static_cast<unsigned>(sceGeEdramGetSize())<LiveResultVramOffset+ResultVramBytes) return false;
    // Horse results do not use confetti. The switch phases can use that VRAM region.
    auto* vram=static_cast<uint8_t*>(sceGeEdramGetAddr())+LiveResultVramOffset+ResultLargeBytes+ResultClearBytes;
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    fallbackPageIndex=ScreenPages;
    const bool prepared=art.prepare(state,elysianSwitchSlot512,vram,ResultConfettiBytes,scratch,ResultScratchBytes);
    art.closeStream();
    return prepared;
}

bool prepareElysianSwitch(unsigned state) {
    return prepareHorseScenery(currentCourse==Elysian514Course?elysianSwitchArt514:elysianSwitchArt512,state);
}

bool prepareElysianTrees515() {
    return prepareHorseScenery(elysianTreesArt515,0);
}

void drawLiveResultSprite(const LiveResultArt::Slot& slot,float slide=0,const GameplayView* view=nullptr,
                          float scale=1,uint32_t tint=White) {
    const auto& sprite=slot.sprite;
    const Vec2 anchor=view?view->position({}):Vec2{ViewX+272.0f,136};
    const bool mirror=view && view->delta({1,0}).x<0;
    const auto extent=[](unsigned size) {
        unsigned power=1;
        while (power<size) power*=2;
        return power;
    };
    for (unsigned i=0;i<sprite.count;++i) {
        const auto& image=slot.images[i];
        const auto& tile=image.tile;
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(tile.format?GU_PSM_5650:GU_PSM_8888,0,0,GU_TRUE);
        sceGuTexImage(0,extent(tile.width),extent(tile.height),tile.pitch,image.pixels);
        sceGuTexFlush();
        const float x=sprite.x+tile.x-272,y=sprite.y+tile.y-136;
        patchScaled({0,0,static_cast<int16_t>(tile.width),static_cast<int16_t>(tile.height),0,0},
                    anchor.x+(mirror?-x-tile.width:x)*scale,anchor.y+y*scale+slide,
                    scale,scale,tint,mirror);
    }
}

float liveResultSlide(unsigned elapsed) {
    const unsigned scaled=std::min(2000u,elapsed)*60;
    const unsigned tick=scaled/1000;
    if (tick>=std::size(ResultSlide040)-1) return 0;
    const float fraction=(scaled%1000)/1000.0f;
    return ResultSlide040[tick]+fraction*(ResultSlide040[tick+1]-ResultSlide040[tick]);
}
