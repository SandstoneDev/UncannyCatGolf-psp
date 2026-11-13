#pragma once

#include "campaign.hpp"

enum class CompletedWorld { None, Tutorial, GolfCentral, GlowstickCity, ChucklePark, FinalFrontier };
enum class WorldResultsChoice { None, LevelSelect, NextWorld };
enum class WorldCue : unsigned { Hop, Unlock, JumpFirst, JumpSecond, LandFirst, LandSecond, Music, HopLate, Wobble, Phwing, Drumroll, DrumrollStop, LandThird, JumpThird, JumpFourth, JumpFifth, JumpSixth, JumpSeventh, JumpEighth, FrontierSilence };

constexpr unsigned GlowstickFirstCourse=TutorialCount+18;
constexpr unsigned ChuckleParkFirstCourse=GlowstickFirstCourse+18;
constexpr unsigned WorldChoiceTimeMs=7950;
constexpr unsigned WorldSkipTimeMs=8950;

constexpr CompletedWorld worldCompletedAt(unsigned nextCourse) {
    if (nextCourse==TutorialCount) return CompletedWorld::Tutorial;
    if (nextCourse==GlowstickFirstCourse) return CompletedWorld::GolfCentral;
    if (nextCourse==ChuckleParkFirstCourse) return CompletedWorld::GlowstickCity;
    if (nextCourse==FinalFrontierFirstCourse) return CompletedWorld::ChucklePark;
    if (nextCourse==ElysianFieldsCourse) return CompletedWorld::FinalFrontier;
    return CompletedWorld::None;
}

constexpr unsigned cueMask(WorldCue cue) {
    return 1u<<static_cast<unsigned>(cue);
}

struct WorldResultsUpdate {
    unsigned elapsedMs,cues;
    WorldResultsChoice choice;
};

class WorldResultsState {
public:
    void begin(CompletedWorld completed,unsigned courseCount,bool first=true) {
        source=completed;
        opening=first;
        available=firstNextCourse()<courseCount;
        selection=available;
        played=0;
    }

    CompletedWorld world() const { return source; }
    bool firstUnlock() const { return opening; }
    unsigned choiceTime() const {
        if (source==CompletedWorld::FinalFrontier) return 9500;
        return source==CompletedWorld::ChucklePark ? 9500 : WorldChoiceTimeMs;
    }
    bool nextAvailable() const { return available; }
    bool nextSelected() const { return selection; }

    unsigned courseFor(WorldResultsChoice choice) const {
        if (choice==WorldResultsChoice::NextWorld && available) return firstNextCourse();
        return source==CompletedWorld::Tutorial?0:firstNextCourse()-1;
    }

    WorldResultsUpdate update(unsigned elapsedMs,bool skip,bool left,bool right,bool confirm,bool back) {
        const unsigned skipTime=source==CompletedWorld::FinalFrontier?10500:
                                source==CompletedWorld::ChucklePark?10950:WorldSkipTimeMs;
        const bool skipped=skip && elapsedMs<skipTime;
        if (skipped) elapsedMs=skipTime;
        const unsigned due=dueCues(elapsedMs);
        unsigned newlyDue=due&~played;
        played|=due;
        if (skipped) newlyDue&=cueMask(WorldCue::Music)|cueMask(WorldCue::FrontierSilence);

        WorldResultsChoice choice=WorldResultsChoice::None;
        if (elapsedMs>=choiceTime()) {
            if (left) selection=false;
            if (right && available) selection=true;
            if (confirm || back)
                choice=confirm && selection ? WorldResultsChoice::NextWorld : WorldResultsChoice::LevelSelect;
        }
        return {elapsedMs,newlyDue,choice};
    }

private:
    CompletedWorld source=CompletedWorld::Tutorial;
    bool available=true,selection=true,opening=true;
    unsigned played=0;

    unsigned firstNextCourse() const {
        if (source==CompletedWorld::FinalFrontier) return ElysianFieldsCourse;
        if (source==CompletedWorld::ChucklePark) return FinalFrontierFirstCourse;
        if (source==CompletedWorld::GlowstickCity) return ChuckleParkFirstCourse;
        return source==CompletedWorld::GolfCentral?GlowstickFirstCourse:TutorialCount;
    }

    unsigned dueCues(unsigned elapsedMs) const {
        if (source==CompletedWorld::FinalFrontier) {
            unsigned cues=0;
            if (elapsedMs>=550) cues|=cueMask(WorldCue::Hop);
            if (opening && elapsedMs>=3550) cues|=cueMask(WorldCue::Unlock);
            constexpr WorldCue jumps[]={WorldCue::JumpFirst,WorldCue::JumpSecond,WorldCue::JumpThird,WorldCue::JumpFourth,
                WorldCue::JumpFifth,WorldCue::JumpSixth,WorldCue::JumpSeventh,WorldCue::JumpEighth};
            for (unsigned i=0;i<sizeof jumps/sizeof *jumps;++i)
                if (elapsedMs>=4550+500*i) cues|=cueMask(jumps[i]);
            if (elapsedMs>=8550) cues|=cueMask(WorldCue::FrontierSilence);
            return cues;
        }
        if (source==CompletedWorld::ChucklePark) {
            unsigned cues=0;
            if (elapsedMs>=550) cues|=cueMask(WorldCue::Hop);
            if (opening && elapsedMs>=3550) cues|=cueMask(WorldCue::Unlock);
            if (elapsedMs>=4550) cues|=cueMask(WorldCue::JumpFirst);
            if (elapsedMs>=5150) cues|=cueMask(WorldCue::Drumroll);
            if (elapsedMs>=6050) cues|=cueMask(WorldCue::Phwing);
            if (elapsedMs>=6083) cues|=cueMask(WorldCue::DrumrollStop);
            if (elapsedMs>=7550) cues|=cueMask(WorldCue::LandFirst);
            if (elapsedMs>=8150) cues|=cueMask(WorldCue::LandSecond);
            if (elapsedMs>=8483) cues|=cueMask(WorldCue::LandThird);
            if (elapsedMs>=9050) cues|=cueMask(WorldCue::Music);
            return cues;
        }
        if (source==CompletedWorld::GlowstickCity) {
            unsigned cues=0;
            if (elapsedMs>=550) cues|=cueMask(WorldCue::Hop);
            if (opening && elapsedMs>=3550) cues|=cueMask(WorldCue::Unlock);
            if (elapsedMs>=4550) cues|=cueMask(WorldCue::JumpFirst);
            if (elapsedMs>=5050) cues|=cueMask(WorldCue::JumpSecond);
            if (elapsedMs>=5750) cues|=cueMask(WorldCue::HopLate);
            if (elapsedMs>=6050) cues|=cueMask(WorldCue::Wobble);
            if (elapsedMs>=7050) cues|=cueMask(WorldCue::Music);
            return cues;
        }
        if (source!=CompletedWorld::GolfCentral)
            return elapsedMs>=6500 ? cueMask(WorldCue::Music) : 0;

        // ToW2 starts 0.30 s into the recorded clip. The offsets are from the PC animation tracks.
        constexpr unsigned cueTimes[]={300,3300,4300,4800,5500,5800,6300};
        unsigned cues=0;
        for (unsigned i=0;i<sizeof cueTimes/sizeof *cueTimes;++i) {
            if (elapsedMs>=cueTimes[i]) cues|=1u<<i;
        }
        return cues;
    }
};
