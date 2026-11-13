#pragma once

struct FrontierLight {
    Vec2 center;
    float diameter,energy;
};

void drawFrontierLighting(const GameplayView& view,Vec2 player,float playerEnergy,
                          const FrontierLight* stationary,unsigned count) {
    rectangle(ViewX+GameplayView::HudWidth,0,GameplayView::Width-GameplayView::HudWidth,Height,GU_RGBA(0,0,0,127));
    unsigned generation=0;
    auto* workspace=clips.textureWorkspace<128*256>(generation);
    if (!frontier1011Lights.useShared(workspace,generation)) return;
    setFrontierTexture(frontier1011Lights);
    sceGuBlendFunc(GU_ADD,GU_DST_COLOR,GU_FIX,0,0xffffffff);
    const auto light=[&](unsigned index,const FrontierLight& source) {
        const auto& art=frontier1011Lights.patch(index);
        const auto center=view.position(source.center);
        const float scale=source.diameter*view.worldScale()/art.width;
        const float energy=std::clamp(source.energy,0.0f,1.0f);
        const auto layer=[&](float amount) {
            const unsigned value=static_cast<unsigned>(255*amount+0.5f);
            if (value) patch(art,center.x+art.x*scale,center.y+art.y*scale,scale,GU_RGBA(value,value,value,255));
        };
        layer(std::min(1.0f,2*energy));
        if (energy>0.5f) layer(energy-0.5f);
    };
    for (unsigned i=0;i<count;++i) light(1,stationary[i]);
    light(0,{player,256,playerEnergy});
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
}

void drawFrontierLighting1011(const GameplayView& view,const Round& round) {
    constexpr FrontierLight lamps[]{{{-256,-256},256,0.6f},{{256,240},256,0.6f},
                                    {{-256,-48},256,0.6f},{{256,48},256,0.6f}};
    const bool patrol=currentCourse==FrontierPatrolCourse;
    drawFrontierLighting(view,round.game.position,0.5f,lamps,patrol?std::size(lamps):0);
}
