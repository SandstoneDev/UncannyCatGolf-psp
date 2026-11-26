#pragma once

#include "villa_checklist_art.hpp"

namespace Villa054 {

constexpr unsigned ChecklistSlotCount=10,ChecklistSlotBytes=36864;
constexpr unsigned ChecklistBarsOffset=FirstPage+ChecklistSlotCount*ChecklistSlotBytes;
static_assert(ChecklistBarsOffset+4*512<=ChecklistOffset);
constexpr int ChecklistVerticalTarget=RightTarget+1,ChecklistHorizontalTarget=RightTarget+2;

struct ChecklistElement {
    unsigned sprite=UINT32_MAX;
    float y=0,height=0;
};
struct ChecklistCache {
    LiveResultArt::Slot image;
};
LiveResultArt checklistArt;
std::array<ChecklistCache,ChecklistSlotCount> checklistCache;
std::array<ChecklistElement,std::size(VillaChecklistArt::Elements)> checklistElements;
LiveResultArt::Slot checklistVerticalTrack,checklistVerticalGrabber,checklistHorizontalTrack,checklistHorizontalGrabber;
int checklistLocale=-1;
bool checklistBankReady=false,checklistErrorLogged=false;
float checklistContentWidth=448,checklistContentHeight=0,checklistHorizontal=0;
bool checklistDragging=false;float checklistDragOrigin=0,checklistDragValue=0;

float checklistPageHeight() { return checklistContentWidth>448?440.0f:448.0f; }
float checklistMaxVertical() { return std::max(0.0f,checklistContentHeight-checklistPageHeight()); }
float checklistMaxHorizontal() { return checklistContentWidth>448?checklistContentWidth-444:0.0f; }

void layoutChecklist() {
    const unsigned locale=russian();
    unsigned states[VillaUnlocks::RowCount]{};
    checklistContentWidth=448;
    for (unsigned i=0;i<VillaUnlocks::RowCount;++i) {
        states[i]=static_cast<unsigned>(VillaUnlocks::status(VillaUnlocks::Rows[i],earned));
        checklistContentWidth=std::max(checklistContentWidth,
            static_cast<float>(VillaChecklistArt::Rows[locale][i][states[i]].width));
    }
    const auto* headers=VillaChecklistArt::Headers[locale];
    const VillaChecklistArt::Header* header=&headers[0];
    for (unsigned i=0;i<VillaChecklistArt::HeaderCounts[locale];++i)
        if (headers[i].width<=checklistContentWidth) header=&headers[i];
    float y=0;
    for (unsigned i=0;i<std::size(VillaChecklistArt::Elements);++i) {
        const auto& source=VillaChecklistArt::Elements[i];
        auto& item=checklistElements[i];
        if (i) y+=12;
        item={UINT32_MAX,y,0};
        switch(source.kind) {
        case 0: item.height=source.value; break;
        case 1: item.sprite=header->sprite; item.height=header->height; break;
        case 2: item.sprite=129+source.value; item.height=locale?25:22; break;
        case 3:
            item.sprite=source.value*3+states[source.value];
            item.height=VillaChecklistArt::Rows[locale][source.value][states[source.value]].height;
            break;
        case 4: item.sprite=132; item.height=4; break;
        }
        y+=item.height;
    }
    checklistContentHeight=y;
    pointer.scroll=std::clamp(pointer.scroll,0.0f,checklistMaxVertical());
    checklistHorizontal=std::clamp(checklistHorizontal,0.0f,checklistMaxHorizontal());
}

VillaPointerArt::Rect checklistVerticalThumb() {
    const float page=checklistPageHeight(),area=page-8;
    const float length=4+area*page/std::max(checklistContentHeight,page);
    const float position=2+area*pointer.scroll/std::max(checklistContentHeight,page);
    return {732,32+position,4,length};
}

VillaPointerArt::Rect checklistHorizontalThumb() {
    constexpr float area=436;
    const float length=8+area*444/checklistContentWidth;
    const float position=area*checklistHorizontal/checklistContentWidth;
    return {288+position,472,length,8};
}

int checklistTarget() {
    if (pointerTarget(true)==BackTarget) return BackTarget;
    const VillaPointerArt::Rect vertical{732,32,4,checklistPageHeight()};
    if (inside(pointer.position,vertical)) return ChecklistVerticalTarget;
    if (checklistContentWidth>448 &&
        inside(pointer.position,VillaPointerArt::Rect{288,472,444,8})) return ChecklistHorizontalTarget;
    return -1;
}

void stepChecklist(Flow& flow,Vec2 motion,unsigned held,unsigned pressed,unsigned released,unsigned deltaMs) {
    if (!checklistReady) checklistHorizontal=0;
    layoutChecklist();
    if (pressed&PSP_CTRL_CIRCLE) {
        pointer.captured=-1;
        pointer.held=false;
        checklistDragging=false;
        flow.screen=Screen::Villa;
        return;
    }
    const bool changed067=pointerMode067.observe(!(held&UiModality067::Directions)&&length(motion)>.18f,held,checklistDragging&&pointer.held);
    if(changed067){pointer.captured=-1;pointer.held=false;checklistDragging=false;}
    if(pointerMode067.cursor){
    const unsigned directions=PSP_CTRL_LEFT|PSP_CTRL_RIGHT|PSP_CTRL_UP|PSP_CTRL_DOWN;
    if (held&directions) motion={};
    movePointer(motion,deltaMs);
    const int vertical=bool(held&PSP_CTRL_DOWN)-bool(held&PSP_CTRL_UP);
    const int horizontal=bool(held&PSP_CTRL_RTRIGGER)-bool(held&PSP_CTRL_LTRIGGER);
    pointer.scroll=std::clamp(pointer.scroll+static_cast<float>(vertical)*deltaMs*.35f,0.0f,checklistMaxVertical());
    if (checklistContentWidth>448)
        checklistHorizontal=std::clamp(checklistHorizontal+static_cast<float>(horizontal)*deltaMs*.2f,0.0f,checklistMaxHorizontal());
    pointer.hover=checklistTarget();
    pointer.held=held&PSP_CTRL_CROSS;
    if (pressed&PSP_CTRL_CROSS) {
        pointer.captured=pointer.hover;
        checklistDragging=false;
        if (pointer.captured==ChecklistVerticalTarget) {
            const auto thumb=checklistVerticalThumb();
            if (inside(pointer.position,thumb)) {
                checklistDragging=true;
                checklistDragOrigin=pointer.position.y;
                checklistDragValue=pointer.scroll;
            } else pointer.scroll=std::clamp(pointer.scroll+
                (pointer.position.y<thumb.y?-1:1)*checklistPageHeight(),0.0f,checklistMaxVertical());
        }
        if (pointer.captured==ChecklistHorizontalTarget) {
            const auto thumb=checklistHorizontalThumb();
            if (inside(pointer.position,thumb)) {
                checklistDragging=true;
                checklistDragOrigin=pointer.position.x;
                checklistDragValue=checklistHorizontal;
            } else checklistHorizontal=std::clamp(checklistHorizontal+
                (pointer.position.x<thumb.x?-444.0f:444.0f),0.0f,checklistMaxHorizontal());
        }
    }
    if (pointer.held && checklistDragging) {
        if (pointer.captured==ChecklistVerticalTarget)
            pointer.scroll=std::clamp(checklistDragValue+
                (pointer.position.y-checklistDragOrigin)*checklistContentHeight/(checklistPageHeight()-8),
                0.0f,checklistMaxVertical());
        if (pointer.captured==ChecklistHorizontalTarget)
            checklistHorizontal=std::clamp(checklistDragValue+
                (pointer.position.x-checklistDragOrigin)*checklistContentWidth/436,0.0f,checklistMaxHorizontal());
    }
    if (released&PSP_CTRL_CROSS) {
        const int target=pointer.captured;
        pointer.captured=-1;
        checklistDragging=false;
        logPointerClick(target);
        if (target==BackTarget && target==pointer.hover) flow.screen=Screen::Villa;
    }
    }else{
    // This board only scrolls: Dpad axes and Circle need no cursor or drag target.
    const int vertical=bool(held&PSP_CTRL_DOWN)-bool(held&PSP_CTRL_UP);
    const int horizontal=bool(held&(PSP_CTRL_RIGHT|PSP_CTRL_RTRIGGER))-
                         bool(held&(PSP_CTRL_LEFT|PSP_CTRL_LTRIGGER));
    pointer.scroll=std::clamp(pointer.scroll+static_cast<float>(vertical)*deltaMs*.35f,0.0f,checklistMaxVertical());
    if (checklistContentWidth>448)
        checklistHorizontal=std::clamp(checklistHorizontal+static_cast<float>(horizontal)*deltaMs*.2f,0.0f,checklistMaxHorizontal());
    pointer.hover=-1;
    pointer.captured=-1;
    pointer.held=false;
    checklistDragging=false;
    }
}

bool checklistElementVisible(const ChecklistElement& item) {
    return item.sprite!=UINT32_MAX && item.y+item.height>pointer.scroll &&
           item.y<pointer.scroll+checklistPageHeight();
}

void reportChecklistError() {
    if (checklistErrorLogged) return;
    checklistErrorLogged=true;
    if (auto* file=std::fopen(pointerLogPath.c_str(),"a")) {
        std::fprintf(file,"build=053 event=checklist_asset_error locale=%u\n",unsigned(russian()));
        std::fclose(file);
    }
}

LiveResultArt::Slot* cachedChecklistSprite(unsigned id) {
    for (auto& cache:checklistCache)
        if (cache.image.owner==&checklistArt && cache.image.id==id) return &cache.image;
    for (unsigned i=0;i<checklistCache.size();++i) {
        auto& cache=checklistCache[i];
        bool needed=false;
        if (cache.image.owner==&checklistArt)
            for (const auto& item:checklistElements)
                needed|=checklistElementVisible(item) && item.sprite==cache.image.id;
        if (needed) continue;
        auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage);
        fallbackPageIndex=ScreenPages;
        if (!checklistArt.prepare(id,cache.image,vram045(FirstPage+i*ChecklistSlotBytes),
                                  ChecklistSlotBytes,scratch,sizeof fallbackPage)) {
            reportChecklistError();
            return nullptr;
        }
        return &cache.image;
    }
    reportChecklistError();
    return nullptr;
}

