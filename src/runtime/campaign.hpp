#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

constexpr unsigned TutorialCount=5;
constexpr unsigned CourseCount=113;
constexpr unsigned TowerCourse=38, CasinoCourse=39, GlowstickFinaleCourse=40;
constexpr unsigned ChuckleFirstCourse=GlowstickFinaleCourse+1;
constexpr unsigned CarouselCourse=ChuckleFirstCourse+1, SwitchCourse=CarouselCourse+1;
constexpr unsigned JumbleCourse=SwitchCourse+1, CoasterCourse=JumbleCourse+1;
constexpr unsigned MartCourse=CoasterCourse+1, FactoryCourse=MartCourse+1;
constexpr unsigned DenseCourse=FactoryCourse+1, MirrorCourse=DenseCourse+1;
constexpr unsigned LightsCourse=MirrorCourse+1, ArcadeCourse=LightsCourse+1;
constexpr unsigned PursuitCourse=ArcadeCourse+1, DogCourse=PursuitCourse+1;
constexpr unsigned BalloonDogCourse=DogCourse+1, ScreenDogCourse=BalloonDogCourse+1;
constexpr unsigned DoubleDogCourse=ScreenDogCourse+1, RacetrackCourse=DoubleDogCourse+1;
constexpr unsigned ChuckleCoasterCourse=RacetrackCourse+1;
constexpr unsigned FinalFrontierFirstCourse=ChuckleCoasterCourse+1;
constexpr unsigned FrontierSwitchCourse=FinalFrontierFirstCourse+1;
constexpr unsigned FrontierPortalCourse=FrontierSwitchCourse+1;
constexpr unsigned WholesomeCourse=FinalFrontierFirstCourse+3;
constexpr unsigned CubeTownCourse=WholesomeCourse+1;
constexpr unsigned FrontierMazeCourse=CubeTownCourse+1;
constexpr unsigned FrontierDogCourse=FrontierMazeCourse+1;
constexpr unsigned PlentifulChucklesCourse=FinalFrontierFirstCourse+7;
constexpr unsigned UncannyValleyCourse=PlentifulChucklesCourse+1;
constexpr unsigned FrontierPatrolCourse=UncannyValleyCourse+1;
constexpr unsigned FrontierMineCourse=FrontierPatrolCourse+1;
constexpr unsigned WeatherAlertCourse=FrontierMineCourse+1;
constexpr unsigned DownfallCourse=WeatherAlertCourse+1;
constexpr unsigned MovieOnceCourse=DownfallCourse+1;
constexpr unsigned AbandonedWorkspaceCourse=MovieOnceCourse+1;
constexpr unsigned CrimsonOutpostCourse=AbandonedWorkspaceCourse+1;
constexpr unsigned CataclysmCourse=CrimsonOutpostCourse+1;
constexpr unsigned TheWallCourse=CataclysmCourse+1;
constexpr unsigned ElysianFieldsCourse=TheWallCourse+1;
constexpr unsigned ElysianRocksCourse=ElysianFieldsCourse+1;
constexpr unsigned HorseBroadcastCourse=ElysianRocksCourse+1;
constexpr unsigned FirstHorseCourse=HorseBroadcastCourse+1;
constexpr unsigned SecondHorseCourse=FirstHorseCourse+1;
constexpr unsigned Elysian506Course=SecondHorseCourse+1;
constexpr unsigned Elysian507Course=Elysian506Course+1;
constexpr unsigned Elysian508Course=Elysian507Course+1;
constexpr unsigned Elysian509Course=Elysian508Course+1;
constexpr unsigned Elysian510Course=Elysian509Course+1;
constexpr unsigned Elysian511Course=Elysian510Course+1;
static_assert(Elysian510Course==86 && Elysian511Course==87);
constexpr unsigned Elysian512Course=Elysian511Course+1;
constexpr unsigned Elysian513Course=Elysian512Course+1;
static_assert(Elysian512Course==88 && Elysian513Course==89);
constexpr unsigned Elysian514Course=Elysian513Course+1;
constexpr unsigned Elysian515Course=Elysian514Course+1;
static_assert(Elysian514Course==90 && Elysian515Course==91);
constexpr unsigned Elysian516Course=Elysian515Course+1;
constexpr unsigned Elysian517Course=Elysian516Course+1;
static_assert(Elysian516Course==92 && Elysian517Course==93);
constexpr unsigned Elysian518Course=Elysian517Course+1;
constexpr unsigned CosmicFirstCourse=Elysian518Course+1;
constexpr unsigned CosmicSecondCourse=CosmicFirstCourse+1;
constexpr unsigned CosmicThirdCourse=CosmicSecondCourse+1;
constexpr unsigned CosmicFourthCourse=CosmicThirdCourse+1;
constexpr unsigned CosmicFifthCourse=CosmicFourthCourse+1;
constexpr unsigned CosmicSixthCourse=CosmicFifthCourse+1;
constexpr unsigned CosmicSeventhCourse=CosmicSixthCourse+1;
static_assert(CosmicSixthCourse==100 && CosmicSeventhCourse==101);
constexpr unsigned CosmicEighthCourse=CosmicSeventhCourse+1;
constexpr unsigned CosmicNinthCourse=CosmicEighthCourse+1;
static_assert(CosmicEighthCourse==102 && CosmicNinthCourse==103);
constexpr unsigned CosmicTenthCourse=CosmicNinthCourse+1, CosmicEleventhCourse=CosmicTenthCourse+1;
constexpr unsigned CosmicTwelfthCourse=CosmicEleventhCourse+1, CosmicThirteenthCourse=CosmicTwelfthCourse+1;
constexpr unsigned CosmicFourteenthCourse=CosmicThirteenthCourse+1, CosmicFifteenthCourse=CosmicFourteenthCourse+1;
constexpr unsigned CosmicSixteenthCourse=CosmicFifteenthCourse+1, CosmicSeventeenthCourse=CosmicSixteenthCourse+1;
constexpr unsigned CosmicEighteenthCourse=CosmicSeventeenthCourse+1;
static_assert(CosmicEighteenthCourse+1==CourseCount);
constexpr bool usesMetronome048(unsigned index) { return index>=CosmicSixthCourse && index<=CosmicEleventhCourse; }
constexpr bool usesPolyrhythm049(unsigned index) { return index>=CosmicNinthCourse && index<=CosmicEleventhCourse; }
static_assert(CosmicFourthCourse==98 && CosmicFifthCourse==99);
static_assert(CosmicSecondCourse==96 && CosmicThirdCourse==97);
static_assert(Elysian518Course==94 && CosmicFirstCourse==95);

