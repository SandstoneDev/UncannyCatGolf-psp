#pragma once

uint32_t cosmicCoinTint049(const Round& round,unsigned index) {
    const unsigned shade=cosmicShade048(CosmicNinthCourse)&255;
    const unsigned alpha=round.rhythmCoinActive(index)?255:128;
    const auto color=CosmicCoinTints609[index];
    return GU_RGBA((color&255)*shade/255,((color>>8)&255)*shade/255,((color>>16)&255)*shade/255,alpha);
}

void drawCosmicPortals608(const GameplayView& view,unsigned tick) {
    if (!cosmicPortalArt608.texture()) return;
    setTexture(GU_PSM_8888,CosmicPortals608Width,CosmicPortals608Height,cosmicPortalArt608.texture());
    const auto shade=cosmicShade048(CosmicEighthCourse);
    const float angle=std::fmod(tick*0.1f,6.2831853f);
    const float zoom=view.worldScale();
    const Vec2 right{std::cos(angle)*zoom,std::sin(angle)*zoom},down{-right.y,right.x};
    for (unsigned i=0;i<CosmicPortals608.count;++i) {
        const auto& node=CosmicPortals608.nodes[i];
        const auto& sprite=cosmicPortalArt608.patch(i);
        if (node.target>=CosmicPortals608.count) {
            actor(view,sprite,node.position,zoom,shade);
            continue;
        }
        const Vec2 center=view.position(node.position);
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
        for (unsigned corner=0;corner<4;++corner) {
            const float u=corner&1?sprite.width:0,v=corner&2?sprite.height:0;
            const auto point=center+right*(sprite.x+u)+down*(sprite.y+v);
            vertices[corner]={sprite.u+u,sprite.v+v,shade,point.x,point.y,0};
        }
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,vertices);
    }
}
