#pragma once

#include "villa_unlocks054.hpp"
#include "villa_art054.hpp"
#include "villa_residents054.hpp"
#include "villa_button_navigation060.hpp"
#include "ui_modality067.hpp"

namespace Villa054 {

constexpr unsigned Pages=12, FirstPage=2*FrameBytes;
constexpr unsigned PanoramaPageBytes=256*272*2;
constexpr unsigned ChecklistOffset=FirstPage+4*PanoramaPageBytes;
constexpr unsigned ButtonOffset=ChecklistOffset+512*272*2;
constexpr unsigned OverlayOffset=ButtonOffset+256*128*4;
constexpr unsigned ButtonBytes=256*128*4;
constexpr unsigned MaxPan=612;
constexpr float Scale=17.0f/32.0f;
static_assert(OverlayOffset+512*256*4<=2*1024*1024);

enum class Action { Checklist, Poster, Gramophone, BortBash, Phone, Easel, Meowls, Dash };
struct Attraction {
    Action action;
    int x,y;
    VillaUnlocks::Id gate;
    const char* en;
    const char* ru;
};
constexpr Attraction Attractions[]={
    {Action::Checklist,322,398,VillaUnlocks::Id::Villa,"Villa Checklist","Список виллы"},
    {Action::Poster,488,176,VillaUnlocks::Id::Villa,"Club Canny","Club Canny"},
    {Action::Gramophone,667,248,VillaUnlocks::Id::SoundTest,"Sound Test","Музыкальный проигрыватель"},
    {Action::BortBash,814,294,VillaUnlocks::Id::BortBash,"Bort Bash","Bort Bash"},
    {Action::Phone,927,221,VillaUnlocks::Id::Phone,"Mystery Phone","Таинственный телефон"},
    {Action::Easel,1077,250,VillaUnlocks::Id::CatArtist,"Cat Artist","Cat Artist"},
    {Action::Meowls,1209,263,VillaUnlocks::Id::Meowls,"Meowls","Meowls"},
    {Action::Dash,1410,252,VillaUnlocks::Id::UncannyDash,"Uncanny Dash","Uncanny Dash"},
};

std::string path;
float camera=0;
int buttonFocus=-1;
UiModality067::State pointerMode067;UiModality067::Repeat pointerRepeat067;uint64_t pointerClock067=0;
unsigned list=0;
bool night=false,panoramaReady=false,checklistReady=false,buttonReady=false,residentsReady=false;
VillaUnlocks::Flags earned=0;
VillaCostumes::ResidentsMotion residentMotion;

bool available() {
    return gifts.opened(GiftReward::Villa);
}

uint32_t openedGifts() {
    uint32_t result=0;
    for (unsigned i=0;i<GiftCount;++i) {
        const auto reward=static_cast<GiftReward>(i);
        if (gifts.opened(reward)) result|=giftBit(reward);
    }
    return result;
}

bool loadPage(unsigned page,unsigned offset,unsigned size) {
    struct Record { uint32_t offset,bytes; };
    uint32_t header[2]{};
    Record records[Pages]{};
    const auto file=openAssetFile(path.c_str());
    if (!file || page>=Pages || sceGeEdramGetSize()<offset+size ||
        std::fread(header,sizeof header,1,file.get())!=1 || header[0]!=0x34564355u || header[1]!=Pages ||
        std::fread(records,sizeof records,1,file.get())!=1 || std::fseek(file.get(),0,SEEK_END)!=0)
        return false;
    const long fileSize=std::ftell(file.get());
    if (fileSize<=0) return false;
    uint32_t end=sizeof header+sizeof records;
    for (const auto& record:records) {
        if (record.offset!=end || !record.bytes || end>static_cast<uint32_t>(fileSize) ||
            record.bytes>static_cast<uint32_t>(fileSize)-end) return false;
        end+=record.bytes;
    }
    const auto& entry=records[page];
    if (end!=static_cast<uint32_t>(fileSize) || entry.bytes>sizeof fallbackPage ||
        std::fseek(file.get(),static_cast<long>(entry.offset),SEEK_SET)!=0 ||
        std::fread(fallbackPage,entry.bytes,1,file.get())!=1) return false;

    auto* output=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(vram045(offset))|0x40000000u);
    unsigned long packed=entry.bytes,decoded=size;
    fallbackPageIndex=ScreenPages;
    horseMeter039.invalidateShared();
    return uncompress2(output,&decoded,reinterpret_cast<uint8_t*>(fallbackPage),&packed)==Z_OK &&
           packed==entry.bytes && decoded==size;
}

