#pragma once

constexpr unsigned FrontierPropsOffset1617=48*1024;
constexpr unsigned FrontierLightsOffset1617=176*1024;
constexpr unsigned FrontierTitlesOffset1617=224*1024;
static_assert(FrontierPropsOffset1617+FrontierPropsWidth416*FrontierPropsHeight416<=FrontierLightsOffset1617);
static_assert(FrontierPropsOffset1617+FrontierPropsWidth417*FrontierPropsHeight417<=FrontierLightsOffset1617);
static_assert(FrontierLightsOffset1617+FrontierLightsWidth1617*FrontierLightsHeight1617<=FrontierTitlesOffset1617);

template<class Atlas,unsigned Offset>
bool bindFrontierCompressed037(Atlas& atlas) {
    unsigned generation=0;
    auto* memory=clips.compressedTextureWorkspace<Offset+Atlas::TextureWidth*Atlas::TextureHeight>(generation)+Offset;
    if (!atlas.useShared(memory,generation)) return false;
    setFrontierTexture(atlas);
    return true;
}

void drawFrontierBlocks416(const GameplayView& view,const Round& round) {
    if (!bindFrontierCompressed037<decltype(frontierProps416),FrontierPropsOffset1617>(frontierProps416)) return;
    const auto& stage=courseHazards(currentCourse);
    for (unsigned i=0;i<stage.blockCount;++i) {
        if (!round.brokenBlocks.has(i))
            actor(view,frontierProps416.patch(FrontierDBlockPatch416),stage.blocks[i].center,view.worldScale());
    }
    for (unsigned i=0;i<stage.blockCount;++i) {
        if (round.brokenBlocks.has(i))
            drawDestruction(view,stage.blocks[i].center,round.ticks-round.blockTicks[i],round.blockValues[i]);
    }
}

unsigned frontierStoplightFrame416(unsigned tick) { return tick/31%3; }

void drawFrontierEnemies416(const GameplayView& view,const Round& round) {
    if (!bindFrontierCompressed037<decltype(frontierProps416),FrontierPropsOffset1617>(frontierProps416)) return;
    const auto draw=[&](unsigned index,Vec2 position) {
        actor(view,frontierProps416.patch(index),position,view.worldScale());
    };
    draw(FrontierStoplightPatch416+frontierStoplightFrame416(round.ticks),{-32,64});
    const auto& cars=round.baddies;
    const auto& skulls=round.skullSpawner416;
    const auto& chuckles=round.chuckles;
    for (unsigned i=0;i<FrontierCars416.spawnerCount;++i)
        draw(FrontierSpawnDoorPatch416+cars.doorFrame(i,round.ticks),FrontierCars416.spawners[i].center);
    draw(FrontierSpawnDoorPatch416+skulls.doorFrame(round.ticks),FrontierSkullSpawner416);
    draw(FrontierSpawnDoorPatch416+chuckles.spawnerFrame(round.ticks,0),FrontierChuckleSpawners416[0].center);
    for (unsigned i=0;i<cars.count;++i) {
        if (cars.alive(i)) draw(FrontierCarPatch416+(cars.velocities[i].y>0),cars.center(i));
    }
    for (unsigned i=0;i<skulls.count;++i) {
        if (skulls.bodies[i].radius>0) draw(FrontierSkullPatch416+round.ticks*24/60%24,skulls.bodies[i].center);
    }
    for (unsigned i=0;i<chuckles.count;++i) {
        if (chuckles.bodies[i].radius>0)
            draw(FrontierChucklePatch416+chuckles.actorTick(i,round.ticks)/2%25,chuckles.bodies[i].center);
    }
    for (unsigned i=0;i<cars.count;++i)
        if (cars.defeatedAt[i]) drawDestruction(view,cars.center(i),round.ticks-cars.defeatedAt[i],0);
    for (unsigned i=0;i<skulls.count;++i)
        if (skulls.defeatedAt[i]) drawDestruction(view,skulls.bodies[i].center,round.ticks-skulls.defeatedAt[i],0);
    for (unsigned i=0;i<chuckles.count;++i)
        if (chuckles.defeatedAt[i])
            drawDestruction(view,chuckles.defeatedPosition[i],round.ticks-chuckles.defeatedAt[i],0);
}