constexpr bool hidesElysianScoring(unsigned index) {
    return index>=ElysianFieldsCourse && index<=HorseBroadcastCourse;
}
constexpr unsigned SaveCourseCapacity=128;
constexpr unsigned MaxCoins=160;

struct Score { unsigned strokeBonus,timeBonus,total,rank; };

inline Score calculateScore(unsigned par,unsigned strokes,unsigned clock,unsigned onTime,
                            unsigned bonus,const unsigned* thresholds) {
    const unsigned strokeLimit=10+par, timeLimit=2500+onTime;
    const unsigned strokeBonus=500*(strokeLimit-std::min(strokes,strokeLimit));
    const unsigned timeBonus=timeLimit-std::min(clock,timeLimit);
    const unsigned total=strokeBonus+timeBonus+bonus;
    unsigned rank=0;
    for (unsigned i=0;i<5;++i) rank+=total>=thresholds[i];
    return {strokeBonus,timeBonus,total,rank};
}

struct CoinRun {
    std::array<uint32_t,(MaxCoins+31)/32> collected{};
    unsigned multiplier=0,bonus=0;
    unsigned ticks[MaxCoins]{}, values[MaxCoins]{};

    bool has(unsigned index) const { return collected[index/32]&(1u<<(index%32)); }

    bool empty() const {
        return std::all_of(collected.begin(),collected.end(),[](uint32_t word) { return !word; });
    }

    unsigned collect(unsigned index,unsigned tick,unsigned base,unsigned step) {
        if (has(index)) return 0;
        collected[index/32]|=1u<<(index%32);
        ticks[index]=tick;
        values[index]=base+step*multiplier;
        bonus+=values[index];
        multiplier=std::min(multiplier+1,10u);
        return values[index];
    }
};

struct LevelBest { uint32_t score=0,rank=0,time=UINT32_MAX; };

struct Progress {
    uint32_t next=0,ticks=0,resets=0,quest=0,terryKey=0;
    LevelBest best[SaveCourseCapacity]{};
    uint32_t golfKey=0,glowstickKey=0,parkKey=0,frontierKey=0,elysianKey=0,cosmicKey=0;

