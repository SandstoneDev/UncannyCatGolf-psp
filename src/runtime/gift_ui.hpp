#pragma once

#include "gift_atlas.hpp"
#include "selector_burst_gift.hpp"

using GiftReward=SelectorGifts::Reward;
constexpr unsigned GiftCount=static_cast<unsigned>(GiftReward::Count);
constexpr uint32_t GiftMask=(1u<<GiftCount)-1;
constexpr uint32_t giftBit(GiftReward reward) { return 1u<<static_cast<unsigned>(reward); }

inline bool validGifts(uint32_t value) {
    const uint32_t earned=value&GiftMask,opened=value>>16;
    return !(value&~(GiftMask|(GiftMask<<16))) && !(opened&~earned);
}
using GiftFile=PreferenceFile<0x31474355,validGifts>;

class GiftSequence {
public:
    void load(const GiftFile& file) { *this={}; value=file.read(); }
    bool opened(GiftReward reward) const { return (value>>16)&giftBit(reward); }
    uint32_t pending() const { return (value&GiftMask)&~(value>>16); }

    bool queue(const GiftFile& file,const Progress& progress) {
        const auto peak=[&](unsigned first,unsigned end) {
            return std::all_of(progress.best+first,progress.best+end,[](const LevelBest& level) {
                return level.rank==5;
            });
        };
        uint32_t earned=0;
        if (progress.next>=TutorialCount) earned|=giftBit(GiftReward::Villa)|giftBit(GiftReward::Chill);
        if (peak(0,TutorialCount)) earned|=giftBit(GiftReward::CatArtist)|giftBit(GiftReward::Copycat);
        if (progress.next>=GlowstickFirstCourse) earned|=giftBit(GiftReward::Phone)|giftBit(GiftReward::Meowls);
        if (peak(TutorialCount,GlowstickFirstCourse)) earned|=giftBit(GiftReward::MsCanny);
        if (progress.totalPrisms()>=100) earned|=giftBit(GiftReward::Wires);
        if (progress.best[TowerCourse].rank==5) earned|=giftBit(GiftReward::Parry);
        if (progress.next>=ChuckleFirstCourse || progress.best[GlowstickFinaleCourse].time!=UINT32_MAX)
            earned|=giftBit(GiftReward::UncannyDash);
        if (peak(GlowstickFirstCourse,ChuckleFirstCourse)) earned|=giftBit(GiftReward::Retro);
        if (progress.best[DenseCourse].rank==5) earned|=giftBit(GiftReward::Burst);
        if (progress.next>=FinalFrontierFirstCourse) earned|=giftBit(GiftReward::BartBash);
        const uint32_t next=value|earned;
        if (next==value || !file.write(next)) return false;
        value=next;
        return true;
    }

    bool update(const GiftFile& file,uint64_t now,bool dismiss) {
        if (reward==GiftCount) {
            const auto available=pending();
            if (!available) return false;
            for (reward=0;!(available&(1u<<reward));++reward) {}
            start=now;
            elapsed=0;
            flew=burst=false;
        }
        elapsed=static_cast<unsigned>((now-start)/1000);
        if (elapsed>=500 && !flew) {
            audio.play(Sound::GiftFly);
            flew=true;
        }
        if (elapsed>=2000 && !burst) {
            audio.play(Sound::GiftBlast);
            burst=true;
        }
        if (elapsed>=2500) {
            const uint32_t next=value|(1u<<(16+reward));
            if (next!=value && file.write(next)) value=next;
            if (dismiss && next==value) reward=GiftCount;
        }
        return true;
    }

    unsigned reward=GiftCount,elapsed=0;

private:
    uint32_t value=0;
    uint64_t start=0;
    bool flew=false,burst=false;
};

