#pragma once
#include "world_awards.hpp"

// Event order from pthru.gd, pthru.tscn, master.tscn and endless.gd. The owner
// supplies the just-recorded profile snapshot and commits the resulting Data.
namespace WorldProgress054 {
template<class Progress>
uint8_t keys(const Progress& p) {
    return uint8_t((!!p.terryKey) | (!!p.golfKey<<1) | (!!p.glowstickKey<<2) |
        (!!p.parkKey<<3) | (!!p.frontierKey<<4) | (!!p.elysianKey<<5) |
        (!!p.cosmicKey<<6) | (!!p.endlessKey<<7));
}
template<class Progress>
bool rememberShared(WorldState::Data& d,const Progress& p) {
    const unsigned phase=p.quest>6 ? 6 : p.quest;
    const uint8_t found=keys(p),previous=d.questKeys;
    const bool changed=phase>d.friendPhase || (found&~previous);
    if (phase>d.friendPhase) d.friendPhase=uint8_t(phase);
    d.questKeys|=found;
    return changed;
}
template<class Progress>
void applyShared(Progress& p,const WorldState::Data& d) {
    if (p.quest<d.friendPhase) p.quest=d.friendPhase;
    p.terryKey|=!!(d.questKeys&1);p.golfKey|=!!(d.questKeys&2);
    p.glowstickKey|=!!(d.questKeys&4);p.parkKey|=!!(d.questKeys&8);
    p.frontierKey|=!!(d.questKeys&16);p.elysianKey|=!!(d.questKeys&32);
    p.cosmicKey|=!!(d.questKeys&64);p.endlessKey|=!!(d.questKeys&128);
}

template<class Progress>
bool completed(WorldState::Data& data,const Progress& progress,unsigned course,
               bool skipped,bool scoreVisible) {
    if (course>=113 || !WorldState::valid(data)) return false;
    bool changed=false;
    const auto give=[&](unsigned id) { changed=WorldState::award(data,id) || changed; };
    if (!skipped || (course!=94 && course!=112)) {
        switch(course) {
        case 4: give(0);give(32);break;
        case 22: give(2);give(3);break;
        case 40: give(4);break;
        case 58: give(5);break;
        case 75: give(29);give(30);give(31);give(6);break;
        case 94: give(33);give(43);break;
        case 112: give(34);give(44);break;
        default: break;
        }
    }
    if (skipped) return changed;
    if (scoreVisible && progress.best[course].rank>=5) {
        switch(course) {
        case 32: give(36);break;
        case 38: give(35);break;
        case 48: give(37);break;
        case 64: give(38);break;
        default: break;
        }
    }
    constexpr struct {unsigned id,prisms;} thresholds[]={
        {23,500},{15,150},{21,400},{25,565},{16,200},{20,375},
        {19,325},{17,250},{18,300},{22,450},{24,525},{14,100}
    };
    const auto prisms=progress.totalPrisms();
    for (const auto& row:thresholds) if (prisms>=row.prisms) give(row.id);
    const unsigned world=course<5 ? 0 : 1+(course-5)/18;
    const unsigned begin=world ? 5+(world-1)*18 : 0,end=world ? begin+18 : 5;
    bool peaked=true;
    for (unsigned i=begin;i<end;++i) if (progress.best[i].rank<5) {peaked=false;break;}
    if (peaked) {
        constexpr struct {unsigned id,world;} worlds[]={
            {12,5},{7,0},{10,3},{8,1},{13,6},{9,2},{11,4},{1,0}
        };
        for (const auto& row:worlds) if (row.world==world) give(row.id);
    }
    return changed;
}

inline bool endless(WorldState::Data& data,unsigned progress,unsigned score,bool lost) {
    bool changed=false;
    const auto give=[&](unsigned id) { changed=WorldState::award(data,id) || changed; };
    // Source dictionary order differs from numerical threshold order.
    if (progress>=100) give(40);
    if (progress>=50) give(39);
    if (progress>=150) give(41);
    if (lost && score>500000) give(26);
    return changed;
}
} // namespace WorldProgress054
