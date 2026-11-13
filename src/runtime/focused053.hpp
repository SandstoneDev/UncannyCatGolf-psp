#pragma once

#include "route_steps052.hpp"

struct Stroke053 { unsigned tick; int x,y; };
constexpr Stroke053 Route616[]={
    {0,-58,57},{114,99,-40},{358,-60,-34},{446,105,8},{693,-91,39},{762,-117,-27},
    {825,-44,-84},{842,-8,-127},{904,80,-101},{923,69,-121},{960,-60,-103},
};
constexpr Stroke053 Route617[]={
    {3,41,90},{155,53,124},{367,-62,42},{456,127,89},{781,121,-71},
    {1012,81,100},{1236,-54,123},{1334,-127,-9},{1508,127,-39},
};
constexpr Stroke053 Route617Backup[]={
    {0,42,92},{156,46,113},{257,-29,-22},{279,83,-84},{489,113,-82},
    {701,104,24},{877,-94,91},{1014,-117,-62},{1169,92,-115},
};
constexpr Stroke053 Route617Backup2[]={
    {0,80,-101},{212,-51,-104},{532,39,-99},{719,-99,85},{931,101,25},
    {1094,-110,114},{1230,-121,-10},{1395,105,96},
};
constexpr Stroke053 Route618[]={{0,-68,-127}};