void beginVisit(unsigned questPhase=0,VillaUnlocks::Flags editorFlags=0) {
    const auto current=std::time(nullptr);
    const auto* local=std::localtime(&current);
    night=local && (local->tm_hour<6 || local->tm_hour>21);
    (void)questPhase;(void)editorFlags;
    earned=campaignUnlockFlags();
    buttonFocus=-1;pointerMode067={};pointerRepeat067={};pointerClock067=0;
    residentMotion.reset(earned,static_cast<uint32_t>(current));
    panoramaReady=checklistReady=residentsReady=false;
    const unsigned stage=std::min(9u,VillaUnlocks::villaMusicStage(earned));
    audio.music(static_cast<Sound>(static_cast<unsigned>(Sound::VillaMusic0)+stage));
}

void releasePointerStreams();

void leaveVisit() {
    releasePointerStreams();
    panoramaReady=checklistReady=buttonReady=residentsReady=false;
}

void drawButtonBody(bool focused,float x,float y) {
    // The existing atlas used six source/destination pixels for each border.
    // Recover its source coordinates piecewise and restore the authored
    // StyleBoxTexture margins (16 PC pixels) at the game's 17/32 scale.
    // This reuses the existing page and adds no texture or allocation.
    constexpr float source[]={0,6,16,32,42,48};
    constexpr float packedWidth=209,packedHeight=47;
    const auto packed=[](float value,float size) {
        return value<=6?value:value>=42?size-(48-value):6+(value-6)*(size-12)/36;
    };
    const auto output=[](float value,float size) {
        return value<=16?value*Scale:value>=32?size-(48-value)*Scale:
               16*Scale+(value-16)*(size-32*Scale)/16;
    };
    const float v=focused?48.0f:0.0f;
    for (unsigned row=0;row<5;++row) for (unsigned column=0;column<5;++column) {
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
        vertices[0]={packed(source[column],packedWidth),v+packed(source[row],packedHeight),White,
                     x+output(source[column],packedWidth),y+output(source[row],packedHeight),0};
        vertices[1]={packed(source[column+1],packedWidth),v+packed(source[row+1],packedHeight),White,
                     x+output(source[column+1],packedWidth),y+output(source[row+1],packedHeight),0};
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       2,nullptr,vertices);
    }
}

bool drawButton(bool focused,bool pressed) {
    if (!buttonReady) {
        // This page borrows title-floor VRAM. Invalidate the other owner before
        // any write, including a failed/partial load, so Title restores its art.
        titleArt=nullptr;
        buttonReady=loadPage(9,ButtonOffset,ButtonBytes);
    }
    if (!buttonReady) return false;
    const int y=122+(pressed?1:0);
    sceGuTexFlush();
    setTexture(GU_PSM_8888,256,128,vram045(ButtonOffset));
    drawButtonBody(focused,ViewX+167,y);
    patch({219,0,26,26,0,0},ViewX+181,y+10);
    localeText(lang("Summer Villa","Летняя вилла"),212,y,1.05f,White,false,151,true);
    localeParagraph(lang("Relax, play or change costume","Отдых, мини-игры и костюмы"),
                    212,y+16,1.0f,151,12);
    return true;
}

