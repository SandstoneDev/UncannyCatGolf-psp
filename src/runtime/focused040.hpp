#pragma once

bool runFocused040(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    struct Stroke { unsigned tick; int x,y; };
    static constexpr Stroke replay505[]={{12,15,-123},{220,94,118},{443,4,45}};
    static constexpr Stroke route506[]={{11,25,-112},{202,-123,-42}};
    static constexpr Stroke route507[]={{2,103,-10},{175,-99,-82},{387,-126,24}};
    static constexpr Stroke route508[]={{13,-54,-127},{283,29,123},{508,63,-96}};
    static constexpr Stroke route509[]={{9,-99,-92},{131,-56,-121},{262,83,80},{296,95,63},{489,119,-32},{582,93,-57}};
    struct Trial {
        unsigned index,health,bounces;
        const Stroke* strokes;
        unsigned shotCount;
    };
    constexpr Trial trials[]={
        {SecondHorseCourse,4,18,replay505,std::size(replay505)},
        {Elysian506Course,6,20,route506,std::size(route506)},
        {Elysian507Course,7,7,route507,std::size(route507)},
        {Elysian508Course,7,14,route508,std::size(route508)},
        {Elysian509Course,3,16,route509,std::size(route509)},
    };
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=Elysian506Course || !progress.frontierKey ||
        progress.totalScore()!=883288 || progress.totalPrisms()!=381 ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_040_seed_error");
    const auto seed=progress;
    round.totalTicks=progress.ticks;
    const auto capture=[&](const char* phase,unsigned index,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && (round.game.won || round.caught) &&
            !clips.count(resultClip(round.game.won,round.parkScare.lightsOut)) && !liveResultPrepared) return false;
        const auto filename=std::string("040-")+phase+"-"+CourseIds[index]+".565";
        return dumpFrame(executable,filename.c_str());
    };
    const auto shoot=[&](int x,int y) {
        round.updateControls({x/127.0f,y/127.0f},true,false);
        return round.updateControls({},false,false)==ControlEvent::Shot;
    };
    if (clips.hasCommonResults()) {
        flow.screen=Screen::Hole;
        const auto retained=[&](Clip clip,unsigned milliseconds,const char* name) {
            const auto* pixels=clips.frame(clip,milliseconds);
            if (!pixels) return false;
            beginFrame();
            drawFull(pixels);
            endFrame();
            return dumpFrame(executable,name);
        };
        if (!changeLanguage(executable,course,config,Language::English)) return fail("test_compact_english_error");
        if (!retained(Clip::EasyEnding,1375,"040-retained-easy-en.565") ||
            !retained(Clip::FakeWin503,4375,"040-retained-broadcast-en.565"))
            return fail("test_compact_retained_en_error");
        for (const unsigned index:{0u,FirstBaddieCourse}) {
            if (!enterCourse(executable,index,course,round)) return fail("test_compact_result_entry_error");
            round.game.won=true;
            if (!capture("source-en-win",index,0,4700)) return fail("test_compact_result_draw_error");
        }
        if (!changeLanguage(executable,course,config,Language::Russian)) return fail("test_compact_russian_error");
        if (!retained(Clip::EasyEnding,1375,"040-retained-easy-ru.565") ||
            !retained(Clip::FakeWin503,4375,"040-retained-broadcast-ru.565") ||
            !retained(Clip::WorldResultsFrontier,1575,"040-retained-frontier-ru.565"))
            return fail("test_compact_retained_ru_error");
        for (const unsigned index:{0u,TutorialCount,FirstBaddieCourse,CataclysmCourse,TheWallCourse,
                                  ElysianFieldsCourse,ElysianRocksCourse}) {
            if (!enterCourse(executable,index,course,round)) return fail("test_compact_result_entry_error");
            round.game.won=true;
            if (!capture("source-slide",index,0,1500) || !capture("source-win",index,0,4700))
                return fail("test_compact_result_draw_error");
            logEvent(log,"test_compact_source_result",round.game);
        }
    }

    const auto go=[&](unsigned index,const char* map,const char* pressed) {
        openLevelSelect(flow,index);
        if (flow.screen!=Screen::LevelSelect || menuCourse!=index || levelSelect.world()!=5 ||
            !capture(map,index,1000) || updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,1100) ||
            !selectorGoDown || !capture(pressed,index,1100) ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,1200) || selectorGoDown) return false;
        flow.update(0,true,false,false,false);
        return flow.screen==Screen::Hole && enterCourse(executable,index,course,round);
    };
    for (const auto& trial:trials) {
        if (!go(trial.index,"map","go") ||
            round.horse.health!=trial.health || !capture("start",trial.index)) return fail("test_040_enter_error");
        if (trial.index==Elysian506Course &&
            (!go(trial.index,"remap","rego") || !capture("reenter",trial.index))) return fail("test_040_reenter_error");

        if (trial.index!=SecondHorseCourse) {
            for (unsigned tick=0;tick<1100 && !round.caught;++tick) round.step();
            if (!round.caught || round.horseGhostCaught || round.horse.phase!=HorsePhase039::Lost ||
                !capture("horse-loss",trial.index,0,2000)) return fail("test_040_horse_loss_error");
            logEvent(log,"test_040_horse_loss",round.game);
            round.restart();
        }
        if (hasHorseCourseGhost040(trial.index)) {
            if (trial.index==Elysian507Course) round.extraGhosts[0].position=round.game.position;
            else if (!shoot(-127,-127)) return fail("test_040_ghost_route_error");
            for (unsigned tick=0;tick<150 && !round.caught;++tick) round.step();
            if (!round.caught || !round.horseGhostCaught || round.horse.phase!=HorsePhase039::Lost ||
                !capture("ghost-loss",trial.index,0,800)) return fail("test_040_ghost_loss_error");
            logEvent(log,"test_040_ghost_loss",round.game);
            round.restart();
        }

        unsigned shot=0, sandTicks=0;
        bool showedTarget=false,showedFlight=false;
        for (unsigned frame=0;frame<1500 && !round.game.won && !round.caught;++frame) {
            if (shot<trial.shotCount && round.ticks==trial.strokes[shot].tick) {
                if (!shoot(trial.strokes[shot].x,trial.strokes[shot].y)) return fail("test_040_shot_error");
                ++shot;
            }
            if (!showedTarget && round.horse.health<trial.health && round.horse.health>0) {
                if (!capture("targets",trial.index)) return fail("test_040_targets_error");
                showedTarget=true;
            }
            const unsigned flightTicks=round.horse.age-round.horse.lastHitAt;
            if (round.horse.flying() && flightTicks==12) {
                if (!capture("lift",trial.index)) return fail("test_040_lift_error");
                showedFlight=true;
            }
            round.step(false,false,round.horse.flying() && flightTicks==1);
            if (round.inSand && !round.horse.flying()) ++sandTicks;
        }
        if (!round.game.won || round.caught || round.horse.health || !showedTarget || !showedFlight ||
            shot!=trial.shotCount || round.game.strokes!=shot || round.game.bounces!=trial.bounces)
            return fail("test_040_route_error");
        if ((trial.index==Elysian506Course && round.blockBonus!=450) ||
            (trial.index==Elysian507Course && (!sandTicks || round.extraGhostCount!=1)) ||
            (trial.index==Elysian508Course && (round.skullCount!=13 || round.skulls.bonus!=125)) ||
            (trial.index==Elysian509Course && (!sandTicks || round.blockBonus!=60 || !round.baddies.count)))
            return fail("test_040_hazards_error");
        if (!capture("win",trial.index,0,4700)) return fail("test_040_result_error");
        logEvent(log,"test_040_goal",round.game);
        const auto expected=trial.index==Elysian509Course?Advance::EndOfBuild:Advance::NextHole;
        if (advanceCampaign(executable,course,round,save,log)!=expected || progress.next!=trial.index+1)
            return fail("test_040_advance_error");
    }

    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.frontierKey) return fail("test_040_save_error");
    for (unsigned i=0;i<SecondHorseCourse;++i) {
        const auto& previous=seed.best[i];
        const auto& current=saved.best[i];
        if (current.score!=previous.score || current.rank!=previous.rank || current.time!=previous.time)
            return fail("test_040_old_best_error");
    }
    return true;
}