void drawFrontierOverlays417(const Round& round) {
    constexpr unsigned switchBytes=FrontierSwitchWidth417*FrontierSwitchHeight417;
    constexpr unsigned gateBytes=FrontierGateWidth417*FrontierGateHeight417;
    unsigned generation=0;
    auto* memory=clips.textureWorkspace<switchBytes+gateBytes>(generation);
    const auto draw=[&](const auto& atlas,unsigned first,unsigned count) {
        setFrontierTexture(atlas);
        for (unsigned i=first;i<first+count;++i) {
            const auto& art=atlas.patch(i);
            patch(art,ViewX+GameplayView::HudWidth+art.x,art.y);
        }
    };
    if (!round.finale417.revealedAt && frontierGateTiles417.useShared(memory+switchBytes,generation))
        draw(frontierGateTiles417,0,FrontierGateCount417);
    const unsigned state=round.switches.state;
    if (state<4 && FrontierSwitchPhases417[state] && frontierSwitchTiles417.useShared(memory,generation)) {
        unsigned first=0;
        for (unsigned i=0;i<state;++i) first+=FrontierSwitchPhases417[i];
        draw(frontierSwitchTiles417,first,FrontierSwitchPhases417[state]);
    }
}

constexpr Vec2 FrontierGuidePath417[]{{432,-64},{432,-256},{432,-448},{240,-448},
                                      {48,-448},{-144,-448},{-336,-448}};

unsigned frontierGuideIndex417(unsigned tick) { return tick/12%std::size(FrontierGuidePath417); }

void drawFrontierProps417(const GameplayView& view,const Round& round) {
    if (!bindFrontierCompressed037<decltype(frontierProps417),FrontierPropsOffset1617>(frontierProps417)) return;
    const float scale=view.worldScale();
    for (unsigned i=0;i<std::size(FrontierSwitchBlocks417);++i)
        actor(view,frontierProps417.patch(FrontierSwitchFacePatch417+round.switches.face(i)),
              FrontierSwitchBlocks417[i].center,scale);
    if (!chillMode)
        actor(view,frontierProps417.patch(FrontierFinalGhostPatch417),round.finale417.finalGhost.position,2*scale);
    if (!round.finale417.revealedAt) return;

    const unsigned age=round.ticks-round.finale417.revealedAt;
    const unsigned guide=frontierGuideIndex417(round.ticks);
    for (unsigned i=0;i<std::size(FrontierGuidePath417);++i) {
        const unsigned frame=FrontierIndicatorPatch417+(i<2?2:0)+(i==guide?1:0);
        actor(view,frontierProps417.patch(frame),FrontierGuidePath417[i],2*scale);
    }
    const unsigned frame=age*25/60;
    if (frame<17) {
        const auto& explosion=frontierProps417.patch(FrontierExplosionPatch417+frame);
        const auto blast=[&](Vec2 world,float width,float height) {
            const Vec2 p=view.position(world);
            patchScaled(explosion,p.x-width*scale/2,p.y-height*scale/2,
                        width*scale/explosion.width,height*scale/explosion.height,White);
        };
        blast({-176,384},160,208);
        blast({-48,224},160,208);
        blast({432,-352},288,272);
    }
    if (age<50) {
        const auto& deadFlag=frontierProps417.patch(FrontierDeadFlagPatch417);
        actor(view,deadFlag,FrontierFakeGoalCenter417+Vec2{32,-48},4*scale,
              GU_RGBA(255,255,255,255*(50-age)/50));
        drawScorePopup(view,FrontierFakeGoalCenter417,age,FrontierFinaleRun417::FakeGoalBonus);
    }
}

