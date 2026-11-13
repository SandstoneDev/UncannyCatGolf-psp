#pragma once

#include "elysian_geometry040.hpp"

inline void stepElysianSkulls040(const TileField& walls,const GolfStage& course,BlockMask broken,
                                SkullRun& skulls,unsigned skullCount,BaddieRun& baddies,
                                float bounds,unsigned tick) {
    skulls.step(walls,skullCount,baddies.skulls,baddies.count,bounds+128.0f,
                nullptr,nullptr,course.blocks,course.blockCount,broken);
    baddies.step(walls,tick,skulls.bodies,skullCount,nullptr,course.blocks,course.blockCount,broken);
}
