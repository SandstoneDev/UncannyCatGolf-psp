#pragma once

#include "route_steps051.hpp"
#include "route_steps052.hpp"

bool runFocused052(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        char context[160];
        std::snprintf(context,sizeof context,"%s tick=%u keys=%u doors=%u camera=%.2f caught=%u",event,round.ticks,
                      unsigned(round.roundKeys.collected),unsigned(round.roundKeys.opened),round.santa.cameraY,
                      unsigned(round.caught));
        logEvent(log,context,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=CosmicFourteenthCourse || !progress.cosmicKey ||
        progress.totalScore()!=1191860 || progress.totalPrisms()!=506 ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_052_seed");
    const Progress seed=progress;
    round.totalTicks=progress.ticks;
    canSwim=earnedSwim045();
    const auto capture=[&](const char* phase,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && !liveResultPrepared) return false;
        if (!result && !round.caught && flow.screen==Screen::Hole) {
            if (currentCourse==CosmicFourteenthCourse && !cosmicWaterSlot614.owner) return false;
            if (currentCourse==CosmicFifteenthCourse &&
                (!santaStuckSlot.owner || !cosmicProps615.texture() ||
                 !cosmicSanta615.texture() ||
                 (round.santa.keyActive(round.ticks) && !santaKeySlot.owner))) return false;
        }
        return dumpFrame(executable,(std::string("052-")+phase+".565").c_str());
    };
    const auto start=[&](unsigned index,const char* menu,const char* field) {
        openLevelSelect(flow,index);
        if (!capture(menu,1000) || levelSelect.world()!=6 || menuCourse!=index ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,1100) || !selectorGoDown ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,1200) || selectorGoDown) return false;
        flow.update(0,true,false,false,false);
        return flow.screen==Screen::Hole && enterCourse(executable,index,course,round) && capture(field);
    };
    const auto play=[&](const auto& route,unsigned limit,const auto& frame) {
        unsigned shot=0;
        while (round.ticks<limit && !round.game.won && !round.caught) {
            if (shot<std::size(route) && round.ticks==route[shot].tick) {
                const auto& aim=route[shot++];
                round.updateControls({aim.x/127.0f,aim.y/127.0f},true,false);
                if (round.updateControls({},false,false)!=ControlEvent::Shot) return false;
                round.coins.multiplier=0;
            }
            round.step();
            if (!frame(round.ticks)) return false;
        }
        return round.game.won && !round.caught && shot==std::size(route);
    };

    flow.screen=Screen::Hole;
    if (!enterCourse(executable,CosmicThirteenthCourse,course,round) ||
        !play(CosmicRoute613,400,[&](unsigned tick) { return tick!=60 || capture("p13-delta"); }) ||
        round.ticks!=CosmicWinTick613 || round.game.bounces!=CosmicWinBounces613 ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicFourteenthCourse) return fail("test_052_p13_advance");
    logEvent(log,"test_052_051_delta",round.game);

    if (!start(CosmicFourteenthCourse,"p14-map-ru","p14-field-ru") || !canSwim ||
        course.par!=12 || walls.onTime!=20 || objects.data.courses[currentCourse].count!=63 ||
        round.stopStage!=&CosmicStop614 || !round.chuckles.spawnerVisible())
        return fail("test_052_p14_setup");
    canSwim=false;
    round.step();
    if (!round.caught || !round.waterDead || !capture("p14-water")) return fail("test_052_p14_water");
    canSwim=true;
    if (!enterCourse(executable,CosmicFourteenthCourse,course,round)) return fail("test_052_p14_retry");
    round.game.position={course.right+25,0};
    round.step();
    if (!round.caught || !round.backrooms || round.fellOut || !capture("p14-backrooms"))
        return fail("test_052_p14_bounds");
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicFourteenthCourse,"p14-map-en","p14-field-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicFourteenthCourse,course,round)) return fail("test_052_p14_en");

    round.ghost.position={0,-6000};
    while (round.ticks<300 && !round.caught) round.step();
    if (round.caught || !round.chuckles.count || !capture("p14-chuckle")) return fail("test_052_p14_spawn");
    round.game.position=CosmicNukes614[0];
    round.step();
    if (!round.nukes.has(0) || round.chuckles.count) return fail("test_052_p14_nuke");
    round.game.position=course.spawn;
    while (round.ticks<600 && !round.caught) round.step();
    if (round.caught || !round.chuckles.count) return fail("test_052_p14_respawn");
    if (!enterCourse(executable,CosmicFourteenthCourse,course,round)) return fail("test_052_p14_replay");

    if (!play(CosmicRoute614,500,[&](unsigned tick) {
            if (tick==41 && (!round.nukes.has(2) || !capture("p14-nuke"))) return false;
            if (tick==68 && (round.fakeGoals.triggered[12]!=68 || round.fakeGoals.bonus!=500 ||
                             round.game.moving() || !capture("p14-decoy"))) return false;
            return true;
        }) || round.ticks!=CosmicWinTick614 || round.game.bounces!=CosmicWinBounces614 ||
        !capture("p14-win",0,4700)) return fail("test_052_p14_route");
    logEvent(log,"test_052_p14_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicFifteenthCourse || progress.next!=CosmicFifteenthCourse)
        return fail("test_052_p14_advance");

    if (!start(CosmicFifteenthCourse,"p15-map-ru","p15-field-ru")) return fail("test_052_p15_start");
    if (course.par!=30 || walls.onTime!=35 || round.extraGhostCount!=2 ||
        round.jumpStage.count!=30 || round.keyStage!=&CosmicKeys615 || !santaWalls.active() ||
        objects.data.courses[currentCourse].count!=36) {
        char setup[160];
        std::snprintf(setup,sizeof setup,"test_052_p15_setup par=%u time=%.0f ghosts=%u pads=%u stuck=%u coins=%u",
                      unsigned(course.par),walls.onTime,round.extraGhostCount,unsigned(round.jumpStage.count),
                      unsigned(santaWalls.active()),unsigned(objects.data.courses[currentCourse].count));
        logEvent(log,setup,round.game);
        return false;
    }
    round.game.position={-300,-900};
    round.step();
    if (!round.caught || !round.fellOut || !round.santa.boundsArmed || !capture("p15-oob",0,100))
        return fail("test_052_p15_bounds");
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicFifteenthCourse,"p15-map-en","p15-field-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicFifteenthCourse,course,round)) return fail("test_052_p15_en");

    if (!play(CosmicPspRoute615,1800,[&](unsigned tick) {
            if (tick==80 && !capture("p15-abduct")) return false;
            if (tick==325 && (!(round.roundKeys.opened&1) || !capture("p15-first-door"))) return false;
            if (tick==659 && (length(round.game.position-CosmicPortalNodes615[1].position)>0.1f ||
                              !capture("p15-portal"))) return false;
            if (tick==CosmicRingStartTick615 && (!round.santa.keyActive(tick) || !capture("p15-ring"))) return false;
            if (tick==CosmicPspRingPass615 && (!round.santa.ringPassed || !(round.roundKeys.collected&2) ||
                                              !capture("p15-key"))) return false;
            if (tick==CosmicPspUpperPortal615 && ((round.roundKeys.opened&3)!=3 || round.brokenBlocks.low!=127 ||
                              length(round.game.position-CosmicPortalNodes615[3].position)>0.1f ||
                              !capture("p15-upper"))) return false;
            return true;
        }) || round.ticks!=CosmicPspWinTick615 || round.game.bounces!=CosmicPspWinBounces615 ||
        round.santa.cameraY!=SantaRun052::Top || !capture("p15-win",0,4700))
        return fail("test_052_p15_route");
    logEvent(log,"test_052_p15_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild)
        return fail("test_052_p15_advance");

    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.cosmicKey ||
        saved.elysianKey!=seed.elysianKey || saved.frontierKey!=seed.frontierKey ||
        !saved.best[CosmicFourteenthCourse].rank || !saved.best[CosmicFifteenthCourse].rank)
        return fail("test_052_save");
    for (unsigned i=0;i<CosmicFourteenthCourse;++i) {
        const auto& before=seed.best[i];
        const auto& after=saved.best[i];
        if (before.score!=after.score || before.rank!=after.rank || before.time!=after.time)
            return fail("test_052_previous_scores");
    }
    return true;
}
