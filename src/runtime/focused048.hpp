#pragma once

#include "route_steps047.hpp"
#include "route_steps048.hpp"

bool runFocused048(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=CosmicSixthCourse ||
        progress.totalScore()!=1109463 || progress.totalPrisms()!=470 || !progress.frontierKey ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_048_seed");
    const Progress seed=progress;
    round.totalTicks=progress.ticks;
    const auto capture=[&](const char* phase,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && !liveResultPrepared) return false;
        if (!result && !round.backrooms && flow.screen==Screen::Hole) {
            if (!cosmicFieldSlot046.owner || !cosmicFlagSlot046.owner ||
                cosmicFieldSlot046.id!=round.rhythm.fieldIndex()*CosmicGloss048Phases+cosmicGlossPhase048(round.ticks)) return false;
            if (currentCourse==CosmicSeventhCourse &&
                (!cosmicWaterSlot048.owner || !cosmicPlantsSlot048.owner)) return false;
        }
        return dumpFrame(executable,(std::string("048-")+phase+".565").c_str());
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
    const auto ghostAt=[&](unsigned index) -> GhostMotion& { return index?round.extraGhosts[index-1]:round.ghost; };
    const auto checkGhosts=[&](unsigned index) {
        const unsigned count=1+round.extraGhostCount;
        for (unsigned i=0;i<count;++i) {
            const unsigned activeAt=i%2?107:54;
            for (bool active:{false,true}) {
                if (!enterCourse(executable,index,course,round) || !runTo(active?activeAt:(i%2?54:107))) return false;
                const auto position=round.game.position;
                const auto velocity=Vec2{7,11};
                ghostAt(i)={position,velocity};
                round.step();
                if (round.caught!=active || round.fellOut || round.backrooms) return false;
                if (!active && (length(ghostAt(i).position-position)>0 || length(ghostAt(i).velocity-velocity)>0)) return false;
            }
        }
        return true;
    };
    const auto bounds=[&](unsigned index,const char* phase) {
        if (!enterCourse(executable,index,course,round)) return false;
        round.game.position={course.right+24,0};
        round.step();
        if (round.caught || round.backrooms) return false;
        round.game.position={course.right+25,0};
        round.step();
        return round.caught && round.backrooms && !round.fellOut && capture(phase);
    };
    flow.screen=Screen::Hole;
    if (!enterCourse(executable,CosmicFifthCourse,course,round)) return fail("test_048_p5_load");
    unsigned shot=0;
    while (round.ticks<1250 && !round.game.won && !round.caught) {
        if (shot<std::size(CosmicRoute605) && round.ticks==CosmicRoute605[shot].tick &&
            !fire(CosmicRoute605[shot++])) return fail("test_048_p5_shot");
        round.step();
    }
    if (!round.game.won || round.caught || round.ticks!=CosmicWinTick605 ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicSixthCourse) return fail("test_048_p5_advance");

    if (!start(CosmicSixthCourse,"p6-map-ru","p6-both-ru") || course.par!=2 || walls.onTime!=5 ||
        round.extraGhostCount!=1 || round.rhythm.phase!=Metronome048::Both ||
        !round.rhythmWalls.activeTiles() || !cosmicMetronomeSlot048.owner)
        return fail("test_048_p6_setup");

    startCourseMusic(CosmicSixthCourse);
    if (!enterCourse(executable,CosmicSixthCourse,course,round)) return fail("test_048_p6_audio_restart");
    const unsigned audioStart=audio.musicPositionFrames(),audibleStart=audio.blocks();
    for (unsigned i=0;i<200 && round.rhythm.beats<2;++i) {
        sceKernelDelayThread(10000);
        round.step();
    }
    if (audio.musicPositionFrames()<=audioStart || audio.blocks()<=audibleStart ||
        round.rhythm.beats!=2 || round.rhythm.phase!=0) return fail("test_048_live_cursor_fail");
    round.togglePause();
    sceKernelDelayThread(100000);
    const unsigned pausedAt=audio.musicPositionFrames(),pausedTicks=round.ticks;
    sceKernelDelayThread(100000);
    round.step();
    if (round.ticks!=pausedTicks || audio.musicPositionFrames()!=pausedAt)
        return fail("test_048_pause_fail");
    round.togglePause();
    for (unsigned i=0;i<50 && audio.musicPositionFrames()==pausedAt;++i) sceKernelDelayThread(10000);
    if (audio.musicPositionFrames()<=pausedAt) return fail("test_048_resume_fail");
    logEvent(log,"test_048_live_cursor_pause",round.game);
    round.rhythmTestClock=true;
    if (!checkGhosts(CosmicSixthCourse)) return fail("test_048_p6_ghosts_fail");
    logEvent(log,"test_048_p6_ghosts",round.game);
    if (!bounds(CosmicSixthCourse,"p6-backrooms")) return fail("test_048_p6_bounds_fail");
    logEvent(log,"test_048_p6_backrooms",round.game);
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicSixthCourse,"p6-map-en","p6-both-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicSixthCourse,course,round)) return fail("test_048_p6_en");

    shot=0;
    while (round.ticks<600 && !round.game.won && !round.caught) {
        if (shot<std::size(CosmicRoute606) && round.ticks==CosmicRoute606[shot].tick &&
            !fire(CosmicRoute606[shot++])) return fail("test_048_p6_shot");
        round.step();
        if ((round.ticks==54 && (round.rhythm.phase!=0 || !capture("p6-rhythm0"))) ||
            (round.ticks==107 && (round.rhythm.phase!=1 || !capture("p6-rhythm1"))) ||
            (round.ticks==CosmicAllCoinsTick606 && (round.rhythm.phase!=1 || !capture("p6-coins"))))
            return fail("test_048_p6_rhythm");
    }
    unsigned coins=0;
    for (unsigned i=0;i<6;++i) if (round.coins.has(i)) coins|=1u<<i;
    if (!round.game.won || round.caught || round.game.strokes!=2 || round.ticks!=CosmicWinTick606 ||
        round.game.bounces!=CosmicWinBounces606 || coins!=CosmicCoinMask606 ||
        round.coins.bonus!=CosmicCoinBonus606 || !capture("p6-win",0,4700))
        return fail("test_048_p6_route");
    logEvent(log,"test_048_p6_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicSeventhCourse || progress.next!=CosmicSeventhCourse)
        return fail("test_048_p6_advance");

    if (!start(CosmicSeventhCourse,"p7-map-ru","p7-both-ru") || course.par!=5 ||
        walls.onTime!=15 || round.extraGhostCount!=3 || round.keyStage!=&CosmicKeys607 ||
        !round.switchStage || round.switchStage->blockCount!=2 || !cosmicDoor607.texture())
        return fail("test_048_p7_setup");
    if (!checkGhosts(CosmicSeventhCourse)) return fail("test_048_p7_ghosts_fail");
    logEvent(log,"test_048_p7_ghosts",round.game);
    if (!bounds(CosmicSeventhCourse,"p7-backrooms")) return fail("test_048_p7_bounds_fail");
    logEvent(log,"test_048_p7_backrooms",round.game);
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicSeventhCourse,"p7-map-en","p7-both-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicSeventhCourse,course,round)) return fail("test_048_p7_en");

    shot=0;
    unsigned switches=0;
    while (round.ticks<900 && !round.game.won && !round.caught) {
        if (shot<std::size(CosmicRoute607) && round.ticks==CosmicRoute607[shot].tick &&
            !fire(CosmicRoute607[shot++])) return fail("test_048_p7_shot");
        const unsigned before=round.switches.state;
        round.step();
        if (round.switches.state!=before && round.switches.hitBlock>=0)
            switches|=1u<<round.switches.hitBlock;
        if ((round.ticks==CosmicSwitchTicks607[0] && !capture("p7-switch1")) ||
            (round.ticks==54 && !capture("p7-rhythm0")) ||
            (round.ticks==107 && !capture("p7-rhythm1")) ||
            (round.ticks==CosmicLockedDoorTicks607[0] && !capture("p7-door-locked")) ||
            (round.ticks==CosmicSwitchTicks607[1] && !capture("p7-switch0")) ||
            (round.ticks==CosmicKeyTick607 && !capture("p7-key")) ||
            (round.ticks==CosmicOpenDoorTick607 && !capture("p7-door-open"))) return fail("test_048_p7_art");
    }
    if (!round.game.won || round.caught || round.game.strokes!=3 || round.ticks!=CosmicWinTick607 ||
        round.game.bounces!=CosmicWinBounces607 || switches!=CosmicSwitchMask607 ||
        round.switches.state || round.roundKeys.collected!=1 || round.roundKeys.opened!=1 ||
        round.roundKeys.bonus!=CosmicDoorBonus607 || !capture("p7-win",0,4700))
        return fail("test_048_p7_route");
    logEvent(log,"test_048_p7_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild) return fail("test_048_p7_advance");

    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.frontierKey ||
        saved.elysianKey!=seed.elysianKey || !saved.best[CosmicSixthCourse].rank ||
        !saved.best[CosmicSeventhCourse].rank) return fail("test_048_save");
    for (unsigned i=0;i<CosmicSixthCourse;++i) {
        const auto& before=seed.best[i];
        const auto& after=saved.best[i];
        if (before.score!=after.score || before.rank!=after.rank || before.time!=after.time)
            return fail("test_048_previous_scores");
    }
    return true;
}
