struct ParkShot029 { unsigned wait; int x,y; };
struct ParkRoute029 { const ParkShot029* shots; unsigned count; };

#include "routes029.hpp"

bool runFocused029(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    if (!save.read(progress) || progress.next!=CarouselCourse ||
        !changeLanguage(executable,course,config,Language::Russian)) return false;
    logEvent(log,"test_029_loaded_028",round.game);
    const unsigned initialAudio=audio.blocks();
    audio.play(Sound::ParkSwitch);
    for (unsigned tick=0;tick<120 && audio.blocks()-initialAudio<8;++tick) sceDisplayWaitVblankStart();
    if (audio.blocks()-initialAudio<8) return false;
    logEvent(log,"test_029_switch_pcm",round.game);

    if (!enterCourse(executable,ChuckleParkFirstCourse,course,round) ||
        parkArt.data.balloonCount!=2 || parkArt.data.smiley.coinCount!=8 ||
        parkArt.data.ufoSpeed<=0 || !parkArt.pixels()) return false;
    logEvent(log,"test_029_legacy_park_stage",round.game);

    const auto capture=[&](const char* suffix,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        char name[64];
        std::snprintf(name,sizeof name,"%s-%s-%s.565",russian()?"ru":"en",CourseIds[menuCourse],suffix);
        return dumpFrame(executable,name);
    };

    for (unsigned index=CarouselCourse;index<=CoasterCourse;++index) {
        openLevelSelect(flow);
        const unsigned cell=index-ChuckleParkFirstCourse;
        if (menuCourse!=index || levelSelect.world()!=3 || levelSelect.cell()!=cell ||
            !levelSelect.unlocked(3,cell) || levelSelect.unlocked(3,cell+1) || !capture("map",3000) ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) ||
            !selectorGoDown || !capture("go",3000) ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090) || selectorGoDown) return false;
        flow.update(0,true,false,false,false);
        if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
            currentCourse!=index || parkArt.data.balloonCount!=(index==JumbleCourse?0u:3u) || !capture("hole")) return false;
        logEvent(log,"test_029_map_cross",round.game);
        for (unsigned tick=0;tick<5 && !round.caught;++tick) round.step();
        if (round.caught || !capture("flag")) return false;
        while (round.ticks<1800 && !round.caught) round.step();
        if (!round.caught || !capture("loss",0,1000)) return false;
        logEvent(log,"test_029_loss",round.game);

        round.restart();
        if (!changeLanguage(executable,course,config,Language::English) || !capture("hole") ||
            !changeLanguage(executable,course,config,Language::Russian)) return false;

        if (index==CarouselCourse) {
            const auto& ring=parkArt.data.smiley;
            if (ring.timeTicks!=600 || ring.coinCount!=9 || parkArt.data.ufoSpeed ||
                !round.stopStage || round.stopStage->count!=1 || courseHazards(index).blockCount!=2 ||
                !round.baddies.stage || round.baddies.stage->spawnerCount!=4) return false;
            chillMode=true;
            round.game.position=ring.center;
            round.step();
            if (round.smiley.phase!=SmileyPhase::Active || !capture("ring-active")) return false;
            for (unsigned i=0;i<ring.coinCount;++i) {
                round.game.position=ring.coins[i];
                round.step();
            }
            if (round.smiley.phase!=SmileyPhase::Passed || round.smiley.bonus!=2000 || !capture("ring-pass"))
                return false;
            logEvent(log,"test_029_nine_ring_coins",round.game);
            round.restart();
            round.game.position=ring.center;
            round.step();
            round.game.position=course.spawn;
            while (round.ticks<=ring.timeTicks && !round.caught) round.step();
            if (round.smiley.phase!=SmileyPhase::Failed || !capture("ring-timeout") || round.baddies.count<4)
                return false;
            logEvent(log,"test_029_ring_timeout",round.game);
            chillMode=false;
            round.restart();
        } else if (index==SwitchCourse || index==JumbleCourse) {
            const unsigned blocks=index==SwitchCourse?8:7;
            if (!round.switchStage || round.switchStage->blockCount!=blocks ||
                round.switchStage->layerCount!=2 || round.switches.state!=0 || parkArt.data.smiley.coinCount)
                return false;
        } else if (index==CoasterCourse) {
            const auto& ring=parkArt.data.smiley;
            if (ring.coinCount!=6 || ring.timeTicks!=300 || parkArt.data.ufoSpeed ||
                round.chuckles.count!=2 || !round.keyStage || round.keyStage->doorCount!=1 ||
                courseHazards(index).blockCount!=1) return false;
            round.controls.update(round.game,course,{-1,0},true,false);
            round.step();
            if (round.controls.update(round.game,course,{},false,false)!=ControlEvent::Shot) return false;
            while (round.ticks<300 && !round.caught && round.smiley.phase!=SmileyPhase::Passed) {
                const auto before=round.smiley.phase;
                round.step();
                if (before!=round.smiley.phase && round.smiley.phase==SmileyPhase::Active &&
                    !capture("ring-active")) return false;
            }
            if (round.smiley.phase!=SmileyPhase::Passed || round.smiley.bonus!=ring.bonus ||
                !capture("ring-pass")) return false;
            logEvent(log,"test_029_six_ring_coins",round.game);
            round.restart();
            round.controls.update(round.game,course,{-44.0f/127.0f,0},true,false);
            if (round.controls.update(round.game,course,{},false,false)!=ControlEvent::Shot) return false;
            while (round.ticks<=ring.timeTicks+12 && !round.caught && round.smiley.phase!=SmileyPhase::Failed)
                round.step();
            if (round.caught || round.smiley.phase!=SmileyPhase::Failed || !capture("ring-timeout")) return false;
            logEvent(log,"test_029_five_second_timeout",round.game);
            round.restart();
        }

        const ParkRoute029 route=routes029[index-CarouselCourse];
        unsigned flips=0;
        if (!route.count) return false;
        for (unsigned shot=0;shot<route.count;++shot) {
            const auto action=route.shots[shot];
            for (unsigned tick=0;tick<action.wait && !round.caught;++tick) round.step();
            const Vec2 aim{action.x/127.0f,action.y/127.0f};
            round.controls.update(round.game,course,aim,true,false);
            if (round.caught || round.controls.update(round.game,course,{},false,false)!=ControlEvent::Shot)
                return false;
            logEvent(log,"test_029_route_shot",round.game);
            for (unsigned tick=0;tick<600 && round.game.moving() && !round.caught && !round.game.won;++tick) {
                const unsigned previous=round.switches.state;
                const auto ringBefore=round.smiley.phase;
                round.step();
                if (index==CarouselCourse && ringBefore!=round.smiley.phase) {
                    if (round.smiley.phase==SmileyPhase::Active && !capture("ring-live")) return false;
                    if (round.smiley.phase==SmileyPhase::Passed && !capture("ring-win")) return false;
                }
                if (round.switches.state!=previous) {
                    ++flips;
                    if (flips==1 && !capture("switch-active")) return false;
                    if (index==JumbleCourse && flips==2 && !capture("switch-return")) return false;
                }
                if (index==CoasterCourse && round.ticks==230 && !capture("coaster")) return false;
            }
            if (round.caught || round.game.moving()) {
                logEvent(log,"test_029_route_failed",round.game);
                return false;
            }
        }
        if (!round.game.won || !capture("win",0,4700)) return false;
        if (index==CarouselCourse) {
            if (round.game.bounces!=63 || round.brokenBlocks!=3 || round.smiley.bonus!=2000 ||
                round.stopRun.collected!=1 || round.baddies.defeatedCount!=2) return false;
            logEvent(log,"test_029_carousel_all_interactives",round.game);
        }
        if (index==SwitchCourse) {
            if (flips!=4 || round.switches.state!=0 || round.game.bounces!=8) return false;
            logEvent(log,"test_029_four_switches",round.game);
        }
        if (index==JumbleCourse) {
            if (flips<2 || !jumbleFlag.texture()) return false;
            logEvent(log,"test_029_jumble_switches_and_goal",round.game);
        }
        if (index==CoasterCourse) {
            if (!round.roundKeys.collected || !round.roundKeys.opened || round.brokenBlocks!=1 ||
                round.chuckles.bonus<25) return false;
            logEvent(log,"test_029_chuckle_key_door_goal",round.game);
        }
        logEvent(log,"test_029_goal",round.game);
        const auto outcome=advanceCampaign(executable,course,round,save,log);
        const auto expected=index==CoasterCourse?Advance::EndOfBuild:Advance::NextHole;
        Progress saved{};
        if (outcome!=expected || progress.next!=index+1 || !save.read(saved) ||
            saved.next!=index+1 || saved.best[index].time==UINT32_MAX) return false;
    }
    return true;
}
