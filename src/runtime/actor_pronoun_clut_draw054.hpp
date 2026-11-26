#pragma once
// Included after actor(), in Main's renderer namespace. Tags are projected when
// their actor is actually drawn, so hidden actors and pointers never add labels.
namespace ActorLabelDraw054 {
using ActorPronouns054::Kind;
using ActorPronouns054::Tag;
struct Sprite {uint16_t paletteOffset;uint8_t page,blocks;Patch patch;};
#include "actor_pronoun_clut054.inc"
static_assert(sizeof(bank)<=UINT16_MAX && sizeof(Sprite)==16 && std::size(frames)==18);
struct Entry {GameplayView::Sprite screen;Tag tag;};
// A batch ends before the next world layer. This upper bound includes both
// independent spawner pools even on levels that only instantiate one of them.
constexpr unsigned Capacity=2*BaddieRun::Capacity+ChuckleRun::SwarmCapacity+
    SkullRun::Capacity+FrontierSkullRun416::Capacity+64;
inline Entry pending[Capacity];
inline unsigned used=0;
inline bool uploaded=false;
inline void add(Tag tag,const GameplayView& view,Vec2 position,Kind kind=Kind::Baddie,float localScale=1) {
    if (!tag.value) return;
    const auto& frame=frames[tag.value-1];
    GameplayView::Sprite screen;
    position.y+=localScale*ActorPronouns054::offset(kind);
    if (view.clipSprite(frame.patch,position,view.worldScale()*localScale,screen) && used<Capacity)
        pending[used++]={screen,tag};
}
inline bool flush() {
    if (!used) return false;
    if (!uploaded) {
        sceKernelDcacheWritebackRange(const_cast<uint8_t*>(bank),sizeof bank);
        uploaded=true;
    }
    sceGuClutMode(GU_PSM_8888,0,255,0);
    unsigned previous=0;
    for (unsigned i=0;i<used;++i) {
        const auto& item=pending[i];
        const auto& frame=frames[item.tag.value-1];
        if (previous!=item.tag.value) {
            sceGuClutLoad(frame.blocks,bank+frame.paletteOffset);
            // setTexture issues TexImage (and its GE texture-cache flush) after
            // loading the palette, including palette changes on the same page.
            setTexture(GU_PSM_T8,256,frame.page?32:128,bank+(frame.page?32768:0));
            previous=item.tag.value;
        }
        const auto& s=item.screen;
        auto* v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
        v[0]={s.uvTopLeft.x,s.uvTopLeft.y,White,s.topLeft.x,s.topLeft.y,0};
        v[1]={s.uvBottomRight.x,s.uvBottomRight.y,White,s.bottomRight.x,s.bottomRight.y,0};
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
    }
    used=0;
    return true;
}
}
