#pragma once

#include "draw_plants051.hpp"

void drawCosmicPlants046(const GameplayView& view) {
    if (cosmicPlantSlot046.owner!=&cosmicPlants046) return;
    sceGuScissor(ViewX+GameplayView::HudWidth,0,ViewX+ViewWidth,Height);
    constexpr CosmicPlant p2[]={{{224,304},0.10f},{{288,-336},0.14f},{{-352,0},0.125f},
                          {{-320,320},0.125f},{{-136,64},0.08f}};
    constexpr CosmicPlant p3[]={{{32,-16},0.12f},{{64,-472},0.12f},{{-384,-464},0.12f},
                          {{-432,-128},0.12f},{{496,24},0.12f},{{-464,472},0.12f}};
    const bool third=currentCourse==CosmicThirdCourse;
    const auto* plants=third?p3:p2;
    const unsigned count=third?std::size(p3):std::size(p2);
    drawCosmicPlantActors(view,cosmicPlantSlot046,plants,count,CosmicPlants046SourceScale);
}

void drawCosmicBlocks602(const GameplayView& view,const Round& round) {
    const auto& stage=courseHazards(currentCourse);
    setTexture(GU_PSM_8888,CosmicBlocks602Width,CosmicBlocks602Height,cosmicBlocks602.texture());
    for (unsigned i=0;i<stage.blockCount;++i)
        if (!round.brokenBlocks.has(i))
            actor(view,cosmicBlocks602.patch(0),stage.blocks[i].center,view.worldScale());
    for (unsigned i=0;i<stage.blockCount;++i)
        if (round.brokenBlocks.has(i))
            drawDestruction(view,stage.blocks[i].center,round.ticks-round.blockTicks[i],round.blockValues[i]);
}

void drawCosmicDoor602(const GameplayView& view,const Round& round) {
    const Vec2 center=CosmicKeys602.doors[0].center;
    const float scale=view.worldScale();
    const auto& state=round.roundKeys;
    setTexture(GU_PSM_8888,CosmicBlocks602Width,CosmicBlocks602Height,cosmicBlocks602.texture());
    if (!(state.opened&1u)) {
        actor(view,cosmicBlocks602.patch(1),center,scale);
        const unsigned alpha=static_cast<unsigned>(255*state.lockAlpha[0]);
        actor(view,cosmicBlocks602.patch(2),center,scale,GU_RGBA(255,255,255,alpha));
    } else {
        drawDestruction(view,center,round.ticks-state.openedAt[0],doorScore(currentCourse));
    }
}
