#include "routes037.hpp"

bool runFocused037(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,const char* log) {
    Flow flow;
    chillMode=false;
    Round round(course);
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=CrimsonOutpostCourse || progress.quest<2 || progress.frontierKey ||
        progress.best[MovieOnceCourse].time==UINT32_MAX ||
        progress.best[AbandonedWorkspaceCourse].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_037_seed_error");
    const Progress seed=progress;
    logEvent(log,"test_037_loaded_036",round.game);
    if (!enterCourse(executable,AbandonedWorkspaceCourse,course,round)) return fail("test_037_predecessor_error");
    flow.screen=Screen::Hole;
    render(flow,course,round,0,0,0,0);
    logEvent(log,"test_037_predecessor_415",round.game);
    const auto capture=[&](const char* suffix,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        char name[88];
        std::snprintf(name,sizeof name,"037-%s-%s-%s.565",russian()?"ru":"en",CourseIds[menuCourse],suffix);
        return dumpFrame(executable,name);
    };
    const auto shoot=[&](int x,int y) {
        round.updateControls({x/127.0f,y/127.0f},true,false);
        return round.updateControls({},false,false)==ControlEvent::Shot;
    };

    for (unsigned index=CrimsonOutpostCourse;index<=CataclysmCourse;++index) {
        const bool outpost=index==CrimsonOutpostCourse;
        const unsigned cell=index-FinalFrontierFirstCourse;
        openLevelSelect(flow);
        if (menuCourse!=index || levelSelect.world()!=4 || levelSelect.cell()!=cell ||
            !levelSelect.unlocked(4,cell) || levelSelect.unlocked(4,cell+1) ||
            !capture("map",3000) ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
            !capture("go",3030) ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090))
            return fail("test_037_selector_error");
        flow.update(0,true,false,false,false);
        if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
            currentCourse!=index || !capture("start"))
            return fail("test_037_enter_error");
        if (outpost) {
            if (courseHazards(index).blockCount!=20 || courseHazards(index).sandCount<4 ||
                round.baddies.stage!=&FrontierCars416 || !round.chuckles.spawnerVisible())
                return fail("test_037_416_layout_error");
        } else if (!round.switchStage || round.switchStage->layerCount!=4 ||
                   round.switchStage->blockCount!=8 || !dogGate.active() || round.extraGhostCount!=2 ||
                   !round.finale417.keyAvailable || round.game.goalEnabled ||
                   objects.data.courses[index].count!=6) {
            return fail("test_037_417_layout_error");
        }
        if (!changeLanguage(executable,course,config,Language::English) || !capture("start") ||
            !changeLanguage(executable,course,config,Language::Russian))
            return fail("test_037_locale_error");
        round.restart();
        while (!round.caught && !round.game.won && round.ticks<2400) round.step();
        if (!round.caught || round.game.won || !capture("loss",0,1000))
            return fail("test_037_loss_error");
        logEvent(log,outpost?"test_037_416_loss":"test_037_417_loss",round.game);
        round.restart();

        const auto route=outpost?Route416:Route417;
        bool sand=false,blasted=false,cars=false,skulls=false,chuckles=false,teleported=false;
        bool switched=false,revealed=false,ufo=false,questKey=false;
        const auto step=[&]() {
            const Vec2 before=round.game.position;
            round.step();
            if (outpost) {
                for (unsigned i=0;i<round.baddies.count;++i) cars|=round.baddies.alive(i);
                for (unsigned i=0;i<round.skullSpawner416.count;++i) skulls|=round.skullSpawner416.bodies[i].radius>0;
                for (unsigned i=0;i<round.chuckles.count;++i) chuckles|=round.chuckles.bodies[i].radius>0;
                if (!sand && round.inSand) {
                    sand=true;
                    if (!capture("sand")) return false;
                }
                for (unsigned i=0;i<std::size(FrontierNukes416) && !blasted;++i) {
                    if (round.nukes.has(i)) {
                        blasted=true;
                        if (round.baddies.count || round.skullSpawner416.count || round.chuckles.count || !capture("nuke"))
                            return false;
                    }
                }
            } else {
                if (!switched && round.switches.state) {
                    switched=true;
                    if (!capture("switch")) return false;
                }
                if (!teleported && round.game.position.x-before.x>600) {
                    teleported=true;
                    if (!capture("portal")) return false;
                }
                if (!revealed && round.finale417.revealedAt) {
                    revealed=true;
                    if (round.finale417.gate(dogGate) || !round.finale417.finalGhostActive ||
                        round.finale417.bonus()!=500 || !capture("fake-goal"))
                        return false;
                }
                if (!ufo && round.finale417.ufoAt) {
                    ufo=true;
                    if (!capture("ufo")) return false;
                }
                if (!questKey && progress.frontierKey) {
                    questKey=true;
                    persistProgress(save,round,log);
                    Progress check{};
                    if (!save.read(check) || !check.frontierKey || !capture("quest-key")) return false;
                }
            }
            return true;
        };
        for (unsigned shot=0;shot<route.count;++shot) {
            const auto input=route.shots[shot];
            for (unsigned wait=0;wait<input.wait && !round.caught && !round.game.won;++wait)
                if (!step()) return fail("test_037_event_error");
            if (round.caught || !shoot(input.x,input.y)) return fail("test_037_shot_error");
            for (unsigned tick=0;tick<2400 && round.game.moving() && !round.caught && !round.game.won;++tick)
                if (!step()) return fail("test_037_event_error");
            logEvent(log,outpost?"test_037_416_shot":"test_037_417_shot",round.game);
            if (round.caught || (!round.game.won && round.game.moving()))
                return fail("test_037_route_stop_error");
        }
        if (!round.game.won || !capture("win",0,4700)) return fail("test_037_goal_error");
        if (outpost ? (!sand || !blasted || !cars || !skulls || !chuckles || !round.blockBonus) :
                      (!switched || !teleported || !revealed || !ufo || !questKey ||
                       !round.finale417.glassConsumed || round.switches.state!=2 ||
                       (round.coins.collected[0]&63)!=63))
            return fail("test_037_mechanic_error");
        logEvent(log,outpost?"test_037_416_spawners_sand_nuke":"test_037_417_switch_portal_fake_key",round.game);
        logEvent(log,outpost?"test_037_416_goal":"test_037_417_goal",round.game);
        const auto outcome=advanceCampaign(executable,course,round,save,log);
        const auto expected=outpost?Advance::NextHole:Advance::EndOfBuild;
        Progress saved{};
        if (outcome!=expected || progress.next!=index+1 || !save.read(saved) ||
            saved.next!=index+1 || saved.best[index].time==UINT32_MAX ||
            saved.frontierKey!=(outpost?0u:1u))
            return fail("test_037_save_error");
        for (unsigned earlier=0;earlier<CrimsonOutpostCourse;++earlier) {
            if (saved.best[earlier].time!=seed.best[earlier].time ||
                saved.best[earlier].rank!=seed.best[earlier].rank ||
                saved.best[earlier].score!=seed.best[earlier].score)
                return fail("test_037_save_history_error");
        }
    }
    return true;
}
