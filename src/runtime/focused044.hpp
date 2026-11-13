#pragma once

struct ElysianStroke044 { unsigned tick; int x,y; };
inline constexpr ElysianStroke044 replay515_044[]={{7,-69,-116},{316,100,-84}};
inline constexpr ElysianStroke044 route516_044[]={{2,119,94},{221,-115,82},{720,-94,-117}};
inline constexpr ElysianStroke044 route517_044[]={{8,41,-127},{223,20,-127},{441,30,127}};

bool runFocused044(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=Elysian516Course || !progress.frontierKey ||
        progress.totalScore()!=1005798 || progress.totalPrisms()!=431 ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_044_seed");
    const auto seed=progress;
    round.totalTicks=progress.ticks;
    const auto capture=[&](const char* phase,unsigned index,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && (round.game.won || round.caught) && !liveResultPrepared) return false;
        const auto filename=std::string("044-")+phase+"-"+CourseIds[index]+".565";
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
    unsigned casts=0,gustTicks=0,strongGustTicks=0,mirroredShots=0,blindBounces=0,goals=0;
    const auto play=[&](const auto& strokes,bool captureStates) {
        unsigned shot=0,captured=0;
        casts=gustTicks=strongGustTicks=mirroredShots=blindBounces=goals=0;
        for (unsigned frame=0;frame<3600 && !round.game.won && !round.caught;++frame) {
            if (shot<std::size(strokes) && round.ticks==strokes[shot].tick) {
                mirroredShots+=round.wizard.mirrored();
                round.updateControls({strokes[shot].x/127.0f,strokes[shot].y/127.0f},true,false);
                if (round.updateControls({},false,false)!=ControlEvent::Shot) return false;
                ++shot;
            }
            const unsigned before=round.game.bounces;
            const float wind=round.wizard.acceleration().x;
            gustTicks+=wind!=0;
            strongGustTicks+=wind==-25;
            const bool blind=round.wizard.blind();
            round.step(false,false,round.horse.flying() && round.horse.age-round.horse.lastHitAt==1);
            if (blind) blindBounces+=round.game.bounces-before;
            if (!captureStates) continue;
            if (currentCourse==Elysian516Course) {
                constexpr const char* phases[]={"","wind-ru","mirror-ru","blind-ru","wizard-death-ru"};
                const unsigned spell=static_cast<unsigned>(round.wizard.spell);
                if (spell) casts|=1u<<spell;
                if (spell && !(captured&(1u<<spell)) && round.wizard.age==11) {
                    if (!capture(phases[spell],currentCourse)) return false;
                    captured|=1u<<spell;
                }
            } else if (currentCourse==Elysian517Course) {
                goals=round.focusedHorseHits044;
                if (round.horse.health<=12 && !(captured&1u)) {
                    if (!capture("mid-ru",currentCourse)) return false;
                    captured|=1u;
                }
            }
        }
        return shot==std::size(strokes) && round.game.won && !round.caught && !round.horse.health &&
            (!captureStates || (currentCourse==Elysian516Course?captured==30:captured==1));
    };
    flow.screen=Screen::Hole;
    if (!enterCourse(executable,Elysian515Course,course,round) || !play(replay515_044,false) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=Elysian516Course) return fail("test_044_previous_advance");

    if (!go(Elysian516Course,"map-ru","go-ru") || !capture("start-ru",Elysian516Course) ||
        round.horse.health!=4 || round.wizard.spell!=WizardSpell516::None || clips.count(Clip::Win516) ||
        clips.count(Clip::Loss516)) return fail("test_044_516_setup");
    round.horse.position=round.game.position;
    round.step();
    if (!round.caught || round.horse.phase!=HorsePhase039::Lost ||
        !capture("loss-ru",Elysian516Course,0,600)) return fail("test_044_516_loss");
    logEvent(log,"test_044_horse_loss",round.game);
    round.restart();
    round.wizard.impact(2);
    if (round.updateControls({1,0},true,false)!=ControlEvent::None ||
        round.updateControls({},false,false)!=ControlEvent::Shot || round.game.velocity.x>=0)
        return fail("test_044_mirror_input");
    round.restart();
    if (round.wizard.mirrored() || round.wizard.blind() || round.wizard.windTick ||
        !changeLanguage(executable,course,config,Language::English) ||
        !go(Elysian516Course,"map-en","go-en") || !capture("start-en",Elysian516Course) ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,Elysian516Course,course,round)) return fail("test_044_516_en");
    startCourseMusic(currentCourse);
    const auto pcmBefore=audio.blocks();
    if (!play(route516_044,true)) return fail("test_044_516_route");
    if (auto* stats=std::fopen(log,"ab")) {
        std::fprintf(stats,"build=044 event=test_044_wizard spells=%u gust=%u strong=%u mirrored=%u blind_bounces=%u\n",
                     casts,gustTicks,strongGustTicks,mirroredShots,blindBounces);
        std::fclose(stats);
    }
    if (casts!=30 || gustTicks!=180 || strongGustTicks!=120 || mirroredShots!=1 || !blindBounces ||
        !round.wizard.changedHat() || !capture("win-ru",Elysian516Course,0,4700)) return fail("test_044_516_win");
    sceKernelDelayThread(300000);
    if (audio.blocks()==pcmBefore) return fail("test_044_pcm");
    logEvent(log,"test_044_pcm",round.game);
    logEvent(log,"test_044_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=Elysian517Course || progress.next!=Elysian517Course) return fail("test_044_516_advance");

    if (!go(Elysian517Course,"map-ru","go-ru") || !capture("start-ru",Elysian517Course) ||
        round.horse.health!=21 || clips.count(Clip::Win517) || clips.count(Clip::Loss517))
        return fail("test_044_517_setup");
    unsigned count=0;
    for (const auto& goal:round.horse.goals) count+=goal.active();
    if (count!=24 || !round.horse.goals[23].active()) return fail("test_044_517_capacity");
    round.horse.position=round.game.position;
    round.step();
    if (!round.caught || round.horse.phase!=HorsePhase039::Lost ||
        !capture("loss-ru",Elysian517Course,0,600)) return fail("test_044_517_loss");
    logEvent(log,"test_044_horse_loss",round.game);
    round.restart();
    if (!changeLanguage(executable,course,config,Language::English) ||
        !go(Elysian517Course,"map-en","go-en") || !capture("start-en",Elysian517Course) ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,Elysian517Course,course,round)) return fail("test_044_517_en");
    if (!play(route517_044,true)) return fail("test_044_517_route");
    if (auto* stats=std::fopen(log,"ab")) {
        std::fprintf(stats,"build=044 event=test_044_plinko ready=%u hit_mask=%06x\n",count,goals);
        std::fclose(stats);
    }
    if (goals!=0xfff7ebu || !capture("win-ru",Elysian517Course,0,4700))
        return fail("test_044_517_win");
    logEvent(log,"test_044_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild)
        return fail("test_044_517_advance");
    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.frontierKey) return fail("test_044_save");
    for (unsigned i=0;i<Elysian516Course;++i) {
        const auto& old=seed.best[i];
        const auto& now=saved.best[i];
        if (old.score!=now.score || old.rank!=now.rank || old.time!=now.time)
            return fail("test_044_old_best");
    }
    return true;
}
