#pragma once

#include "baddie_run.hpp"
#include "chuckle_run.hpp"
#include "nuke.hpp"

inline void stepElysianThreats514(BaddieRun& skulls,ChuckleRun& chuckles,const TileField& walls,
                                 const SwitchRun& switches,Vec2 player,unsigned tick) {
    skulls.step(walls,tick,chuckles.bodies,chuckles.count,nullptr,nullptr,0,{},&switches);
    chuckles.step(walls,player,nullptr,0,{},nullptr,0,0,&switches);
}

inline void clearElysianThreats514(BaddieRun& skulls,ChuckleRun& chuckles) {
    for (unsigned i=0;i<skulls.count;++i) {
        if (skulls.alive(i)) skulls.remove(i);
        skulls.defeatedAt[i]=0;
    }
    skulls.count=0;
    chuckles.clearSpawned();
}
