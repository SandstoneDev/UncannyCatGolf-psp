#pragma once

bool runFocused037Frontier(const char* executable,Course& course,SaveFile& save,
                           const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* name) {
        logEvent(log,name,round.game);
        return false;
    };
    const auto capture=[&](const char* suffix,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        char name[80];
        std::snprintf(name,sizeof name,"037-%s-%s-%s.565",russian()?"ru":"en",CourseIds[menuCourse],suffix);
        return dumpFrame(executable,name);
    };
    if (!save.read(progress) || progress.next!=MovieOnceCourse ||
        progress.best[DownfallCourse].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_037_seed_error");
    const Progress seed=progress;

    constexpr int movie[][2]={{-66,-102},{-72,-96}};
    constexpr int workspace[][2]={{-84,-72},{96,-96},{-108,60},{-90,96},{-126,-36}};
    for (unsigned index=MovieOnceCourse;index<=AbandonedWorkspaceCourse;++index) {
        const bool windy=index==MovieOnceCourse;
        const unsigned cell=index-FinalFrontierFirstCourse;
        openLevelSelect(flow);
        if (menuCourse!=index || levelSelect.world()!=4 || levelSelect.cell()!=cell ||
            !levelSelect.unlocked(4,cell) || levelSelect.unlocked(4,cell+1) ||
            !capture("map",3000) ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
            !capture("go",3030) || !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090))
            return fail("test_037_selector_error");
        flow.update(0,true,false,false,false);
        if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
            currentCourse!=index || round.extraGhostCount!=2 || !capture("start"))
            return fail("test_037_enter_error");
        if (!changeLanguage(executable,course,config,Language::English) || !capture("start") ||
            !changeLanguage(executable,course,config,Language::Russian))
            return fail("test_037_locale_error");
        round.restart();

        if (windy) {
            if (round.keyStage!=&FrontierKeys414 || round.keyStage->doorCount!=22 ||
                round.keyStage->keyCount!=22 || !round.frontierWind.movieOnce)
                return fail("test_037_414_layout_error");
            while (round.ticks<150 && !round.caught) round.step();
            const float before=round.game.position.y;
            round.step();
            if (round.caught || !round.frontierWind.drifting() || round.game.position.y>=before || !capture("wind"))
                return fail("test_037_414_wind_error");
            logEvent(log,"test_037_414_wind",round.game);
        } else {
            if (round.keyStage!=&FrontierKeys415 || round.stopStage!=&FrontierStop415 ||
                round.skullCount!=5 || !round.baddies.alive(0) || round.frontierWind.active)
                return fail("test_037_415_layout_error");
            round.ticks=120;
            if (frontierSpotlight415(0).y!=192 || frontierSpotlight415(round.ticks).y!=-192 || !capture("spotlight"))
                return fail("test_037_415_spotlight_error");
            logEvent(log,"test_037_415_spotlight",round.game);
        }
        round.restart();
        while (round.ticks<2400 && !round.caught && !round.game.won) round.step();
        if (!round.caught || round.game.won || !capture("loss",0,1000))
            return fail("test_037_idle_loss_error");
        logEvent(log,windy?"test_037_414_loss":"test_037_415_loss",round.game);
        round.restart();

        const auto route=windy?movie:workspace;
        const unsigned count=windy?std::size(movie):std::size(workspace);
        bool doorCaptured=false;
        for (unsigned i=0;i<count;++i) {
            round.updateControls({route[i][0]/127.0f,route[i][1]/127.0f},true,false);
            if (round.updateControls({},false,false)!=ControlEvent::Shot)
                return fail("test_037_shot_error");
            for (unsigned tick=0;tick<2400 && !round.caught && !round.game.won &&
                 (windy?!round.frontierWind.drifting():round.game.moving());++tick) {
                round.step();
                if (!doorCaptured && round.roundKeys.opened) {
                    if (!capture("door")) return fail("test_037_door_capture_error");
                    doorCaptured=true;
                }
            }
            logEvent(log,windy?"test_037_414_shot":"test_037_415_shot",round.game);
            if (round.caught || (!round.game.won && (windy?!round.frontierWind.drifting():round.game.moving())))
                return fail("test_037_route_error");
        }
        if (!round.game.won || !doorCaptured || round.game.strokes!=count || !capture("win",0,4700))
            return fail("test_037_goal_error");
        if (windy ? (!(round.roundKeys.collected&0x100000u) || !(round.roundKeys.opened&0x40000u)) :
            (round.roundKeys.collected!=15 || round.roundKeys.opened!=15 || round.baddies.alive(0) ||
             !round.stopRun.collected || round.skulls.bonus!=5*SkullRun::Bonus))
            return fail("test_037_route_mechanics_error");
        logEvent(log,windy?"test_037_414_goal":"test_037_415_goal",round.game);
        Progress saved{};
        if (advanceCampaign(executable,course,round,save,log)!=(windy?Advance::NextHole:Advance::EndOfBuild) ||
            !save.read(saved) || saved.next!=index+1 || saved.best[index].time==UINT32_MAX)
            return fail("test_037_save_error");
        for (unsigned previous=0;previous<MovieOnceCourse;++previous) {
            const auto& before=seed.best[previous];
            const auto& after=saved.best[previous];
            if (before.time!=after.time || before.score!=after.score || before.rank!=after.rank)
                return fail("test_037_history_error");
        }
    }
    return true;
}