void prepareChecklist(unsigned elapsedMs) {
    if (!checklistReady) {
        checklistReady=loadPage(8,ChecklistOffset,512*272*2);
        residentsReady=false;
    }
    layoutChecklist();
    if (checklistLocale!=int(russian())) {
        checklistLocale=russian();
        const auto filename=companion(pointerArtPath.c_str(),russian()?"villa-checklist-ru.ucr":"villa-checklist-en.ucr");
        checklistBankReady=checklistArt.open(filename,VillaChecklistArt::BankSprites[russian()],sizeof fallbackPage);
        for (auto& cache:checklistCache) cache.image={};
        checklistErrorLogged=false;
    }
    if (!checklistReady || !checklistBankReady) reportChecklistError();
    if (checklistBankReady)
        for (const auto& item:checklistElements)
            if (checklistElementVisible(item)) cachedChecklistSprite(item.sprite);
    preparePointerArt(elapsedMs,true);
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage);
    if (pointerLabelsReady) {
        const unsigned vgrab=pointer.held && pointer.captured==ChecklistVerticalTarget?VillaChecklistArt::ScrollPressed:
            pointer.hover==ChecklistVerticalTarget?VillaChecklistArt::ScrollHover:VillaChecklistArt::ScrollGrabber;
        const unsigned hgrab=pointer.held && pointer.captured==ChecklistHorizontalTarget?VillaChecklistArt::HorizontalPressed:
            pointer.hover==ChecklistHorizontalTarget?VillaChecklistArt::HorizontalHover:VillaChecklistArt::HorizontalGrabber;
        pointerLabelArt.prepare(VillaChecklistArt::ScrollTrack,checklistVerticalTrack,vram045(ChecklistBarsOffset),512,scratch,sizeof fallbackPage);
        pointerLabelArt.prepare(vgrab,checklistVerticalGrabber,vram045(ChecklistBarsOffset+512),512,scratch,sizeof fallbackPage);
        pointerLabelArt.prepare(VillaChecklistArt::HorizontalTrack,checklistHorizontalTrack,vram045(ChecklistBarsOffset+1024),512,scratch,sizeof fallbackPage);
        pointerLabelArt.prepare(hgrab,checklistHorizontalGrabber,vram045(ChecklistBarsOffset+1536),512,scratch,sizeof fallbackPage);
    }
}

