#pragma once

void drawCosmicDoor610(const GameplayView& view,const Round& round) {
    if (!cosmicDoor610.texture()) return;
    const auto& state=round.roundKeys;
    const auto center=CosmicKeys610.doors[0].center;
    const bool opened=state.opened&1;
    setTexture(GU_PSM_8888,CosmicDoor610Width,CosmicDoor610Height,cosmicDoor610.texture());
    if (!opened) actor(view,cosmicDoor610.patch(0),center,view.worldScale());
    const unsigned alpha=static_cast<unsigned>(255*state.lockAlpha[0]);
    if (alpha) actor(view,cosmicDoor610.patch(1),center,view.worldScale(),GU_RGBA(255,255,255,alpha));
    if (opened) drawDestruction(view,center,round.ticks-state.openedAt[0],CosmicDoorScore610);
}

void drawCosmicPortals611(const GameplayView& view,unsigned tick) {
    if (!cosmicPortalArt611.texture()) return;
    setTexture(GU_PSM_8888,CosmicPortals611Width,CosmicPortals611Height,cosmicPortalArt611.texture());
    const auto& sprite=cosmicPortalArt611.patch(0);
    const float angle=std::fmod(tick*0.1f,6.2831853f),zoom=view.worldScale();
    const Vec2 right{std::cos(angle)*zoom,std::sin(angle)*zoom},down{-right.y,right.x};
    for (const auto& node:CosmicPortalNodes611) {
        const Vec2 center=view.position(node.position);
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
        for (unsigned corner=0;corner<4;++corner) {
            const float u=corner&1?sprite.width:0,v=corner&2?sprite.height:0;
            const auto point=center+right*(sprite.x+u)+down*(sprite.y+v);
            vertices[corner]={sprite.u+u,sprite.v+v,CosmicShade611,point.x,point.y,0};
        }
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       4,nullptr,vertices);
    }
}

void drawCosmicRing611(const GameplayView& view,const Round& round) {
    unsigned generation=0;
    auto* workspace=clips.textureWorkspace<ParkArt::Side*ParkArt::Side*4>(generation);
    const auto* pixels=parkArt.ringTexture(workspace,generation);
    if (pixels) drawSmileyRing(view,CosmicRing611,round.smiley,round.ticks,CosmicShade611,CosmicShade611,pixels);
}