void drawFrontierUfos417(const GameplayView& view,const Round& round) {
    if (!bindFrontierCompressed037<decltype(frontierProps417),FrontierPropsOffset1617>(frontierProps417)) return;
    if (round.finale417.keyAvailable) {
        if (!round.finale417.ufoAt) {
            const unsigned frame=std::min(10u,round.ticks/4);
            actor(view,frontierProps417.patch(FrontierKeyHintPatch417+frame),FrontierKeyHint417,
                  view.worldScale(),GU_RGBA(255,255,255,160));
        }
        const unsigned phase=round.finale417.keyAt?2+(round.ticks-round.finale417.keyAt)/4%2:round.ticks/20%2;
        actor(view,frontierProps417.patch(FrontierUfoPatch417+phase),round.finale417.ufoPosition(round.ticks),
              2*view.worldScale(),GU_RGBA(179,179,179,255));
    }
    if (round.finale417.revealedAt) {
        const unsigned age=round.ticks-round.finale417.revealedAt;
        const auto& art=frontierProps417.patch(FrontierChuckleUfoPatch417+age/6%3);
        const float x=ViewX+(944-2*static_cast<float>(age))*17.0f/32;
        if (x+art.x+art.width>ViewX && x+art.x<ViewX+ViewWidth)
            patch(art,x+art.x,153+art.y,1,GU_RGBA(158,122,204,255));
    }
}

void drawFrontierThinker417(const Round& round) {
    const GameplayView view(round.scene,ViewX);
    const auto center=round.finale417.finalGhostActive?view.position(round.finale417.finalGhost.position):
                                                      Vec2{ViewX+204*17.0f/32,468*17.0f/32};
    const auto& art=visual.cat;
    const float scale=(88*17.0f/32)/art.width;
    sceGuScissor(ViewX,0,ViewX+132,Height);
    setTexture(GU_PSM_8888,ActorsWidth,ActorsHeight,actors);
    patch(art,center.x+art.x*scale,center.y+art.y*scale,scale);
    sceGuScissor(0,0,Width,Height);
}

void drawFrontierLighting1617(const GameplayView& view,const Round& round) {
    if (!bindFrontierCompressed037<decltype(frontierLights1617),FrontierLightsOffset1617>(frontierLights1617)) return;
    const bool outpost=currentCourse==CrimsonOutpostCourse;
    drawFrontierLightMap(view,frontierLights1617,FrontierW6LightPatch1617,
                         {0.49f,0.245f,outpost?0.49f:0.408333f},
        [&](const auto& light,const auto& lightSprite) {
            if (outpost) {
                constexpr FrontierLightColor stoplight[]{{1,0,0},{1,1,0},{0,1,0}};
                light({-32,64},192,stoplight[frontierStoplightFrame416(round.ticks)],1);
                return;
            }
            for (Vec2 torch:{Vec2{-496,16},Vec2{480,496},Vec2{352,-32}})
                lightSprite(FrontierTorchLightPatch417,torch,{256,256},FrontierWhiteLight,0.6f);
            light(round.game.position,128,FrontierWhiteLight,0.5f);
            if (!round.finale417.revealedAt) return;
            light({-512,-448},384,FrontierWhiteLight,0.5f);
            const unsigned phase=round.ticks%360,segment=phase/90;
            const float t=(phase%90)/90.0f;
            constexpr float channels[]{1,1,0,1,1},gains[]{1,0,1,0,1};
            const float cyan=channels[segment]+t*(channels[segment+1]-channels[segment]);
            const float alpha=gains[segment]+t*(gains[segment+1]-gains[segment]);
            light({-4,0},1024,{1,cyan,cyan},0.4f*alpha);
            lightSprite(FrontierIndicatorLightPatch417,FrontierGuidePath417[frontierGuideIndex417(round.ticks)],
                        {256,256},{0,1,0},0.6f);
        });
}