void drawChecklistStyle(const LiveResultArt::Slot& slot,VillaPointerArt::Rect area,bool horizontal) {
    if (!slot.owner || !slot.sprite.count) return;
    const auto& image=slot.images[0];
    setTexture(GU_PSM_8888,image.tile.pitch,image.tile.rows,image.pixels);
    sceGuTexFlush();
    const float sourceWidth=horizontal?16:8,sourceHeight=8;
    const float marginX=horizontal?4:2,marginY=horizontal?4:2;
    const float u[]={0,marginX,sourceWidth-marginX,sourceWidth};
    const float v[]={0,marginY,sourceHeight-marginY,sourceHeight};
    const float x[]={area.x,area.x+marginX,area.x+area.width-marginX,area.x+area.width};
    const float y[]={area.y,area.y+marginY,area.y+area.height-marginY,area.y+area.height};
    for (unsigned row=0;row<3;++row)
        for (unsigned column=0;column<3;++column) {
            const float width=u[column+1]-u[column],height=v[row+1]-v[row];
            if (width<=0 || height<=0 || x[column+1]<=x[column] || y[row+1]<=y[row]) continue;
            const Patch art{static_cast<int16_t>(u[column]),static_cast<int16_t>(v[row]),
                            static_cast<int16_t>(width),static_cast<int16_t>(height),0,0};
            patchScaled(art,ViewX+x[column]*Scale,y[row]*Scale,
                        (x[column+1]-x[column])*Scale/width,(y[row+1]-y[row])*Scale/height,White);
        }
}

