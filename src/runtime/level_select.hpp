#pragma once

#include "campaign.hpp"
#include "world_transition.hpp"

enum class LevelSelectDirection { Left, Right, Up, Down };

class LevelSelectState {
public:
    static constexpr unsigned WorldCount=7, Columns=3, NoCourse=UINT32_MAX;

    static constexpr unsigned firstCourse(unsigned world) {
        constexpr unsigned starts[]={0,TutorialCount,GlowstickFirstCourse,ChuckleParkFirstCourse,FinalFrontierFirstCourse,ElysianFieldsCourse,CosmicFirstCourse};
        return world<WorldCount ? starts[world] : NoCourse;
    }

    static constexpr unsigned cells(unsigned world) {
        constexpr unsigned counts[]={TutorialCount,GlowstickFirstCourse-TutorialCount,
                                     ChuckleParkFirstCourse-GlowstickFirstCourse,18,18,18,18};
        return world<WorldCount ? counts[world] : 0;
    }

    struct Travel {
        unsigned from=NoCourse, startMs=0;
    };

    void open(const Progress& progress,unsigned playableCount,unsigned preferredCourse=NoCourse) {
        const unsigned count=std::min(playableCount,CourseCount);
        last=count ? std::min(static_cast<unsigned>(progress.next),count-1) : NoCourse;
        cursor={};
        travel={};
        const unsigned selected=last!=NoCourse && preferredCourse<last ? preferredCourse : last;
        currentWorld=selected==NoCourse ? 0 : worldForCourse(selected);
        if (last!=NoCourse) cursor[currentWorld]=selected-firstCourse(currentWorld);
    }

    unsigned world() const { return currentWorld; }
    unsigned cell(unsigned world) const { return cursor[world]; }
    unsigned cell() const { return cell(currentWorld); }
    unsigned course(unsigned world) const {
        return unlocked(world,cell(world)) ? firstCourse(world)+cell(world) : NoCourse;
    }
    unsigned course() const { return course(currentWorld); }
    const Travel& transition() const { return travel; }

    bool unlocked(unsigned world,unsigned index) const {
        return last!=NoCourse && index<cells(world) && firstCourse(world)+index<=last;
    }

    bool canChangeWorld(int direction) const {
        if (direction<0) return direction==-1 && currentWorld>0;
        return direction==1 && unlocked(currentWorld+1,0);
    }

    bool changeWorld(int direction,unsigned elapsedMs=0) {
        if (!canChangeWorld(direction)) return false;
        travel={currentWorld,elapsedMs};
        currentWorld=static_cast<unsigned>(static_cast<int>(currentWorld)+direction);
        return true;
    }

    bool move(LevelSelectDirection direction) {
        if (last==NoCourse) return false;
        const unsigned index=cell(), column=index%Columns;
        unsigned next=index;
        switch (direction) {
        case LevelSelectDirection::Left:
            if (column) next=index-1;
            break;
        case LevelSelectDirection::Right:
            if (column+1<Columns) next=index+1;
            break;
        case LevelSelectDirection::Up:
            if (index>=Columns) next=index-Columns;
            break;
        case LevelSelectDirection::Down: {
            const unsigned rowStart=(index/Columns+1)*Columns;
            if (unlocked(currentWorld,rowStart))
                next=std::min({rowStart+column,last-firstCourse(currentWorld),cells(currentWorld)-1});
            break;
        }
        }
        if (next==index || !unlocked(currentWorld,next)) return false;
        cursor[currentWorld]=next;
        return true;
    }

private:
    static_assert(CourseCount<=CosmicFirstCourse+18,"Add a selector world for the new courses");

    static unsigned worldForCourse(unsigned course) {
        if (course<firstCourse(1)) return 0;
        if (course<firstCourse(2)) return 1;
        if (course<firstCourse(3)) return 2;
        if (course<firstCourse(4)) return 3;
        if (course<firstCourse(5)) return 4;
        return course<firstCourse(6)?5:6;
    }

    std::array<unsigned,WorldCount> cursor{};
    unsigned last=NoCourse,currentWorld=0;
    Travel travel;
};
