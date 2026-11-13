#pragma once

#include "frontier_routes033.hpp"

bool runFocused033Frontier(const char* executable,Course& course,SaveFile& save,
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
    const auto shot=[&](int x,int y) {
        round.controls.update(round.game,course,{x/127.0f,y/127.0f},true,false);
        return round.controls.update(round.game,course,{},false,false)==ControlEvent::Shot;
    };
    if (!save.read(progress) || progress.next!=FrontierSwitchCourse ||
        progress.best[FinalFrontierFirstCourse].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_frontier_seed_error");
    const Progress seed=progress;
    logEvent(log,"test_frontier_seed_60",round.game);

    openLevelSelect(flow);
    if (levelSelect.world()!=4 || levelSelect.cell()!=1 || levelSelect.unlocked(4,2) ||
        menuCourse!=FrontierSwitchCourse || !capture("frontier-ru-map-4_02.565",3000) ||
        updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
        !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090))
        return fail("test_frontier_select_402_error");
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
        !round.switchStage || round.switchStage->layerCount!=4 || round.switchStage->blockCount!=31 ||
        !round.speedStage || round.speedStage.count!=2 || !capture("frontier-ru-4_02-state0.565"))
        return fail("test_frontier_enter_402_error");
    while (!round.caught && round.ticks<1000) round.step();
    if (!round.caught || !capture("frontier-ru-4_02-loss.565",0,1000))
        return fail("test_frontier_loss_402_error");
    round.restart();
    if (!shot(FrontierRoute402033[0].x,FrontierRoute402033[0].y)) return fail("test_frontier_shot_402_error");
    unsigned switchHits=0;
    constexpr const char* phases[]{"frontier-ru-4_02-state0.565","frontier-ru-4_02-state1.565",
                                   "frontier-ru-4_02-state2.565","frontier-ru-4_02-state3.565"};
    for (unsigned i=0;i<1000 && round.game.moving() && !round.caught && !round.game.won;++i) {
        const unsigned oldState=round.switches.state;
        round.step();
        if (round.switches.state!=oldState) {
            if (round.switches.state!=(oldState+1)%4)
                return fail("test_frontier_phase_order_error");
            if (++switchHits<4 && !capture(phases[round.switches.state]))
                return fail("test_frontier_phase_capture_error");
        }
    }
    if (round.caught || !round.game.won || switchHits!=4 || round.switches.state!=0 ||
        round.speedRun.collected!=3 || round.game.strokes!=1 ||
        !capture("frontier-ru-4_02-win.565",0,4700))
        return fail("test_frontier_goal_402_error");
    logEvent(log,"test_frontier_goal_402",round.game);

    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=FrontierPortalCourse || progress.next!=FrontierPortalCourse ||
        round.switchStage || !capture("frontier-ru-4_03-start.565"))
        return fail("test_frontier_enter_403_error");

    const PortalStage portalStage=frontierPortals(currentCourse);
    if (portalStage.count!=8) return fail("test_frontier_portal_count_error");
    for (unsigned source=0;source<portalStage.count;++source) {
        const auto& node=portalStage.nodes[source];
        Game probe(course);
        probe.position=node.position;
        probe.velocity={33,-45};
        PortalRun portals;
        portals.restart(portalStage);
        const bool active=node.target<portalStage.count;
        if (portals.step(probe,course.playerRadius)!=active) return fail("test_frontier_portal_entry_error");
        const Vec2 expected=portalStage.nodes[active?node.target:source].position;
        for (unsigned tick=0;tick<4;++tick) {
            if (length(probe.position-expected)>0.001f || probe.velocity.x!=33 || probe.velocity.y!=-45 ||
                portals.step(probe,course.playerRadius))
                return fail("test_frontier_portal_repeat_error");
        }
    }
    logEvent(log,"test_frontier_portals_403",round.game);
    unsigned transfers=0,exitMask=0;
    for (const auto& input:FrontierRoute403033) {
        if (!shot(input.x,input.y)) return fail("test_frontier_shot_403_error");
        for (unsigned i=0;i<1000 && round.game.moving() && !round.caught && !round.game.won;++i) {
            const Vec2 before=round.game.position;
            round.step();
            if (length(round.game.position-before)>100) {
                ++transfers;
                for (unsigned p=0;p<portalStage.count;++p) {
                    if (length(round.game.position-portalStage.nodes[p].position)<0.001f) exitMask|=1u<<p;
                }
                if (transfers==1 && !capture("frontier-ru-4_03-portal.565"))
                    return fail("test_frontier_portal_capture_error");
            }
        }
        if (round.caught || (!round.game.won && round.game.moving()))
            return fail("test_frontier_route_403_error");
    }
    if (!round.game.won || transfers!=4 || exitMask!=170 || round.game.strokes!=2 ||
        !capture("frontier-ru-4_03-win.565",0,4700))
        return fail("test_frontier_goal_403_error");
    logEvent(log,"test_frontier_goal_403",round.game);

    Progress saved{};
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild ||
        !save.read(saved) || saved.next!=CourseCount ||
        saved.best[FrontierSwitchCourse].time==UINT32_MAX || saved.best[FrontierPortalCourse].time==UINT32_MAX ||
        saved.totalScore()<=seed.totalScore() || saved.totalPrisms()<=seed.totalPrisms())
        return fail("test_frontier_save_error");
    for (unsigned i=0;i<FrontierSwitchCourse;++i) {
        if (saved.best[i].score!=seed.best[i].score || saved.best[i].rank!=seed.best[i].rank ||
            saved.best[i].time!=seed.best[i].time) return fail("test_frontier_history_error");
    }

    openLevelSelect(flow,FrontierPortalCourse);
    if (levelSelect.world()!=4 || levelSelect.cell()!=2 || !capture("frontier-ru-map-4_03.565",3000))
        return fail("test_frontier_select_403_error");
    return true;
}
