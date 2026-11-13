bool runFocused031(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    const auto fail=[&](const char* stage) {
        logEvent(log,stage,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=LightsCourse || progress.totalScore()!=518138 ||
        progress.totalPrisms()!=232 || progress.best[MirrorCourse].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,MirrorCourse,course,round) || round.chuckles.count!=6)
        return fail("test_031_seed_error");
    logEvent(log,"test_031_loaded_030",round.game);
    const auto capture=[&](const char* suffix,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        char name[80];
        std::snprintf(name,sizeof name,"%s-%s-%s.565",russian()?"ru":"en",CourseIds[menuCourse],suffix);
        return dumpFrame(executable,name);
    };

    for (unsigned index=LightsCourse;index<=DogCourse;++index) {
        openLevelSelect(flow);
        const unsigned cell=index-ChuckleParkFirstCourse;
        if (menuCourse!=index || levelSelect.world()!=3 || levelSelect.cell()!=cell ||
            !levelSelect.unlocked(3,cell) || levelSelect.unlocked(3,cell+1) || !capture("map",3000) ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
            !capture("go",3000) || !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090) || selectorGoDown)
            return fail("test_031_selector_error");
        flow.update(0,true,false,false,false);
        if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
            currentCourse!=index || !capture("hole"))
            return fail("test_031_enter_error");
        logEvent(log,"test_031_map_cross",round.game);
        for (unsigned tick=0;tick<5 && !round.caught;++tick) round.step();
        if (round.caught || !capture("flag")) return fail("test_031_flag_error");
        while (round.ticks<1800 && !round.caught) round.step();
        if (!round.caught || !capture("loss",0,1000)) return fail("test_031_loss_error");
        logEvent(log,"test_031_loss",round.game);

        round.restart();
        if (!changeLanguage(executable,course,config,Language::English) || !capture("hole") ||
            !changeLanguage(executable,course,config,Language::Russian))
            return fail("test_031_locale_error");
        if (index==LightsCourse) {
            round.game.position={160,-32};
            round.step();
            if (!round.parkScare.lightsOut || round.parkScare.ashTriggered || !capture("lights-out"))
                return fail("test_031_lights_error");
            const unsigned startAudio=audio.blocks();
            for (unsigned tick=0;tick<120 && audio.blocks()-startAudio<8;++tick) sceDisplayWaitVblankStart();
            if (audio.blocks()-startAudio<8) return fail("test_031_lights_audio_error");
            round.game.position=course.spawn;
            while (round.ticks<1800 && !round.caught) round.step();
            if (!round.caught || !round.parkScare.lightsOut || resultClip(false,true)!=Clip::Loss310Dark ||
                !capture("loss-dark",0,1000)) return fail("test_031_loss_dark_error");
            logEvent(log,"test_031_loss_dark_pcm",round.game);

            chillMode=true;
            round.restart();
            round.game.position={-12,156};
            round.step();
            if (!round.parkScare.ashTriggered || round.parkScare.ashAlpha)
                return fail("test_031_ash_easy_error");
            chillMode=false;
            round.restart();
        } else if (index==ArcadeCourse) {
            if (round.chuckles.count!=22 || !round.keyStage || round.keyStage->keyCount!=3 ||
                round.keyStage->doorCount!=3 || !round.jumpStage || round.jumpStage->count!=3 ||
                !parkArt.pixels()) return fail("test_031_arcade_assets_error");
            chillMode=true;
            round.game.position=ParkRingStages[0].center;
            round.step();
            round.game.position=ParkRingStages[1].center;
            round.step();
            const unsigned redStart=round.arcadeRings[0].remainingTicks(ParkRingStages[0],round.ticks);
            const unsigned greenStart=round.arcadeRings[1].remainingTicks(ParkRingStages[1],round.ticks);
            if (redStart!=599 || greenStart!=1200 || !capture("ring-pair"))
                return fail("test_031_arcade_rings_error");
            const unsigned startAudio=audio.blocks();
            for (unsigned tick=0;tick<120 && audio.blocks()-startAudio<8;++tick) sceDisplayWaitVblankStart();
            if (audio.blocks()-startAudio<8) return fail("test_031_arcade_audio_error");
            round.game.position=round.jumpStage->pads[0].center;
            round.step();
            if (!round.jump.airborne() || round.roundKeys.collected!=1 ||
                round.roundKeys.open(*round.keyStage,1,round.ticks) || !capture("jump-red"))
                return fail("test_031_arcade_jump_error");
            const unsigned before=round.arcadeRings[0].remainingTicks(ParkRingStages[0],round.ticks);
            round.step();
            if (!round.jump.airborne() || round.arcadeRings[0].remainingTicks(ParkRingStages[0],round.ticks)+1!=before)
                return fail("test_031_arcade_air_timer_error");
            chillMode=false;
            round.restart();
            logEvent(log,"test_031_arcade_independent_timers_pcm",round.game);
        } else if (index==PursuitCourse) {
            if (!round.switchStage || round.switchStage->blockCount!=8 ||
                !round.speedStage || round.speedStage.count!=10 ||
                !round.stopStage || round.stopStage->count!=9 ||
                !pursuitMotion || !coasterArt.pixels() || !parkArt.pixels())
                return fail("test_031_pursuit_assets_error");
            chillMode=true;
            round.game.position=ParkPursuitSand.center;
            round.game.velocity={180,0};
            round.step();
            if (!round.inSand || length(round.game.velocity)>=153.0f)
                return fail("test_031_sand_error");
            for (const auto& ring:ParkPursuitRingStages) {
                round.game.position=ring.center;
                round.game.velocity={};
                round.step();
            }
            if (!capture("rings-active")) return fail("test_031_pursuit_rings_frame_error");
            for (unsigned i=0;i<std::size(ParkPursuitRingStages);++i) {
                if (round.pursuitRings[i].phase!=SmileyPhase::Active)
                    return fail("test_031_pursuit_ring_count_error");
            }
            const auto& big=ParkPursuitRingStages[4];
            for (const auto coin:big.coins) {
                round.game.position=coin;
                round.game.velocity={};
                round.step();
            }
            if (round.pursuitRings[4].phase!=SmileyPhase::Passed ||
                !round.pursuitRings[4].collected(18) || !capture("ring-19"))
                return fail("test_031_ring_19_error");
            chillMode=false;
            round.restart();
            logEvent(log,"test_031_pursuit_five_rings_19_coins_sand",round.game);
        } else if (!round.switchStage || round.switchStage->blockCount!=4 ||
                   !dogGate.active() || round.dog.winner || !capture("dog-gate-closed")) {
            return fail("test_031_dog_assets_error");
        }

        constexpr int lights[][2]={{36,-106},{-78,102},{56,0}};
        constexpr int arcade[][2]={{-126,-25},{-122,37},{126,-25},{-37,122},{25,126},
                                   {-122,-37},{-100,67},{-30,56},{-127,13},{104,0}};
        constexpr int pursuit[][2]={{-65,113},{66,-94},{-113,65},{94,34}};
        constexpr int dog[][2]={{44,122},{74,43}};
        const int (*routes[])[2]={lights,arcade,pursuit,dog};
        constexpr unsigned shotCounts[]={std::size(lights),std::size(arcade),std::size(pursuit),std::size(dog)};
        const auto* route=routes[index-LightsCourse];
        const unsigned shotCount=shotCounts[index-LightsCourse];
        unsigned jumpCount=0,ringWins=0,switchCount=0;
        for (unsigned shot=0;shot<shotCount;++shot) {
            const Vec2 aim{route[shot][0]/127.0f,route[shot][1]/127.0f};
            round.controls.update(round.game,course,aim,true,false);
            if (round.controls.update(round.game,course,{},false,false)!=ControlEvent::Shot)
                return fail("test_031_route_shot_error");
            logEvent(log,"test_031_route_shot",round.game);
            for (unsigned tick=0;tick<900 && (round.game.moving() || round.jump.airborne()) &&
                 !round.caught && !round.game.won;++tick) {
                const auto beforeJump=round.jump.airborne();
                const bool beforeLights=round.parkScare.lightsOut,beforeAsh=round.parkScare.ashTriggered;
                const unsigned beforeDoors=round.roundKeys.opened;
                const unsigned beforeSwitch=round.switches.state;
                const bool beforeDog=round.dog.winner;
                round.step();
                if (round.switches.state!=beforeSwitch) {
                    ++switchCount;
                    if (index==PursuitCourse && !capture("switch-changed"))
                        return fail("test_031_switch_frame_error");
                }
                if (index==DogCourse && !beforeDog && round.dog.winner) {
                    if (round.dog.body.radius!=0 || !capture("dog-gate-open"))
                        return fail("test_031_dog_gate_error");
                    logEvent(log,"test_031_dog_opened_gate",round.game);
                }
                if (index==PursuitCourse && round.ticks==460 && !capture("coaster"))
                    return fail("test_031_pursuit_coaster_frame_error");
                if (!beforeJump && round.jump.airborne()) ++jumpCount;
                if (!beforeLights && round.parkScare.lightsOut && !capture("route-dark"))
                    return fail("test_031_dark_frame_error");
                if (!beforeAsh && round.parkScare.ashTriggered) {
                    if (round.parkScare.ashAlpha!=254 || !capture("ash-baby"))
                        return fail("test_031_ash_frame_error");
                }
                if (round.roundKeys.opened!=beforeDoors && !capture("door-open"))
                    return fail("test_031_door_frame_error");
                if (index==ArcadeCourse) {
                    unsigned wins=0;
                    for (const auto& ring:round.arcadeRings) wins+=ring.bonus!=0;
                    if (wins>ringWins && !capture("ring-pass")) return fail("test_031_ring_frame_error");
                    ringWins=wins;
                }
            }
            if (round.caught || round.game.moving() || round.jump.airborne())
                return fail("test_031_route_stop_error");
        }
        if (!round.game.won || !capture("win",0,4700)) return fail("test_031_win_error");
        if (index==LightsCourse) {
            if (!round.parkScare.lightsOut || !round.parkScare.ashTriggered || !round.parkScare.ashAlpha)
                return fail("test_031_scare_route_error");
            logEvent(log,"test_031_lights_ash_goal",round.game);
        } else if (index==ArcadeCourse) {
            if (round.roundKeys.collected!=7 || round.roundKeys.opened!=7 || jumpCount<3 || ringWins!=4 ||
                round.chuckles.count!=22 || round.bonus()<8600)
                return fail("test_031_arcade_route_error");
            audio.stopRingTimers();
            logEvent(log,"test_031_rgb_keys_doors_four_rings_goal",round.game);
        } else if (index==PursuitCourse) {
            unsigned started=0,won=0;
            for (const auto& ring:round.pursuitRings) {
                started+=ring.everStarted;
                won+=ring.bonus!=0;
            }
            if (switchCount<3 || !round.stopRun.collected || !round.speedRun.collected || started!=5 || !won)
                return fail("test_031_pursuit_route_error");
            audio.stopRingTimers();
            logEvent(log,"test_031_pursuit_switch_tokens_rings_goal",round.game);
        } else {
            if (!round.dog.winner || !switchCount || round.dog.graphicScale>=ParkDogScene.graphicScale)
                return fail("test_031_dog_route_error");
            logEvent(log,"test_031_dog_switches_gate_goal",round.game);
        }
        logEvent(log,"test_031_goal",round.game);
        const auto outcome=advanceCampaign(executable,course,round,save,log);
        const auto expected=index==DogCourse?Advance::EndOfBuild:Advance::NextHole;
        Progress saved{};
        if (outcome!=expected || progress.next!=index+1 || !save.read(saved) ||
            saved.next!=index+1 || saved.best[index].time==UINT32_MAX)
            return fail("test_031_advance_error");
    }
    return true;
}
