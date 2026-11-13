#pragma once

struct Stroke045 { unsigned tick; int x,y; };
constexpr Stroke045 route517_045[]={{8,41,-127},{223,20,-127},{441,30,127}};
constexpr Stroke045 route518_045[]={{62,33,-14},{118,-36,-20},{175,-5,-87},{321,20,51},
    {374,75,-44},{462,-38,-61},{631,26,43},{681,5,-89},{813,3,-53},{878,1,-49},
    {953,-5,-76},{1075,49,46},{1190,-51,-8}};

bool runFocused045(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=Elysian518Course || progress.totalScore()!=1043698 ||
        progress.totalPrisms()!=441 || !progress.frontierKey || progress.elysianKey ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_045_seed");
    const Progress seed=progress;
    round.totalTicks=progress.ticks;
    const auto capture=[&](const char* phase,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && !liveResultPrepared) return false;
        return dumpFrame(executable,(std::string("045-")+phase+".565").c_str());
    };
    const auto stroke=[&](int x,int y) {
        round.updateControls({x/127.0f,y/127.0f},true,false);
        return round.updateControls({},false,false)==ControlEvent::Shot;
    };
    const auto go=[&](unsigned index,unsigned clock) {
        if (flow.screen!=Screen::LevelSelect || menuCourse!=index ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,clock) || !selectorGoDown ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,clock+100) || selectorGoDown) return false;
        flow.update(0,true,false,false,false);
        return flow.screen==Screen::Hole && enterCourse(executable,index,course,round);
    };
    flow.screen=Screen::Hole;
    if (!enterCourse(executable,Elysian517Course,course,round)) return fail("test_045_517_load");
    unsigned shot=0;
    for (unsigned frame=0;frame<3600 && !round.caught && !round.game.won;++frame) {
        if (shot<std::size(route517_045) && round.ticks==route517_045[shot].tick) {
            if (!stroke(route517_045[shot].x,route517_045[shot].y)) return fail("test_045_517_stroke");
            ++shot;
        }
        round.step(false,false,round.horse.flying() && round.horse.age-round.horse.lastHitAt==1);
    }
    if (shot!=3 || !round.game.won || advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=Elysian518Course) return fail("test_045_517_advance");
    openLevelSelect(flow,Elysian518Course);
    if (levelSelect.world()!=5 || !capture("elysian-map-ru",1000) || !go(Elysian518Course,1100) ||
        round.horse.health!=7 || !capture("5_18-ru") || clips.count(Clip::Win518) || clips.count(Clip::Loss518))
        return fail("test_045_518_setup");
    round.game.position={-280,-425};
    round.step();
    if (!round.caught || !round.finaleMineLoss || !capture("5_18-mine-ru")) return fail("test_045_518_mine");
    if (!enterCourse(executable,Elysian518Course,course,round)) return fail("test_045_518_retry");
    round.game.position={380,0};
    round.step();
    if (!round.caught || !round.waterDead || !capture("5_18-water-ru")) return fail("test_045_518_water");
    if (!changeLanguage(executable,course,config,Language::English)) return fail("test_045_518_en");
    openLevelSelect(flow,Elysian518Course);
    if (!capture("elysian-map-en",1000) || !go(Elysian518Course,1100) || !capture("5_18-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,Elysian518Course,course,round)) return fail("test_045_518_en");
    startCourseMusic(currentCourse);
    for (unsigned i=0;i<100 && audio.musicPositionSeconds()==0;++i) sceKernelDelayThread(10000);
    if (audio.musicPositionSeconds()==0) return fail("test_045_horse_music");
    logEvent(log,"test_045_horse_music",round.game);
    shot=0;
    for (unsigned frame=0;frame<1500 && !round.caught && round.finale518.phase==FinalePhase518::Hunt;++frame) {
        if (shot<std::size(route518_045) && round.ticks==route518_045[shot].tick) {
            if (!stroke(route518_045[shot].x,route518_045[shot].y)) return fail("test_045_518_stroke");
            ++shot;
        }
        round.step();
    }
    if (round.caught || round.ticks!=1300 || shot!=13 || round.game.strokes!=13 || round.horse.health ||
        round.finale518.spent!=127 || round.finale518.phase!=FinalePhase518::Defeat)
        return fail("test_045_518_route");
    logEvent(log,"test_045_mines_7",round.game);
    for (unsigned i=1;i<=720;++i) {
        round.step();
        if (round.ticks!=1300 || round.game.won!=(i==720)) return fail("test_045_518_defeat_clock");
        if ((i==100 && !capture("5_18-defeat")) || (i==560 && !capture("5_18-ghost")))
            return fail("test_045_518_defeat_art");
    }
    if (!capture("5_18-win-ru",0,4700) || !canAdvance(4700)) return fail("test_045_518_win");
    const auto preKeyRun=round.finale518;
    const auto preKeyTicks=round.totalTicks;
    round.finale518.keyWaiting=true;
    for (unsigned i=1;i<=HorseFinale518Run::KeyAwardTick;++i) {
        round.step();
        if (i==3708 && (!capture("5_18-secret-ru",0,4700) || !secretLabelSlots518[0].owner ||
                        !secretLabelSlots518[1].owner)) return fail("test_045_secret_art");
    }
    if (!progress.elysianKey || round.finale518.keyWaiting) return fail("test_045_secret_key");
    logEvent(log,"test_045_secret_key",round.game);
    progress.elysianKey=0;
    round.finale518=preKeyRun;
    round.totalTicks=preKeyTicks;
    for (Sound sound:{Sound::HorseKeyHoly,Sound::HorseKeyProverb,Sound::HorseKeyProverbDelay,Sound::SecretKey})
        audio.stop(sound);
    acceptGoldenTee045(course,round,save,log);
    if (!canSwim || progress.next!=Elysian518Course || !progress.best[Elysian518Course].rank)
        return fail("test_045_518_early_unlock");
    for (unsigned i=0;i<30;++i) round.step();
    if (round.finale518.phase!=FinalePhase518::TeeReady || !capture("5_18-tee") || !stroke(-46,-77))
        return fail("test_045_518_tee");
    for (unsigned i=0;i<20 && round.finale518.phase==FinalePhase518::TeeReady;++i) round.step();
    if (round.finale518.phase!=FinalePhase518::Attract || !round.finale518.cameraEntered)
        return fail("test_045_518_attract");
    for (unsigned i=0;i<1100;++i) round.step();
    if (round.finale518.cameraTicks!=60 || !capture("5_18-swing")) return fail("test_045_518_swing");
    while (!round.finale518.canEnterEnding()) round.step(true);
    if (round.finale518.attractTicks!=1170 ||
        advanceCampaign(executable,course,round,save,log)!=Advance::ElysianEnding ||
        progress.next!=CosmicFirstCourse) return fail("test_045_ending_unlock");
    flow.screen=Screen::ElysianEnding;
    EndingClock045 clock;
    const auto blocks=audio.blocks();
    if (updateElysianEnding045(flow,clock,500000,false,false) || !clock.music ||
        !capture("ending-first",500) || !endingFrameReady045 ||
        !capture("ending-delta",1250) || !endingFrameReady045 ||
        !capture("ending-middle",85000) || !endingFrameReady045 ||
        !capture("ending-last",158650) || !endingFrameReady045) return fail("test_045_ending_decode");
    for (unsigned i=0;i<100 && audio.musicPositionSeconds()==0;++i) sceKernelDelayThread(10000);
    if (audio.musicPositionSeconds()==0) return fail("test_045_ending_music");
    logEvent(log,"test_045_ending_music",round.game);
    if (audio.blocks()==blocks || !updateElysianEnding045(flow,clock,158700000,true,false) ||
        levelSelect.world()!=6 || !capture("cosmic-map-ru",1000)) return fail("test_045_ending_finish");
    logEvent(log,"test_045_ending_complete",round.game);
    Flow skipped=flow;
    skipped.screen=Screen::ElysianEnding;
    EndingClock045 skipClock;
    if (!updateElysianEnding045(skipped,skipClock,0,false,true) ||
        skipped.screen!=Screen::LevelSelect || levelSelect.world()!=6) return fail("test_045_ending_skip");
    if (updateLevelSelector(PSP_CTRL_LTRIGGER,0,PSP_CTRL_LTRIGGER,1200) ||
        levelSelect.world()!=5 || !selectorTravelling(1201) || !capture("cosmic-to-elysian",1320) ||
        updateLevelSelector(PSP_CTRL_RTRIGGER,0,PSP_CTRL_RTRIGGER,3000) ||
        levelSelect.world()!=6 || !selectorTravelling(3001) || !capture("elysian-to-cosmic",3120) ||
        !go(CosmicFirstCourse,4600) || !capture("6_01-ru") || clips.count(Clip::Win601) || clips.count(Clip::Loss601))
        return fail("test_045_cosmic_map");
    const Progress unlocked=progress;
    progress={};
    for (unsigned i=0;i<75;++i) progress.best[i].rank=5;
    progress.best[74].rank=4;
    round.restart();
    round.game.position={0,224};
    round.game.velocity={0,-600};
    for (unsigned i=0;i<12;++i) round.step();
    if (round.cosmicGateEligible || round.cosmicGateBroken || !round.cosmicGatePoor ||
        !round.game.bounces || !capture("6_01-374")) return fail("test_045_gate_374");
    logEvent(log,"test_045_gate_374",round.game);
    progress.best[74].rank=5;
    round.restart();
    round.game.position={0,224};
    round.game.velocity={0,-600};
    for (unsigned i=0;i<12;++i) round.step();
    if (!round.cosmicGateEligible || !round.cosmicGateBroken || round.game.bounces ||
        !capture("6_01-375")) return fail("test_045_gate_375");
    logEvent(log,"test_045_gate_375",round.game);
    progress=unlocked;
    if (!changeLanguage(executable,course,config,Language::English)) return fail("test_045_601_en");
    openLevelSelect(flow,CosmicFirstCourse);
    if (!capture("cosmic-map-en",1000) || !go(CosmicFirstCourse,1100) || !capture("6_01-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicFirstCourse,course,round)) return fail("test_045_601_en");
    round.ghost.position=round.game.position;
    round.step();
    if (!round.caught || !capture("6_01-loss-ru",0,600) ||
        !enterCourse(executable,CosmicFirstCourse,course,round) || !capture("6_01-retry-ru"))
        return fail("test_045_601_loss");
    round.step();
    if (!stroke(-16,-126)) return fail("test_045_601_stroke");
    for (unsigned i=0;i<120 && !round.game.won && !round.caught;++i) round.step();
    if (round.caught || !round.game.won || round.ticks!=53 || round.cosmicGateBroken!=9 ||
        round.game.bounces!=2 || !capture("6_01-win-ru",0,4700)) return fail("test_045_601_route");
    logEvent(log,"test_045_601_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild) return fail("test_045_601_advance");
    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.frontierKey ||
        !saved.best[Elysian518Course].rank || !saved.best[CosmicFirstCourse].rank) return fail("test_045_save");
    for (unsigned i=0;i<Elysian518Course;++i) {
        const auto& old=seed.best[i];
        const auto& now=saved.best[i];
        if (old.score!=now.score || old.rank!=now.rank || old.time!=now.time)
            return fail("test_045_old_best");
    }
    return true;
}
