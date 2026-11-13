#pragma once

#include "route_steps049.hpp"
#include "route_steps050.hpp"

bool runFocused050(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=CosmicTenthCourse ||
        progress.totalScore()!=1146655 || progress.totalPrisms()!=487 || !progress.frontierKey ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_050_seed");
    const auto trace=[&](const char* event) {
        char data[152];
        std::snprintf(data,sizeof data,"%s tick=%u key=%u door=%u mode=%u v=%.3f,%.3f cars=%u skulls=%u",
            event,round.ticks,unsigned(round.roundKeys.collected),unsigned(round.roundKeys.opened),round.polyrhythm.mask,
            round.game.velocity.x,round.game.velocity.y,round.baddies.defeatedCount,round.cosmicSkulls.defeatedCount);
        logEvent(log,data,round.game);
    };
    const Progress seed=progress;
    round.totalTicks=progress.ticks;
    round.rhythmTestClock=true;
    const auto capture=[&](const char* phase,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && !liveResultPrepared) return false;
        if (!result && !round.caught && flow.screen==Screen::Hole) {
            const unsigned mask=round.polyrhythm.mask;
            const auto& art=currentCourse==CosmicNinthCourse?cosmicGloss049[1]:
                cosmicGloss050[currentCourse==CosmicTenthCourse?0:1+mask/4];
            const unsigned frame=mask%4*CosmicGloss048Phases+cosmicGlossPhase048(round.ticks);
            if (cosmicFieldSlot046.owner!=&art || cosmicFieldSlot046.id!=frame || !cosmicFlagSlot046.owner) return false;
            if (currentCourse==CosmicEleventhCourse && cosmicWaterSlot048.owner!=&cosmicWater611) return false;
        }
        return dumpFrame(executable,(std::string("050-")+phase+".565").c_str());
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
    if (!enterCourse(executable,CosmicNinthCourse,course,round) ||
        !play(CosmicRoute609,1100,[&](unsigned tick) { return tick!=160 || capture("p9-delta"); }) ||
        round.ticks!=CosmicWinTick609 || round.game.bounces!=CosmicWinBounces609 ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicTenthCourse) return fail("test_050_p9_advance");
    logEvent(log,"test_050_049_delta",round.game);

    if (!start(CosmicTenthCourse,"p10-map-ru","p10-both-ru") ||
        course.par!=5 || course.right!=192 || walls.onTime!=20 || round.extraGhostCount ||
        round.keyStage!=&CosmicKeys610 || round.baddies.stage!=&CosmicCars610 ||
        round.cosmicSkulls.stage!=&CosmicSkulls610 || round.polyrhythm.mask!=3 || !cosmicDoor610.texture())
        return fail("test_050_p10_setup");
    round.game.position={course.right+25,0};
    round.step();
    if (!round.caught || !round.fellOut || round.backrooms || !capture("p10-oob",0,100))
        return fail("test_050_p10_oob");
    logEvent(log,"test_050_p10_oob",round.game);
    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicTenthCourse,"p10-map-en","p10-both-en") ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicTenthCourse,course,round)) return fail("test_050_p10_en");
    if (!play(CosmicPspRoute610,750,[&](unsigned tick) {
            if (tick==80 || tick==107 || tick==168 || tick==204 || tick==212 || tick==240 || tick==267 || tick==301 || tick==348) trace("trace_p10");
            if (tick==CosmicNukeTick610) {
                if (!round.nukes.has(0) || round.baddies.count || round.cosmicSkulls.count ||
                    !capture("p10-nuke")) return false;
                logEvent(log,"test_050_p10_nuke",round.game);
            }
            if (tick==50 && (!round.baddies.count || !round.cosmicSkulls.count)) return false;
            if (tick==80 && (round.polyrhythm.mask!=1 || !capture("p10-pink"))) return false;
            if (tick==107 && (round.polyrhythm.mask || round.rhythmWalls.activeTiles() ||
                             !capture("p10-neither"))) return false;

            if (tick==CosmicPspKeyTick610 && !round.roundKeys.collected) return false;
            if (tick==CosmicPspDoorTick610) {
                if (!round.roundKeys.opened || round.roundKeys.bonus!=CosmicDoorBonus610 ||
                    !capture("p10-door")) return false;
                logEvent(log,"test_050_p10_door",round.game);
            }
            return true;
        }) || round.ticks!=CosmicPspWinTick610 || round.game.bounces!=CosmicPspWinBounces610 ||
        !capture("p10-win",0,4700)) { trace("trace_p10_end"); return fail("test_050_p10_route"); }
    logEvent(log,"test_050_p10_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CosmicEleventhCourse || progress.next!=CosmicEleventhCourse)
        return fail("test_050_p10_advance");

    if (!start(CosmicEleventhCourse,"p11-map-ru","p11-all-ru") || course.par!=6 ||
        walls.onTime!=25 || course.right!=384 || round.extraGhostCount!=4 ||
        round.polyrhythm.mask!=7 || !round.jumpStage || !cosmicPortalArt611.texture())
        return fail("test_050_p11_setup");
    round.game.position={course.right+25,0};
    round.step();
    if (!round.caught || round.fellOut || !round.backrooms || !capture("p11-backrooms"))
        return fail("test_050_p11_backrooms");
    if (!enterCourse(executable,CosmicEleventhCourse,course,round)) return fail("test_050_p11_bounds_retry");
    round.game.position={course.right+25,0};
    round.jump.timer=1;
    round.step();
    if (!round.caught || !round.fellOut || round.backrooms || !capture("p11-oob",0,100))
        return fail("test_050_p11_oob");
    logEvent(log,"test_050_p11_bounds",round.game);

    if (!changeLanguage(executable,course,config,Language::English) ||
        !start(CosmicEleventhCourse,"p11-map-en","p11-all-en")) return fail("test_050_p11_en");
    round.game.position=CosmicPortalNodes611[1].position;
    round.step();
    if (length(round.game.position-CosmicPortalNodes611[0].position)>0.01f || round.jump.airborne())
        return fail("test_050_p11_reverse_portal");
    if (!changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CosmicEleventhCourse,course,round)) return fail("test_050_p11_route_setup");
    unsigned seen=0;
    if (!play(CosmicRoute611,1400,[&](unsigned tick) {
            if (tick==61 || tick==339 || tick==381 || tick==489 || tick==560 || tick==590 || tick==798 || tick==884 || tick==905) trace("trace_p11");
            const unsigned mask=round.polyrhythm.mask;
            if (!(seen&(1u<<mask))) {
                seen|=1u<<mask;
                char phase[32];
                std::snprintf(phase,sizeof phase,"p11-mask-%u",mask);
                if (!capture(phase)) return false;
            }
            if (tick==CosmicRingStart611 && round.smiley.phase!=SmileyPhase::Active) return false;
            if (tick==CosmicPortalTick611) {
                if (!round.jump.airborne() || length(round.game.position-CosmicJump611[0].center)>0.01f ||
                    !capture("p11-portal-jump")) return false;
                logEvent(log,"test_050_p11_portal_jump",round.game);
            }
            if (tick==CosmicRingPass611) {
                if (round.smiley.phase!=SmileyPhase::Passed || round.smiley.bonus!=CosmicRingBonus611 ||
                    !capture("p11-ring")) return false;
                logEvent(log,"test_050_p11_ring",round.game);
            }
            return true;
        }) || round.ticks!=CosmicWinTick611 || round.game.bounces!=CosmicWinBounces611 ||
        !capture("p11-win",0,4700)) { trace("trace_p11_end"); return fail("test_050_p11_route"); }
    logEvent(log,"test_050_p11_win",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild)
        return fail("test_050_p11_advance");

    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.frontierKey ||
        saved.elysianKey!=seed.elysianKey || !saved.best[CosmicTenthCourse].rank ||
        !saved.best[CosmicEleventhCourse].rank) return fail("test_050_save");
    for (unsigned i=0;i<CosmicTenthCourse;++i) {
        const auto& before=seed.best[i];
        const auto& after=saved.best[i];
        if (before.score!=after.score || before.rank!=after.rank || before.time!=after.time)
            return fail("test_050_previous_scores");
    }
    return true;
}
