#pragma once

// Included by main.cpp after the GU sprite and HUD helpers.
void slotQuad(Vec2 center,float width,float height,float u,float v,float spanU,float spanV,
              uint32_t color=White) {
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
    vertices[0]={u,v,color,center.x-width*.5f,center.y-height*.5f,0};
    vertices[1]={u+spanU,v+spanV,color,center.x+width*.5f,center.y+height*.5f,0};
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,vertices);
}

void drawSlots(const GameplayView& view,const SlotMachineRun& slots) {
    constexpr Vec2 centers[]={{-192,-176},{0,-176},{192,-176}};
    constexpr Vec2 marquee{0,-288};
    setTexture(GU_PSM_5650,SlotMachineArt::ReelWidth,SlotMachineArt::ReelHeight,slotArt.reels());
    for (unsigned i=0;i<SlotMachineRun::ReelCount;++i) {
        const unsigned tile=i*SlotMachineRun::SymbolCount+slots.frames[i];
        const Patch symbol{static_cast<int16_t>(tile%8*64),static_cast<int16_t>(tile/8*64),46,46,-23,-23};
        actor(view,symbol,centers[i]);
    }
    if (const auto* text=slotArt.marquee(slots.outcome,russian())) {
        setTexture(GU_PSM_4444,SlotMachineArt::TextWidth,SlotMachineArt::TextHeight,text);
        actor(view,{0,0,256,32,-128,-16},marquee,2*view.worldScale());
    }

    constexpr unsigned size=SlotMachineArt::OverlaySize;
    setTexture(GU_PSM_8888,size,size,slotArt.overlay((slots.ticks/2)%2));
    sceGuTexWrap(GU_REPEAT,GU_REPEAT);
    const auto overlay=[&](Vec2 world,float width,float height) {
        const float u=width/2.375f, v=height/2;
        slotQuad(view.position(world),width*view.worldScale(),height*view.worldScale(),
                 -u/2,-v/2,u,v,GU_RGBA(255,255,255,40));
    };
    for (const Vec2 center:centers) overlay(center,128,128);
    overlay(marquee,512,64);
    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
}

void slotEyesTexture() {
    setTexture(GU_PSM_4444,SlotMachineArt::EyeSize,SlotMachineArt::EyeSize,slotArt.eyes());
}

void drawSlotWorldEyes(const GameplayView& view,const Round& round) {
    slotEyesTexture();
    const auto eyes=[&](Vec2 world,float width,float height) {
        slotQuad(view.position(world),width*view.worldScale(),height*view.worldScale(),0,0,120,93);
    };
    const auto attached=[&](Vec2 world,float scale=1) {
        eyes(world+Vec2{-2*scale,0},44*scale,32*scale);
    };
    eyes({-180,-352},200,64);
    eyes(round.scene.goal+Vec2{47,-66},50,52);
    eyes(round.scene.goal,24,16);
    if (round.jumpStage) attached(round.jumpStage.pads[0].center);
    if (round.keyStage && round.keyStage->doorCount) attached(round.keyStage->doors[0].center);
    if (round.keyStage && round.keyStage->keyCount &&
        (!(round.roundKeys.collected&1) || (round.roundKeys.available&1)))
        attached(round.roundKeys.motion[0].position);
    if (round.skulls.bodies[0].radius>0 || round.skulls.defeatedAt[0]) attached(round.skulls.bodies[0].center);
    if (!round.caught) attached(round.game.position,round.jump.scale/JumpRun::BaseScale);
    if (!chillMode && !round.game.won) attached(round.ghost.position);
}

void drawSlotScreenEffect(const SlotMachineRun& slots) {
    if (slots.outcome==SlotOutcome::Eye) {
        slotEyesTexture();
        constexpr float scale=GameplayView::RasterScale;
        slotQuad({ViewX+192*scale,468*scale},112*scale,88*scale,0,0,120,93);
    } else if (const unsigned alpha=slots.blueAlpha()) {
        rectangle(ViewX,0,ViewWidth,Height,GU_RGBA(0u,0u,255u,alpha));
    }
}