    bool record(unsigned level,Score score,unsigned clock,bool advance) {
        auto& entry=best[level];
        entry.score=std::max(entry.score,static_cast<uint32_t>(score.total));
        entry.rank=std::max(entry.rank,static_cast<uint32_t>(score.rank));
        entry.time=std::min(entry.time,static_cast<uint32_t>(clock));
        if (advance) next=std::max(next,static_cast<uint32_t>(level+1));
        return advance && level+1==TutorialCount;
    }

    unsigned totalScore() const {
        unsigned score=0;
        for (unsigned i=0;i<CourseCount;++i) score+=best[i].score;
        return score;
    }

    unsigned totalPrisms() const {
        unsigned prisms=0;
        for (unsigned i=0;i<CourseCount;++i) prisms+=best[i].rank;
        return prisms;
    }
};

enum class TerryEvent { Locked, Grow, Key };

struct TerryRun {
    unsigned clicks=0;
    unsigned keyTick=0;
    bool running=false;

    TerryEvent click(Progress& progress,unsigned tick) {
        if (progress.quest<2 || progress.terryKey) return TerryEvent::Locked;
        if (clicks<15) {
            ++clicks;
            return TerryEvent::Grow;
        }
        progress.terryKey=1;
        running=true;
        keyTick=tick;
        return TerryEvent::Key;
    }
};

class SaveFile {
public:
    explicit SaveFile(const std::string& path) : base(path) {}

    bool read(Progress& progress) {
        const Saved a=readSlot(0), b=readSlot(1);
        const Saved& newest=latest(a,b);
        if (!newest.sequence) return false;
        progress=newest.progress;
        return true;
    }

    bool write(const Progress& progress) {
        const Saved a=readSlot(0), b=readSlot(1);
        const Saved& newest=latest(a,b);
        const unsigned slot=newest.sequence && &newest==&a ? 1 : 0;
        Record<Progress> record{};
        record.magic=Magic;
        record.version=8;
        record.sequence=newest.sequence+1;
        if (!record.sequence) return false;
        record.progress=progress;
        record.check=checksum(record);

        std::FILE* file=std::fopen(path(slot).c_str(),"wb");
        if (!file) return false;
        const bool written=std::fwrite(&record,sizeof record,1,file)==1;
        const bool closed=std::fclose(file)==0;
        return written && closed && readSlot(slot).sequence==record.sequence;
    }

private:
    static constexpr uint32_t Magic=0x53474355;
    struct LegacyProgress {
        uint32_t next,ticks,resets,quest,terryKey;
        LevelBest best[TutorialCount];
    };
    struct V2Progress {
        uint32_t next,ticks,resets,quest,terryKey;
        LevelBest best[SaveCourseCapacity];
    };
    struct V3Progress {
        uint32_t next,ticks,resets,quest,terryKey;
        LevelBest best[SaveCourseCapacity];
        uint32_t golfKey;
    };
    struct V4Progress {
        uint32_t next,ticks,resets,quest,terryKey;
        LevelBest best[SaveCourseCapacity];
        uint32_t golfKey,glowstickKey;
    };
    struct V5Progress {
        uint32_t next,ticks,resets,quest,terryKey;
        LevelBest best[SaveCourseCapacity];
        uint32_t golfKey,glowstickKey,parkKey;
    };
    struct V6Progress {
        uint32_t next,ticks,resets,quest,terryKey;
        LevelBest best[SaveCourseCapacity];
        uint32_t golfKey,glowstickKey,parkKey,frontierKey;
    };
    struct V7Progress {
        uint32_t next,ticks,resets,quest,terryKey;
        LevelBest best[SaveCourseCapacity];
        uint32_t golfKey,glowstickKey,parkKey,frontierKey,elysianKey;
    };
    template<class P> struct Record { uint32_t magic,version,sequence; P progress; uint32_t check; };
    struct Saved { uint32_t sequence=0; Progress progress{}; };
    static_assert(sizeof(Record<LegacyProgress>)==96 && sizeof(Record<V2Progress>)==1572 && sizeof(Record<V3Progress>)==1576);
    static_assert(sizeof(Record<V4Progress>)==1580 && sizeof(Record<V5Progress>)==1584 && sizeof(Record<V6Progress>)==1588 && sizeof(Record<V7Progress>)==1592 && sizeof(Record<Progress>)==1596);
    std::string base;

    std::string path(unsigned index) const { return base+(index?".1":".0"); }

    template<class P> static uint32_t checksum(const Record<P>& record) {
        uint32_t hash=2166136261u;
        const auto* bytes=reinterpret_cast<const unsigned char*>(&record);
        for (unsigned i=0;i<sizeof(record)-sizeof(record.check);++i) hash=(hash^bytes[i])*16777619u;
        return hash;
    }

