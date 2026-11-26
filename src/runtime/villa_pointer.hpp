#pragma once

#include "villa_pointer_art.hpp"
#include "villa_pointer_labels.hpp"

namespace Villa054 {

struct PointerState {
    Vec2 position{512,256};
    int hover=-1,captured=-1;
    bool held=false;
    float scroll=0;
} pointer;

constexpr int BackTarget=static_cast<int>(FocusCount);
constexpr int LeftTarget=BackTarget+1,RightTarget=BackTarget+2;
constexpr unsigned PointerUiOffset=OverlayOffset+512*256*4;
static_assert(PointerUiOffset+49152<=2*1024*1024);
constexpr unsigned PointerNameOffset=ChecklistOffset+VillaCostumes::Art::ResidentsBytes;
static_assert(PointerNameOffset+16384<=ButtonOffset);
LiveResultArt pointerArt,pointerLabelArt;
LiveResultArt::Slot pointerBack,pointerLeft,pointerRight,pointerCursor,pointerHover;
std::string pointerArtPath,pointerLabelPath,pointerLogPath;
bool pointerArtReady=false,pointerLabelsReady=false;
LiveResultArt::Slot pointerName,pointerBackLabel,pointerWhiteOutline,pointerSelectedOutline;

void setPointerPaths(const char* executable) {
    pointerArtPath=assetPath(executable,"villa-pointer.ucr");
    pointerLabelPath=assetPath(executable,"villa-pointer-labels.ucr");
    pointerLogPath=statePath(executable,"session.log");
}

bool inside(Vec2 point,const VillaPointerArt::Rect& rect) {
    return point.x>=rect.x && point.y>=rect.y &&
           point.x<rect.x+rect.width && point.y<rect.y+rect.height;
}

bool objectContains(unsigned id,Vec2 world) {
    const auto& object=VillaPointerArt::Objects[id];
    if (!inside(world,object.bounds)) return false;
    if (!object.maskWidth) return true;
    const auto x=static_cast<unsigned>((world.x-object.bounds.x)*object.maskWidth/object.bounds.width);
    const auto y=static_cast<unsigned>((world.y-object.bounds.y)*object.maskHeight/object.bounds.height);
    const auto index=y*object.maskWidth+x;
    return VillaPointerArt::Masks[object.maskOffset+index/8]&(1u<<(index%8));
}

int pointerTarget(bool checklist) {
    if (inside(pointer.position,VillaPointerArt::Back)) return BackTarget;
    if (checklist || pointer.position.x<256) return -1;
    if (inside(pointer.position,VillaPointerArt::LeftButton)) return LeftTarget;
    if (inside(pointer.position,VillaPointerArt::RightButton)) return RightTarget;
    Vec2 world=pointer.position;
    world.x+=camera/Scale;
    int target=-1;
    for (unsigned id=0;id<AttractionCount;++id)
        if (VillaUnlocks::unlocked(earned,Attractions[id].gate) && objectContains(id,world))
            target=static_cast<int>(id);
    for (const auto id:VillaCostumes::ResidentOrder) {
        if (!VillaCostumes::available(id,earned)) continue;
        const Vec2 center=residentPosition(id)*(1/Scale);
        const auto& resident=VillaCostumes::row(id);
        const auto angle=residentMotion.offset(id).rotation;
        const Vec2 difference=(world-center)*(1/resident.scale);
        const float cosine=std::cos(angle),sine=std::sin(angle);
        const float y=difference.y/std::cos(resident.skew);
        const float x=difference.x+std::sin(resident.skew)*y;
        const Vec2 local{x*cosine+y*sine,-x*sine+y*cosine};
        if (inside(local,VillaPointerArt::CostumeButton))
            target=static_cast<int>(AttractionCount+static_cast<unsigned>(id));
    }
    return target;
}

void movePointer(Vec2 motion,unsigned deltaMs) {
    const float magnitude=length(motion);
    if (magnitude<=0.18f) return;
    const float speed=(std::min(magnitude,1.0f)-0.18f)/0.82f;
    const Vec2 movement=motion*(400*speed*deltaMs/(1000*magnitude));
    pointer.position.x=std::clamp(pointer.position.x+movement.x,0.0f,767.0f);
    pointer.position.y=std::clamp(pointer.position.y+movement.y,0.0f,511.0f);
}

void logPointerClick(int target) {
    if (auto* file=std::fopen(pointerLogPath.c_str(),"a")) {
        std::fprintf(file,"build=053 event=villa_pointer_click target=%d x=%.2f y=%.2f pan=%.2f\n",
                     target,pointer.position.x,pointer.position.y,camera);
        std::fclose(file);
    }
}

bool preparePointerSprite(unsigned id,LiveResultArt::Slot& slot,unsigned offset,unsigned capacity) {
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage);
    fallbackPageIndex=ScreenPages;
    return pointerArt.prepare(id,slot,vram045(offset),capacity,scratch,sizeof fallbackPage);
}

