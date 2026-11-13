#pragma once

bool runFocused038(const char* executable,Course& course,SaveFile& save,
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
    if (!save.read(progress) || progress.next!=TheWallCourse || !progress.frontierKey ||
        progress.best[CataclysmCourse].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_038_seed_error");
    const Progress seed=progress;
    if (worldCompletedAt(ElysianFieldsCourse)!=CompletedWorld::FinalFrontier)
        return fail("test_038_world_index_error");

    flow.screen=Screen::Hole;
    const auto setPrisms=[](unsigned count) {
        for (unsigned i=0;i<CourseCount;++i) {
            progress.best[i].rank=std::min(count,5u);
            count-=progress.best[i].rank;
        }
    };
    setPrisms(249);
    if (!enterCourse(executable,TheWallCourse,course,round) || round.wall418.eligible || !shoot(0,-127))
        return fail("test_038_249_enter_error");
    for (unsigned i=0;i<160 && !round.wall418.poor;++i) round.step();
    if (!round.wall418.poor || round.wall418.shattered || round.game.won || !capture("038-4_18-gate-249.565"))
        return fail("test_038_249_gate_error");
    if (advanceCampaign(executable,course,round,save,log,true)!=Advance::Failed)
        return fail("test_038_skip_wall_error");
    logEvent(log,"test_038_249_gate",round.game);

    setPrisms(250);
    if (!enterCourse(executable,TheWallCourse,course,round) || !round.wall418.eligible || !shoot(0,-127))
        return fail("test_038_250_enter_error");
    for (unsigned i=0;i<160 && !round.wall418.shattered;++i) round.step();
    if (!round.wall418.shattered || !capture("038-4_18-gate-250.565"))
        return fail("test_038_250_gate_error");
    logEvent(log,"test_038_250_gate",round.game);
    if (!enterCourse(executable,TheWallCourse,course,round) || !shoot(8,-125))
        return fail("test_038_418_angled_shot_error");
    for (unsigned i=0;i<300 && !round.game.won && !round.caught;++i) round.step();
    if (!round.game.won || !round.wall418.shattered || round.game.position.x<40 ||
        round.game.position.y<=-497)
        return fail("test_038_418_angled_goal_error");
    logEvent(log,"test_038_418_angled_goal",round.game);

    chillMode=true;
    progress=seed;
    if (!enterCourse(executable,TheWallCourse,course,round) || !round.wall418.easyMode ||
        !dogGate.active() || !capture("038-4_18-easy-bed.565") || !shoot(0,-127))
        return fail("test_038_easy_enter_error");
    for (unsigned i=0;i<160 && !round.wall418.bedActive;++i) round.step();
    if (!round.wall418.bedActive || round.game.won || round.wall418.shattered || progress.next!=TheWallCourse)
        return fail("test_038_bed_error");
    if (!capture("038-4_18-easy-start.565")) return fail("test_038_easy_frame_error");
    for (unsigned i=0;i<WallRun418::EndingTicks/4;++i) round.step(true);
    if (!round.wall418.endingFinished() || round.game.won || progress.next!=TheWallCourse ||
        !capture("038-4_18-easy-end.565"))
        return fail("test_038_easy_end_error");
    logEvent(log,"test_038_easy_ending",round.game);

    progress=seed;
    chillMode=false;
    openLevelSelect(flow);
    if (menuCourse!=TheWallCourse || levelSelect.world()!=4 || levelSelect.cell()!=17 ||
        levelSelect.canChangeWorld(1) || !capture("038-4_18-map.565",3000) ||
        updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
        !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090))
        return fail("test_038_418_map_error");
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
        !capture("038-4_18-start.565") || !shoot(0,-127))
        return fail("test_038_418_enter_error");
    for (unsigned i=0;i<300 && !round.game.won && !round.caught;++i) round.step();
    if (!round.game.won || !round.wall418.shattered || round.game.strokes!=1 ||
        !capture("038-4_18-win.565",0,4700))
        return fail("test_038_418_goal_error");
    logEvent(log,"test_038_418_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::WorldCleared ||
        progress.next!=ElysianFieldsCourse || !worldResults.firstUnlock() || !worldResults.nextAvailable())
        return fail("test_038_world_clear_error");
    flow.screen=Screen::WorldResults;
    if (!capture("038-world5-unlock.565",3950) || !capture("038-world5-results.565",10200))
        return fail("test_038_world_frames_error");
    auto choice=worldResults.update(10200,false,true,false,true,false).choice;
    if (choice!=WorldResultsChoice::LevelSelect) return fail("test_038_world_choice_error");
    leaveWorldResults(flow,false);
    if (flow.screen!=Screen::LevelSelect || menuCourse!=TheWallCourse || !levelSelect.canChangeWorld(1))
        return fail("test_038_map_return_error");

    if (!moveLevelSelection(PSP_CTRL_RTRIGGER,3200) || !capture("038-map-4-to-5.565",3480) ||
        updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3500) || levelSelect.world()!=5 ||
        !capture("038-5_01-map.565",4100) || !moveLevelSelection(PSP_CTRL_LTRIGGER,4200) ||
        !capture("038-map-5-to-4.565",4500) || levelSelect.world()!=4)
        return fail("test_038_map_scroll_error");
    logEvent(log,"test_038_map_scroll_4_5",round.game);

    worldResults.begin(CompletedWorld::FinalFrontier,playableCourseCount,false);
    flow.screen=Screen::WorldResults;
    if (!capture("038-world5-replay.565",10200)) return fail("test_038_world_replay_error");
    choice=worldResults.update(10200,false,false,true,true,false).choice;
    if (choice!=WorldResultsChoice::NextWorld) return fail("test_038_world_next_error");
    leaveWorldResults(flow,true);
    if (flow.screen!=Screen::Hole || menuCourse!=ElysianFieldsCourse ||
        !enterCourse(executable,menuCourse,course,round) || !capture("038-5_01-start.565"))
        return fail("test_038_501_enter_error");
    for (unsigned i=0;i<60;++i) round.step();
    if (!capture("038-5_01-clouds.565")) return fail("test_038_501_clouds_error");
    round.restart();
    if (advanceCampaign(executable,course,round,save,log,true)!=Advance::Failed || !shoot(127,-23))
        return fail("test_038_501_skip_error");
    for (unsigned i=0;i<300 && !round.game.won && !round.caught;++i) round.step();
    if (!round.game.won || round.game.strokes!=1 || round.game.bounces!=3 ||
        !capture("038-5_01-win.565",0,2200)) return fail("test_038_501_goal_error");
    logEvent(log,"test_038_501_goal",round.game);
    Progress saved{};
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild || !save.read(saved) ||
        saved.next!=CourseCount || !saved.frontierKey || saved.best[TheWallCourse].time==UINT32_MAX ||
        saved.best[ElysianFieldsCourse].rank!=5 || saved.best[ElysianFieldsCourse].score!=seed.best[ElysianFieldsCourse].score ||
        saved.best[ElysianFieldsCourse].time!=seed.best[ElysianFieldsCourse].time)
        return fail("test_038_save_error");
    return true;
}
