#pragma once

template<class Atlas>
void setFrontierTexture(const Atlas& atlas) {
    setTexture(GU_PSM_T8,Atlas::TextureWidth,Atlas::TextureHeight,atlas.texture());
    sceGuClutMode(GU_PSM_8888,0,0xff,0);
    sceGuClutLoad(32,atlas.clut());
}

template<class Atlas>
void drawFrontierTiles(const Atlas& atlas,const unsigned (&counts)[4],unsigned state) {
    if (!atlas.texture() || state>=4) return;
    unsigned first=0;
    for (unsigned phase=0;phase<state;++phase) first+=counts[phase];
    setFrontierTexture(atlas);
    for (unsigned index=first;index<first+counts[state];++index) {
        const auto& art=atlas.patch(index);
        patch(art,ViewX+GameplayView::HudWidth+art.x,art.y);
    }
}

template<class Atlas>
void drawFrontierBlocks(const GameplayView& view,const Round& round,const Atlas& faces) {
    if (!faces.texture() || !round.switchStage) return;
    setFrontierTexture(faces);
    for (unsigned index=0;index<round.switchStage->blockCount;++index)
        sharedActor(view,faces.patch(round.switches.face(index)),round.switchStage->blocks[index].center);
}

template<unsigned Width=0,unsigned Height=0,class Atlas>
void drawFrontierPortals(const GameplayView& view,const Atlas& art,PortalStage stage,unsigned tick,
                         unsigned firstPatch=0,bool worldSized=Width!=0) {
    if (!stage.count || !art.texture()) return;
    if constexpr (Width) setTexture(GU_PSM_8888,Width,Height,art.texture());
    else setFrontierTexture(art);
    const float angle=std::fmod(tick*0.1f,6.2831853f);
    const float cosine=std::cos(angle),sine=std::sin(angle);
    const float zoom=worldSized?view.worldScale():view.worldZoom();
    for (unsigned index=0;index<stage.count;++index) {
        const auto& node=stage.nodes[index];
        unsigned patchId=firstPatch+index;
        if constexpr (Width) patchId=node.target>=stage.count?2:node.target==3?1:0;
        const auto& sprite=art.patch(patchId);
        if (node.target>=stage.count) {
            actor(view,sprite,node.position,zoom);
            continue;
        }
        const Vec2 center=view.position(node.position);
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
        for (unsigned corner=0;corner<4;++corner) {
            const float u=corner&1?sprite.width:0;
            const float v=corner&2?sprite.height:0;
            const float x=zoom*(sprite.x+u),y=zoom*(sprite.y+v);
            vertices[corner]={sprite.u+u,sprite.v+v,White,center.x+x*cosine-y*sine,
                              center.y+x*sine+y*cosine,0};
        }
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       4,nullptr,vertices);
    }
}
