#pragma once

#include "frontier_routes036.hpp"

bool runFocused036Frontier(const char* executable,Course& course,SaveFile& save,
                           const LanguageFile& config,const char* log) {
    Flow flow;
    chillMode=false;
    Round round(course);
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    const auto capture=[&](const char* name,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        return dumpFrame(executable,name);
    };
    const auto shot=[&](const FrontierStroke036& stroke) {
        round.controls.update(round.game,course,{stroke.x/127.0f,stroke.y/127.0f},true,false);
        return round.controls.update(round.game,course,{},false,false)==ControlEvent::Shot;
    };
    if (!save.read(progress) || progress.next!=FrontierPatrolCourse ||
        progress.best[UncannyValleyCourse].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_frontier_seed_68_error");
    const Progress seed=progress;
    logEvent(log,"test_frontier_seed_68",round.game);

    openLevelSelect(flow);
    if (levelSelect.world()!=4 || levelSelect.cell()!=9 || levelSelect.unlocked(4,10) ||
        menuCourse!=FrontierPatrolCourse || !capture("frontier-ru-4_10-map.565",3000) ||
        updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
        !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090))
        return fail("test_frontier_select_410_error");
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
        round.extraGhostCount!=3 || dogGate.active() || !capture("frontier-ru-4_10-start.565"))
        return fail("test_frontier_enter_410_error");
    bool dogCaptured=false;
    const auto stepPatrol=[&]() {
        round.step();
        if (round.dog.winner && !dogCaptured) {
            dogCaptured=capture("frontier-ru-4_10-dog.565");
            if (!dogCaptured) return false;
            logEvent(log,"test_frontier_dog_410",round.game);
        }
        return !round.caught;
    };
    for (const auto& stroke:FrontierRoute410036) {
        for (unsigned wait=0;wait<stroke.wait;++wait)
            if (!stepPatrol()) return fail("test_frontier_wait_410_error");
        if (!shot(stroke)) return fail("test_frontier_shot_410_error");
        for (unsigned i=0;i<1500 && round.game.moving() && !round.game.won;++i)
            if (!stepPatrol()) return fail("test_frontier_route_410_error");
        if (!round.game.won && round.game.moving()) return fail("test_frontier_stop_410_error");
    }
    if (!round.game.won || !dogCaptured || round.game.strokes!=3 || round.game.bounces!=33 ||
        !capture("frontier-ru-4_10-win.565",0,4700))
        return fail("test_frontier_goal_410_error");
    logEvent(log,"test_frontier_goal_410",round.game);

    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=FrontierMineCourse || progress.next!=FrontierMineCourse ||
        round.skullCount!=27 || std::size(round.mineDogs)!=5 || !capture("frontier-ru-4_11-start.565"))
        return fail("test_frontier_enter_411_error");

    bool blocksCaptured=false;
    for (const auto& stroke:FrontierRoute411036) {
        if (!shot(stroke)) return fail("test_frontier_shot_411_error");
        for (unsigned i=0;i<1500 && round.game.moving() && !round.caught && !round.game.won;++i) {
            round.step();
            if (round.brokenBlocks.high && !blocksCaptured) {
                blocksCaptured=capture("frontier-ru-4_11-blocks.565");
                if (!blocksCaptured) return fail("test_frontier_blocks_capture_error");
            }
        }
        if (round.caught || (!round.game.won && round.game.moving()))
            return fail("test_frontier_route_411_error");
    }
    if (!round.game.won || !blocksCaptured || round.game.strokes!=4 || round.game.bounces!=52 ||
        round.blockMultiplier!=44 || round.blockBonus!=9900 ||
        round.brokenBlocks.high!=6 || round.brokenBlocks.low!=0x0c8efc00efbfffffULL ||
        !capture("frontier-ru-4_11-win.565",0,4700))
        return fail("test_frontier_goal_411_error");
    logEvent(log,"test_frontier_blocks_411",round.game);
    logEvent(log,"test_frontier_goal_411",round.game);

    Progress saved{};
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild ||
        !save.read(saved) || saved.next!=CourseCount ||
        saved.best[FrontierPatrolCourse].time==UINT32_MAX || saved.best[FrontierMineCourse].time==UINT32_MAX ||
        saved.totalScore()<=seed.totalScore() || saved.totalPrisms()<=seed.totalPrisms())
        return fail("test_frontier_save_70_error");
    for (unsigned i=0;i<FrontierPatrolCourse;++i) {
        if (saved.best[i].score!=seed.best[i].score || saved.best[i].rank!=seed.best[i].rank ||
            saved.best[i].time!=seed.best[i].time) return fail("test_frontier_history_error");
    }
    openLevelSelect(flow,FrontierMineCourse);
    if (levelSelect.world()!=4 || levelSelect.cell()!=10 || !capture("frontier-ru-4_11-map.565",3000))
        return fail("test_frontier_select_411_error");
    return true;
}
