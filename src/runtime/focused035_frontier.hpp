#pragma once

#include "frontier_routes035.hpp"

bool runFocused035Frontier(const char* executable,Course& course,SaveFile& save,
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
    const auto shot=[&](const FrontierStroke035& stroke) {
        for (unsigned i=0;i<stroke.wait;++i) round.step();
        round.controls.update(round.game,course,{stroke.x/127.0f,stroke.y/127.0f},true,false);
        return round.controls.update(round.game,course,{},false,false)==ControlEvent::Shot;
    };
    if (!save.read(progress) || progress.next!=FrontierMazeCourse ||
        progress.best[CubeTownCourse].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_frontier_seed_64_error");
    const Progress seed=progress;
    logEvent(log,"test_frontier_seed_64",round.game);

    openLevelSelect(flow);
    if (levelSelect.world()!=4 || levelSelect.cell()!=5 || levelSelect.unlocked(4,6) ||
        menuCourse!=FrontierMazeCourse || !capture("frontier-ru-4_06-map.565",3000) ||
        updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
        !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090))
        return fail("test_frontier_select_406_error");
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
        round.jumpStage.count!=20 || !capture("frontier-ru-4_06-start.565"))
        return fail("test_frontier_enter_406_error");

    round.game.position=FrontierBigCoin406;
    round.coins.multiplier=3;
    round.step();
    const unsigned bigCoin=objects.data.courses[currentCourse].count;
    if (!round.coins.has(bigCoin) || round.coins.values[bigCoin]!=515 || round.coins.multiplier!=4)
        return fail("test_frontier_big_coin_error");
    logEvent(log,"test_frontier_big_coin",round.game);
    round.restart();

    unsigned jumps=0,transfers=0;
    for (const auto& stroke:FrontierRoute406035) {
        if (!shot(stroke)) return fail("test_frontier_shot_406_error");
        for (unsigned i=0;i<1600 && (round.game.moving() || round.jump.airborne()) &&
            !round.caught && !round.game.won;++i) {
            const Vec2 before=round.game.position;
            const bool airborne=round.jump.airborne();
            round.step();
            if (!airborne && round.jump.airborne() && ++jumps==1 &&
                !capture("frontier-ru-4_06-jump.565")) return fail("test_frontier_jump_capture_error");
            if (length(round.game.position-before)>100) {
                ++transfers;
                if (transfers==1 && !capture("frontier-ru-4_06-portal.565"))
                    return fail("test_frontier_portal_capture_error");
            }
        }
        if (round.caught || (!round.game.won && round.game.moving()))
            return fail("test_frontier_route_406_error");
    }
    if (!round.game.won || jumps<2 || !transfers || round.game.strokes!=3 ||
        !capture("frontier-ru-4_06-win.565",0,4700))
        return fail("test_frontier_goal_406_error");
    logEvent(log,"test_frontier_goal_406",round.game);

    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=FrontierDogCourse || progress.next!=FrontierDogCourse ||
        round.skullCount!=32 || round.chuckles.count!=26 || round.jumpStage.count!=1 ||
        round.dog.winner || !capture("frontier-ru-4_07-start.565"))
        return fail("test_frontier_enter_407_error");

    bool dogCaptured=false,nukeCaptured=false;
    for (const auto& stroke:FrontierRoute407035) {
        if (!shot(stroke)) return fail("test_frontier_shot_407_error");
        for (unsigned i=0;i<1500 && (round.game.moving() || round.jump.airborne()) &&
            !round.caught && !round.game.won;++i) {
            round.step();
            if (round.dog.winner && !dogCaptured) {
                dogCaptured=capture("frontier-ru-4_07-dog.565");
                if (!dogCaptured) return fail("test_frontier_dog_capture_error");
                logEvent(log,"test_frontier_dog",round.game);
            }
            if (round.nukes.has(0) && round.ticks-round.nukes.pickedAt[0]>=10 && !nukeCaptured) {
                nukeCaptured=capture("frontier-ru-4_07-nuke.565");
                if (!nukeCaptured) return fail("test_frontier_nuke_capture_error");
            }
        }
        if (round.caught || (!round.game.won && round.game.moving()))
            return fail("test_frontier_route_407_error");
    }
    if (!round.game.won || !dogCaptured || !round.nukes.has(0) ||
        !nukeCaptured || round.game.strokes!=2 ||
        !std::all_of(round.skulls.bodies,round.skulls.bodies+round.skullCount,
                     [](const CircleCollider& body){return body.radius<=0;}) ||
        !std::all_of(round.chuckles.bodies,round.chuckles.bodies+round.chuckles.count,
                     [](const CircleCollider& body){return body.radius<=0;}) ||
        !capture("frontier-ru-4_07-win.565",0,4700))
        return fail("test_frontier_goal_407_error");
    logEvent(log,"test_frontier_nuke",round.game);
    logEvent(log,"test_frontier_goal_407",round.game);

    Progress saved{};
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild ||
        !save.read(saved) || saved.next!=CourseCount ||
        saved.best[FrontierMazeCourse].time==UINT32_MAX || saved.best[FrontierDogCourse].time==UINT32_MAX ||
        saved.totalScore()<=seed.totalScore() || saved.totalPrisms()<=seed.totalPrisms())
        return fail("test_frontier_save_66_error");
    for (unsigned i=0;i<FrontierMazeCourse;++i) {
        if (saved.best[i].score!=seed.best[i].score || saved.best[i].rank!=seed.best[i].rank ||
            saved.best[i].time!=seed.best[i].time) return fail("test_frontier_history_error");
    }
    openLevelSelect(flow,FrontierDogCourse);
    if (levelSelect.world()!=4 || levelSelect.cell()!=6 || !capture("frontier-ru-4_07-map.565",3000))
        return fail("test_frontier_select_407_error");
    return true;
}
