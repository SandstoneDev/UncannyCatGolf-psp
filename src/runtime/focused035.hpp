#include "routes035.hpp"

bool runFocused035(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,const char* log) {
    Flow flow;
    chillMode=false;
    Round round(course);
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=PlentifulChucklesCourse ||
        progress.best[FrontierMazeCourse].time==UINT32_MAX ||
        progress.best[FrontierDogCourse].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_035_seed_error");
    const Progress seed=progress;
    logEvent(log,"test_035_loaded_034",round.game);
    const auto capture=[&](const char* suffix,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        char name[80];
        std::snprintf(name,sizeof name,"035-%s-%s-%s.565",russian()?"ru":"en",CourseIds[menuCourse],suffix);
        return dumpFrame(executable,name);
    };
    const auto shoot=[&](int x,int y) {
        round.controls.update(round.game,course,{x/127.0f,y/127.0f},true,false);
        return round.controls.update(round.game,course,{},false,false)==ControlEvent::Shot;
    };

    for (unsigned index=PlentifulChucklesCourse;index<=UncannyValleyCourse;++index) {
        const bool chuckleCourse=index==PlentifulChucklesCourse;
        const unsigned cell=index-FinalFrontierFirstCourse;
        openLevelSelect(flow);
        if (menuCourse!=index || levelSelect.world()!=4 || levelSelect.cell()!=cell ||
            !levelSelect.unlocked(4,cell) || levelSelect.unlocked(4,cell+1) ||
            !capture("map",3000) ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
            !capture("go",3030) ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090))
            return fail("test_035_selector_error");
        flow.update(0,true,false,false,false);
        if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
            currentCourse!=index || objects.data.courses[index].count || !round.stopStage ||
            round.stopStage->count!=(chuckleCourse?4u:9u) ||
            round.extraGhostCount!=(chuckleCourse?0u:27u) || !capture("start"))
            return fail("test_035_enter_error");
        if (chuckleCourse) {
            if (!round.switchStage || round.switchStage->layerCount!=4 ||
                round.switchStage->blockCount!=6 || !round.chuckles.spawnerVisible())
                return fail("test_035_408_layout_error");
        } else if (round.switchStage || round.baddies.stage!=&FrontierCars409 ||
                   round.jumpStage.count!=1 || round.keyStage!=&FrontierKeys409) {
            return fail("test_035_409_layout_error");
        }
        if (!changeLanguage(executable,course,config,Language::English) || !capture("start") ||
            !changeLanguage(executable,course,config,Language::Russian))
            return fail("test_035_locale_error");
        round.restart();

        while (!round.caught && round.ticks<2400) round.step();
        if (!round.caught || round.game.won || !capture("loss",0,1000))
            return fail("test_035_loss_error");
        logEvent(log,chuckleCourse?"test_035_408_loss":"test_035_409_loss",round.game);
        round.restart();

        const auto route=chuckleCourse?Route408:Route409;
        unsigned colors=1,nukeMask=0,maxActors=0;
        bool airborne=false,opened=false,keyPicked=false,nukeCleared=false;
        constexpr const char* colorNames[]={"green","red","blue","yellow"};
        const auto liveActors=[&]() {
            unsigned count=0;
            if (chuckleCourse) {
                for (unsigned i=0;i<round.chuckles.count;++i) count+=round.chuckles.bodies[i].radius>0;
            } else {
                for (unsigned i=0;i<round.baddies.count;++i) count+=round.baddies.alive(i);
            }
            return count;
        };
        const auto step=[&]() {
            const unsigned before=liveActors();
            round.step();
            maxActors=std::max(maxActors,liveActors());
            const unsigned state=round.switches.state;
            if (!(colors&(1u<<state))) {
                colors|=1u<<state;
                if (!capture(colorNames[state])) return false;
            }
            unsigned mask=0;
            for (unsigned i=0;i<NukeRun::Capacity;++i) if (round.nukes.has(i)) mask|=1u<<i;
            if (mask!=nukeMask) {
                nukeMask=mask;
                if (before && liveActors()==0) nukeCleared=true;
                if (!capture("nuke")) return false;
            }
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
            return true;
        };
        for (unsigned shot=0;shot<route.count;++shot) {
            const auto input=route.shots[shot];
            for (unsigned wait=0;wait<input.wait && !round.caught;++wait)
                if (!step()) return fail("test_035_capture_event_error");
            if (round.caught || !shoot(input.x,input.y)) return fail("test_035_shot_error");
            for (unsigned tick=0;tick<1400 && (round.game.moving() || round.jump.airborne()) &&
                 !round.caught && !round.game.won;++tick)
                if (!step()) return fail("test_035_capture_event_error");
            logEvent(log,chuckleCourse?"test_035_408_shot":"test_035_409_shot",round.game);
            if (round.caught || (!round.game.won && (round.game.moving() || round.jump.airborne())))
                return fail("test_035_route_stop_error");
        }
        if (!round.game.won || !capture("win",0,4700))
            return fail("test_035_goal_error");
        if (!nukeCleared || !maxActors || (chuckleCourse && colors!=15) ||
            (!chuckleCourse && (!airborne || !opened || !keyPicked ||
                               round.roundKeys.bonus!=KeyDoorRun::DoorBonus)))
            return fail("test_035_mechanic_error");
        logEvent(log,chuckleCourse?"test_035_408_switches_swarm_nuke":"test_035_409_jump_key_gate_cars_nuke",round.game);
        logEvent(log,chuckleCourse?"test_035_408_goal":"test_035_409_goal",round.game);
        const auto outcome=advanceCampaign(executable,course,round,save,log);
        const auto expected=chuckleCourse?Advance::NextHole:Advance::EndOfBuild;
        Progress saved{};
        if (outcome!=expected || progress.next!=index+1 || !save.read(saved) ||
            saved.next!=index+1 || saved.best[index].time==UINT32_MAX)
            return fail("test_035_save_error");
        for (unsigned earlier=0;earlier<PlentifulChucklesCourse;++earlier) {
            if (saved.best[earlier].time!=seed.best[earlier].time ||
                saved.best[earlier].rank!=seed.best[earlier].rank ||
                saved.best[earlier].score!=seed.best[earlier].score)
                return fail("test_035_save_history_error");
        }
    }
    return true;
}
