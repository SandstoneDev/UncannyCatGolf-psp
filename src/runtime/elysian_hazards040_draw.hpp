#pragma once

void drawElysianBlocks509(const GameplayView& view,const Round& round) {
    static_assert(ElysianClouds509Width*ElysianClouds509Height==ElysianClouds510Width*ElysianClouds510Height);
    constexpr unsigned offset=HorseCloudOffset039+ElysianClouds509Width*ElysianClouds509Height;
    if (!bindWallAtlas418<decltype(elysianBlock509),offset>(elysianBlock509)) return;
    const auto& course=courseHazards(currentCourse);
    for (unsigned i=0;i<course.blockCount;++i) {
        const auto& block=course.blocks[i];
        if (round.brokenBlocks.has(i)) {
            drawDestruction(view,block.center,round.horse.age-round.blockTicks[i],round.blockValues[i]);
        } else {
            const unsigned type=block.halfSize.y==64?2:block.halfSize.x==48?1:0;
            actor(view,elysianBlock509.patch(type),block.center,view.worldScale(),
                  elysianHazardTint040(currentCourse),true);
        }
    }
}
