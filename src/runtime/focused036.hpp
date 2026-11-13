#include "routes036.hpp"

bool runFocused036(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,const char* log) {
    Flow flow;
    chillMode=false;
    Round round(course);
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=WeatherAlertCourse ||
        progress.best[FrontierPatrolCourse].time==UINT32_MAX ||
        progress.best[FrontierMineCourse].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_036_seed_error");
    const Progress seed=progress;
    logEvent(log,"test_036_loaded_035",round.game);
    flow.screen=Screen::Hole;
    for (unsigned inherited:{FrontierMazeCourse,FrontierDogCourse,PlentifulChucklesCourse}) {
        if (!enterCourse(executable,inherited,course,round)) return fail("test_036_inherited_course_error");
        render(flow,course,round,0,0,0,0);
    }
    if (!round.switchStage || round.switchStage->blockCount!=6 || !frontierSwitchFaces.texture() ||
        !dumpFrame(executable,"036-ru-4_08-inherited.565"))
        return fail("test_036_inherited_408_error");
    logEvent(log,"test_036_inherited_408",round.game);
    if (!enterCourse(executable,FrontierMineCourse,course,round))
        return fail("test_036_predecessor_error");
    render(flow,course,round,0,0,0,0);
    logEvent(log,"test_036_predecessor_411",round.game);
    const auto capture=[&](const char* suffix,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        char name[80];
        std::snprintf(name,sizeof name,"036-%s-%s-%s.565",russian()?"ru":"en",CourseIds[menuCourse],suffix);
        return dumpFrame(executable,name);
    };
    const auto shoot=[&](int x,int y) {
        round.updateControls({x/127.0f,y/127.0f},true,false);
        return round.updateControls({},false,false)==ControlEvent::Shot;
    };

    for (unsigned index=WeatherAlertCourse;index<=DownfallCourse;++index) {
        const bool weather=index==WeatherAlertCourse;
        const unsigned cell=index-FinalFrontierFirstCourse;
        openLevelSelect(flow);
        if (menuCourse!=index || levelSelect.world()!=4 || levelSelect.cell()!=cell ||
            !levelSelect.unlocked(4,cell) || levelSelect.unlocked(4,cell+1) ||
            !capture("map",3000) ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
            !capture("go",3030) ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090))
            return fail("test_036_selector_error");
        flow.update(0,true,false,false,false);
        if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
            currentCourse!=index || objects.data.courses[index].count ||
            round.extraGhostCount!=(weather?0u:2u) || !round.frontierWind.active || !capture("start"))
            return fail("test_036_enter_error");
        if (weather) {
            if (round.keyStage!=&FrontierKeys412 || round.jumpStage.count!=1 || round.skullCount)
                return fail("test_036_412_layout_error");
        } else if (round.keyStage || round.jumpStage || round.skullCount!=3 ||
                   courseHazards(index).blockCount!=27) {
            return fail("test_036_413_layout_error");
        }
        if (!changeLanguage(executable,course,config,Language::English) || !capture("start") ||
            !changeLanguage(executable,course,config,Language::Russian))
            return fail("test_036_locale_error");
        round.restart();

        while (round.ticks<120 && !round.caught && !round.game.won) round.step();
        const Vec2 before=round.game.position;
        round.step();
        const Vec2 drift=round.game.position-before;
        if (round.caught || round.game.won || !round.frontierWind.drifting() ||
            (weather?drift.x:drift.y)<=0 || !round.game.moving() || !capture("wind") ||
            !shoot(-80,-20) || round.frontierWind.drifting())
            return fail("test_036_wind_control_error");
        logEvent(log,weather?"test_036_412_wind":"test_036_413_wind",round.game);
        round.restart();

        while (!round.caught && !round.game.won && round.ticks<2400) round.step();
        if (!round.caught || round.game.won || !capture("loss",0,1000))
            return fail("test_036_loss_error");
        logEvent(log,weather?"test_036_412_loss":"test_036_413_loss",round.game);
        round.restart();

        const auto route=weather?Route412:Route413;
        bool airborne=false,opened=false,keyPicked=false,blockBroken=false,skullDefeated=false;
        const auto step=[&]() {
            round.step();
            if (!airborne && round.jump.airborne()) {
                airborne=true;
                if (!capture("jump")) return false;
            }
            if (!keyPicked && round.roundKeys.collected) {
                keyPicked=true;
                if (!capture("key")) return false;
            }
            if (!opened && round.roundKeys.opened) {
                opened=true;
                if (!capture("gate")) return false;
            }
            if (!blockBroken && round.blockBonus) {
                blockBroken=true;
                if (!capture("block")) return false;
            }
            if (!skullDefeated && round.skulls.bonus) {
                skullDefeated=true;
                if (!capture("skull")) return false;
            }
            return true;
        };
        for (unsigned shot=0;shot<route.count;++shot) {
            const auto input=route.shots[shot];
            for (unsigned wait=0;wait<input.wait && !round.caught && !round.game.won;++wait)
                if (!step()) return fail("test_036_capture_event_error");
            if (round.caught || !shoot(input.x,input.y)) return fail("test_036_shot_error");
            for (unsigned tick=0;tick<2400 && (!round.frontierWind.drifting() || round.jump.airborne()) &&
                 !round.caught && !round.game.won;++tick)
                if (!step()) return fail("test_036_capture_event_error");
            logEvent(log,weather?"test_036_412_shot":"test_036_413_shot",round.game);
            if (round.caught || (!round.game.won && (!round.frontierWind.drifting() || round.jump.airborne())))
                return fail("test_036_route_stop_error");
        }
        if (!round.game.won || !capture("win",0,4700))
            return fail("test_036_goal_error");
        if (weather ? (!airborne || !opened || !keyPicked || round.roundKeys.bonus!=KeyDoorRun::DoorBonus) :
                      (!blockBroken || !skullDefeated))
            return fail("test_036_mechanic_error");
        logEvent(log,weather?"test_036_412_jump_key_gate":"test_036_413_blocks_skulls",round.game);
        logEvent(log,weather?"test_036_412_goal":"test_036_413_goal",round.game);
        const auto outcome=advanceCampaign(executable,course,round,save,log);
        const auto expected=weather?Advance::NextHole:Advance::EndOfBuild;
        Progress saved{};
        if (outcome!=expected || progress.next!=index+1 || !save.read(saved) ||
            saved.next!=index+1 || saved.best[index].time==UINT32_MAX)
            return fail("test_036_save_error");
        for (unsigned earlier=0;earlier<WeatherAlertCourse;++earlier) {
            if (saved.best[earlier].time!=seed.best[earlier].time ||
                saved.best[earlier].rank!=seed.best[earlier].rank ||
                saved.best[earlier].score!=seed.best[earlier].score)
                return fail("test_036_save_history_error");
        }
    }
    return true;
}