bool runFocused053(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    round.rhythmTestClock=true;
    const auto fail=[&](const char* event) {
        char line[128];
        std::snprintf(line,sizeof line,"%s tick=%u keys=%u doors=%u switch=%u fake=%u caught=%u",
                      event,round.ticks,unsigned(round.roundKeys.collected),unsigned(round.roundKeys.opened),
                      round.switches.state,round.lastHurrah.revealed,unsigned(round.caught));
        logEvent(log,line,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=CosmicSixteenthCourse || progress.totalScore()!=1224444 ||
        progress.totalPrisms()!=514 || !progress.cosmicKey ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_053_seed");
    const Progress seed=progress;
    round.totalTicks=progress.ticks;
    canSwim=earnedSwim045();

    const auto capture=[&](const char* label,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && !liveResultPrepared) return false;
        if (flow.screen==Screen::CosmicEnding && !cosmicEndingFrameReady053) return false;
        return dumpFrame(executable,(std::string("053-")+label+".565").c_str());
    };
    const auto start=[&](unsigned courseIndex,const char* map,const char* field) {
        openLevelSelect(flow,courseIndex);
        if (!capture(map,1000) || levelSelect.world()!=6 || menuCourse!=courseIndex ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,1100) || !selectorGoDown ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,1200) || selectorGoDown) return false;
        flow.update(0,true,false,false,false);
        return flow.screen==Screen::Hole && enterCourse(executable,courseIndex,course,round) && capture(field);
    };
    const auto locale=[&](unsigned index,const char* ruMap,const char* ruField,const char* enMap,const char* enField) {
        return start(index,ruMap,ruField) &&
               changeLanguage(executable,course,config,Language::English) &&
               start(index,enMap,enField) &&
               changeLanguage(executable,course,config,Language::Russian) &&
               enterCourse(executable,index,course,round);
    };
    const auto trace=[&](const char* label) {
        char event[140];
        std::snprintf(event,sizeof event,"%s tick=%u vx=%.4f vy=%.4f keys=%u doors=%u switch=%u trap=%u",
                      label,round.ticks,round.game.velocity.x,round.game.velocity.y,
                      unsigned(round.roundKeys.collected),unsigned(round.roundKeys.opened),
                      round.switches.state,round.lastHurrah.revealed);
        logEvent(log,event,round.game);
    };
    const auto play=[&](const auto& route,unsigned limit,const auto& frame) {
        unsigned i=0;
        while (round.ticks<limit && !round.game.won && !round.caught) {
            if (i<std::size(route) && round.ticks==route[i].tick) {
                const auto& shot=route[i++];
                if (currentCourse>=CosmicSixteenthCourse) trace("trace_shot053");
                round.updateControls({shot.x/127.0f,shot.y/127.0f},true,false);
                if (round.updateControls({},false,false)!=ControlEvent::Shot) return false;
                round.coins.multiplier=0;
            }
            round.step();
            if (!frame(round.ticks)) return false;
        }
        return round.game.won && !round.caught && i==std::size(route);
    };
    const auto always=[](unsigned) { return true; };
    flow.screen=Screen::Hole;
    if (!enterCourse(executable,CosmicFifteenthCourse,course,round) ||
        !play(CosmicPspRoute615,1500,[&](unsigned tick){ return tick!=706 || capture("p15-delta"); }) ||
        round.ticks!=CosmicPspWinTick615 || round.game.bounces!=CosmicPspWinBounces615 ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicSixteenthCourse) return fail("test_053_p15_advance");

    if (!locale(CosmicSixteenthCourse,"p16-map-ru","p16-field-ru","p16-map-en","p16-field-en") ||
        course.par!=20 || walls.onTime!=30 || round.extraGhostCount!=5 || round.jumpStage.count!=2 ||
        objects.data.courses[currentCourse].count!=23) return fail("test_053_p16_setup");
    const float initial=lateWalls053.sweep({32,0},{256,0},15.8f).time;
    lateWalls053.setRotation(500);
    const float rotated=lateWalls053.sweep({32,0},{256,0},15.8f).time;
    lateWalls053.setRotation(0);
    if (initial>0.002f || rotated<0.07f || rotated>0.09f) return fail("test_053_p16_rotation");
    if (!play(Route616,1500,[&](unsigned tick) {
            if (tick==125) return capture("p16-jump");
            if (tick==500) return capture("p16-rotation");
            return true;
        }) || !capture("p16-win",0,4700)) return fail("test_053_p16_route");
    logEvent(log,"test_053_p16_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicSeventeenthCourse) return fail("test_053_p16_advance");

    if (!locale(CosmicSeventeenthCourse,"p17-map-ru","p17-field-ru","p17-map-en","p17-field-en") ||
        course.par!=30 || walls.onTime!=90 || round.extraGhostCount || round.jumpStage.count!=1 ||
        objects.data.courses[currentCourse].count!=79 || !round.switchStage) return fail("test_053_p17_setup");
    round.game.position={670,0};
    round.step();
    if (!round.caught || !round.fellOut || !capture("p17-oob",0,100)) return fail("test_053_p17_bounds");
    if (!enterCourse(executable,CosmicSeventeenthCourse,course,round)) return fail("test_053_p17_retry");
    round.dog.body.center=CosmicDog617.goal;
    round.dog.velocity={};
    round.game.position=CosmicNukes617[0];
    round.step();
    if (!round.dog.winner || !lateWalls053.dogOpen || !round.nukes.has(0) ||
        !capture("p17-dog-nuke")) return fail("test_053_p17_dog");
    round.game.position=CosmicFakeGoal617;
    round.step();
    if (!round.lastHurrah.revealed || !round.lastHurrah.hunterActive || !lateWalls053.trapOpen ||
        round.fakeGoals.bonus!=500 || !capture("p17-trap")) return fail("test_053_p17_trap");
    round.switches.strike(0);
    if (round.switches.state!=1 || !capture("p17-switch")) return fail("test_053_p17_switch");
    if (!enterCourse(executable,CosmicSeventeenthCourse,course,round)) return fail("test_053_p17_ring_retry");
    round.game.position=CosmicRing617.center;
    round.step();
    if (round.smiley.phase!=SmileyPhase::Active || !capture("p17-ring")) return fail("test_053_p17_ring");
    for (unsigned i=0;i<CosmicRing617.coinCount;++i) {
        round.game.position=CosmicRing617.coins[i];
        round.step();
    }
    if (round.smiley.phase!=SmileyPhase::Passed || round.smiley.bonus!=5000 || !capture("p17-ring-win"))
        return fail("test_053_p17_ring_pass");
    if (!enterCourse(executable,CosmicSeventeenthCourse,course,round) ||
        !(play(Route617Backup,2400,always) ||
          (enterCourse(executable,CosmicSeventeenthCourse,course,round) && play(Route617,2400,always)) ||
          (enterCourse(executable,CosmicSeventeenthCourse,course,round) && play(Route617Backup2,2400,always))) ||
        !capture("p17-win",0,4700))
        return fail("test_053_p17_route");
    logEvent(log,"test_053_p17_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicEighteenthCourse) return fail("test_053_p17_advance");

    if (!locale(CosmicEighteenthCourse,"p18-map-ru","p18-field-ru","p18-map-en","p18-field-en") ||
        course.par!=2 || walls.onTime!=5 || !cannotSkipCourse(currentCourse) ||
        advanceCampaign(executable,course,round,save,log,true)!=Advance::Failed ||
        !play(Route618,500,always) || !capture("p18-win",0,4700))
        return fail("test_053_p18_route");
    logEvent(log,"test_053_p18_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::CosmicEnding ||
        progress.next!=CourseCount) return fail("test_053_p18_advance");
    flow.screen=Screen::CosmicEnding;
    for (unsigned i=0;i<24;++i) updateCosmicEnding053(250,0,0);
    if (cosmicEnding053.results() || !capture("ending-intro-ru")) return fail("test_053_ending_intro");
    updateCosmicEnding053(16,PSP_CTRL_START,0);
    if (!cosmicEnding053.results() || cosmicEnding053.countVisible() || !capture("ending-results-reveal"))
        return fail("test_053_ending_reveal");
    for (unsigned i=0;i<9;++i) updateCosmicEnding053(250,0,0);
    if (!cosmicEnding053.countVisible() || !capture("ending-results-ru") ||
        !updateCosmicEnding053(16,PSP_CTRL_CROSS,0)) return fail("test_053_ending_count");
    resetCosmicEnding053();

    if (!changeLanguage(executable,course,config,Language::English)) return fail("test_053_ending_en");
    startCosmicEnding053(true);
    for (unsigned i=0;i<23;++i) updateCosmicEnding053(250,0,PSP_CTRL_TRIANGLE);
    for (unsigned i=0;i<9;++i) updateCosmicEnding053(250,0,0);
    if (!cosmicEnding053.countVisible() || !capture("ending-results-en")) return fail("test_053_ending_en_count");
    drawCosmicEnding053(565);
    if (!cosmicEndingFrameReady053 || !dumpFrame(executable,"053-ending-max-en.565")) return fail("test_053_ending_565");
    resetCosmicEnding053();
    if (!changeLanguage(executable,course,config,Language::Russian)) return fail("test_053_restore_ru");
    openLevelSelect(flow,CosmicEighteenthCourse);
    if (!capture("ending-return-map",1000) || menuCourse!=CosmicEighteenthCourse)
        return fail("test_053_ending_return");
    logEvent(log,"test_053_ending_pass",round.game);

    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || saved.cosmicKey!=seed.cosmicKey ||
        saved.frontierKey!=seed.frontierKey || saved.elysianKey!=seed.elysianKey)
        return fail("test_053_save");
    for (unsigned i=0;i<CosmicSixteenthCourse;++i) {
        const auto& before=seed.best[i];
        const auto& after=saved.best[i];
        if (before.score!=after.score || before.rank!=after.rank || before.time!=after.time)
            return fail("test_053_previous_scores");
    }
    for (unsigned i=CosmicSixteenthCourse;i<CourseCount;++i)
        if (!saved.best[i].rank) return fail("test_053_new_scores");
    return true;
}
