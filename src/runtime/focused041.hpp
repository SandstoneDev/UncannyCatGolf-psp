#pragma once

struct ElysianStroke041 { unsigned tick; int x,y; };
inline constexpr ElysianStroke041 ElysianRoute510[]={{7,101,-127},{547,-118,40},{754,107,-77}};
inline constexpr ElysianStroke041 ElysianRoute511[]={{24,91,93},{273,-127,43},{485,124,1}};

template<class Shoot,std::size_t Count>
bool playElysian041Route(Round& round,Shoot shoot,const ElysianStroke041 (&strokes)[Count]) {
    unsigned shot=0,jumps=0;
    for (unsigned frame=0;frame<1900 && !round.game.won && !round.caught;++frame) {
        if (shot<Count && round.ticks==strokes[shot].tick) {
            if (!shoot(strokes[shot].x,strokes[shot].y)) return false;
            ++shot;
        }
        const bool wasAirborne=round.jump.airborne();
        round.step(false,false,round.horse.flying() && round.horse.age-round.horse.lastHitAt==1);
        if (!wasAirborne && round.jump.airborne()) ++jumps;
    }
    if (currentCourse==Elysian511Course && (jumps!=2 || round.baddies.defeatedCount!=2)) return false;
    return shot==Count && round.game.strokes==Count && round.game.won && !round.caught && !round.horse.health;
}

bool runFocused041(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=Elysian510Course || !progress.frontierKey ||
        progress.totalScore()!=928728 || progress.totalPrisms()!=401 ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_041_seed_error");
    const auto seed=progress;
    const auto capture=[&](const char* phase,unsigned index,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && (round.game.won || round.caught) && !liveResultPrepared) return false;
        const auto filename=std::string("041-")+phase+"-"+CourseIds[index]+".565";
        return dumpFrame(executable,filename.c_str());
    };
    const auto go=[&](unsigned index,const char* map,const char* pressed) {
        openLevelSelect(flow,index);
        if (flow.screen!=Screen::LevelSelect || menuCourse!=index || levelSelect.world()!=5 ||
            !capture(map,index,1000) || updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,1100) ||
            !selectorGoDown || !capture(pressed,index,1100) ||
            !updateLevelSelector(0,PSP_CTRL_CROSS,0,1200) || selectorGoDown) return false;
        flow.update(0,true,false,false,false);
        return flow.screen==Screen::Hole && enterCourse(executable,index,course,round);
    };
    const auto shoot=[&](int x,int y) {
        round.updateControls({x/127.0f,y/127.0f},true,false);
        return round.updateControls({},false,false)==ControlEvent::Shot;
    };
    round.totalTicks=progress.ticks;
    flow.screen=Screen::Hole;
    if (!enterCourse(executable,Elysian509Course,course,round)) return fail("test_041_previous_error");
    constexpr struct Stroke { unsigned tick; int x,y; } previous[]={
        {9,-99,-92},{131,-56,-121},{262,83,80},{296,95,63},{489,119,-32},{582,93,-57}
    };
    unsigned shot=0;
    for (unsigned frame=0;frame<1500 && !round.game.won && !round.caught;++frame) {
        if (shot<std::size(previous) && round.ticks==previous[shot].tick) {
            if (!shoot(previous[shot].x,previous[shot].y)) return fail("test_041_previous_shot");
            ++shot;
        }
        round.step(false,false,round.horse.flying() && round.horse.age-round.horse.lastHitAt==1);
    }
    if (!round.game.won || shot!=std::size(previous) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=Elysian510Course) return fail("test_041_previous_advance");

    if (!go(Elysian510Course,"map-ru","go-ru") || !capture("start-ru",Elysian510Course) ||
        round.horse.health!=20 || round.speedStage.count!=36 || !round.stopStage ||
        round.stopStage->count!=4 || courseHazards(currentCourse).blockCount!=8 ||
        objects.data.courses[currentCourse].count!=48 || clips.count(Clip::Win510) ||
        clips.count(Clip::Loss510)) return fail("test_041_510_setup");
    for (unsigned goal=0;goal<20;++goal)
        if (round.horse.goals[goal].phase!=HorseGoalPhase039::Ready) return fail("test_041_goal_setup");

    round.game.position=ElysianSpeed510.positions[35];
    round.game.velocity={100,0};
    round.step();
    if (!round.speedRun.has(35) || round.speedRun.has(3) || length(round.game.velocity)<800)
        return fail("test_041_510_high_speed");
    round.game.position=ElysianStop510.positions[0];
    round.step();
    if (!round.stopRun.has(0) || round.game.moving()) return fail("test_041_510_stop");
    logEvent(log,"test_041_tokens",round.game);
    round.restart();

