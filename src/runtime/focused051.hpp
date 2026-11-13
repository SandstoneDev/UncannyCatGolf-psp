#pragma once

#include "route_steps050.hpp"
#include "route_steps051.hpp"

bool runFocused051(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        char context[96];
        std::snprintf(context,sizeof context,"%s tick=%u state=%u caught=%u",event,round.ticks,
                      round.switches.state,static_cast<unsigned>(round.caught));
        logEvent(log,context,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=CosmicTwelfthCourse || progress.cosmicKey ||
        progress.totalScore()!=1168822 || progress.totalPrisms()!=496 ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_051_seed");
    const Progress seed=progress;
    round.totalTicks=progress.ticks;
    round.rhythmTestClock=true;
    const auto capture=[&](const char* phase,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && !liveResultPrepared) return false;
        if (!result && !round.caught && flow.screen==Screen::Hole) {
            if (currentCourse==CosmicTwelfthCourse &&
                (cosmicFieldSlot046.id!=round.switches.state || cosmicLightSlot612.id!=round.switches.state ||
                 !cosmicFlagSlot046.owner || !cosmicLightSlot612.owner)) return false;
            if (currentCourse==CosmicThirteenthCourse &&
                (!cosmicGlassSlot613.owner || !cosmicPlantsSlot613.owner || !cosmicMovingGoal613.texture())) return false;
        }
        return dumpFrame(executable,(std::string("051-")+phase+".565").c_str());
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
    const auto parkGhosts=[&]() {
        round.ghost.position={0,-6000};
        for (unsigned i=0;i<round.extraGhostCount;++i)
            round.extraGhosts[i].position={float(i*80+80),-6000};
    };

    flow.screen=Screen::Hole;
    if (!enterCourse(executable,CosmicEleventhCourse,course,round) ||
        !play(CosmicRoute611,1300,[&](unsigned tick) { return tick!=590 || capture("p11-delta"); }) ||
        round.ticks!=CosmicWinTick611 || round.game.bounces!=CosmicWinBounces611 ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicTwelfthCourse) return fail("test_051_p11_advance");
    logEvent(log,"test_051_050_delta",round.game);

    if (!start(CosmicTwelfthCourse,"p12-map-ru","p12-field-ru") ||
        course.par!=12 || walls.onTime!=25 || course.right!=384 || round.extraGhostCount!=1 ||
        round.jumpStage.count!=11 || !round.switchStage || round.switchStage->blockCount!=10 ||
        round.switchStage->layerCount!=4) return fail("test_051_p12_setup");
    round.game.position={course.right+25,0};
    round.step();
    if (!round.caught || !round.fellOut || round.backrooms || !capture("p12-oob",0,100))
        return fail("test_051_p12_oob");
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicTwelfthCourse,"p12-map-en","p12-field-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicTwelfthCourse,course,round)) return fail("test_051_p12_en");

    unsigned state=0,jumps=0;
    bool airborne=false;
    if (!play(CosmicRoute612,1500,[&](unsigned tick) {
            if (round.switches.state!=state) {
                state=round.switches.state;
                if (!state || state>std::size(CosmicSwitchTicks612) || tick!=CosmicSwitchTicks612[state-1]) return false;
                char label[24];
                std::snprintf(label,sizeof label,"p12-switch-%u",state);
                if (!capture(label)) return false;
            }
            const bool next=round.jump.airborne();
            if (next && !airborne && ++jumps==1 &&
                (tick!=CosmicFirstJumpTick612 || !capture("p12-jump"))) return false;
            airborne=next;
            return true;
        }) || round.ticks!=CosmicWinTick612 || round.game.bounces!=CosmicWinBounces612 ||
        state!=3 || jumps!=CosmicJumpCount612 || !capture("p12-win",0,4700))
        return fail("test_051_p12_route");
    logEvent(log,"test_051_p12_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicThirteenthCourse || progress.next!=CosmicThirteenthCourse)
        return fail("test_051_p12_advance");

    if (!start(CosmicThirteenthCourse,"p13-map-ru","p13-field-ru") ||
        course.par!=5 || walls.onTime!=17 || course.right!=512 || round.extraGhostCount ||
        length(round.movingGoal.position()-Vec2{-128,448})>0.01f)
        return fail("test_051_p13_setup");
    round.game.position={course.right+25,0};
    round.step();
    if (!round.caught || !round.backrooms || round.fellOut || !capture("p13-backrooms"))
        return fail("test_051_p13_bounds");
    logEvent(log,"test_051_p13_bounds",round.game);
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicThirteenthCourse,"p13-map-en","p13-field-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicThirteenthCourse,course,round)) return fail("test_051_p13_en");

    parkGhosts();
    while (round.ticks<MovingGoal051::LastTick && !round.caught && !round.game.won) round.step();
    if (round.caught || round.game.won || round.movingGoal.face!=MovingGoal051::Face::Laugh ||
        length(round.movingGoal.position()-Vec2{-207.65346f,321.16217f})>0.02f ||
        !capture("p13-laugh")) return fail("test_051_p13_laugh");
    if (!enterCourse(executable,CosmicThirteenthCourse,course,round)) return fail("test_051_p13_key_retry");
    const auto previousQuest=progress.quest;
    progress.quest=std::max<uint32_t>(previousQuest,2);
    for (unsigned i=0;i<60;++i) round.step();
    round.game.position=CosmicKeyTrigger613.center;
    round.step();
    if (!progress.cosmicKey || !round.questKeyTick || round.movingGoal.face!=MovingGoal051::Face::Cry)
        return fail("test_051_p13_key");
    const auto frozen=round.movingGoal.position();
    round.step();
    if (length(frozen-round.movingGoal.position())>0 || !capture("p13-key"))
        return fail("test_051_p13_key_freeze");
    progress.quest=previousQuest;
    persistProgress(save,round,log);
    Progress keyed{};
    if (!save.read(keyed) || !keyed.cosmicKey) return fail("test_051_p13_key_save");
    logEvent(log,"test_051_p13_key_freeze",round.game);
    if (!enterCourse(executable,CosmicThirteenthCourse,course,round)) return fail("test_051_p13_retry");
    if (!play(CosmicRoute613,400,[&](unsigned tick) { return tick!=60 || capture("p13-moving"); }) ||
        round.ticks!=CosmicWinTick613 || round.game.bounces!=CosmicWinBounces613 ||
        round.movingGoal.face!=MovingGoal051::Face::Cry || !capture("p13-win",0,4700))
        return fail("test_051_p13_route");
    logEvent(log,"test_051_p13_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild)
        return fail("test_051_p13_advance");

    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.cosmicKey ||
        saved.elysianKey!=seed.elysianKey || saved.frontierKey!=seed.frontierKey ||
        !saved.best[CosmicTwelfthCourse].rank || !saved.best[CosmicThirteenthCourse].rank)
        return fail("test_051_save");
    for (unsigned i=0;i<CosmicTwelfthCourse;++i) {
        const auto& before=seed.best[i];
        const auto& after=saved.best[i];
        if (before.score!=after.score || before.rank!=after.rank || before.time!=after.time)
            return fail("test_051_previous_scores");
    }
    return true;
}
