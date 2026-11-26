#pragma once
#include "campaign.hpp"
#include <array>

namespace EndlessRules {

constexpr unsigned First=CourseCount;
constexpr unsigned Unique=10;
constexpr unsigned E04=First+4;
constexpr unsigned BagSize=81, RotationSize=BagSize+1;
constexpr unsigned TickRate=60, StartingTime=61*TickRate, DemonMax=120*TickRate;
enum class Advance { Win, Reset, Skip };
enum Mod : unsigned { Panic=1, Jumpy=2, Dizzy=4, Missiles=8, Sticky=16,
                      Bort=32, Bees=64, Overheat=128, Pharaoh=256 };
constexpr unsigned AllMods=511;

inline bool unlocked(const Progress& p) {
    return p.next>CataclysmCourse || p.best[CataclysmCourse].time!=UINT32_MAX;
}

struct Run {
    std::array<unsigned,BagSize> bag{};
    uint32_t random=1;
    unsigned slot=0,rotations=0,time=StartingTime,skips=3,score=0;
    int64_t scoreTillSkip=200000;
    unsigned demon=DemonMax,modCountdown=8,modBase=0,minimum=0,mods=0;
    bool demonActive=false,radical=false;

    unsigned draw(unsigned limit) {
        random^=random<<13;
        random^=random>>17;
        random^=random<<5;
        return random%limit;
    }

    void shuffle() {
        for (unsigned n=BagSize;n>1;--n) std::swap(bag[n-1],bag[draw(n)]);
    }

    void begin(uint32_t seed) {
        *this={};
        random=seed?seed:1;
        unsigned i=0;
        for (unsigned n=1;n<Unique;++n) bag[i++]=First+n;
        for (unsigned world=0;world<=4;++world) {
            const unsigned begin=world==0?0:world==1?TutorialCount:world==2?GlowstickFirstCourse:
                                 world==3?ChuckleFirstCourse:FinalFrontierFirstCourse;
            const unsigned count=world==0?TutorialCount-1:17;
            for (unsigned n=0;n<count;++n) bag[i++]=begin+n;
        }
        shuffle();
    }

    unsigned active() const { return slot?bag[slot-1]:First; }
    unsigned next() const { return slot<BagSize?bag[slot]:First; }
    unsigned cleared() const { return rotations*RotationSize+slot; }
    unsigned progressForUnlock() const { return rotations*BagSize+slot; }

    void updateModifiers(unsigned blacklist) {
        unsigned candidates=AllMods&~blacklist;
        mods=0;
        unsigned add=minimum;
        if (--modCountdown==0) {
            const unsigned age=progressForUnlock()-modBase;
            modCountdown=age<24?8:age<48?4:age<72?2:8;
            if (age>=72) { modBase=progressForUnlock(); ++minimum; }
            ++add;
        }
        for (unsigned i=0;i<add && candidates;++i) {
            unsigned which=draw(static_cast<unsigned>(__builtin_popcount(candidates)));
            unsigned bit=1;
            for (;;bit<<=1) {
                if (!(candidates&bit)) continue;
                if (!which--) break;
            }
            mods|=bit;
            candidates&=~bit;
        }
    }

    bool transition(Advance how,unsigned finalScore,unsigned onTimeTicks) {
        if (how==Advance::Skip && !skips) return false;
        if (how==Advance::Reset) {
            const unsigned penalty=(demonActive && demon<DemonMax?5:30)*TickRate;
            demon=demon>penalty?demon-penalty:0;
        } else {
            if (++slot>BagSize) { shuffle(); ++rotations; slot=0; }
            if (how==Advance::Skip) --skips;
            else {
                score+=finalScore;
                time+=onTimeTicks;
                scoreTillSkip-=finalScore;
                if (scoreTillSkip<=0) {
                    ++skips;
                    scoreTillSkip+=500000;
                }
            }
        }
        if (!demon && !demonActive) { demonActive=true; radical=draw(100)==0; }
        else if (demon>=DemonMax && demonActive) demonActive=radical=false;
        return true;
    }

    void tick(bool timer) {
        if (timer && time) --time;
        if (mods&Overheat) { if (demon) --demon; }
        else if (demon<DemonMax) ++demon;
    }

    void demonHit() { time=time>5*TickRate?time-5*TickRate:0; }
};

struct Rank { unsigned score=0,cleared=0; char name[32]{}; };
constexpr Rank DeveloperRanks[]={
    {1319091,168,"FOGWAVES"},{924992,113,"SH2K"},{500000,75,"TUFFKNUCKLES"},
    {116940,16,"PENNY RIGATE"},{98330,14,"BIBIZU"},{53969,7,"H-L"}
};

} // namespace EndlessRules
