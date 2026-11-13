#pragma once

struct ElysianStroke042 { unsigned tick; int x,y; };
inline constexpr ElysianStroke042 replay511_042[]={{24,91,93},{273,-127,43},{485,124,1}};
inline constexpr ElysianStroke042 route512_042[]={{25,88,86},{237,-90,-109},{451,54,120}};
inline constexpr ElysianStroke042 route513_042[]={{4,107,-54},{207,-93,-115}};

bool runFocused042(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=Elysian512Course || !progress.frontierKey ||
        progress.totalScore()!=953093 || progress.totalPrisms()!=411 ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_042_seed_error");
    const auto seed=progress;
    round.totalTicks=progress.ticks;
    const auto capture=[&](const char* phase,unsigned index,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && (round.game.won || round.caught) && !liveResultPrepared) return false;
        const auto filename=std::string("042-")+phase+"-"+CourseIds[index]+".565";
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
    unsigned flips=0,dogHits=0;
    const auto play=[&](const auto& strokes,bool captureStates) {
        unsigned shot=0;
        flips=dogHits=0;
        bool switchCapture=false,trafficCapture=false;
        for (unsigned frame=0;frame<2400 && !round.game.won && !round.caught;++frame) {
            if (shot<std::size(strokes) && round.ticks==strokes[shot].tick) {
                round.updateControls({strokes[shot].x/127.0f,strokes[shot].y/127.0f},true,false);
                if (round.updateControls({},false,false)!=ControlEvent::Shot) return false;
                ++shot;
            }
            const unsigned previous=round.switches.state;
            const bool grounded=!round.horse.flying();
            round.step(false,false,round.horse.flying() && round.horse.age-round.horse.lastHitAt==1);
            flips+=round.switches.state!=previous;
            dogHits+=grounded && round.game.passiveBounceIndex>=0;
            if (captureStates && currentCourse==Elysian512Course && round.switches.state && !switchCapture) {
                if (!capture("switch-ru",currentCourse)) return false;
                switchCapture=true;
            }
            if (captureStates && currentCourse==Elysian513Course && round.ticks==65 && !trafficCapture) {
                if (round.chuckles.count<4 || !capture("traffic-ru",currentCourse)) return false;
                trafficCapture=true;
            }
        }
        return shot==std::size(strokes) && round.game.won && !round.caught && !round.horse.health &&
            (!captureStates || (currentCourse==Elysian512Course?switchCapture:trafficCapture));
    };
    flow.screen=Screen::Hole;
    if (!enterCourse(executable,Elysian511Course,course,round) || !play(replay511_042,false) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=Elysian512Course) return fail("test_042_previous_advance");

    if (!go(Elysian512Course,"map-ru","go-ru") || !capture("start-ru",Elysian512Course) ||
        round.horse.health!=5 || !round.switchStage || round.switchStage->blockCount!=16 ||
        round.switchStage->layerCount!=2 || round.switches.state || clips.count(Clip::Win512) ||
        clips.count(Clip::Loss512)) return fail("test_042_512_setup");
    if (!play(route512_042,true) || flips!=4 || round.switches.state ||
        round.smiley.bonus!=5000 || round.game.bounces!=27) return fail("test_042_512_route");
    if (!capture("win-ru",Elysian512Course,0,4700)) return fail("test_042_512_win");
    logEvent(log,"test_042_ring_and_four_switches",round.game);
    logEvent(log,"test_042_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=Elysian513Course || progress.next!=Elysian513Course) return fail("test_042_512_advance");

    if (!go(Elysian513Course,"map-ru","go-ru") || !capture("start-ru",Elysian513Course) ||
        round.horse.health!=6 || round.chuckles.spawners()!=4 || clips.count(Clip::Win513) ||
        clips.count(Clip::Loss513)) return fail("test_042_513_setup");
    round.horse.position=round.game.position;
    round.step();
    if (!round.caught || round.horse.phase!=HorsePhase039::Lost ||
        !capture("loss-ru",Elysian513Course,0,600)) return fail("test_042_513_loss");
    logEvent(log,"test_042_horse_loss",round.game);
    round.restart();
    if (!changeLanguage(executable,course,config,Language::English) ||
        !go(Elysian512Course,"map-en","go-en") || !capture("start-en",Elysian512Course) ||
        !go(Elysian513Course,"map-en","go-en") || !capture("start-en",Elysian513Course) ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,Elysian513Course,course,round)) return fail("test_042_en");
    if (!play(route513_042,true) || dogHits!=2 || round.chuckles.defeatedCount<7 ||
        round.game.bounces!=10) return fail("test_042_513_route");
    if (!capture("win-ru",Elysian513Course,0,4700)) return fail("test_042_513_win");
    logEvent(log,"test_042_dogs_and_chuckles",round.game);
    logEvent(log,"test_042_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild)
        return fail("test_042_513_advance");
    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.frontierKey) return fail("test_042_save");
    for (unsigned i=0;i<Elysian512Course;++i) {
        const auto& old=seed.best[i];
        const auto& now=saved.best[i];
        if (old.score!=now.score || old.rank!=now.rank || old.time!=now.time)
            return fail("test_042_old_best");
    }
    return true;
}
