#pragma once

#include "draw_plants051.hpp"

void prepareCosmicDraw051(const Round& round) {
    const bool disco=currentCourse==CosmicTwelfthCourse;
    const unsigned state=disco?round.switches.state:0;
    const unsigned course=currentCourse-CosmicSecondCourse;
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    const auto prepare=[&](LiveResultArt& art,unsigned frame,LiveResultArt::Slot& slot,uint8_t* address,unsigned capacity,bool cached=false) {
        art.prepare(frame,slot,address,capacity,scratch,ResultScratchBytes,cached);
        art.closeStream();
    };
    prepare(cosmicField046[course],state,cosmicFieldSlot046,
            disco?vram045(CosmicFieldOffset045):goalFlags.scratch(),
            disco?CosmicFieldBytes046:CosmicGoalScratchOffset613,!disco);
    if (disco) {
        prepare(cosmicFlag046[course],round.ticks/5%4,cosmicFlagSlot046,vram045(CosmicFlagOffset046),CosmicFlagBytes046);
        prepare(cosmicLightField612,state,cosmicLightSlot612,goalFlags.scratch(),GoalFlagArt::ScratchBytes,true);
    } else {
        prepare(cosmicGlass613,cosmicGlossPhase048(round.ticks),cosmicGlassSlot613,
                vram045(CosmicGlassOffset613),CosmicGloss613VramBytes);
        prepare(cosmicPlants613,round.ticks*CosmicPlants613Fps/60%CosmicPlants613Count,
                cosmicPlantsSlot613,vram045(CosmicPlantsOffset613),CosmicPlants613VramBytes);
    }
    fallbackPageIndex=ScreenPages;
}

void drawDiscoLights612(const DiscoLight051& lights) {
    if (cosmicLightSlot612.owner!=&cosmicLightField612) return;
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_FIX,0,0xffffffffu);
    constexpr unsigned Step=16;
    for (unsigned i=0;i<cosmicLightSlot612.sprite.count;++i) {
        const auto& image=cosmicLightSlot612.images[i];
        const auto& tile=image.tile;
        const auto extent=[](unsigned size) {
            unsigned power=1;
            while (power<size) power*=2;
            return power;
        };
        setTexture(GU_PSM_8888,extent(tile.width),extent(tile.height),image.pixels);
        sceGuTexImage(0,extent(tile.width),extent(tile.height),tile.pitch,image.pixels);
        const float originX=ViewX+cosmicLightSlot612.sprite.x+tile.x;
        const float originY=cosmicLightSlot612.sprite.y+tile.y;
        const unsigned columns=(tile.width+Step-1)/Step,rows=(tile.height+Step-1)/Step;
        const unsigned count=(columns+1)*2;
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(rows*count*sizeof(TextureVertex)));
        for (unsigned row=0;row<rows;++row) {
            auto* strip=vertices+row*count;
            for (unsigned column=0;column<=columns;++column) {
                const float x=std::min<unsigned>(column*Step,tile.width);
                for (unsigned edge=0;edge<2;++edge) {
                    const float y=std::min<unsigned>((row+edge)*Step,tile.height);
                    const Vec2 position{originX+x,originY+y};
                    strip[column*2+edge]={x,y,lights.tint(position,false),position.x,position.y,0};
                }
            }
            sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                           count,nullptr,strip);
        }
    }
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
}

void drawCosmicSky613(unsigned tick) {

    constexpr unsigned Side=CosmicSky613Width;
    static_assert(Side==CosmicSky613Height);
    const float offset=std::fmod(tick*(50.0f/60.0f),static_cast<float>(Side));
    constexpr unsigned Colors[][3]={{255,0,0},{170,255,0},{0,255,170},{0,0,255},{255,0,0}};
    const unsigned segment=tick%600/150,phase=tick%150;
    const auto channel=[&](unsigned i) { return (Colors[segment][i]*(150-phase)+Colors[segment+1][i]*phase)/150; };
    const auto tint=GU_RGBA(channel(0),channel(1),channel(2),255);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_T8,0,0,GU_TRUE);
    sceGuClutMode(GU_PSM_8888,0,255,0);
    sceGuClutLoad(32,cosmicSkyPalette613);
    sceGuTexImage(0,Side,Side,Side,vram045(CosmicEarthboundOffset045));
    sceGuTexFlush();
    sceGuTexWrap(GU_REPEAT,GU_REPEAT);
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
    vertices[0]={0,Side/2.0f-offset,tint,ViewX+136.0f,0,0};
    vertices[1]={2*Side,Side*2.5f-offset,tint,ViewX+408.0f,272,0};
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,vertices);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
}

