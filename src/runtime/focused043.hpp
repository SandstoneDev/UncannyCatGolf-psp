#pragma once

struct ElysianStroke043 { unsigned tick; int x,y; };
inline constexpr ElysianStroke043 replay513_043[]={{4,107,-54},{207,-93,-115}};
inline constexpr ElysianStroke043 route514_043[]={{7,-127,-3},{228,-98,-57},{415,109,68}};
inline constexpr ElysianStroke043 route515_043[]={{7,-69,-116},{316,100,-84}};

bool runFocused043(const char* executable,Course& course,SaveFile& save,
                   const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    chillMode=false;
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=Elysian514Course || !progress.frontierKey ||
        progress.totalScore()!=983454 || progress.totalPrisms()!=421 ||
        !changeLanguage(executable,course,config,Language::Russian)) return fail("test_043_seed_error");
    const auto seed=progress;
    round.totalTicks=progress.ticks;
    const auto capture=[&](const char* phase,unsigned index,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        if (result && (round.game.won || round.caught) && !liveResultPrepared) return false;
        const auto filename=std::string("043-")+phase+"-"+CourseIds[index]+".565";
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
    unsigned flips=0,jumps=0,teleports=0,phaseMask=0,nukesPicked=0;
    bool nukeClearedBoth=false;
    const auto play=[&](const auto& strokes,bool captureStates) {
        unsigned shot=0;
        flips=jumps=teleports=nukesPicked=0;
        phaseMask=1;
        nukeClearedBoth=false;
        bool traffic=false,motion=false,nukeCapture=false;
        for (unsigned frame=0;frame<2400 && !round.game.won && !round.caught;++frame) {
            if (shot<std::size(strokes) && round.ticks==strokes[shot].tick) {
                round.updateControls({strokes[shot].x/127.0f,strokes[shot].y/127.0f},true,false);
                if (round.updateControls({},false,false)!=ControlEvent::Shot) return false;
                ++shot;
            }
            const unsigned previous=round.switches.state;
            const unsigned previousJump=round.jump.timer;
            const Vec2 position=round.game.position;
            const unsigned skulls=round.baddies.liveCount[0]+round.baddies.liveCount[1];
            unsigned chuckles=0;
            if (currentCourse==Elysian514Course) {
                for (unsigned i=0;i<round.chuckles.count;++i)
                    chuckles+=round.chuckles.bodies[i].radius>0;
            }
            round.step(false,false,round.horse.flying() && round.horse.age-round.horse.lastHitAt==1);
            flips+=round.switches.state!=previous;
            const unsigned mask=1u<<round.switches.state;
            if (captureStates && currentCourse==Elysian514Course) {
                if (!(phaseMask&mask)) {
                    const std::string tag="switch-ru-"+std::to_string(round.switches.state);
                    if (!capture(tag.c_str(),currentCourse)) return false;
                }
                if (!traffic && skulls && chuckles) {
                    if (!capture("traffic-ru",currentCourse)) return false;
                    traffic=true;
                }
                for (unsigned i=0;i<std::size(ElysianNukes514);++i) {
                    if (!nukeCapture && round.nukes.has(i) && round.ticks-round.nukes.pickedAt[i]==18) {
                        if (!capture("nuke-ru",currentCourse)) return false;
                        nukeCapture=true;
                    }
                    if (!round.nukes.has(i) || round.nukes.pickedAt[i]!=round.ticks || round.horse.flying())
                        continue;
                    ++nukesPicked;
                    if (round.baddies.count || round.chuckles.count) return false;
                    if (!nukeClearedBoth && skulls && chuckles) nukeClearedBoth=true;
                }
            }
            phaseMask|=mask;
            if (captureStates && currentCourse==Elysian515Course) {
                // Launch and teleport are separately exercised below. A teleport travels farther
                // than any one grounded step or jump-pad snap along this route.
                teleports+=length(round.game.position-position)>80.0f;
                if (round.jump.timer==1 && previousJump!=1) {
                    if (!jumps && !capture("jump-ru",currentCourse)) return false;
                    ++jumps;
                }
                if (!motion && round.ticks==150) {
                    if (!capture("motion-ru",currentCourse)) return false;
                    motion=true;
                }
            }
        }
        return shot==std::size(strokes) && round.game.won && !round.caught && !round.horse.health &&
            (!captureStates || (currentCourse==Elysian514Course?traffic && nukeClearedBoth && nukeCapture:motion));
    };
    flow.screen=Screen::Hole;
    if (!enterCourse(executable,Elysian513Course,course,round) || !play(replay513_043,false) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=Elysian514Course) return fail("test_043_previous_advance");

    if (!go(Elysian514Course,"map-ru","go-ru") || !capture("start-ru",Elysian514Course) ||
        round.horse.health!=8 || !round.switchStage || round.switchStage->blockCount!=4 ||
        round.switchStage->layerCount!=4 || round.switches.state || round.chuckles.spawners()!=2 ||
        !round.baddies.stage || round.baddies.stage->spawnerCount!=2 || clips.count(Clip::Win514) ||
        clips.count(Clip::Loss514)) return fail("test_043_514_setup");
    round.horse.position=round.game.position;
    round.step();
    if (!round.caught || round.horse.phase!=HorsePhase039::Lost ||
        !capture("loss-ru",Elysian514Course,0,600)) return fail("test_043_514_loss");
    logEvent(log,"test_043_horse_loss",round.game);
    round.restart();
    if (!changeLanguage(executable,course,config,Language::English) ||
        !go(Elysian514Course,"map-en","go-en") || !capture("start-en",Elysian514Course) ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,Elysian514Course,course,round)) return fail("test_043_514_en");
    if (!play(route514_043,true)) return fail("test_043_514_route");
    if (auto* stats=std::fopen(log,"ab")) {
        std::fprintf(stats,"build=043 event=test_043_switch_nuke flips=%u phase_mask=%u nukes=%u\n",
                     flips,phaseMask,nukesPicked);
        std::fclose(stats);
    }
    if (phaseMask!=15 || flips<4 || !nukesPicked ||
        !capture("win-ru",Elysian514Course,0,4700)) return fail("test_043_514_win");
    logEvent(log,"test_043_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=Elysian515Course || progress.next!=Elysian515Course) return fail("test_043_514_advance");

    if (!go(Elysian515Course,"map-ru","go-ru") || !capture("start-ru",Elysian515Course) ||
        round.horse.health!=3 || round.jumpStage.count!=7 || round.elysianPortals515[2].position.x!=160 ||
        clips.count(Clip::Win515) || clips.count(Clip::Loss515)) return fail("test_043_515_setup");
    round.game.position={18,350};
    round.game.velocity={0,600};
    round.step();
    if (round.game.bounces!=1 || round.game.velocity.y>=0 ||
        std::abs(round.game.position.y-356.2f)>0.01f) return fail("test_043_515_rock");
    logEvent(log,"test_043_rock",round.game);
    round.restart();
    round.game.position={32,256};
    round.step();
    if (length(round.game.position-round.elysianPortals515[1].position)>0.01f)
        return fail("test_043_515_portal_ab");
    round.step();
    if (round.game.position.y>0) return fail("test_043_515_portal_cooldown");
    round.restart();
    round.game.position={160,-416};
    round.step();
    if (length(round.game.position-round.elysianPortals515[3].position)>0.01f)
        return fail("test_043_515_portal_cd");
    round.step();
    if (length(round.game.position-round.elysianPortals515[3].position)>0.01f)
        return fail("test_043_515_portal_exit");
    round.restart();
    round.game.position=round.elysianPads515.pads[0].center;
    round.step();
    if (!round.jump.airborne()) return fail("test_043_515_launch");
    round.game.position=round.elysianPortals515[0].position;
    round.step();
    if (!round.jump.airborne() || round.game.position.y<200) return fail("test_043_515_air_portal");
    round.game.position={course.right+JumpRun::OutOfBoundsMargin+1,0};
    round.step();
    if (!round.caught || !round.fellOut || round.horse.phase!=HorsePhase039::Lost ||
        !capture("ob-ru",Elysian515Course,0,400)) return fail("test_043_515_ob");
    logEvent(log,"test_043_portals_jumps_ob",round.game);
    round.restart();
    if (!changeLanguage(executable,course,config,Language::English) ||
        !go(Elysian515Course,"map-en","go-en") || !capture("start-en",Elysian515Course) ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,Elysian515Course,course,round)) return fail("test_043_515_en");
    startCourseMusic(currentCourse);
    const auto pcmBefore=audio.blocks();
    if (!play(route515_043,true)) return fail("test_043_515_route");
    if (auto* stats=std::fopen(log,"ab")) {
        std::fprintf(stats,"build=043 event=test_043_route_motion jumps=%u teleports=%u\n",jumps,teleports);
        std::fclose(stats);
    }
    if (jumps<2 || teleports<2 || !capture("win-ru",Elysian515Course,0,4700))
        return fail("test_043_515_win");
    sceKernelDelayThread(300000);
    const auto pcmBlocks=audio.blocks()-pcmBefore;
    if (!pcmBlocks) return fail("test_043_audio_output");
    if (auto* stats=std::fopen(log,"ab")) {
        std::fprintf(stats,"build=043 event=test_043_pcm blocks=%u\n",pcmBlocks);
        std::fclose(stats);
    }
    logEvent(log,"test_043_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild)
        return fail("test_043_515_advance");
    Progress saved{};
    if (!save.read(saved) || saved.next!=CourseCount || !saved.frontierKey) return fail("test_043_save");
    for (unsigned i=0;i<Elysian514Course;++i) {
        const auto& old=seed.best[i];
        const auto& now=saved.best[i];
        if (old.score!=now.score || old.rank!=now.rank || old.time!=now.time)
            return fail("test_043_old_best");
    }
    return true;
}
