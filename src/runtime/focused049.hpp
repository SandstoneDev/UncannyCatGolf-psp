#pragma once

#include "route_steps048.hpp"
#include "route_steps049.hpp"

bool runFocused049(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=CosmicEighthCourse ||
        progress.totalScore()!=1126722 || progress.totalPrisms()!=479 || !progress.frontierKey ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_049_seed");
    const Progress seed=progress;
    round.totalTicks=progress.ticks;
    round.rhythmTestClock=true;
    const auto capture=[&](const char* phase,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && !liveResultPrepared) return false;
        if (!result && !round.caught && flow.screen==Screen::Hole) {
            const auto& gloss=currentCourse==CosmicSeventhCourse?cosmicGloss048[1+round.switches.state]:
                cosmicGloss049[currentCourse-CosmicEighthCourse];
            const auto frame=(currentCourse==CosmicNinthCourse?round.polyrhythm.mask:round.rhythm.fieldIndex())*
                CosmicGloss048Phases+cosmicGlossPhase048(round.ticks);
            if (cosmicFieldSlot046.owner!=&gloss || cosmicFieldSlot046.id!=frame || !cosmicFlagSlot046.owner) return false;
        }
        return dumpFrame(executable,(std::string("049-")+phase+".565").c_str());
    };
    const auto start=[&](unsigned index,const char* menu,const char* field) {
        openLevelSelect(flow,index);
        if (!capture(menu,1000) || levelSelect.world()!=6 || menuCourse!=index ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,1100) || !selectorGoDown ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,1200) || selectorGoDown) return false;
        flow.update(0,true,false,false,false);
        return flow.screen==Screen::Hole && enterCourse(executable,index,course,round) && capture(field);
    };
    const auto fire=[&](const auto& shot) {
        round.updateControls({shot.x/127.0f,shot.y/127.0f},true,false);
        if (round.updateControls({},false,false)!=ControlEvent::Shot) return false;
        round.coins.multiplier=0;
        return true;
    };
    const auto runTo=[&](unsigned tick) {
        while (round.ticks<tick && !round.caught && !round.game.won) round.step();
        return round.ticks==tick && !round.caught && !round.game.won;
    };
    const auto parkGhosts=[&]() {
        round.ghost.position={0,-4000};
        for (unsigned i=0;i<round.extraGhostCount;++i) round.extraGhosts[i].position={float(i*80),-4000};
    };
    const auto play=[&](const auto& route,unsigned limit,const auto& frame) {
        unsigned shot=0;
        while (round.ticks<limit && !round.game.won && !round.caught) {
            if (shot<std::size(route) && round.ticks==route[shot].tick && !fire(route[shot++])) return false;
            round.step();
            if (!frame(round.ticks)) return false;
        }
        return round.game.won && !round.caught && shot==std::size(route);
    };

    flow.screen=Screen::Hole;
    if (!enterCourse(executable,CosmicSeventhCourse,course,round) ||
        !play(CosmicRoute607,900,[&](unsigned tick) { return tick!=107 || capture("p7-delta"); }) ||
        round.ticks!=CosmicWinTick607 || round.game.bounces!=CosmicWinBounces607 ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicEighthCourse) return fail("test_049_p7_advance");
    logEvent(log,"test_049_048_delta",round.game);

    if (!start(CosmicEighthCourse,"p8-map-ru","p8-both-ru") || course.par!=5 || walls.onTime!=15 ||
        round.extraGhostCount!=9 || round.stopStage!=&CosmicStop608 || !cosmicPortalArt608.texture() ||
        round.rhythm.phase!=Metronome048::Both || courseHazards(currentCourse).sandCount!=1 ||
        objects.data.courses[currentCourse].count!=30) return fail("test_049_p8_setup");

    for (unsigned phase=0;phase<2;++phase) {
        if (!enterCourse(executable,CosmicEighthCourse,course,round) || !runTo(phase?107:54) ||
            round.rhythm.phase!=phase) return fail("test_049_p8_phase");
        unsigned mask=0;
        for (unsigned i=0;i<10;++i) if (round.rhythmGhostActive(i)) mask|=1u<<i;
        if (mask!=CosmicGhostMasks608[phase]) return fail("test_049_p8_ghost_masks");
        auto& inactive=round.extraGhosts[phase?0:4];
        inactive={round.game.position,{12,7}};
        round.step();
        if (round.caught || length(inactive.position-round.game.position)>0 ||
            length(inactive.velocity-Vec2{12,7})>0) return fail("test_049_p8_ghost_freeze");
        auto& active=phase?round.extraGhosts[4]:round.ghost;
        active.position=round.game.position;
        round.step();
        if (!round.caught || round.fellOut || round.backrooms) return fail("test_049_p8_ghost_catch");
    }
    logEvent(log,"test_049_p8_ghost_groups",round.game);
    if (!enterCourse(executable,CosmicEighthCourse,course,round)) return fail("test_049_p8_sand_retry");
    parkGhosts();
    round.game.position={-390,390};
    round.game.velocity={30,0};
    const auto before=round.game.velocity;
    round.step();
    if (!round.inSand || length(round.game.velocity)>=length(before)*0.86f ||
        !capture("p8-sand")) return fail("test_049_p8_sand_fail");
    logEvent(log,"test_049_p8_sand",round.game);

    if (!enterCourse(executable,CosmicEighthCourse,course,round)) return fail("test_049_p8_portal_retry");
    parkGhosts();
    for (unsigned i:{0u,2u}) {
        round.game.position=CosmicPortals608.nodes[i].position;
        round.game.velocity={};
        round.step();
        if (length(round.game.position-CosmicPortals608.nodes[i+1].position)>0) return fail("test_049_p8_portal");
        for (unsigned j=0;j<4;++j) round.step();
        if (length(round.game.position-CosmicPortals608.nodes[i+1].position)>0)
            return fail("test_049_p8_portal_direction");
    }
    logEvent(log,"test_049_p8_portals",round.game);
    if (!enterCourse(executable,CosmicEighthCourse,course,round)) return fail("test_049_p8_bounds_retry");
    round.game.position={course.right+25,0};
    round.step();
    if (!round.caught || !round.backrooms || round.fellOut || !capture("p8-backrooms"))
        return fail("test_049_p8_bounds");
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicEighthCourse,"p8-map-en","p8-both-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicEighthCourse,course,round)) return fail("test_049_p8_en");

    const auto p8Frame=[&](unsigned tick) {
        if (tick==54) return round.rhythm.phase==0 && capture("p8-rhythm0");
        if (tick==76) return length(round.game.position-CosmicPortals608.nodes[1].position)<1 && capture("p8-portal1");
        if (tick==107) return round.rhythm.phase==1 && capture("p8-rhythm1");
        if (tick==166) return round.stopRun.has(0) && capture("p8-stop");
        if (tick==312) return length(round.game.position-CosmicPortals608.nodes[3].position)<1 && capture("p8-portal2");
        return true;
    };
    if (!play(CosmicRoute608,1000,p8Frame) || round.ticks!=CosmicWinTick608 ||
        round.game.bounces!=CosmicWinBounces608 || round.coins.collected[0]!=CosmicCoinMask608 ||
        round.coins.bonus!=CosmicCoinBonus608 || !capture("p8-win",0,4700))
        return fail("test_049_p8_route");
    logEvent(log,"test_049_p8_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicNinthCourse || progress.next!=CosmicNinthCourse)
        return fail("test_049_p8_advance");

    if (!start(CosmicNinthCourse,"p9-map-ru","p9-both-ru") || course.par!=5 || walls.onTime!=20 ||
        round.extraGhostCount || round.polyrhythm.mask!=Polyrhythm049::All ||
        objects.data.courses[currentCourse].count!=7) return fail("test_049_p9_setup");
    const auto& coins=objects.data.courses[currentCourse];
    parkGhosts();
    if (!runTo(80) || round.polyrhythm.mask!=Polyrhythm049::Pink || !capture("p9-pink"))
        return fail("test_049_p9_pink");
    round.game.position=coins.coins[3];
    round.step();
    if (round.coins.has(3) || !runTo(107) || round.polyrhythm.mask || round.rhythmWalls.activeTiles() ||
        !capture("p9-neither")) return fail("test_049_p9_disabled");
    round.game.position=coins.coins[0];
    round.step();
    if (round.coins.has(0)) return fail("test_049_p9_pink_disabled");
    round.game.position=course.spawn;
    if (!runTo(134) || round.polyrhythm.mask ||
        !runTo(160) || round.polyrhythm.mask!=Polyrhythm049::All)
        return fail("test_049_p9_schedule");
    if (!runTo(214) || round.polyrhythm.mask!=Polyrhythm049::Cyan || !capture("p9-cyan"))
        return fail("test_049_p9_cyan");
    round.game.position=coins.coins[0];
    round.step();
    if (round.coins.has(0)) return fail("test_049_p9_pink_gating");
    round.game.position=coins.coins[3];
    round.step();
    if (!round.coins.has(3)) return fail("test_049_p9_cyan_collect");
    logEvent(log,"test_049_p9_polyrhythm",round.game);
    if (!enterCourse(executable,CosmicNinthCourse,course,round)) return fail("test_049_p9_bounds_retry");
    round.game.position={course.right+25,0};
    round.step();
    if (!round.caught || !round.fellOut || round.backrooms || !capture("p9-oob",0,100))
        return fail("test_049_p9_bounds");
    logEvent(log,"test_049_p9_oob",round.game);
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicNinthCourse,"p9-map-en","p9-both-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicNinthCourse,course,round)) return fail("test_049_p9_en");

    if (!play(CosmicRoute609,1100,[&](unsigned tick) { return tick!=160 || capture("p9-cycle"); }) ||
        round.ticks!=CosmicWinTick609 || round.game.bounces!=CosmicWinBounces609 ||
        round.coins.collected[0]!=CosmicCoinMask609 || round.coins.bonus!=CosmicCoinBonus609 ||
        !capture("p9-win",0,4700)) return fail("test_049_p9_route");
    logEvent(log,"test_049_p9_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild) return fail("test_049_p9_advance");

    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.frontierKey ||
        saved.elysianKey!=seed.elysianKey || !saved.best[CosmicEighthCourse].rank ||
        !saved.best[CosmicNinthCourse].rank) return fail("test_049_save");
    for (unsigned i=0;i<CosmicEighthCourse;++i) {
        const auto& before=seed.best[i];
        const auto& after=saved.best[i];
        if (before.score!=after.score || before.rank!=after.rank || before.time!=after.time)
            return fail("test_049_previous_scores");
    }
    return true;
}
