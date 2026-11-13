struct RouteShot028 {
    unsigned wait;
    int x,y;
};

struct Route028 {
    const RouteShot028* shots;
    unsigned count;
};

#include "routes028.hpp"

bool runFocused028(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,
                   const GiftFile& giftFile,const char* log) {
    Flow flow;
    Round round(course);
    if (!save.read(progress) || progress.next!=TowerCourse ||
        !changeLanguage(executable,course,config,Language::Russian)) return false;
    logEvent(log,"test_028_loaded_027",round.game);
    const auto capture=[&](const char* suffix,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        char name[64];
        std::snprintf(name,sizeof name,"%s-%s-%s.565",russian()?"ru":"en",CourseIds[menuCourse],suffix);
        return dumpFrame(executable,name);
    };
    for (unsigned index=TowerCourse;index<=GlowstickFinaleCourse;++index) {
        openLevelSelect(flow);
        const unsigned cell=index-GlowstickFirstCourse;
        if (menuCourse!=index || levelSelect.world()!=2 || levelSelect.cell()!=cell ||
            !levelSelect.unlocked(2,cell) ||
            (index<GlowstickFinaleCourse && levelSelect.unlocked(2,cell+1)) ||
            !capture("map",3000) ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) ||
            !selectorGoDown || !capture("go",3000) ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090) || selectorGoDown) return false;
        flow.update(0,true,false,false,false);
        if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
            currentCourse!=index) return false;
        logEvent(log,"test_028_map_cross",round.game);
        if (!capture("hole")) return false;
        for (unsigned i=0;i<5 && !round.caught;++i) round.step();
        if (round.caught || !capture("flag")) return false;
        if (index==TowerCourse) {
            if (round.extraGhostCount!=3 || !round.jumpStage || round.jumpStage->count!=8) return false;
            const auto center0=towerPadCenter(3,0,round.jumpStage->pads[3].center);
            const auto center1=towerPadCenter(3,45,round.jumpStage->pads[3].center);
            if (dot(center1-center0,center1-center0)<1.0f) return false;
        }
        if (index==GlowstickFinaleCourse &&
            (!round.baddies.stage || round.baddies.stage->skull || round.baddies.stage->spawnerCount!=3))
            return false;
        while (round.ticks<4000 && !round.caught) round.step();
        if (!round.caught || !capture("loss",0,1000)) return false;
        logEvent(log,"test_028_loss",round.game);
        round.restart();
        if (!changeLanguage(executable,course,config,Language::English) || !capture("hole") ||
            !changeLanguage(executable,course,config,Language::Russian)) return false;
        const auto playRoute=[&](const Route028& route,bool captureMechanics) {
            for (unsigned shot=0;shot<route.count;++shot) {
                const auto action=route.shots[shot];
                for (unsigned wait=0;wait<action.wait && !round.caught;++wait) round.step();
                const Vec2 aim{action.x/127.0f,action.y/127.0f};
                round.controls.update(round.game,course,aim,true,false);
                if (round.caught ||
                    round.controls.update(round.game,course,{},false,false)!=ControlEvent::Shot) return false;
                round.coins.multiplier=0;
                logEvent(log,"test_028_route_shot",round.game);
                bool captured=false;
                for (unsigned tick=0;tick<500 && (round.game.moving() || round.jump.airborne()) &&
                                       !round.caught && !round.game.won;++tick) {
                    round.step();
                    if (captureMechanics && !captured &&
                        (round.jump.airborne() || (index==CasinoCourse && round.stopRun.collected))) {
                        char suffix[24];
                        std::snprintf(suffix,sizeof suffix,"mechanic-%u",shot+1);
                        if (!capture(suffix)) return false;
                        captured=true;
                    }
                }
                if (round.caught || round.game.moving() || round.jump.airborne()) {
                    logEvent(log,"test_028_route_failed",round.game);
                    return false;
                }
            }
            return true;
        };
        if (index==CasinoCourse) {
            if (!playRoute({skullRoute,std::size(skullRoute)},false) ||
                round.slots.outcome!=SlotOutcome::Skull || round.slots.bonus!=2000 ||
                round.slots.outcomeTick!=440 || round.baddies.stage!=&baddieStages[SlotBaddieStage] ||
                round.baddies.count!=24 || !round.roundKeys.collected || !capture("slots-skull")) return false;
            logEvent(log,"test_028_triple_skull_24_spawns",round.game);
            round.restart();
            if (round.slots.stopped || round.slots.bonus || round.baddies.stage) return false;
        }
        if (!playRoute(routes028[index-TowerCourse],true) || !round.game.won) return false;
        if (index==CasinoCourse) {
            if (round.slots.outcome!=SlotOutcome::Good || round.slots.bonus!=3000 ||
                round.slots.frames[0]!=2 || round.slots.frames[1]!=5 || round.slots.frames[2]!=6 ||
                round.stopRun.collected!=7 || round.roundKeys.opened!=1) return false;
            logEvent(log,"test_028_slots_good_key_door",round.game);
        }
        if (index==GlowstickFinaleCourse) {
            if (round.baddies.count<3 || !round.baddies.defeatedCount) return false;
            logEvent(log,"test_028_finale_car_hit",round.game);
        }
        if (!capture("win",0,4700)) return false;
        logEvent(log,"test_028_goal",round.game);
        const auto outcome=advanceCampaign(executable,course,round,save,log);
        const auto expected=index==GlowstickFinaleCourse?Advance::WorldCleared:Advance::NextHole;
        if (outcome!=expected || progress.next!=index+1) return false;
        Progress saved{};
        if (!save.read(saved) || saved.next!=index+1 || saved.best[index].time==UINT32_MAX) return false;
        const uint32_t pending=gifts.pending();
        gifts.queue(giftFile,progress);
        if (index==TowerCourse) {
            if (saved.best[index].rank!=5 || !(gifts.pending()&giftBit(GiftReward::Parry))) return false;
            logEvent(log,"test_028_parry_gift_saved",round.game);
        }
        if (index==GlowstickFinaleCourse &&
            (!(gifts.pending()&giftBit(GiftReward::UncannyDash)) ||
              ((pending&giftBit(GiftReward::UncannyDash))!=0))) return false;
    }
    GiftSequence reloaded;
    reloaded.load(giftFile);
    if (!(reloaded.pending()&giftBit(GiftReward::UncannyDash))) return false;
    logEvent(log,"test_028_dash_gift_saved",round.game);
    openLevelSelect(flow,GlowstickFinaleCourse);
    for (unsigned n=0;n<GiftCount && gifts.pending();++n) {
        const uint64_t now=10000000ull+3000000ull*n;
        if (!gifts.update(giftFile,now,false) || !gifts.update(giftFile,now+2050000,false)) return false;
        if (gifts.reward==static_cast<unsigned>(GiftReward::UncannyDash) && !capture("gift-dash",3000)) return false;
        if (gifts.reward==static_cast<unsigned>(GiftReward::Parry) && !capture("gift-parry",3000)) return false;
        if (!gifts.update(giftFile,now+2600000,true)) return false;
    }
    reloaded.load(giftFile);
    if (!reloaded.opened(GiftReward::Parry) || !reloaded.opened(GiftReward::UncannyDash)) return false;
    logEvent(log,"test_028_gifts_opened_and_reloaded",round.game);
    flow.screen=Screen::WorldResults;
    if (!worldResults.firstUnlock() || worldResults.world()!=CompletedWorld::GlowstickCity ||
        !worldResults.nextAvailable() || !capture("world-first-unlock",3700) ||
        !capture("world-first",WorldSkipTimeMs)) return false;
    const auto first=worldResults.update(3550,false,false,false,false,false);
    if (!(first.cues&cueMask(WorldCue::Unlock))) return false;
    const auto next=worldResults.update(WorldSkipTimeMs,false,false,false,true,false);
    if (next.choice!=WorldResultsChoice::NextWorld) return false;
    leaveWorldResults(flow,true);
    if (flow.screen!=Screen::Hole || menuCourse!=ChuckleParkFirstCourse) return false;

    if (advanceCampaign(executable,course,round,save,log)!=Advance::WorldCleared ||
        worldResults.firstUnlock()) return false;
    flow.screen=Screen::WorldResults;
    const auto replay=worldResults.update(3550,false,false,false,false,false);
    if ((replay.cues&cueMask(WorldCue::Unlock)) || !capture("world-replay",3700)) return false;
    const auto back=worldResults.update(WorldSkipTimeMs,false,true,false,true,false);
    if (back.choice!=WorldResultsChoice::LevelSelect || !capture("world-replay-map",WorldSkipTimeMs)) return false;
    leaveWorldResults(flow,false);
    if (flow.screen!=Screen::LevelSelect || menuCourse!=GlowstickFinaleCourse) return false;
    logEvent(log,"test_028_w2_first_and_replay",round.game);

    const Progress afterWin=progress;
    progress.next=GlowstickFinaleCourse;
    progress.best[GlowstickFinaleCourse]={};
    SaveFile skipSave(companion(executable,"progress-rule-test.sav"));
    const GiftFile skipGiftFile(companion(executable,"gifts-rule-test.cfg"));
    GiftSequence skipGifts;
    skipGifts.queue(skipGiftFile,progress);
    if (skipGifts.pending()&giftBit(GiftReward::UncannyDash)) return false;
    round.restart();
    if (advanceCampaign(executable,course,round,skipSave,log,true)!=Advance::WorldCleared ||
        progress.next!=ChuckleFirstCourse || progress.best[GlowstickFinaleCourse].time!=UINT32_MAX ||
        !skipGifts.queue(skipGiftFile,progress) ||
        !(skipGifts.pending()&giftBit(GiftReward::UncannyDash))) return false;
    logEvent(log,"test_028_dash_awarded_on_skip",round.game);
    if (skipGifts.pending()&giftBit(GiftReward::Retro)) return false;
    for (unsigned i=GlowstickFirstCourse;i<ChuckleFirstCourse;++i) progress.best[i].rank=5;
    if (!skipGifts.queue(skipGiftFile,progress) || !(skipGifts.pending()&giftBit(GiftReward::Retro))) return false;
    logEvent(log,"test_028_retro_all_18_peak",round.game);
    progress=afterWin;
    openLevelSelect(flow,GlowstickFinaleCourse);

    updateLevelSelector(PSP_CTRL_RTRIGGER,0,0,1000);
    if (levelSelect.world()!=3 || menuCourse!=ChuckleParkFirstCourse || !selectorTravelling(1200) ||
        !levelSelect.unlocked(3,0) || levelSelect.unlocked(3,1) ||
        !capture("map-from-city-04",1134) || !capture("map-from-city-10",1334) ||
        !capture("map-from-city-18",1600) || !capture("map",1959)) return false;
    updateLevelSelector(PSP_CTRL_LTRIGGER,0,0,2000);
    if (levelSelect.world()!=2 || menuCourse!=GlowstickFinaleCourse || !selectorTravelling(2200) ||
        !capture("map-from-park-04",2134) || !capture("map-from-park-10",2334) ||
        !capture("map-from-park-18",2600)) return false;
    logEvent(log,"test_028_map_w2_w3",round.game);
    updateLevelSelector(PSP_CTRL_RTRIGGER,0,0,3000);
    if (levelSelect.world()!=3 || !capture("map-return",4000) ||
        updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,4010) ||
        !selectorGoDown || !capture("go",4010) ||
        !updateLevelSelector(0,PSP_CTRL_CROSS,0,4090) || selectorGoDown) return false;
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
        currentCourse!=ChuckleParkFirstCourse || !capture("hole") ||
        !changeLanguage(executable,course,config,Language::English) || !capture("hole") ||
        !changeLanguage(executable,course,config,Language::Russian)) return false;

    constexpr Vec2 parkShots[]={{-1.0f,-22/127.0f},{-85/127.0f,-120/127.0f}};
    for (const auto aim:parkShots) {
        round.controls.update(round.game,course,aim,true,false);
        if (round.controls.update(round.game,course,{},false,false)!=ControlEvent::Shot) return false;
        for (unsigned tick=0;tick<500 && round.game.moving() && !round.game.won && !round.caught;++tick) {
            round.step();
            if (round.ticks==70 && !capture("ring-active")) return false;
        }
        if (round.caught || (round.game.moving() && !round.game.won)) return false;
    }
    if (!round.game.won || round.smiley.bonus!=2000 || !capture("win",0,4700)) return false;
    for (unsigned i=0;i<8;++i) if (!round.smiley.collected(i)) return false;
    logEvent(log,"test_028_park_ring_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild) return false;
    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || saved.best[ChuckleParkFirstCourse].time==UINT32_MAX)
        return false;
    logEvent(log,"test_028_save42",round.game);
    return true;
}
