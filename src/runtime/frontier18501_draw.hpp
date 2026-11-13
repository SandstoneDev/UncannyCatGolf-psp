#pragma once

constexpr unsigned WallOverlayBytes418=FrontierPrism418Width*FrontierPrism418Height;

template<class Atlas,unsigned Offset=0>
bool bindWallAtlas418(Atlas& atlas) {
    unsigned generation=0;
    auto* workspace=clips.textureWorkspace<Offset+Atlas::TextureWidth*Atlas::TextureHeight>(generation)+Offset;
    if (!atlas.useShared(workspace,generation)) return false;
    setFrontierTexture(atlas);
    return true;
}

void drawWallObjects418(const GameplayView& view,const Round& round) {
    const auto& wall=round.wall418;
    if (wall.easyMode) {
        if (bindWallAtlas418(frontierBed418)) {
            const auto& bed=frontierBed418.patch(0);
            const auto position=view.position(WallBed418);
            patch(bed,position.x+bed.x,position.y+bed.y);
        }
        return;
    }
    if (!bindWallAtlas418(frontierPrism418)) return;
    const auto center=view.position(WallPrismGate418.center);
    if (!wall.shattered) {
        const unsigned hue=static_cast<unsigned>(wall.hue(round.ticks)*FrontierPrismHueCount418)%FrontierPrismHueCount418;
        const auto& gate=frontierPrism418.patch(FrontierPrismBody418+hue);
        patch(gate,center.x+gate.x,center.y+gate.y);
        const float scale=wall.lockScale(round.ticks);
        const unsigned prismFrame=round.ticks/4%FrontierPrismLockCount418;
        const auto& lock=frontierPrism418.patch(FrontierPrismLock418+prismFrame);
        patch(lock,center.x+lock.x*scale,center.y+lock.y*scale,scale);
        return;
    }
    const unsigned age=round.ticks-wall.shatteredAt;
    if (age>=50) return;
    const float elapsed=age*(1.0f/60.0f);
    for (unsigned i=0;i<12;++i) {
        const float x=20.0f*(i%6)-50.0f;
        const float y=i<6?-9.0f:9.0f;
        const Vec2 position=WallPrismGate418.center+Vec2{x*(1+3*elapsed),y+(100*elapsed-65)*elapsed};
        actor(view,frontierPrism418.patch(FrontierPrismShards418+i%4),position,view.worldScale(),
              GU_RGBA(255,255,255,255-age*5));
    }
}

void drawWallLighting418(const GameplayView& view,const Round& round) {
    if (!bindWallAtlas418<decltype(frontierLight418),WallOverlayBytes418>(frontierLight418)) return;
    drawFrontierLightMap(view,frontierLight418,0,{0.5f,0.5f,0.5f},
        [&](const auto& light,const auto& scaled) {
            scaled(0,{0,-664},{1024,944},FrontierWhiteLight,0.5f);
            light(round.game.position,256,FrontierWhiteLight,0.5f);
        });
}

template<unsigned Offset=0,class Atlas>
void drawElysianCloudLayer039(Atlas& atlas,unsigned ticks,float coverage,float origin,float uOffset=0,bool mirrored=false,float vOffset=0) {
    if (!bindWallAtlas418<Atlas,Offset>(atlas)) return;
    const auto& cloud=atlas.patch(0);
    constexpr float left=ViewX+GameplayView::HudWidth;
    const float drift=std::fmod(ticks*(0.15f/60),2.0f);
    const float u=cloud.u+(origin+uOffset+drift)*cloud.width,v=cloud.v+(origin+vOffset-0.5f*drift)*cloud.height;
    const float right=u+coverage*cloud.width;
    sceGuTexWrap(GU_REPEAT,GU_REPEAT);
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
    vertices[0]={mirrored?right:u,v,White,left,0,0};
    vertices[1]={mirrored?u:right,v+coverage*cloud.height,White,ViewX+ViewWidth,Height,0};
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,vertices);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
}

template<unsigned Offset=0>
void drawElysianClouds501(unsigned ticks,float coverage=1,float origin=0) {
    drawElysianCloudLayer039<Offset>(frontierClouds501,ticks,coverage,origin);
}
