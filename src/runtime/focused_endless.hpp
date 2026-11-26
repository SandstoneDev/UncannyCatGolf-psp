#pragma once
#include "campaign_friend_routes.hpp"
#include "club_route.hpp"
#include "endless_e04_route.hpp"

bool runFocusedEndless(const char* executable,Course& course,SaveFile& save,const LanguageFile&,const char* log) {
    using namespace CampaignFriendRoute;
    Round round(course);
    Flow flow;
    bool passed=true;
    const auto check=[&](const char* name,bool pass) {
        logEvent(log,name,round.game);
        if (!pass) logEvent(log,"endless_focus_assertion_failed",round.game);
        passed=passed && pass;
        return pass;
    };
    const auto capture=[&](const char* en,const char* ru) {
        render(flow,course,round,1000,round.ticks*5/3,round.totalTicks*5/3,0,0);
        return dumpFrame(executable,russian()?ru:en);
    };
    const auto load=[&](unsigned index) { return enterCourse(executable,index,course,round); };

    if (!check("focus_new_save_empty",!save.read(progress))) return false;
    progress={};
    if (!check("focus_new_save_written",save.write(progress) && !Endless054::available() && !canSwim)) return false;

    if (!check("focus_23_opening_holes_input",playOpening(executable,course,round,save,log))) return false;

    // Isolated map checkpoint: route verification exercises gimmicks from their actual tees.
    // Earlier campaign clears are not part of this focused run.
    progress.next=CosmicThirteenthCourse+1;
    if (!check("focus_fixture_campaign_map_access",save.write(progress))) return false;
    if (!check("focus_518_defeat_input",load(Elysian518Course) && defeatHorse(round))) return false;
    acceptGoldenTee045(course,round,save,log);
    if (!check("focus_518_swim_grant",canSwim && earnedSwim045())) return false;
    if (!check("focus_pool_phase1_input",load(PoolCourse) && swimToFriend(round))) return false;
    persistProgress(save,round,log);

    const auto callFriend=[&]() {
        flow.screen=Screen::Phone;
        Phone054::enter();
        for (const char digit:std::string_view(ClubText054::Quest::FriendNumber)) {
            const unsigned key=digit=='0'?10:static_cast<unsigned>(digit-'1');
            while (Phone054::selected/3!=key/3) Phone054::step(flow,PSP_CTRL_DOWN,16);
            while (Phone054::selected%3!=key%3) Phone054::step(flow,PSP_CTRL_RIGHT,16);
            Phone054::step(flow,PSP_CTRL_CROSS,16);
        }
        while (Phone054::selected/3!=3) Phone054::step(flow,PSP_CTRL_DOWN,16);
        while (Phone054::selected%3!=2) Phone054::step(flow,PSP_CTRL_RIGHT,16);
        Phone054::step(flow,PSP_CTRL_CROSS,16);
        for (unsigned i=0;i<40;++i) Phone054::step(flow,0,100);
        return Phone054::state==Phone054::State::Talking && !Phone054::saveFailed;
    };
    if (!check("focus_phone_phase2_input",callFriend() && progress.quest==2)) return false;
    if (!check("focus_phone_phase2_frame",capture("endless-friend-call-en.565","endless-friend-call-ru.565"))) return false;
    Phone054::leave();

    if (!check("focus_terry_key_input",load(1) && talkToTerry(round,save,log))) return false;
    if (!check("focus_jumpy_unlock_input",load(JumpCourse) && finishJumpyChallenge(round))) return false;
    recordCourseWin(currentCourse,scoreFor(course,round),round.ticks*5/3,false);
    if (!check("focus_jumpy_peak",progress.best[JumpCourse].rank==5)) return false;
    if (!check("focus_bunker_jumpy_toggle",load(TutorialCount+6) && toggleBunkerJumpy(round))) return false;
    if (!check("focus_bunker_key_input",collectBunker(round))) return false;
    persistProgress(save,round,log);
    if (!check("focus_city_key_input",load(CityCourse) && collectCity(round))) return false;
    persistProgress(save,round,log);
    if (!check("focus_marcy_key_input",load(MartCourse) && collectMarcy(round))) return false;
    persistProgress(save,round,log);
    if (!check("focus_frontier_key_input",load(CataclysmCourse) && collectFrontier(round))) return false;
    persistProgress(save,round,log);
    if (!check("focus_elysian_key_input",load(Elysian518Course) && defeatHorse(round) && waitForElysianKey(round))) return false;
    persistProgress(save,round,log);
    if (!check("focus_cosmic_key_input",load(CosmicThirteenthCourse) && collectCosmic(round))) return false;
    persistProgress(save,round,log);
    const auto keys=Phone054::fragments();
    if (!check("focus_seven_campaign_keys_persisted",std::count(keys.begin(),keys.end(),true)==7)) return false;

    // Seed the shuffle PRNG so real completion of E_00 leads to E_04.
    uint32_t seed=0;
    do { Endless054::run.begin(++seed); } while (Endless054::run.next()!=EndlessRules::E04);
    flow.screen=Screen::EndlessMenu;
    Endless054::enter(executable);
    if (!check("focus_endless_menu_frame",capture("endless-menu-en.565","endless-menu-ru.565"))) return false;
    const auto input=[&](Vec2 analog={},unsigned held=0,unsigned pressed=0) {
        Endless054::step(flow,executable,course,round,save,log,analog,held,pressed,16667,seed);
    };
    input({},PSP_CTRL_CROSS,PSP_CTRL_CROSS);
    if (!check("focus_e00_loaded",flow.screen==Screen::EndlessHole && Endless054::run.active()==EndlessRules::First))
        return false;
    if (!check("focus_e00_frame",capture("endless-e00-en.565","endless-e00-ru.565"))) return false;
    input();
    input({123.f/127,31.f/127},PSP_CTRL_CROSS,PSP_CTRL_CROSS);
    for (unsigned i=0;i<260 && Endless054::run.active()==EndlessRules::First;++i) input();
    if (!check("focus_e00_real_clear",Endless054::run.active()==EndlessRules::E04 &&
               Endless054::run.cleared()==1 && Endless054::run.score>0 && Endless054::run.skips==3)) return false;
    if (!check("focus_e04_trap_frame",Endless054::trapEnabled &&
               capture("endless-e04-en.565","endless-e04-ru.565"))) return false;

    if (!check("focus_e04_key_input",playEndlessE04(input))) return false;

    Progress saved;
    if (!check("focus_e04_key_persisted",save.read(saved) && saved.endlessKey && saved.quest==2)) return false;
    Endless054::active=false;
    if (!check("focus_phone_phase4_input",callFriend() && progress.quest==4 && Phone054::fragmentsLeft==0)) return false;
    Phone054::leave();

    Villa054::beginVisit(progress.quest);
    flow.screen=Screen::Club;
    Club054::enter(Villa054::earned);
    if (!check("focus_club_entry_frame",capture("endless-club-en.565","endless-club-ru.565"))) return false;
    Club054::step(flow,{},0,0,17);
    for (const auto& shot:FriendClubRoute::Shots) {
        const Vec2 analog{shot.x/127.0f,shot.y/127.0f};
        Club054::step(flow,analog,PSP_CTRL_CROSS,PSP_CTRL_CROSS,17);
        Club054::step(flow,{},0,0,17);
        for (unsigned frame=0;frame<450 && length(Club054::physics.velocity)>0 &&
             Club054::scene==Club054::Scene::Exploring;++frame) Club054::step(flow,{},0,0,17);
        if (shot.stage==1 && !check("focus_club_breaker_door_input",Club054::physics.breakerDoor)) return false;
        if (shot.stage==2 && !check("focus_club_breaker_switch_input",Club054::physics.breakerOn)) return false;
        if (shot.stage==3 && !check("focus_club_robert_input",Club054::scene!=Club054::Scene::Exploring &&
                                    Club054::roomX==7 && Club054::roomY==4)) return false;
    }
    for (unsigned i=0;i<40 && Club054::scene!=Club054::Scene::Speak;++i) Club054::step(flow,{},0,0,100);
    if (!check("focus_robert_dialogue_frame",Club054::scene==Club054::Scene::Speak &&
               capture("endless-robert-en.565","endless-robert-ru.565"))) return false;
    for (unsigned i=0;i<1500 && Club054::scene!=Club054::Scene::Recital;++i) Club054::step(flow,{},0,0,100);
    if (!check("focus_robert_15_lines_saved",Club054::saved && save.read(saved) && saved.quest==5 &&
               saved.endlessKey && saved.terryKey && saved.golfKey && saved.glowstickKey &&
               saved.parkKey && saved.frontierKey && saved.elysianKey && saved.cosmicKey)) return false;
    Club054::leave();
    Villa054::beginVisit(progress.quest);
    if (!check("focus_nothing_unlocked",VillaUnlocks::unlocked(Villa054::earned,VillaUnlocks::Id::Nothing))) return false;
    if (!check("focus_phone_phase6_input",callFriend() && save.read(saved) && saved.quest==6)) return false;
    Phone054::leave();

    // Exercise the replay boards that stressed the mixer and the rotating field.
    audio.music(Sound::EndlessMusic);
    for (unsigned i=0;i<150 && !audio.musicPositionFrames(Sound::EndlessMusic);++i)
        sceKernelDelayThread(16000);
    if (!check("focus_endless_music_streaming",audio.musicPositionFrames(Sound::EndlessMusic)>0)) return false;
    const auto openReplay=[&](unsigned index) {
        Endless054::run.begin(1);
        Endless054::run.slot=1;
        Endless054::run.bag[0]=index;
        flow.screen=Screen::EndlessHole;
        return Endless054::openHole(executable,course,round,log,true);
    };
    if (!check("focus_replay_3_01_with_audio",openReplay(ChuckleParkFirstCourse))) return false;
    if (!check("focus_replay_3_08_with_audio",openReplay(DenseCourse))) return false;
    if (!check("focus_replay_4_02_with_audio",openReplay(FrontierSwitchCourse))) return false;
    if (!check("focus_e05_buffer_loaded",openReplay(EndlessRules::First+5))) return false;
    for (unsigned i=0;i<60;++i) input();
    if (!check("focus_e05_rotating_frame",EndlessRotating::ready &&
               capture("endless-e05-en.565","endless-e05-ru.565"))) return false;
    Endless054::active=false;
    audio.muteMusic();
    return passed;
}