    static const Saved& latest(const Saved& a,const Saved& b) {
        return b.sequence>a.sequence ? b : a;
    }

    template<class P> static bool readRecord(std::FILE* file,Record<P>& record,unsigned version) {
        return std::fseek(file,0,SEEK_SET)==0 && std::fread(&record,sizeof record,1,file)==1 &&
               std::fgetc(file)==EOF && !std::ferror(file) && record.magic==Magic &&
               record.version==version && record.sequence && record.check==checksum(record);
    }

    Saved readSlot(unsigned index) const {
        std::FILE* file=std::fopen(path(index).c_str(),"rb");
        if (!file) return {};
        uint32_t header[2]{};
        Saved saved{};
        if (std::fread(header,sizeof header,1,file)==1 && header[0]==Magic) {
            if (header[1]==1) {
                Record<LegacyProgress> record{};
                if (readRecord(file,record,1) && record.progress.next<=TutorialCount) {
                    const auto& old=record.progress;
                    saved.sequence=record.sequence;
                    saved.progress={old.next,old.ticks,old.resets,old.quest,old.terryKey,{}};
                    std::copy_n(old.best,TutorialCount,saved.progress.best);
                }
            } else if (header[1]==2) {
                Record<V2Progress> record{};
                if (readRecord(file,record,2)) {
                    const auto& old=record.progress;
                    saved.sequence=record.sequence;
                    saved.progress={old.next,old.ticks,old.resets,old.quest,old.terryKey,{}};
                    std::copy_n(old.best,SaveCourseCapacity,saved.progress.best);
                }
            } else if (header[1]==3) {
                Record<V3Progress> record{};
                if (readRecord(file,record,3)) {
                    const auto& old=record.progress;
                    saved.sequence=record.sequence;
                    saved.progress={old.next,old.ticks,old.resets,old.quest,old.terryKey,{},old.golfKey};
                    std::copy_n(old.best,SaveCourseCapacity,saved.progress.best);
                }
            } else if (header[1]==4) {
                Record<V4Progress> record{};
                if (readRecord(file,record,4)) {
                    const auto& old=record.progress;
                    saved.sequence=record.sequence;
                    saved.progress={old.next,old.ticks,old.resets,old.quest,old.terryKey,{},old.golfKey,old.glowstickKey};
                    std::copy_n(old.best,SaveCourseCapacity,saved.progress.best);
                }
            } else if (header[1]==5) {
                Record<V5Progress> record{};
                if (readRecord(file,record,5)) {
                    const auto& old=record.progress;
                    saved.sequence=record.sequence;
                    saved.progress={old.next,old.ticks,old.resets,old.quest,old.terryKey,{},
                                    old.golfKey,old.glowstickKey,old.parkKey};
                    std::copy_n(old.best,SaveCourseCapacity,saved.progress.best);
                }
            } else if (header[1]==6) {
                Record<V6Progress> record{};
                if (readRecord(file,record,6)) {
                    const auto& old=record.progress;
                    saved.sequence=record.sequence;
                    saved.progress={old.next,old.ticks,old.resets,old.quest,old.terryKey,{},
                                    old.golfKey,old.glowstickKey,old.parkKey,old.frontierKey};
                    std::copy_n(old.best,SaveCourseCapacity,saved.progress.best);
                }
            } else if (header[1]==7) {
                Record<V7Progress> record{};
                if (readRecord(file,record,7)) {
                    const auto& old=record.progress;
                    saved.sequence=record.sequence;
                    saved.progress={old.next,old.ticks,old.resets,old.quest,old.terryKey,{},
                                    old.golfKey,old.glowstickKey,old.parkKey,old.frontierKey,old.elysianKey};
                    std::copy_n(old.best,SaveCourseCapacity,saved.progress.best);
                }
            } else if (header[1]==8) {
                Record<Progress> record{};
                if (readRecord(file,record,8)) saved={record.sequence,record.progress};
            }
        }
        std::fclose(file);
        const Progress& progress=saved.progress;
        if (!saved.sequence || progress.next>CourseCount || progress.quest>6 || progress.terryKey>1 || progress.golfKey>1 ||
            progress.glowstickKey>1 || progress.parkKey>1 || progress.frontierKey>1 || progress.elysianKey>1 || progress.cosmicKey>1) return {};
        for (const auto& level:progress.best) {
            if (level.rank>5 || level.score>100000) return {};
        }
        return saved;
    }
};
