#pragma once
#include "attract_demo.hpp"

// Included after the existing Round, loader and profile definitions. This
// dispatcher never calls ordinary Hole input, rewards, progress or SaveFile.
namespace AttractRuntime {
struct Snapshot {
    Progress progress{};
    CampaignMods::Options options{};
    Course course{};
    unsigned profile=0,current=0,menu=0;
    bool chill=false,swim=false,menuReader=false,held=false;
};
inline Snapshot owner;
inline Attract::Playback playback;
inline Attract::Timer clipTimer;
inline unsigned selected=Attract::Count,eligibleWorld=1,terminalTick=0,levelTicks=0;
inline bool active() {return owner.held;}
inline unsigned completedWorld(unsigned next) {
    // Godot WORLDS_PROGRESS records the world that was COMPLETED. Unlocking a
    // world or earning a rank on a retry cannot expose its demos early.
    return next>=FinalFrontierFirstCourse?3:next>=ChuckleFirstCourse?2:1;
}
inline unsigned globalWorld() {
    const int shared=sharedAttractWorld();
    if (shared>=0) return unsigned(shared);
    unsigned world=completedWorld(progress.next);
    profiles.refresh();
    for (unsigned i=0;i<profiles.size();++i)
        world=std::max(world,completedWorld(profiles.snapshot(profiles.slot(i)).next));
    return world;
}
inline void stopLevelAudio() {
    audio.stageCue(StageCue::None);
    audio.stopRingTimers();
    for (unsigned id=0;id<static_cast<unsigned>(Sound::Count);++id)
        audio.stop(static_cast<Sound>(id));
}
inline void invalidateArt() {
    // Loading a title overwrites borrowed course VRAM. Force the established
    // same-course loader path, and drop address-keyed caches before it runs.
    titleArt=nullptr;
    titleReusedCourseVram=true;
    switchTiles.unload();parkBlockArt.unload();denseBlockArt.unload();
    if (chuckleArt.borrowed()) chuckleArt.unload();
    glowstickArt.discardPreparedCourse();
    liveResultLarge=liveResultClear=liveResultConfetti={};
    releaseCourseTransitionStreams();
}
inline bool load(const char* executable,Course& course,Round& round,unsigned index) {
    stopLevelAudio();
    round.attractDemo=true;
    progress={};campaignOptions={};chillMode=false;canSwim=false;
    // Even a repeated course with another recording starts a fresh authored level.
    titleReusedCourseVram=true;
    if (!enterCourse(executable,Attract::course(Attract::recordings[index]),course,round)) return false;
    round.controls={};round.totalTicks=0;round.demoScale=.003f/.064f;
    selected=index;clipTimer={};terminalTick=levelTicks=0;playback.begin(index);
    thoughts.begin(0);
    audio.play(Sound::Join);
    return true;
}
inline bool begin(const char* executable,Course& course,Round& round) {
    if (active()) return true;
    eligibleWorld=globalWorld();
    owner={progress,campaignOptions,course,activeProfile,currentCourse,menuCourse,chillMode,canSwim,menuStreaming.available(),true};
    selected=Attract::Count;
    menuStreaming.stop();
    clips.clearTitleIntro();menuStreaming.clearArtResult();audio.setTitleAudio(false);
    // Release the old title decoder before loading gameplay; do not retain an
    // audio worker reading title/menu-owned memory while course art is borrowed.
    if (!audio.releaseMusicForContent()) return false;
    const unsigned index=Attract::choose(eligibleWorld,selected,[](){return static_cast<unsigned>(std::rand());});
    if (!load(executable,course,round,index)) return false;
    audio.music(static_cast<Sound>(268));
    return true;
}
inline void leave(Course& course,Round& round) {
    if (!active()) return;
    stopLevelAudio();round.stopCampaignAudio();
    // A failed/partial load has exactly the same ownership exit as cancellation.
    progress=owner.progress;campaignOptions=owner.options;course=owner.course;
    activeProfile=owner.profile;currentCourse=owner.current;menuCourse=owner.menu;
    chillMode=owner.chill;canSwim=owner.swim;
    round.attractDemo=false;round.demoScale=1;round.controls={};
    owner.held=false;selected=Attract::Count;clipTimer={};
    invalidateArt();
    if (owner.menuReader) menuStreaming.start(prepareMainMenuArt);
}
inline bool step(const char* executable,Course& course,Round& round) {
    if (!active() || selected>=Attract::Count) return false;
    if (clipTimer.tick(Attract::recordings[selected].duration)) {
        const unsigned index=Attract::choose(eligibleWorld,selected,[](){return static_cast<unsigned>(std::rand());});
        // Do not restart the dedicated music at recording boundaries.
        return load(executable,course,round,index);
    }
    const bool won=round.game.won,caught=round.caught;
    const auto oldBounces=round.game.bounces;
    const auto oldBlocks=round.brokenBlocks;
    const auto oldSkulls=round.skulls.bonus,oldBaddies=round.baddies.defeatedCount;
    if (won) playback.winTick();
    const auto event=playback.tick(round.game,round.jump.airborne(),won || caught);
    round.demoScale=playback.renderScale();
    if (!playback.spawning() && !won && !caught) ++levelTicks;
    if (event.pickup) audio.play(Sound::Grab);
    if (event.shot) {
        round.coins.multiplier=0;
        audio.playTuned(Sound::Shot,0,.9f+.2f*(length(round.game.velocity)-100.f)/775.f);
        thoughts.play(Thought::Moving,round.tvTick());
    }
    // No pad state, skip, gift-parry, abilities or retry can enter the round.
    round.step(false,false,false);
    if (round.game.bounces>oldBounces && round.brokenBlocks==oldBlocks &&
        round.skulls.bonus==oldSkulls && round.baddies.defeatedCount==oldBaddies) audio.play(Sound::Bounce);
    if (!won && round.game.won) {
        terminalTick=round.totalTicks;
        audio.play(Sound::Win);thoughts.play(Thought::Win,round.tvTick());thoughts.setFallback(Thought::Win);
    }
    if (!caught && round.caught) {
        terminalTick=round.totalTicks;
        thoughts.play(Thought::Death,round.tvTick());
        thoughts.setFallback(Thought::Death);
        if (!round.backrooms && !round.waterDead) audio.play(round.fellOut?Sound::OutOfBounds:Sound::Loss);
    }
    return true;
}
inline unsigned resultMs(const Round& round) {
    return terminalTick?(round.totalTicks-terminalTick)*1000/60:0;
}
} // namespace AttractRuntime
