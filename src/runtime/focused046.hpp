#pragma once

#include "route_steps046.hpp"

bool runFocused046(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=CosmicSecondCourse || progress.totalScore()!=1067444 ||
        progress.totalPrisms()!=451 || !progress.frontierKey ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_046_seed");
    const Progress seed=progress;
    round.totalTicks=progress.ticks;
    const auto capture=[&](const char* phase,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && !liveResultPrepared) return false;
        if (!result && flow.screen==Screen::Hole && currentCourse>=CosmicSecondCourse &&
            (!cosmicFieldSlot046.owner || !cosmicFlagSlot046.owner || !cosmicPlantSlot046.owner)) return false;
        return dumpFrame(executable,(std::string("046-")+phase+".565").c_str());
    };
    const auto go=[&](unsigned index,unsigned clock=1100) {
        if (flow.screen!=Screen::LevelSelect || levelSelect.world()!=6 || menuCourse!=index ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,clock) || !selectorGoDown ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,clock+100) || selectorGoDown) return false;
        flow.update(0,true,false,false,false);
        return flow.screen==Screen::Hole && enterCourse(executable,index,course,round);
    };
    const auto start=[&](unsigned index,const char* menu,const char* field) {
        openLevelSelect(flow,index);
        return capture(menu,1000) && go(index) && capture(field);
    };
    const auto shoot=[&](int x,int y) {
        round.updateControls({x/127.0f,y/127.0f},true,false);
        if (round.updateControls({},false,false)!=ControlEvent::Shot) return false;
        round.coins.multiplier=0;
        return true;
    };

    flow.screen=Screen::Hole;
    if (!enterCourse(executable,CosmicFirstCourse,course,round)) return fail("test_046_601_load");
    round.step();
    if (!shoot(-16,-126)) return fail("test_046_601_stroke");
    for (unsigned i=0;i<120 && !round.game.won && !round.caught;++i) round.step();
    if (!round.game.won || round.caught || round.ticks!=53 || round.game.bounces!=2 ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicSecondCourse) return fail("test_046_601_advance");

    if (!start(CosmicSecondCourse,"p2-map-ru","p2-start-ru") ||
        course.par!=7 || walls.onTime!=20 || !round.jumpStage || round.extraGhostCount!=2 ||
        round.skullCount!=2 || round.chuckles.count!=20 || round.keyStage!=&CosmicKeys602 ||
        courseHazards(currentCourse).blockCount!=2 || clips.count(Clip::Win602) || clips.count(Clip::Loss602))
        return fail("test_046_p2_setup");
    if (cosmicWorldSlot046.owner!=nullptr) return fail("test_046_map_cache");
    for (unsigned ghostIndex=0;ghostIndex<3;++ghostIndex) {
        round.restart();
        round.game.position=ghostIndex?round.extraGhosts[ghostIndex-1].position:round.ghost.position;
        round.step();
        if (!round.caught || (ghostIndex==2 && !capture("p2-ghost-ru",0,600)))
            return fail("test_046_p2_ghosts");
    }
    if (!enterCourse(executable,CosmicSecondCourse,course,round)) return fail("test_046_p2_retry");
    round.game.position={256,-100};
    round.game.velocity={0,600};
    for (unsigned i=0;i<4;++i) round.step();
    if (round.game.bounces!=1 || round.game.velocity.y>=0 || round.roundKeys.opened)
        return fail("test_046_p2_locked_door");
    logEvent(log,"test_046_p2_locked_door",round.game);
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicSecondCourse,"p2-map-en","p2-start-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicSecondCourse,course,round)) return fail("test_046_p2_en");

    startCourseMusic(CosmicSecondCourse);
    for (unsigned i=0;i<100 && audio.musicPositionSeconds()==0;++i) sceKernelDelayThread(10000);
    if (audio.musicPositionSeconds()==0) return fail("test_046_wp_music");
    logEvent(log,"test_046_wp_music",round.game);
    unsigned shot=0,jumps=0;
    bool glass=false;
    for (unsigned frame=0;frame<650 && !round.game.won && !round.caught;++frame) {
        if (shot<std::size(CosmicRoute602) && round.ticks==CosmicRoute602[shot].tick) {
            const auto& next=CosmicRoute602[shot++];
            if (!shoot(next.x,next.y)) return fail("test_046_p2_shot");
        }
        const unsigned bounces=round.game.bounces;
        round.step();
        jumps+=round.jump.timer==1;
        glass|=round.game.bounces!=bounces && round.game.fixedTileBounce;
        if (round.ticks==59 && !capture("p2-key")) return fail("test_046_p2_key_art");
        if (round.ticks==310 && !capture("p2-jump")) return fail("test_046_p2_jump_art");
        if (round.ticks==492 && !capture("p2-combat")) return fail("test_046_p2_combat_art");
    }
    if (!round.game.won || round.caught || shot!=3 || round.game.strokes!=3 || jumps!=1 ||
        round.ticks!=CosmicWinTick602 || round.game.bounces!=CosmicWinBounces602 ||
        round.roundKeys.collected!=1 || round.roundKeys.opened!=1 || round.roundKeys.bonus!=200 ||
        !round.brokenBlocks.has(0) || !round.brokenBlocks.has(1) || round.blockBonus!=30 ||
        round.skulls.bonus!=50 || round.chuckles.defeatedCount!=2 || round.speedRun.collected!=1 ||
        !glass || !capture("p2-win-ru",0,4700)) return fail("test_046_p2_route");
    logEvent(log,"test_046_p2_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicThirdCourse || progress.next!=CosmicThirdCourse) return fail("test_046_p2_advance");

    openLevelSelect(flow,CosmicThirdCourse);
    if (!capture("p3-map-ru",1000) || !cosmicWorldSlot046.owner ||
        updateLevelSelector(PSP_CTRL_LTRIGGER,0,PSP_CTRL_LTRIGGER,1200) ||
        levelSelect.world()!=5 || !capture("p3-map-to-elysian",1320) ||
        updateLevelSelector(PSP_CTRL_RTRIGGER,0,PSP_CTRL_RTRIGGER,3000) ||
        levelSelect.world()!=6 || !capture("p3-map-from-elysian",3120) ||
        !go(CosmicThirdCourse,4600) || !capture("p3-start-ru")) return fail("test_046_p3_map");
    if (course.par!=7 || walls.onTime!=20 || !round.jumpStage || round.extraGhostCount ||
        round.speedStage.count!=2 || round.stopStage!=&CosmicStop603 || !round.switchStage ||
        round.switchStage->layerCount!=2 || round.switchStage->blockCount!=2 ||
        objects.data.courses[currentCourse].count!=32 || clips.count(Clip::Win603) || clips.count(Clip::Loss603))
        return fail("test_046_p3_setup");

    const auto active=[&](Vec2 point) {
        return round.switches.activeTiles()->sweep(point,{40,0},course.playerRadius).time<=1;
    };
    if (round.switches.state || !active({-312,-152}) || active({-288,-72}))
        return fail("test_046_switch_initial");
    round.game.position={-198,18};
    round.game.velocity={600,0};
    for (unsigned i=0;i<4;++i) round.step();
    if (round.switches.state!=1 || active({-312,-152}) || !active({-288,-72}) ||
        !capture("p3-switch-state1")) return fail("test_046_switch_state1");
    round.game.position={-198,48};
    round.game.velocity={600,0};
    for (unsigned i=0;i<4;++i) round.step();
    if (round.switches.state || !active({-312,-152}) || active({-288,-72}) ||
        !capture("p3-switch-state0")) return fail("test_046_switch_state0");
    logEvent(log,"test_046_switch_both",round.game);
    round.restart();
    round.game.position={-144,144};
    round.step();
    if (round.coins.bonus!=50 || !capture("p3-coin")) return fail("test_046_p3_coin");
    logEvent(log,"test_046_p3_coin",round.game);
    round.restart();
    round.game.position={537,0};
    round.step();
    if (!round.caught || !round.fellOut || round.backrooms || !capture("p3-oob",0,100))
        return fail("test_046_p3_oob");
    logEvent(log,"test_046_p3_oob",round.game);
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicThirdCourse,"p3-map-en","p3-start-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicThirdCourse,course,round)) return fail("test_046_p3_en");

    round.game.position=round.ghost.position;
    round.step();
    if (!round.caught || !capture("p3-ghost-ru",0,600) ||
        !enterCourse(executable,CosmicThirdCourse,course,round)) return fail("test_046_p3_ghost");
    shot=jumps=0;
    for (unsigned frame=0;frame<550 && !round.game.won && !round.caught;++frame) {
        if (shot<std::size(CosmicRoute603) && round.ticks==CosmicRoute603[shot].tick) {
            const auto& next=CosmicRoute603[shot++];
            if (!shoot(next.x,next.y)) return fail("test_046_p3_shot");
        }
        round.step();
        jumps+=round.jump.timer==1;
        if (round.ticks==110 && !capture("p3-jump")) return fail("test_046_p3_jump_art");
        if (round.ticks==152 && !capture("p3-stop")) return fail("test_046_p3_stop_art");
    }
    if (!round.game.won || round.caught || shot!=4 || round.game.strokes!=4 || jumps!=1 ||
        round.ticks!=CosmicWinTick603 || round.game.bounces!=CosmicWinBounces603 ||
        round.switches.state!=1 || round.speedRun.collected!=3 || round.stopRun.collected!=2 ||
        !capture("p3-win-ru",0,4700)) return fail("test_046_p3_route");
    logEvent(log,"test_046_p3_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild) return fail("test_046_p3_advance");

    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.frontierKey ||
        !saved.best[CosmicSecondCourse].rank || !saved.best[CosmicThirdCourse].rank ||
        saved.elysianKey!=seed.elysianKey) return fail("test_046_save");
    for (unsigned i=0;i<CosmicSecondCourse;++i) {
        const auto& before=seed.best[i];
        const auto& after=saved.best[i];
        if (before.score!=after.score || before.rank!=after.rank || before.time!=after.time)
            return fail("test_046_previous_scores");
    }
    return true;
}