constexpr unsigned AttractionCount=std::size(Attractions);
constexpr unsigned FocusCount=AttractionCount+VillaCostumes::Count;

Vec2 residentPosition(Costumes054::Id id) {
    const auto& resident=VillaCostumes::row(id);
    const auto motion=residentMotion.offset(id);
    return {(resident.x+resident.scale*(motion.x-std::sin(resident.skew)*motion.y))*Scale,
            (resident.y+resident.scale*std::cos(resident.skew)*motion.y)*Scale};
}

}

#include "villa_pointer.hpp"

namespace Villa054 {

void stepVilla(Flow& flow,Vec2 motion,unsigned held,unsigned pressed,unsigned released,unsigned deltaMs) {
    if (pressed&PSP_CTRL_CIRCLE) {
        pointer.captured=-1;
        flow.screen=Screen::Main;
        return;
    }
    residentMotion.advance(deltaMs*0.001f);
    pointerClock067+=uint64_t(deltaMs)*1000;
    const bool changed067=pointerMode067.observe(!(held&UiModality067::Directions)&&length(motion)>.18f,held,pointer.held&&pointer.captured>=0);
    if(changed067){pointer.captured=-1;pointer.held=false;}
    if(pointerMode067.cursor){
        movePointer((held&UiModality067::Directions)?Vec2{}:motion,deltaMs);pointer.hover=pointerTarget(false);
        buttonFocus=pointer.hover>=0&&pointer.hover<int(FocusCount)?pointer.hover:-1;
        pointerRepeat067.step(0,0,pointerClock067);
    }else{
    VillaButtons060::Target targets[FocusCount]{};
    for (unsigned i=0;i<AttractionCount;++i)
        targets[i]={static_cast<int>(i),float(Attractions[i].x),VillaUnlocks::unlocked(earned,Attractions[i].gate)};
    for (unsigned i=0;i<VillaCostumes::Count;++i) {
        const auto id=static_cast<Costumes054::Id>(i);
        targets[AttractionCount+i]={static_cast<int>(AttractionCount+i),float(VillaCostumes::row(id).x),
                                   VillaCostumes::available(id,earned)};
    }
    const unsigned navigation067=pointerRepeat067.step(held,pressed,pointerClock067);
    const int direction=bool(navigation067&(PSP_CTRL_RIGHT|PSP_CTRL_DOWN))-bool(navigation067&(PSP_CTRL_LEFT|PSP_CTRL_UP));
    buttonFocus=VillaButtons060::focus(targets,FocusCount,buttonFocus,direction);
    pointer.hover=buttonFocus;
    if (buttonFocus>=0) {
        Vec2 point;
        if (buttonFocus<static_cast<int>(AttractionCount))
            point={Attractions[buttonFocus].x*Scale,Attractions[buttonFocus].y*Scale};
        else point=residentPosition(static_cast<Costumes054::Id>(buttonFocus-AttractionCount));
        camera=VillaButtons060::camera(point.x,float(MaxPan));
    }
    }
    pointer.held=held&PSP_CTRL_CROSS;
    if (pressed&PSP_CTRL_CROSS) {
        pointer.captured=pointer.hover;
        if (pointer.hover>=static_cast<int>(AttractionCount) &&
            pointer.hover<static_cast<int>(FocusCount))
            Costumes054::select(static_cast<Costumes054::Id>(pointer.hover-AttractionCount),earned);
    }
    if(pointerMode067.cursor&&pointer.held&&(pointer.captured==LeftTarget||pointer.captured==RightTarget)){
        const int direction=pointer.captured==RightTarget?1:-1;
        camera=std::clamp(camera+float(direction)*deltaMs*.3f*Scale,0.f,float(MaxPan));
    }
    if (!(released&PSP_CTRL_CROSS)) return;
    const int target=pointer.captured;
    pointer.captured=-1;
    logPointerClick(target);
    if (target<0 || target!=pointer.hover) return;
    if (target==BackTarget) {
        flow.screen=Screen::Main;
    } else if (target<static_cast<int>(AttractionCount)) {
        switch (Attractions[target].action) {
        case Action::Checklist: list=0; pointer.scroll=0; flow.screen=Screen::VillaChecklist; break;
        case Action::Poster: flow.screen=Screen::Club; break;
        case Action::Phone: flow.screen=Screen::Phone; break;
        case Action::Easel:
            if (VillaUnlocks::unlocked(earned,VillaUnlocks::Id::CatArtist)) flow.screen=Screen::CatArtist;
            break;
        case Action::Gramophone: if (VillaUnlocks::unlocked(earned,VillaUnlocks::Id::SoundTest)) flow.screen=Screen::SoundTest; break;
        case Action::BortBash: if (VillaUnlocks::unlocked(earned,VillaUnlocks::Id::BortBash)) flow.screen=Screen::Bort; break;
        case Action::Meowls: if (VillaUnlocks::unlocked(earned,VillaUnlocks::Id::Meowls)) flow.screen=Screen::Meowls; break;
        case Action::Dash: if (VillaUnlocks::unlocked(earned,VillaUnlocks::Id::UncannyDash)) flow.screen=Screen::Dash; break;
        default: break;
        }
    }
}

void drawHud(unsigned tick) {
    MenuScroll::Textures art{};
    if (clips.menu(menuScrollPack.c_str(),2*static_cast<unsigned>(russian()),art)) {
        setTexture(GU_PSM_5650,MenuScroll::LargeWidth,512,art.hud);
        hudPatch({0,0,136,272,0,0});
    }
    drawTv(thoughts.menuFrame(tick));
}

void drawResidents(int pan) {
    if (!residentsReady) return;
    using namespace VillaCostumes;
    sceGuTexFlush();
    setTexture(GU_PSM_8888,Art::ResidentsWidth,Art::ResidentsHeight,vram045(ChecklistOffset));
    for (const auto id:ResidentOrder) {
        if (!available(id,earned)) continue;
        const auto& resident=row(id);
        const auto& image=residentPatch(id);
        const Vec2 center=residentPosition(id);
        const float x=center.x-pan;
        if (image.width<=0 || x+40<=136 || x-40>=408) continue;
        const uint32_t tint=night?(resident.lightMask==2?GU_RGBA(142,140,199,255):GU_RGBA(160,155,204,255)):White;
        const float angle=residentMotion.offset(id).rotation;
        const float cosAngle=std::cos(angle),sinAngle=std::sin(angle);
        const float sinSkew=std::sin(resident.skew),cosSkew=std::cos(resident.skew);
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
        for (unsigned corner=0;corner<4;++corner) {
            const float dx=(image.x+(corner&1?image.width:0))*resident.scale;
            const float dy=(image.y+(corner&2?image.height:0))*resident.scale;
            vertices[corner]={float(image.u+(corner&1?image.width:0)),float(image.v+(corner&2?image.height:0)),tint,
                              ViewX+x+dx*(cosAngle-sinSkew*sinAngle)-dy*(sinAngle+sinSkew*cosAngle),
                              center.y+cosSkew*(dx*sinAngle+dy*cosAngle),0};
        }
        if (id==Id::Rainbow) {
            const TextureVertex quad[4]={vertices[0],vertices[1],vertices[3],vertices[2]};
            CostumeShader056::drawQuad(quad);
        } else sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,vertices);
    }
}