void drawCosmicField051(const GameplayView& view,const Round& round) {
    if (round.game.won || round.caught || !cosmicFieldSlot046.owner) return;
    sceGuScissor(ViewX+136,0,ViewX+408,Height);
    if (currentCourse==CosmicTwelfthCourse) {
        const DiscoLight051 lights(view,round.ticks);
        drawCosmicEarthbound045(round.ticks,0.125f,1.625f,0.125f,1.625f,CosmicShade611,true,&lights);
        drawCosmicParticles601(round.ticks,view.worldScale(),CosmicShade611);
        drawLiveResultSprite(cosmicFieldSlot046);
        drawDiscoLights612(lights);
    } else {
        rectangle(ViewX+136,0,272,272,0xff000000u);
        drawCosmicSky613(round.ticks);
        drawCosmicParticles601(round.ticks,view.worldScale());
        drawLiveResultSprite(cosmicFieldSlot046);
        if (cosmicGlassSlot613.owner) drawLiveResultSprite(cosmicGlassSlot613);
        constexpr CosmicPlant plants[]={{{0,-128},0.10f},{{216,352},0.10f},{{-80,-352},0.14f},
                                         {{512,16},0.15f},{{-368,-248},0.125f},{{-176,144},0.125f},
                                         {{-352,352},0.08f}};
        if (cosmicPlantsSlot613.owner)
            drawCosmicPlantActors(view,cosmicPlantsSlot613,plants,std::size(plants),CosmicPlants613SourceScale);
    }
    sceGuScissor(0,0,Width,Height);
}

void drawCosmicSwitches612(const GameplayView& view,const Round& round) {
    if (!cosmicSwitchFaces046.texture()) return;
    setTexture(GU_PSM_8888,ElysianSwitchFaces514Width,ElysianSwitchFaces514Height,cosmicSwitchFaces046.texture());
    const DiscoLight051 lights(view,round.ticks);
    for (const auto& block:CosmicSwitches612)
        actor(view,cosmicSwitchFaces046.patch(round.switches.state),block.center,view.worldScale(),
              lights.tint(view.position(block.center),true));
}

void drawCosmicKey613(const GameplayView& view,const Round& round) {
    if (progress.quest<2 || progress.cosmicKey || round.game.won || round.ticks<28 || round.ticks>=68) return;
    setTexture(GU_PSM_8888,CosmicMovingGoal613Width,CosmicMovingGoal613Height,cosmicMovingGoal613.texture());
    const unsigned frame=CosmicKeyHint613Patch+(round.ticks-28)/4;
    actor(view,cosmicMovingGoal613.patch(frame),CosmicKeyHint613,view.worldScale(),GU_RGBA(255,255,255,160));
}

void drawMovingGoal613(const GameplayView& view,const Round& round,unsigned resultMs) {
    if (!cosmicMovingGoal613.texture()) return;
    const auto& goal=round.movingGoal;
    const auto position=goal.position();
    const float zoom=view.worldScale();
    const auto draw=[&](unsigned frame,Vec2 offset) {
        actor(view,cosmicMovingGoal613.patch(frame),position+offset,zoom);
    };
    setTexture(GU_PSM_8888,CosmicMovingGoal613Width,CosmicMovingGoal613Height,cosmicMovingGoal613.texture());
    draw(0,{});
    draw(1+round.ticks/5%4,{});
    const unsigned pathTick=goal.pathTick;
    const unsigned animationTick=goal.face==MovingGoal051::Face::Normal?round.ticks:round.ticks-goal.changedAt+resultMs*60/1000;
    const auto& legs=cosmicMovingGoal613.patch(5+pathTick*20/60%11);
    GameplayView::Sprite sprite;
    if (view.clipSprite(legs,position,zoom,sprite)) {
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
        const auto u=[&](float value) { return goal.flipLegs()?2*legs.u+legs.width-value:value; };
        vertices[0]={u(sprite.uvTopLeft.x),sprite.uvTopLeft.y,White,sprite.topLeft.x,sprite.topLeft.y,0};
        vertices[1]={u(sprite.uvBottomRight.x),sprite.uvBottomRight.y,White,sprite.bottomRight.x,sprite.bottomRight.y,0};
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,vertices);
    }
    const unsigned face=goal.face==MovingGoal051::Face::Normal?16+animationTick*5/60%3:
                        goal.face==MovingGoal051::Face::Laugh?19+animationTick*7/60%2:21+animationTick*4/60%2;
    draw(face,{});
}
