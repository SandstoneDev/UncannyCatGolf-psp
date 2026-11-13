#pragma once

#include "campaign.hpp"
#include "game.hpp"
#include "thought_bank.hpp"

struct CourseObjects {
    uint32_t count,firstCoin;
    unsigned ranks[5];
    Vec2 coins[MaxCoins];
};
static_assert(sizeof(CourseObjects)==1308);

struct ObjectHeader {
    uint32_t magic;
    float coinRadius;
    unsigned coinValue,coinStep;
    Vec2 terryPosition;
    float terryScale;
    CourseObjects courses[CourseCount];
};
static_assert(sizeof(ObjectHeader)==28+CourseCount*sizeof(CourseObjects));

constexpr unsigned CoinAtlasWidth=256, CoinAtlasHeight=512;
constexpr unsigned SharedCoin=UINT32_MAX;
constexpr unsigned CoinSprites=CoinAtlasWidth*CoinAtlasHeight/(32*32);

struct ObjectAssets {
    ObjectHeader data{};
    alignas(64) uint16_t coin[CoinAtlasWidth*CoinAtlasHeight]{};
    alignas(64) uint32_t sharedCoin[16*16]{};
    alignas(64) uint32_t terry[128*256]{};
    alignas(64) uint32_t key[512*256]{};

    bool load(const char* path) {
        std::FILE* f=std::fopen(path,"rb");
        if (!f) return false;
        const bool complete=std::fread(&data,sizeof data,1,f)==1 &&
                            std::fread(coin,sizeof coin,1,f)==1 &&
                            std::fread(terry,sizeof terry,1,f)==1 &&
                            std::fread(key,sizeof key,1,f)==1 &&
                            std::fread(sharedCoin,sizeof sharedCoin,1,f)==1 &&
                            std::fgetc(f)==EOF && !std::ferror(f);
        std::fclose(f);
        if (!complete || data.magic!=0x324f4355 || !std::isfinite(data.coinRadius) ||
            data.coinRadius<=0 || data.coinRadius>32 || data.coinValue>1000 || data.coinStep>100 ||
            !std::isfinite(data.terryPosition.x) || !std::isfinite(data.terryPosition.y) ||
            !std::isfinite(data.terryScale) || data.terryScale<=0 || data.terryScale>1) return false;
        for (unsigned index=0;index<CourseCount;++index) {
            const auto& course=data.courses[index];
            const float limitY=index==CosmicFifteenthCourse?2176.0f:1024.0f;
            if (course.count>MaxCoins ||
                (course.firstCoin!=SharedCoin && (course.firstCoin>CoinSprites || course.count>CoinSprites-course.firstCoin)) ||
                !std::is_sorted(std::begin(course.ranks),std::end(course.ranks)) ||
                !course.ranks[0] || course.ranks[4]>50000) return false;
            for (unsigned i=0;i<course.count;++i) {
                const Vec2 coin=course.coins[i];
                if (!std::isfinite(coin.x) || !std::isfinite(coin.y) ||
                    std::abs(coin.x)>1024 || std::abs(coin.y)>limitY) return false;
            }
        }
        sceKernelDcacheWritebackRange(coin,sizeof coin);
        sceKernelDcacheWritebackRange(sharedCoin,sizeof sharedCoin);
        sceKernelDcacheWritebackRange(terry,sizeof terry);
        sceKernelDcacheWritebackRange(key,sizeof key);
        return true;
    }
};

enum class Thought { None, Terry, Money, Sand, Skull, Swim, Jump, Police, Idle, Moving, Car, Win, Death, Lights, Dog, Chuckle, Portal, Horse, Metronome };

class ThoughtPlayer {
public:
    bool open(const char* path) {
        cached=0;
        displayed=nullptr;
        return bank.open(path);
    }

    void begin(unsigned tick,unsigned entropy=sceKernelGetSystemTimeLow()) {
        begin(game,tick,entropy);
    }
    void setFallback(Thought thought) { game.fallback=thought; }

    void play(Thought thought,unsigned tick,unsigned entropy=sceKernelGetSystemTimeLow()) {
        play(game,thought,tick,entropy);
    }

    const uint16_t* frame(unsigned tick) { return frame(game,tick); }

    void beginMenu(unsigned tick,unsigned entropy=sceKernelGetSystemTimeLow()) {
        begin(menu,tick,entropy);
    }

    const uint16_t* menuFrame(unsigned tick) {
        if (menu.category==Thought::None) beginMenu(tick);
        return frame(menu,tick);
    }

private:
    struct Playback {
        Thought category=Thought::None,fallback=Thought::None;
        unsigned variant=0,started=0;
    };

    static void begin(Playback& player,unsigned tick,unsigned entropy) {
        player.category=Thought::None;
        player.fallback=Thought::Idle;
        play(player,player.fallback,tick,entropy);
    }

    static void play(Playback& player,Thought thought,unsigned tick,unsigned entropy) {
        if (thought==player.category) return;
        constexpr unsigned starts[]={0,0,3,11,18,32,39,44,48,123,194,195,250,342,344,353,365,370,381};
        constexpr unsigned counts[]={1,3,8,7,14,7,5,3,75,71,1,55,92,2,9,12,5,11,4};
        const unsigned index=static_cast<unsigned>(thought);
        player.variant=starts[index]+entropy%counts[index];
        player.category=thought;
        player.started=tick;
    }

    const uint16_t* frame(Playback& player,unsigned tick) {
        if (player.category==Thought::None || tick-player.started<12) return staticFrame(tick);
        const auto& entry=bank.entry(player.variant);
        const unsigned index=(tick-player.started-12)*entry.fps/60;
        if (index>=entry.count*(entry.loops+1)) {
            player.category=Thought::None;
            if (player.fallback!=Thought::None)
                play(player,player.fallback,tick,sceKernelGetSystemTimeLow());
            return staticFrame(tick);
        }
        return readFrame(entry.first+index%entry.count);
    }

    const uint16_t* staticFrame(unsigned tick) {
        const auto& entry=bank.entry(StaticClip);
        return readFrame(entry.first+(tick/2)%entry.count);
    }

    const uint16_t* readFrame(unsigned source) {
        if (source+1==cached) return displayed;
        cached=0;
        displayed=bank.frame(source);
        if (!displayed) return nullptr;
        sceKernelDcacheWritebackRange(displayed,ThoughtBank::FrameBytes);
        cached=source+1;
        return displayed;
    }

    static constexpr unsigned StaticClip=47;
    ThoughtBank bank;
    Playback game{},menu{};
    const uint16_t* displayed=nullptr;
    unsigned cached=0;
};
