#include "routes034.hpp"

bool runFocused034(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,const char* log) {
    Flow flow;
    chillMode=false;
    Round round(course);
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=WholesomeCourse ||
        progress.best[WholesomeCourse-1].time==UINT32_MAX ||
        progress.best[WholesomeCourse-2].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian))
        return fail("test_034_seed_error");
    const Progress seed=progress;
    logEvent(log,"test_034_loaded_033",round.game);
    const auto capture=[&](const char* suffix,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        char name[80];
        std::snprintf(name,sizeof name,"034-%s-%s-%s.565",russian()?"ru":"en",CourseIds[menuCourse],suffix);
        return dumpFrame(executable,name);
    };
    const auto shoot=[&](int x,int y) {
        round.controls.update(round.game,course,{x/127.0f,y/127.0f},true,false);
        if (round.controls.update(round.game,course,{},false,false)!=ControlEvent::Shot) return false;
        round.coins.multiplier=0;
        return true;
    };

    for (unsigned index=WholesomeCourse;index<=CubeTownCourse;++index) {
        const bool wholesome=index==WholesomeCourse;
        const unsigned cell=index-FinalFrontierFirstCourse;
        openLevelSelect(flow);
        if (menuCourse!=index || levelSelect.world()!=4 || levelSelect.cell()!=cell ||
            !levelSelect.unlocked(4,cell) || levelSelect.unlocked(4,cell+1) ||
            !capture("map",3000) ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
            !capture("go",3030) ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090))
            return fail("test_034_selector_error");
        flow.update(0,true,false,false,false);
        if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
            currentCourse!=index || !round.switchStage ||
            round.switchStage->layerCount!=4 || round.switchStage->blockCount!=(wholesome?18u:26u) ||
            round.extraGhostCount!=(wholesome?0u:2u) ||
            objects.data.courses[index].count!=(wholesome?60u:0u) || !capture("start"))
            return fail("test_034_enter_error");
        if (!changeLanguage(executable,course,config,Language::English) || !capture("start") ||
            !changeLanguage(executable,course,config,Language::Russian))
            return fail("test_034_locale_error");
        round.restart();

        if (wholesome && (!round.stopStage || !round.speedStage ||
                          round.stopStage->count!=5 || round.speedStage.count!=4))
            return fail("test_034_token_layout_error");
        while (!round.caught && round.ticks<2400) round.step();
        if (!round.caught || round.game.won || !capture("loss",0,1000))
            return fail("test_034_loss_error");
        logEvent(log,wholesome?"test_034_404_loss":"test_034_405_loss",round.game);
        round.restart();

        const auto route=wholesome?Route404:Route405;
        bool teleported=false;
        unsigned colors=1;
        constexpr const char* colorNames[]={"green","red","blue","yellow"};
        for (unsigned step=0;step<route.count;++step) {
            const auto input=route.shots[step];
            for (unsigned wait=0;wait<input.wait && !round.caught;++wait) round.step();
            if (round.caught || !shoot(input.x,input.y)) return fail("test_034_shot_error");
            for (unsigned tick=0;tick<1400 && round.game.moving() && !round.caught && !round.game.won;++tick) {
                const Vec2 previous=round.game.position;
                round.step();
                if (wholesome && !teleported && round.game.position.x-previous.x>200) {
                    teleported=true;
                    if (!capture("portal")) return fail("test_034_capture_portal_error");
                    logEvent(log,"test_034_portal",round.game);
                }
                const unsigned state=round.switches.state;
                if (!(colors&(1u<<state))) {
                    colors|=1u<<state;
                    if (!capture(colorNames[state])) return fail("test_034_capture_color_error");
                }
            }
            if (round.caught || (!round.game.won && round.game.moving()))
                return fail("test_034_route_stop_error");
            if (!wholesome && step==0 && colors!=15)
                return fail("test_034_missing_first_shot_color");
        }
        if (!round.game.won || !capture("win",0,4700))
            return fail("test_034_goal_error");
        if (wholesome) {
            unsigned picked=0;
            for (unsigned coin=0;coin<60;++coin) picked+=round.coins.has(coin);
            if (!teleported || picked!=27 || round.coins.bonus!=2425 ||
                round.stopRun.collected!=1 || round.speedRun.collected!=6)
                return fail("test_034_portal_or_collectible_error");
        }
        if (colors!=15) return fail("test_034_missing_switch_color");
        if (!wholesome) logEvent(log,"test_034_four_colors",round.game);
        logEvent(log,wholesome?"test_034_404_goal":"test_034_405_goal",round.game);
        const auto outcome=advanceCampaign(executable,course,round,save,log);
        const auto expected=wholesome?Advance::NextHole:Advance::EndOfBuild;
        Progress saved{};
        if (outcome!=expected || progress.next!=index+1 || !save.read(saved) ||
            saved.next!=index+1 || saved.best[index].time==UINT32_MAX)
            return fail("test_034_save_error");
        for (unsigned earlier=0;earlier<WholesomeCourse;++earlier) {
            if (saved.best[earlier].time!=seed.best[earlier].time ||
                saved.best[earlier].rank!=seed.best[earlier].rank ||
                saved.best[earlier].score!=seed.best[earlier].score)
                return fail("test_034_save_history_error");
        }
    }
    return true;
}
