#pragma once

bool runFocused039(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    const auto capture=[&](const char* name,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        return dumpFrame(executable,name);
    };
    const auto shoot=[&](int x,int y) {
        round.updateControls({x/127.0f,y/127.0f},true,false);
        return round.updateControls({},false,false)==ControlEvent::Shot;
    };
    const auto runShot=[&](int x,int y) {
        if (!shoot(x,y)) return false;
        for (unsigned tick=0;tick<600 && !round.game.won && !round.caught && round.game.moving();++tick)
            round.step();
        return round.game.won && !round.caught;
    };
    const auto go=[&](unsigned index,const char* map,const char* pressed,const char* field) {
        openLevelSelect(flow,index);
        if (flow.screen!=Screen::LevelSelect || menuCourse!=index || levelSelect.world()!=5 ||
            !capture(map,1000) || updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,1100) ||
            !selectorGoDown || !capture(pressed,1100) ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,1200) || selectorGoDown) return false;
        flow.update(0,true,false,false,false);
        return flow.screen==Screen::Hole && enterCourse(executable,index,course,round) &&
            capture(field);
    };

    if (!save.read(progress) || progress.next!=ElysianRocksCourse || !progress.frontierKey ||
        progress.totalScore()!=864318 || progress.totalPrisms()!=362 ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_039_seed_error");
    const Progress seed=progress;
    round.totalTicks=progress.ticks;

    flow.screen=Screen::Hole;
    round.backrooms=round.caught=true;
    if (!capture("039-backrooms.565")) return fail("test_039_backrooms_frame_error");
    round.restart();

    if (!go(ElysianFieldsCourse,"039-map-5_01.565","039-go-5_01.565","039-start-5_01.565") ||
        !runShot(127,-23) || round.game.bounces!=3)
        return fail("test_039_501_goal_error");
    logEvent(log,"test_039_go_501",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=ElysianRocksCourse || progress.next!=ElysianRocksCourse ||
        !capture("039-transition-5_02.565"))
        return fail("test_039_501_502_error");

    if (!go(ElysianRocksCourse,"039-map-5_02.565","039-go-5_02.565","039-start-5_02.565") ||
        advanceCampaign(executable,course,round,save,log,true)!=Advance::Failed ||
        !runShot(-125,-6) || round.game.bounces!=2 ||
        !capture("039-win-5_02.565",0,2200))
        return fail("test_039_502_goal_error");
    logEvent(log,"test_039_502_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=HorseBroadcastCourse || progress.next!=HorseBroadcastCourse)
        return fail("test_039_502_503_error");

    if (!go(HorseBroadcastCourse,"039-map-5_03.565","039-go-5_03.565","039-start-5_03.565") ||
        !runShot(0,127) || !round.horseBroadcast.active() || round.game.bounces ||
        !capture("039-fake-5_03.565"))
        return fail("test_039_503_trigger_error");
    logEvent(log,"test_039_503_trigger",round.game);
    for (unsigned tick=1;tick<=HorseBroadcast039::AdvanceTick;++tick) {
        round.step();
        if (round.horseBroadcast.ticks!=tick || canAdvance(round.horseBroadcast.elapsedMs()) ||
            round.horseBroadcast.ready()!=(tick==HorseBroadcast039::AdvanceTick) ||
            progress.next!=HorseBroadcastCourse)
            return fail("test_039_503_timing_error");
        if (tick==120 && !capture("039-alarm-5_03.565"))
            return fail("test_039_503_alarm_error");
        if (tick==390 && !capture("039-warning-5_03.565"))
            return fail("test_039_503_warning_error");
    }
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=FirstHorseCourse || progress.next!=FirstHorseCourse ||
        progress.totalScore()!=seed.totalScore() || progress.totalPrisms()!=seed.totalPrisms()+10)
        return fail("test_039_503_auto_advance_error");
    logEvent(log,"test_039_503_auto_advance",round.game);

    constexpr struct Stroke { unsigned tick; int x,y; } first[]={{51,17,-126},{300,103,74}},
        second[]={{12,15,-123},{220,94,118},{443,4,45}};
    const auto horseRoute=[&](const auto& route,unsigned health,const char* active,const char* lift) {
        unsigned shot=0;
        bool capturedTarget=false;
        for (unsigned frame=0;frame<2200 && !round.game.won && !round.caught;++frame) {
            if (shot<std::size(route) && round.ticks==route[shot].tick) {
                if (!shoot(route[shot].x,route[shot].y)) return false;
                ++shot;
            }
            if (!capturedTarget && round.horse.health<health && round.horse.health>0) {
                if (!capture(active)) return false;
                capturedTarget=true;
            }
            const unsigned flightTicks=round.horse.age-round.horse.lastHitAt;
            if (round.horse.flying() && flightTicks==12 && !capture(lift)) return false;
            round.step(false,false,round.horse.flying() && flightTicks==1);
        }
        return round.game.won && !round.caught && !round.horse.health &&
            shot==std::size(route) && round.game.strokes==shot;
    };

    if (!go(FirstHorseCourse,"039-map-5_04.565","039-go-5_04.565","039-start-5_04.565"))
        return fail("test_039_504_go_error");
    for (unsigned tick=0;tick<1100 && !round.caught && !round.game.won;++tick) round.step();
    if (!round.caught || round.game.won || round.horse.phase!=HorsePhase039::Lost ||
        !capture("039-loss-5_04.565",0,2000))
        return fail("test_039_504_loss_error");
    logEvent(log,"test_039_504_loss",round.game);
    round.restart();
    if (!horseRoute(first,3,"039-targets-5_04.565","039-lift-5_04.565") ||
        !capture("039-win-5_04.565",0,4700))
        return fail("test_039_504_goal_error");
    logEvent(log,"test_039_504_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=SecondHorseCourse || progress.next!=SecondHorseCourse)
        return fail("test_039_504_505_error");

    if (!go(SecondHorseCourse,"039-map-5_05.565","039-go-5_05.565","039-start-5_05.565"))
        return fail("test_039_505_go_error");
    for (unsigned tick=0;tick<1100 && !round.caught && !round.game.won;++tick) round.step();
    if (!round.caught || round.game.won || round.horse.phase!=HorsePhase039::Lost ||
        !capture("039-loss-5_05.565",0,2000))
        return fail("test_039_505_loss_error");
    logEvent(log,"test_039_505_loss",round.game);
    round.restart();
    if (!horseRoute(second,4,"039-targets-5_05.565","039-lift-5_05.565") ||
        !capture("039-win-5_05.565",0,4700))
        return fail("test_039_505_goal_error");
    logEvent(log,"test_039_505_goal",round.game);

    Progress saved{};
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild || !save.read(saved) ||
        saved.next!=CourseCount || !saved.frontierKey)
        return fail("test_039_save_error");
    for (unsigned i=0;i<CourseCount;++i) {
        const auto& previous=seed.best[i];
        const auto& current=saved.best[i];
        if (i<ElysianRocksCourse &&
            (current.score!=previous.score || current.rank!=previous.rank || current.time!=previous.time))
            return fail("test_039_old_best_error");
        if (i>=ElysianRocksCourse && i<=HorseBroadcastCourse &&
            (current.score!=previous.score || current.rank!=5 || current.time!=previous.time))
            return fail("test_039_hidden_best_error");
    }
    return true;
}