void drawChecklist(unsigned elapsedMs) {
    prepareChecklist(elapsedMs);
    beginFrame();
    drawHud(elapsedMs*60/1000);
    if (checklistReady) {
        sceGuTexFlush();
        setTexture(GU_PSM_5650,512,512,vram045(ChecklistOffset));
        patch({0,0,272,272,0,0},ViewX+136,0);
    }
    sceGuScissor(ViewX+int(288*Scale),int(32*Scale),
                 ViewX+int(std::ceil(732*Scale)),int(std::ceil((32+checklistPageHeight())*Scale)));
    for (const auto& item:checklistElements) {
        if (!checklistElementVisible(item)) continue;
        for (const auto& cache:checklistCache)
            if (cache.image.owner==&checklistArt && cache.image.id==item.sprite) {
                drawPointerSprite(cache.image,(288-checklistHorizontal)*Scale,(32+item.y-pointer.scroll)*Scale);
                break;
            }
    }
    sceGuScissor(0,0,Width,Height);
    drawChecklistStyle(checklistVerticalTrack,{730,32,6,checklistPageHeight()},false);
    auto vertical=checklistVerticalThumb();
    vertical.x-=2;vertical.y-=2;vertical.width+=2;vertical.height+=4;
    drawChecklistStyle(checklistVerticalGrabber,vertical,false);
    if (checklistContentWidth>448) {
        drawChecklistStyle(checklistHorizontalTrack,{288,472,444,8},true);
        drawChecklistStyle(checklistHorizontalGrabber,checklistHorizontalThumb(),true);
    }
    drawPointerControls(true);
    endFrame();
}

void releasePointerStreams() {
    pointerArt.closeStream();
    pointerLabelArt.closeStream();
    checklistArt.closeStream();
}

}