void preparePointerArt(unsigned elapsedMs,bool checklist) {
    if (!pointerArtReady) pointerArtReady=pointerArt.open(pointerArtPath,VillaPointerArt::SpriteCount,sizeof fallbackPage);
    if (!pointerArtReady) return;
    if (!pointerLabelsReady)
        pointerLabelsReady=pointerLabelArt.open(pointerLabelPath,
                                               VillaPointerLabels::LabelSprites,sizeof fallbackPage);
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage);
    if (pointerLabelsReady) {
        if (!checklist) {
            pointerLabelArt.prepare(70,pointerWhiteOutline,vram045(PointerNameOffset+16384),
                                    4096,scratch,sizeof fallbackPage);
            pointerLabelArt.prepare(71,pointerSelectedOutline,vram045(PointerNameOffset+20480),
                                    4096,scratch,sizeof fallbackPage);
        }
        pointerLabelArt.prepare(VillaPointerLabels::Back[russian()].sprite,pointerBackLabel,
                                vram045(PointerUiOffset+45056),4096,scratch,sizeof fallbackPage);
        if (!checklist && pointer.hover>=0 && pointer.hover<static_cast<int>(FocusCount)) {
            const auto name=pointer.hover<static_cast<int>(AttractionCount)?
                VillaPointerLabels::Objects[russian()][pointer.hover]:
                VillaPointerLabels::Costumes[russian()][pointer.hover-AttractionCount];
            pointerLabelArt.prepare(name.sprite,pointerName,vram045(PointerNameOffset),16384,scratch,sizeof fallbackPage);
        }
    }
    preparePointerSprite((elapsedMs*30/1000)%VillaPointerArt::BackFrames,pointerBack,PointerUiOffset,8192);
    preparePointerSprite(VillaPointerArt::Left,pointerLeft,PointerUiOffset+8192,16384);
    preparePointerSprite(VillaPointerArt::Right,pointerRight,PointerUiOffset+24576,16384);
    if (!pointerCursor.owner) preparePointerSprite(VillaPointerArt::Cursor,pointerCursor,PointerUiOffset+40960,4096);
    if (!checklist && pointer.hover>=0 && pointer.hover<static_cast<int>(AttractionCount)) {
        const auto& object=VillaPointerArt::Objects[pointer.hover];
        const bool pressed=pointer.held && pointer.captured==pointer.hover;
        const unsigned sprite=night?
            (pressed?VillaPointerArt::NightPressed[pointer.hover]:VillaPointerArt::NightHover[pointer.hover]):
            (pressed?object.pressed:object.hover);
        preparePointerSprite(sprite,pointerHover,ButtonOffset,ButtonBytes);
    }
}

void drawPointerSprite(const LiveResultArt::Slot& slot,float x,float y,float scale=1.0f) {
    if (!slot.owner) return;
    if (scale!=1.0f && slot.sprite.count) {
        int left=slot.images[0].tile.x,right=left,top=slot.images[0].tile.y,bottom=top;
        for (unsigned i=0;i<slot.sprite.count;++i) {
            const auto& tile=slot.images[i].tile;
            left=std::min(left,static_cast<int>(tile.x));
            right=std::max(right,static_cast<int>(tile.x+tile.width));
            top=std::min(top,static_cast<int>(tile.y));
            bottom=std::max(bottom,static_cast<int>(tile.y+tile.height));
        }
        x-=(left+right)*0.5f*(scale-1);
        y-=bottom*(scale-1);
        x=std::max(x,138-left*scale);
        x=std::min(x,406-right*scale);
        y=std::max(y,2-top*scale);
        y=std::min(y,270-bottom*scale);
    }
    for (unsigned i=0;i<slot.sprite.count;++i) {
        const auto& image=slot.images[i];
        const auto& tile=image.tile;
        sceGuTexFlush();
        setTexture(tile.format?GU_PSM_5650:GU_PSM_8888,tile.pitch,tile.rows,image.pixels);
        patch({0,0,static_cast<int16_t>(tile.width),static_cast<int16_t>(tile.height),0,0},
              ViewX+x+tile.x*scale,y+tile.y*scale,scale);
    }
}