void residentOutline(Costumes054::Id id,uint32_t color) {
    const auto& style=color==White?pointerWhiteOutline:pointerSelectedOutline;
    drawResidentPointerSprite(style,id,{-28*Scale,-52*Scale});
}

void drawVilla(unsigned elapsedMs) {
    if (!panoramaReady) {
        panoramaReady=true;
        for (unsigned i=0;i<4 && panoramaReady;++i)
            panoramaReady=loadPage((night?4:0)+i,FirstPage+i*PanoramaPageBytes,PanoramaPageBytes);
        if (panoramaReady) panoramaReady=loadPage(10+night,OverlayOffset,512*256*4);
    }
    if (!residentsReady) {
        namespace Art=VillaCostumes::Art;
        static_assert(Art::ResidentsBytes<=ButtonOffset-ChecklistOffset);
        auto* pixels=reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(vram045(ChecklistOffset))|0x40000000u);
        residentsReady=Costumes054::decode(Costumes054::residentPath,pixels,Art::ResidentsBytes);
        checklistReady=false;
    }
    preparePointerArt(elapsedMs,false);
    beginFrame();
    drawHud(elapsedMs*60/1000);
    if (panoramaReady) {
        const int pan=static_cast<int>(camera+0.5f);
        sceGuScissor(ViewX+136,0,ViewX+408,Height);
        for (unsigned i=0;i<4;++i) {
            const int x=static_cast<int>(i)*256-pan;
            const int left=std::max(136,x),right=std::min(408,x+(i==3?252:256));
            if (right<=left) continue;
            sceGuTexFlush();
            setTexture(GU_PSM_5650,256,512,vram045(FirstPage+i*PanoramaPageBytes));
            patch({static_cast<int16_t>(left-x),0,static_cast<int16_t>(right-left),272,0,0},ViewX+left,0);
        }
        sceGuTexFlush();
        setTexture(GU_PSM_8888,512,256,vram045(OverlayOffset));
        const auto drawObject=[&](const Patch& patchInfo) {
            const int x=patchInfo.x-pan;
            if (x+patchInfo.width>136 && x<408 && patchInfo.width>0)
                patch(patchInfo,ViewX+x,patchInfo.y);
        };
        for (unsigned i=0;i<std::size(Attractions);++i)
            drawObject(VillaArt054::Objects[night][i][VillaUnlocks::unlocked(earned,Attractions[i].gate)]);
        if (pointer.hover>=0 && pointer.hover<static_cast<int>(AttractionCount)) {
            const auto& object=VillaPointerArt::Objects[pointer.hover];
            drawPointerSprite(pointerHover,object.bounds.x*Scale-camera,object.bounds.y*Scale);
        }
        if (residentsReady) drawResidents(pan);
        else for (const auto& resident:VillaArt054::Residents[night]) drawObject(resident);
        const bool hoveredResident=pointer.hover>=static_cast<int>(AttractionCount) &&
                                   pointer.hover<static_cast<int>(FocusCount);
        for (const auto id:VillaCostumes::ResidentOrder) {
            if (!VillaCostumes::available(id,earned)) continue;
            const bool selected=Costumes054::selected==id;
            const bool hovering=hoveredResident && pointer.hover==static_cast<int>(AttractionCount+static_cast<unsigned>(id));
            if (selected || hovering)
                residentOutline(id,selected?VillaPointerArt::CostumeSelectedColor:White);
        }
        if (pointer.hover>=0 && pointer.hover<static_cast<int>(AttractionCount)) {
            const auto& label=VillaPointerLabels::Objects[russian()][pointer.hover];
            drawPointerSprite(pointerName,label.x*Scale-camera,label.y*Scale,1.5f);
        } else if (hoveredResident) {
            const auto id=static_cast<Costumes054::Id>(pointer.hover-AttractionCount);
            const auto& label=VillaPointerLabels::Costumes[russian()][static_cast<unsigned>(id)];
            drawResidentPointerSprite(pointerName,id,{label.x*Scale,label.y*Scale},1.5f);
        }
        sceGuScissor(0,0,Width,Height);
    } else {
        localeText(lang("Villa data could not be read","Не удалось прочитать данные виллы"),272,128,0.8f,White,true,255,true);
    }
    drawPointerControls(false);
    endFrame();
}

}

#include "checklist_pointer.hpp"