#ifdef UCG_041_ROUTES
    if (!playElysian041Route(round,shoot,ElysianRoute510)) return fail("test_041_route_510");
    if (round.brokenBlocks.low!=0xff || !round.stopRun.has(1) || !round.speedRun.has(35) ||
        !capture("win-ru",Elysian510Course,0,4700)) return fail("test_041_win_510");
    logEvent(log,"test_041_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        progress.next!=Elysian511Course || currentCourse!=Elysian511Course)
        return fail("test_041_510_advance");
#else
    progress.next=Elysian511Course;
#endif
    if (!go(Elysian511Course,"map-ru","go-ru") || !capture("start-ru",Elysian511Course) ||
        round.horse.health!=5 || round.jumpStage.count!=5 || round.extraGhostCount!=3 ||
        round.baddies.stage!=&ElysianCars511 || clips.count(Clip::Win511) ||
        clips.count(Clip::Loss511)) return fail("test_041_511_setup");
    round.game.position={-288,-448};
    round.game.velocity={875,0};
    round.step();
    if (!round.jump.airborne()) return fail("test_041_511_launch");
    const auto age=round.horse.age;
    round.horse.goals[0].position=round.game.position;
    round.horse.position=round.game.position;
    round.step();
    if (!round.jump.airborne() || round.caught || round.horse.age!=age+1 ||
        round.horse.goals[0].phase!=HorseGoalPhase039::Ready)
        return fail("test_041_511_airborne");
    if (!capture("air-ru",Elysian511Course)) return fail("test_041_511_air_render");
    round.game.position={-600,-448};
    round.step();
    if (!round.caught || !round.fellOut || round.backrooms ||
        !capture("out-ru",Elysian511Course,0,100) ||
        !capture("out-off-ru",Elysian511Course,0,300))
        return fail("test_041_511_out");
    logEvent(log,"test_041_out_of_bounds",round.game);
    round.restart();
    round.ghost.position=round.game.position;
    round.step();
    if (!round.caught || !round.horseGhostCaught || round.horse.phase!=HorsePhase039::Lost ||
        !capture("ghost-loss-ru",Elysian511Course,0,600))
        return fail("test_041_511_ghost");
    logEvent(log,"test_041_ghost_loss",round.game);
    round.restart();
    round.horse.position=round.game.position;
    round.step();
    if (!round.caught || round.horseGhostCaught || round.horse.phase!=HorsePhase039::Lost ||
        !capture("horse-loss-ru",Elysian511Course,0,600))
        return fail("test_041_511_horse_loss");
    logEvent(log,"test_041_horse_loss",round.game);
    round.restart();
    for (unsigned i=0;i<60 && !round.caught;++i) round.step();
    if (round.caught || round.baddies.liveCount[0]!=1 || round.baddies.liveCount[1]!=1 ||
        round.baddies.velocities[0].x!=200 || round.baddies.velocities[1].x!=-200)
        return fail("test_041_511_cars");
    for (unsigned i=0;i<25 && !round.caught;++i) round.step();
    if (round.caught || !capture("cars-ru",Elysian511Course)) return fail("test_041_511_car_render");
    logEvent(log,"test_041_cars",round.game);

    if (!changeLanguage(executable,course,config,Language::English) ||
        !go(Elysian510Course,"map-en","go-en") || !capture("start-en",Elysian510Course) ||
        !go(Elysian511Course,"map-en","go-en") || !capture("start-en",Elysian511Course) ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,Elysian511Course,course,round))
        return fail("test_041_en");
#ifdef UCG_041_ROUTES
    if (!playElysian041Route(round,shoot,ElysianRoute511)) return fail("test_041_route_511");
    if (!capture("win-ru",Elysian511Course,0,4700)) return fail("test_041_win_511");
    logEvent(log,"test_041_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild)
        return fail("test_041_511_advance");

    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.frontierKey) return fail("test_041_save");
    for (unsigned i=0;i<Elysian510Course;++i) {
        const auto& old=seed.best[i];
        const auto& now=saved.best[i];
        if (old.score!=now.score || old.rank!=now.rank || old.time!=now.time)
            return fail("test_041_old_best");
    }
#else
    (void)seed;
    logEvent(log,"test_041_smoke_only",round.game);
#endif
    return true;
}
