#pragma once

#include "route_steps046.hpp"
#include "route_steps047.hpp"

bool runFocused047(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=CosmicFourthCourse ||
        progress.totalScore()!=1088586 || progress.totalPrisms()!=460 || !progress.frontierKey ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_047_seed");
    const Progress seed=progress;
    round.totalTicks=progress.ticks;
    const auto capture=[&](const char* phase,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && !liveResultPrepared) return false;
        if (!result && flow.screen==Screen::Hole &&
            (!cosmicFieldSlot046.owner || !cosmicFlagSlot046.owner)) return false;
        return dumpFrame(executable,(std::string("047-")+phase+".565").c_str());
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
    const auto oob=[&]() {
        round.game.position={course.right+25,0};
        round.step();
        return round.caught && round.fellOut && !round.backrooms;
    };

    flow.screen=Screen::Hole;
    if (!enterCourse(executable,CosmicThirdCourse,course,round)) return fail("test_047_p3_load");
    unsigned shot=0;
    while (round.ticks<550 && !round.game.won && !round.caught) {
        if (shot<std::size(CosmicRoute603) && round.ticks==CosmicRoute603[shot].tick &&
            !fire(CosmicRoute603[shot++])) return fail("test_047_p3_shot");
        round.step();
    }
    if (!round.game.won || round.caught || round.ticks!=CosmicWinTick603 ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicFourthCourse) return fail("test_047_p3_advance");

    if (!start(CosmicFourthCourse,"p4-map-ru","p4-start-ru") || course.par!=5 ||
        walls.onTime!=20 || round.extraGhostCount!=1 || !round.switchStage ||
        round.switchStage->blockCount!=2 || round.switchStage->layerCount!=2)
        return fail("test_047_p4_setup");
    const auto active=[&](Vec2 point,Vec2 delta) {
        return round.switches.activeTiles()->sweep(point,delta,1.0f).time<=1;
    };
    const auto switchState=[&](unsigned expected) {
        return round.switches.state==expected &&
            active({40,16},{0,-24})==(expected==0) && active({80,8},{24,0})==(expected==1);
    };
    if (!switchState(0)) return fail("test_047_p4_switch_initial");
    for (const auto& block:CosmicSwitches604) {
        for (unsigned expected:{1u,0u}) {
            round.game.position=block.center+Vec2{0,42};
            round.game.velocity={0,-600};
            for (unsigned i=0;i<4;++i) round.step();
            if (!switchState(expected)) return fail("test_047_p4_switch_collision");
        }
    }
    round.game.position=CosmicSwitches604[0].center+Vec2{0,42};
    round.game.velocity={0,-600};
    for (unsigned i=0;i<4;++i) round.step();
    if (!switchState(1) || !capture("p4-switch1")) return fail("test_047_p4_switch_art");
    logEvent(log,"test_047_p4_switches",round.game);
    for (unsigned i=0;i<2;++i) {
        if (!enterCourse(executable,CosmicFourthCourse,course,round)) return fail("test_047_p4_retry");
        round.game.position=i?round.extraGhosts[0].position:round.ghost.position;
        round.step();
        if (!round.caught || !capture(i?"p4-ghost2":"p4-ghost1",0,600)) return fail("test_047_p4_ghost");
    }
    logEvent(log,"test_047_p4_ghosts",round.game);
    if (!enterCourse(executable,CosmicFourthCourse,course,round) || !oob() ||
        !capture("p4-oob",0,100)) return fail("test_047_p4_oob_fail");
    logEvent(log,"test_047_p4_oob",round.game);
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicFourthCourse,"p4-map-en","p4-start-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicFourthCourse,course,round)) return fail("test_047_p4_en");

    shot=0;
    while (round.ticks<550 && !round.game.won && !round.caught) {
        if (shot<std::size(CosmicRoute604) && round.ticks==CosmicRoute604[shot].tick &&
            !fire(CosmicRoute604[shot++])) return fail("test_047_p4_shot");
        round.step();
        if (round.ticks==275 && !capture("p4-coins")) return fail("test_047_p4_coin_art");
    }
    if (!round.game.won || round.caught || shot!=3 || round.game.strokes!=3 ||
        round.ticks!=CosmicWinTick604 || round.game.bounces!=CosmicWinBounces604 ||
        round.switches.state!=1 || round.coins.bonus!=215 || !capture("p4-win-ru",0,4700))
        return fail("test_047_p4_route");
    logEvent(log,"test_047_p4_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicFifthCourse || progress.next!=CosmicFifthCourse)
        return fail("test_047_p4_advance");

    if (!start(CosmicFifthCourse,"p5-map-ru","p5-start-ru") || course.par!=4 ||
        walls.onTime!=45 || !round.frontierWind.active || round.speedStage.count!=20 ||
        round.stopStage!=&CosmicStop605 || courseHazards(currentCourse).blockCount!=8 ||
        !starstormWeather047.texture() || !cosmicBlocks605.texture()) return fail("test_047_p5_setup");
    round.ghost.position=round.game.position;
    round.step();
    if (!round.caught || round.fellOut || !capture("p5-ghost-ru",0,600) ||
        !enterCourse(executable,CosmicFifthCourse,course,round) || !oob() ||
        !capture("p5-oob",0,100)) return fail("test_047_p5_loss");
    logEvent(log,"test_047_p5_oob",round.game);
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicFifthCourse,"p5-map-en","p5-start-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicFifthCourse,course,round)) return fail("test_047_p5_en");

    audio.muteMusic();
    for (unsigned i=0;i<100 && audio.musicPositionSeconds()!=0;++i) sceKernelDelayThread(10000);
    const unsigned windBlocks=audio.blocks();
    sceKernelDelayThread(300000);
    if (audio.blocks()<=windBlocks || audio.musicPositionSeconds()!=0) return fail("test_047_wind_pcm_fail");
    logEvent(log,"test_047_wind_pcm",round.game);
    startCourseMusic(CosmicFifthCourse);
    for (unsigned i=0;i<100 && audio.musicPositionSeconds()==0;++i) sceKernelDelayThread(10000);
    if (audio.musicPositionSeconds()==0) return fail("test_047_wp_music_fail");
    logEvent(log,"test_047_wp_music",round.game);

    const auto& ring=CosmicRings605[0];
    const auto placeBall=[&](Vec2 point) {
        round.ghost.position={0,-10000};
        round.game.position=point;
        round.game.velocity={};
        round.step();
    };
    placeBall(ring.coins[0]);
    if (round.smiley.phase!=SmileyPhase::Ready) return fail("test_047_p5_ring_early");
    placeBall(ring.center);
    if (round.smiley.remainingTicks(ring,round.ticks)!=1800) return fail("test_047_p5_ring_start");
    for (unsigned i=0;i<ring.coinCount;++i) {
        placeBall(ring.coins[i]);
        if (!round.smiley.collected(i)) return fail("test_047_p5_ring_coin");
    }
    if (round.smiley.phase!=SmileyPhase::Passed || round.smiley.bonus!=2000 ||
        !capture("p5-ring-pass")) return fail("test_047_p5_ring_pass_fail");
    logEvent(log,"test_047_p5_ring_pass",round.game);
    if (!enterCourse(executable,CosmicFifthCourse,course,round)) return fail("test_047_p5_timeout_retry");
    placeBall(ring.center);
    for (unsigned i=1;i<ring.timeTicks;++i) placeBall(ring.center);
    if (round.smiley.phase!=SmileyPhase::Active) return fail("test_047_p5_timeout_early");
    placeBall(ring.center);
    if (round.smiley.phase!=SmileyPhase::Failed || round.smiley.bonus ||
        !capture("p5-ring-timeout")) return fail("test_047_p5_timeout_fail");
    for (unsigned i=0;i<ring.lossTicks;++i) placeBall(ring.center);
    if (round.smiley.phase!=SmileyPhase::Hidden) return fail("test_047_p5_timeout_hide");
    logEvent(log,"test_047_p5_ring_timeout",round.game);
    if (!enterCourse(executable,CosmicFifthCourse,course,round)) return fail("test_047_p5_retry");

    shot=0;
    unsigned gustTicks=0,peakTicks=0;
    while (round.ticks<1250 && !round.game.won && !round.caught) {
        if (shot<std::size(CosmicRoute605) && round.ticks==CosmicRoute605[shot].tick &&
            !fire(CosmicRoute605[shot++])) return fail("test_047_p5_shot");
        const float wind=CosmicWindVelocity605(round.ticks).x;
        gustTicks+=wind!=0;
        peakTicks+=wind==-25;
        round.step();
        if ((round.ticks==151 && !capture("p5-gust")) ||
            (round.ticks==192 && !capture("p5-blocks")) ||
            (round.ticks==291 && !capture("p5-ring-start")) ||
            (round.ticks==409 && !capture("p5-stop")) ||
            (round.ticks==846 && !capture("p5-ring-eight"))) return fail("test_047_p5_route_art");
    }
    unsigned ringCoins=0;
    for (unsigned i=0;i<ring.coinCount;++i) ringCoins+=round.smiley.collected(i);
    if (!round.game.won || round.caught || shot!=3 || round.game.strokes!=3 ||
        round.ticks!=CosmicWinTick605 || round.game.bounces!=CosmicWinBounces605 ||
        round.blockBonus!=60 || !round.brokenBlocks.has(0) || !round.brokenBlocks.has(5) ||
        !round.brokenBlocks.has(6) || round.stopRun.collected!=1 || round.speedRun.collected!=0x1fef7 ||
        round.smiley.phase!=SmileyPhase::Active || ringCoins!=8 || gustTicks!=272 || peakTicks!=120 ||
        !capture("p5-win-ru",0,4700)) return fail("test_047_p5_route");
    logEvent(log,"test_047_p5_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild)
        return fail("test_047_p5_advance");
    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.frontierKey ||
        saved.elysianKey!=seed.elysianKey || !saved.best[CosmicFourthCourse].rank ||
        !saved.best[CosmicFifthCourse].rank) return fail("test_047_save");
    for (unsigned i=0;i<CosmicFourthCourse;++i) {
        const auto& before=seed.best[i];
        const auto& after=saved.best[i];
        if (before.score!=after.score || before.rank!=after.rank || before.time!=after.time)
            return fail("test_047_previous_scores");
    }
    return true;
}