void drawResidentPointerSprite(const LiveResultArt::Slot& slot,Costumes054::Id id,Vec2 origin,float textScale=1.0f) {
    if (!slot.owner) return;
    if (textScale!=1.0f && slot.sprite.count) {
        int left=slot.images[0].tile.x,right=left,bottom=slot.images[0].tile.y;
        for (unsigned i=0;i<slot.sprite.count;++i) {
            const auto& tile=slot.images[i].tile;
            left=std::min(left,static_cast<int>(tile.x));
            right=std::max(right,static_cast<int>(tile.x+tile.width));
            bottom=std::max(bottom,static_cast<int>(tile.y+tile.height));
        }
        origin.x-=(left+right)*0.5f*(textScale-1);
        origin.y-=bottom*(textScale-1);
    }
    Vec2 center=residentPosition(id);
    const auto& resident=VillaCostumes::row(id);
    const float angle=residentMotion.offset(id).rotation;
    const float cosine=std::cos(angle),sine=std::sin(angle);
    const float sinSkew=std::sin(resident.skew),cosSkew=std::cos(resident.skew);
    if (textScale!=1.0f && slot.sprite.count) {
        float left=10000,right=-10000,top=10000,bottom=-10000;
        for (unsigned i=0;i<slot.sprite.count;++i) {
            const auto& tile=slot.images[i].tile;
            for (unsigned corner=0;corner<4;++corner) {
                const float u=corner&1?tile.width:0,v=corner&2?tile.height:0;
                const float dx=(origin.x+(tile.x+u)*textScale)*resident.scale;
                const float dy=(origin.y+(tile.y+v)*textScale)*resident.scale;
                const float x=center.x-camera+dx*(cosine-sinSkew*sine)-dy*(sine+sinSkew*cosine);
                const float y=center.y+cosSkew*(dx*sine+dy*cosine);
                left=std::min(left,x); right=std::max(right,x);
                top=std::min(top,y); bottom=std::max(bottom,y);
            }
        }
        float dx=std::max(0.0f,138-left),dy=std::max(0.0f,2-top);
        if (right+dx>406) dx=406-right;
        if (bottom+dy>270) dy=270-bottom;
        center.x+=dx;
        center.y+=dy;
    }
    for (unsigned i=0;i<slot.sprite.count;++i) {
        const auto& image=slot.images[i];
        const auto& tile=image.tile;
        sceGuTexFlush();
        setTexture(GU_PSM_8888,tile.pitch,tile.rows,image.pixels);
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
        for (unsigned corner=0;corner<4;++corner) {
            const float u=corner&1?tile.width:0,v=corner&2?tile.height:0;
            const float dx=(origin.x+(tile.x+u)*textScale)*resident.scale;
            const float dy=(origin.y+(tile.y+v)*textScale)*resident.scale;
            vertices[corner]={u,v,White,
                ViewX+center.x-camera+dx*(cosine-sinSkew*sine)-dy*(sine+sinSkew*cosine),
                center.y+cosSkew*(dx*sine+dy*cosine),0};
        }
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       4,nullptr,vertices);
    }
}

void drawPointerControls(bool checklist) {
    drawPointerSprite(pointerBack,(264-5.5f)*Scale,3*Scale);
    const auto& label=VillaPointerLabels::Back[russian()];
    drawPointerSprite(pointerBackLabel,label.x*Scale,label.y*Scale);
    if (!checklist) {
        drawPointerSprite(pointerLeft,264*Scale,456*Scale);
        drawPointerSprite(pointerRight,712*Scale,456*Scale);
    }
    if(pointerMode067.cursor)drawPointerSprite(pointerCursor,pointer.position.x*Scale,pointer.position.y*Scale);
}

}