GiftSequence gifts;
alignas(64) uint32_t bartGiftPixels[64*64]{};
alignas(64) uint32_t burstGiftPixels[SelectorBurstGift::Width*SelectorBurstGift::Height]{};
alignas(64) uint32_t giftPixels[SelectorGifts::Width*GiftArt::IconRows]{};
alignas(64) uint32_t giftBlast[SelectorGifts::BlastPages][SelectorGifts::Width*GiftArt::ExplosionRows]{};

void drawGiftIndicators(unsigned elapsed) {
    using namespace SelectorGifts;
    const uint32_t pending=gifts.pending();
    if (!pending) return;
    setTexture(GU_PSM_8888,SelectorGifts::Width,SelectorGifts::Height,giftPixels);
    const Patch& icon=Spin[elapsed/125%std::size(Spin)];
    unsigned slot=0;
    for (unsigned i=0;i<GiftCount;++i) {
        if (!(pending&(1u<<i))) continue;
        const float x=121-14.0f*slot++;
        patch(icon,ViewX+x+0.2f*icon.x,68+0.2f*icon.y,0.2f);
    }
}

void drawGift(unsigned elapsed) {
    using namespace SelectorGifts;
    drawGiftIndicators(elapsed);
    if (gifts.reward==GiftCount) return;
    const unsigned ms=gifts.elapsed;
    const unsigned darkness=230*std::min(ms,500u)/500;
    rectangle(ViewX+136,0,272,272,GU_RGBA(0u,0u,0u,darkness));
    if (ms<2000) {
        const float t=ms<=500?0.0f:std::min(1.0f,(ms-500)/1000.0f);
        const float eased=1.0f-(1.0f-t)*(1.0f-t)*(1.0f-t);
        const float scale=0.2f+0.8f*eased;
        const float shake=ms>1500?std::sin(ms*0.095f)*2.0f:0.0f;
        const float x=121+151*eased+shake,y=68+68*eased;
        const auto& icon=Spin[ms/125%std::size(Spin)];
        setTexture(GU_PSM_8888,SelectorGifts::Width,SelectorGifts::Height,giftPixels);
        patch(icon,ViewX+x+icon.x*scale,y+icon.y*scale,scale);
        return;
    }
    if (ms<2000+std::size(Blast)*50) {
        const auto& frame=Blast[(ms-2000)/50];
        setTexture(GU_PSM_8888,SelectorGifts::Width,SelectorGifts::Height,giftBlast[frame.page]);
        patch(frame.art,ViewX+272+frame.art.x,136+frame.art.y);
    }
    const float scale=std::clamp((ms-2000)/500.0f,0.1f,1.0f);
    if (gifts.reward==static_cast<unsigned>(GiftReward::BartBash)) {
        // Reuse the standard minigame award heading and description.
        const auto& minigame=Panels[russian()][static_cast<unsigned>(GiftReward::Meowls)];
        setTexture(GU_PSM_8888,SelectorGifts::Width,SelectorGifts::Height,giftPixels);
        for (const unsigned i:{0u,3u}) {
            const auto& art=minigame[i];
            patch(art,ViewX+272+(art.x-272)*scale,136+(art.y-136)*scale,scale);
        }
        setTexture(GU_PSM_8888,64,64,bartGiftPixels);
        patch({0,0,64,64,0,0},ViewX+272-32*scale,136-44*scale,scale);
        localeText(lang("Minigame: Bort Bash","Мини-игра: Bort Bash"),272,136+36*scale,0.8f*scale,White,true);
        return;
    }
    const bool burstGift=gifts.reward==static_cast<unsigned>(GiftReward::Burst);
    if (burstGift) setTexture(GU_PSM_8888,SelectorBurstGift::Width,SelectorBurstGift::Height,burstGiftPixels);
    else setTexture(GU_PSM_8888,SelectorGifts::Width,SelectorGifts::Height,giftPixels);
    const auto& panels=burstGift?SelectorBurstGift::Panels[russian()]:Panels[russian()][gifts.reward];
    for (const auto& art:panels) {
        patch(art,ViewX+272+(art.x-272)*scale,136+(art.y-136)*scale,scale);
    }
}
