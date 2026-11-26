#pragma once
// Included inside Club054, with the PSP RTC headers already visible.
using Actor=ClubTracks::Actor;
ClubEvents::Visit visit;
struct WorldCallbacks {
    void* context=nullptr;
    ClubEvents::ClubPersistent (*read)(void*)=nullptr;
    bool (*write)(void*,const ClubEvents::ClubPersistent&)=nullptr;
    unsigned (*quest)(void*)=nullptr;
    bool (*rememberQuest)(void*,unsigned)=nullptr;
    // Optional mixer adapter for the current Greenhead voice. Without an
    // adapter the exact authored frame count determines its visit lifetime.
    bool (*finishedBoom)(void*)=nullptr;
};
WorldCallbacks world{};
inline void setWorldCallbacks(WorldCallbacks value) {world=value;}
ClubEvents::Clock readClock() {
    ClubEvents::Clock now;
    ScePspDateTime utc{},local{},epoch{};
    epoch.year=1970;epoch.month=1;epoch.day=1;
    u64 timeTick=0,epochTick=0;
    if(sceRtcGetCurrentClock(&utc,0)<0 || sceRtcGetCurrentClockLocalTime(&local)<0 ||
       sceRtcGetTick(&utc,&timeTick)<0 || sceRtcGetTick(&epoch,&epochTick)<0 || timeTick<epochTick)return now;
    now.unixSeconds=static_cast<int64_t>((timeTick-epochTick)/1000000);
    now.utcHour=utc.hour;now.utcWeekday=sceRtcGetDayOfWeek(utc.year,utc.month,utc.day);
    int dst=0;sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_DAYLIGHTSAVINGS,&dst);
    now.local={local.year,local.month,local.day,sceRtcGetDayOfWeek(local.year,local.month,local.day),
               local.hour,local.minute,local.second,dst!=0};
    return now;
}
uint32_t dice=1;
unsigned roll(unsigned count) {
    dice^=dice<<13;dice^=dice>>17;dice^=dice<<5;return dice%count;
}
unsigned questPhase() {return world.quest?world.quest(world.context):progress.quest;}
void enterVisit() {
    dice=sceKernelGetSystemTimeLow()|1u;
    const auto snapshot=world.read?world.read(world.context):ClubEvents::ClubPersistent{};
    const auto now=readClock();
    const auto computer=roll(2);
    visit.enter(snapshot,now,1+roll(2),computer);
    physics.visit=&visit;
}
void playReaction(Actor id,ClubEvents::Cue cue) {
    using Cue=ClubEvents::Cue;
    switch(cue) {
    case Cue::Talk:
        audio.playTuned(static_cast<Sound>(unsigned(Sound::ClubTalk1)+roll(5)),id==Actor::Mbx?-80.f:0.f,
                        id==Actor::Bobmunculua?0.17f:1.1f);break;
    case Cue::Hurt:
        audio.playTuned(static_cast<Sound>(unsigned(Sound::ClubHurt1)+roll(7)),id==Actor::Mbx?-80.f:0.f,1.1f);break;
    case Cue::AppleDeath:audio.playTuned(Sound::ClubAppleJohnDeath,10,1);break;
    case Cue::Bibizu:audio.playTuned(Sound::ClubBibizuSplat,3,1.1f);break;
    case Cue::Bob:audio.playTuned(Sound::ClubBobScare,8,0.4f);break;
    case Cue::None:break;
    }
}
