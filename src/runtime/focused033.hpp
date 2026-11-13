#include "coaster_route033.hpp"

bool runFocused033(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,const GiftFile& giftFile,const char* log) {
    Flow flow;
    chillMode=false;
    Round round(course);
    const auto fail=[&](const char* stage) {
        logEvent(log,stage,round.game);
        return false;
    };
    const auto shot=[&](int x,int y) {
        round.controls.update(round.game,course,{x/127.0f,y/127.0f},true,false);
        return round.controls.update(round.game,course,{},false,false)==ControlEvent::Shot;
    };
    const auto capture=[&](const char* name,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        return dumpFrame(executable,name);
    };
    if (!save.read(progress) || progress.next!=ChuckleCoasterCourse ||
        progress.best[RacetrackCourse].time==UINT32_MAX ||
        progress.best[DoubleDogCourse].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_033_seed_error");
    logEvent(log,"test_033_loaded_032",round.game);

    openLevelSelect(flow);
    if (menuCourse!=ChuckleCoasterCourse || levelSelect.world()!=3 || levelSelect.cell()!=17 ||
        levelSelect.canChangeWorld(1) || !capture("033-ru-map-park.565",3000) ||
        updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
        !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090))
        return fail("test_033_selector_park_error");
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
        !capture("033-ru-3_18-start.565"))
        return fail("test_033_enter_ride_error");

    if (!shot(0,-65)) return fail("test_033_board_error");
    bool warned=false;
    while (!round.caught && round.ticks<1300) {
        round.step();
        for (const auto warning:round.ride.ghostWarnings) warned|=warning>0;
    }
    if (!round.caught || round.ticks!=660 || !warned || round.ride.death!=CoasterDeath033::Ghost ||
        !capture("033-ru-3_18-loss.565",0,1000))
        return fail("test_033_ride_loss_error");
    logEvent(log,"test_033_ride_loss",round.game);

    round.restart();
    if (round.ride.phase!=CoasterPhase033::Boarding || round.ride.bonus || round.caught)
        return fail("test_033_retry_error");
    if (!changeLanguage(executable,course,config,Language::English) ||
        !capture("033-en-3_18-start.565") ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_033_ride_locale_error");
    round.restart();
    bool airborne=false, boardingCapture=false, ridingCapture=false, stationCapture=false, fastStep=false;
    unsigned index=0;
    while (!round.caught && !round.game.won && round.ticks<5200) {
        if (index<std::size(CoasterRoute033) && round.ticks==CoasterRoute033[index].tick) {
            const auto input=CoasterRoute033[index++];
            if (!shot(input.x,input.y)) return fail("test_033_ride_shot_error");
        }
        const unsigned before=round.ticks;
        const auto phase=round.ride.phase;
        round.step(phase==CoasterPhase033::Leaving || phase==CoasterPhase033::Slowing);
        fastStep|=round.ticks==before+4;
        airborne|=round.ride.jump.airborne();
        if (!boardingCapture && round.ride.phase==CoasterPhase033::Starting) {
            boardingCapture=capture("033-ru-3_18-boarding.565");
            if (!boardingCapture) return fail("test_033_ride_capture_error");
            round.togglePause();
            const unsigned pausedAt=round.ticks;
            const float pathAt=round.ride.pathProgress;
            for (unsigned frame=0;frame<30;++frame) round.step();
            if (round.ticks!=pausedAt || round.ride.pathProgress!=pathAt)
                return fail("test_033_ride_pause_error");
            round.togglePause();
        }
        const bool visibleObject=std::any_of(round.ride.objects.begin(),round.ride.objects.end(),
            [](const CoasterObject033& object) {
                return object.kind!=CoasterObjectKind033::None && !object.collected &&
                       object.x>20 && object.x<280;
            });
        if (!ridingCapture && round.ride.collectedCoins>=8 && visibleObject) {
            ridingCapture=capture("033-ru-3_18-ride.565");
            if (!ridingCapture) return fail("test_033_ride_capture_error");
        }
        if (!stationCapture && round.ride.phase==CoasterPhase033::Station) {
            stationCapture=capture("033-ru-3_18-station.565");
            if (!stationCapture) return fail("test_033_ride_capture_error");
        }
    }
    if (!round.game.won || round.ticks!=4766 || round.caught || !airborne || !fastStep ||
        round.ride.collectedCoins!=25 ||
        round.ride.bonus!=1420 || !boardingCapture || !ridingCapture || !stationCapture ||
        !capture("033-ru-3_18-win.565",0,4700))
        return fail("test_033_ride_goal_error");
    logEvent(log,"test_033_ride_goal",round.game);

    const Progress seed=progress;
    if (advanceCampaign(executable,course,round,save,log)!=Advance::WorldCleared ||
        progress.next!=FinalFrontierFirstCourse ||
        worldResults.world()!=CompletedWorld::ChucklePark || !worldResults.firstUnlock())
        return fail("test_033_world_unlock_error");
    if (!gifts.queue(giftFile,progress) || !(gifts.pending()&giftBit(GiftReward::BartBash)) ||
        gifts.queue(giftFile,progress)) return fail("test_033_bart_gift_error");
    GiftSequence storedGifts;
    storedGifts.load(giftFile);
    if (!(storedGifts.pending()&giftBit(GiftReward::BartBash))) return fail("test_033_bart_gift_save_error");
    flow.screen=Screen::WorldResults;
    const unsigned choice=worldResults.choiceTime();
    if (!capture("033-ru-w3-results-transition.565",4000) ||
        !capture("033-ru-w3-results-first.565",choice+1450))
        return fail("test_033_world_first_error");
    const auto beforeChoice=worldResults.update(choice-1,false,false,false,true,false);
    if (beforeChoice.choice!=WorldResultsChoice::None || !(beforeChoice.cues&cueMask(WorldCue::Unlock)))
        return fail("test_033_world_locked_choice_error");
    const auto first=worldResults.update(choice+250,false,false,false,true,false);
    if (first.choice!=WorldResultsChoice::NextWorld)
        return fail("test_033_world_choice_error");
    playWorldCues(first.cues);
    leaveWorldResults(flow,true);
    if (flow.screen!=Screen::Hole || menuCourse!=FinalFrontierFirstCourse)
        return fail("test_033_world_next_error");

    worldResults.begin(CompletedWorld::ChucklePark,CourseCount,false);
    flow.screen=Screen::WorldResults;
    const auto replay=worldResults.update(choice+250,false,true,false,true,false);
    if (worldResults.firstUnlock() || replay.choice!=WorldResultsChoice::LevelSelect ||
        (replay.cues&cueMask(WorldCue::Unlock)) || !capture("033-ru-w3-results-replay.565",choice+1450))
        return fail("test_033_world_replay_error");
    leaveWorldResults(flow,false);
    if (flow.screen!=Screen::LevelSelect || menuCourse!=ChuckleCoasterCourse ||
        !levelSelect.changeWorld(1,3000))
        return fail("test_033_world_map_error");
    menuCourse=levelSelect.course();
    if (menuCourse!=FinalFrontierFirstCourse || levelSelect.world()!=4 || levelSelect.cell()!=0 ||
        levelSelect.unlocked(4,1) || !capture("033-ru-map-frontier-slide.565",3300) ||
        !capture("033-ru-map-frontier.565",4000))
        return fail("test_033_frontier_map_error");
    logEvent(log,"test_033_world_clear",round.game);
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
        !capture("033-ru-4_01-start.565"))
        return fail("test_033_enter_snow_error");

    const auto& stage=courseHazards(currentCourse);
    if (stage.blockCount!=19 || stage.sandCount!=2) return fail("test_033_snow_geometry_error");
    while (!round.caught && round.ticks<2400) round.step();
    if (!round.caught || !capture("033-ru-4_01-loss.565",0,1000))
        return fail("test_033_snow_loss_error");
    round.restart();
    if (!changeLanguage(executable,course,config,Language::English) ||
        !capture("033-en-4_01-start.565") ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_033_snow_locale_error");
    round.restart();
    constexpr int snowShots[][2]={{72,86},{120,-44},{0,-127},{120,10}};
    unsigned sandTicks=0;
    for (const auto& input:snowShots) {
        if (!shot(input[0],input[1])) return fail("test_033_snow_shot_error");
        for (unsigned i=0;i<1000 && round.game.moving() && !round.caught && !round.game.won;++i) {
            round.step();
            if (round.inSand && !sandTicks++ &&
                !capture("033-ru-4_01-sand.565")) return fail("test_033_snow_capture_error");
        }
        if (round.caught || (!round.game.won && round.game.moving()))
            return fail("test_033_snow_route_error");
    }
    if (!round.game.won || sandTicks!=9 || round.brokenBlocks!=0x52801 ||
        round.blockMultiplier!=5 || round.blockBonus!=150 ||
        !capture("033-ru-4_01-win.565",0,4700))
        return fail("test_033_snow_goal_error");
    logEvent(log,"test_033_snow_goal",round.game);
    Progress saved{};
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild ||
        progress.next!=CourseCount || !save.read(saved) || saved.next!=CourseCount ||
        saved.best[ChuckleCoasterCourse].time==UINT32_MAX ||
        saved.best[FinalFrontierFirstCourse].time==UINT32_MAX ||
        saved.totalScore()<=seed.totalScore() || saved.totalPrisms()<=seed.totalPrisms())
        return fail("test_033_save_error");
    for (unsigned i=0;i<ChuckleCoasterCourse;++i) {
        if (saved.best[i].score!=seed.best[i].score || saved.best[i].rank!=seed.best[i].rank ||
            saved.best[i].time!=seed.best[i].time) return fail("test_033_save_history_error");
    }
    return true;
}
