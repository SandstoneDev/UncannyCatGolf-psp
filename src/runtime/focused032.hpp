bool runFocused032(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    const auto fail=[&](const char* event) {
        logEvent(log,event,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=DoubleDogCourse || progress.totalScore()!=592855 ||
        progress.totalPrisms()!=259 || !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,ScreenDogCourse,course,round))
        return fail("test_032_extra_seed_error");
    logEvent(log,"test_032_extra_loaded_56",round.game);
    const auto capture=[&](const char* suffix,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        char name[80];
        std::snprintf(name,sizeof name,"%s-%s-%s.565",russian()?"ru":"en",CourseIds[menuCourse],suffix);
        return dumpFrame(executable,name);
    };
    for (unsigned index=DoubleDogCourse;index<=RacetrackCourse;++index) {
        const bool race=index==RacetrackCourse;
        openLevelSelect(flow);
        const unsigned cell=index-ChuckleParkFirstCourse;
        if (menuCourse!=index || levelSelect.world()!=3 || levelSelect.cell()!=cell ||
            !levelSelect.unlocked(3,cell) || levelSelect.unlocked(3,cell+1) || !capture("map",3000) ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
            !capture("go",3000) || !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090) || selectorGoDown)
            return fail("test_032_extra_selector_error");
        flow.update(0,true,false,false,false);
        if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
            currentCourse!=index || !capture("hole"))
            return fail("test_032_extra_enter_error");
        logEvent(log,"test_032_extra_map_cross",round.game);
        for (unsigned tick=0;tick<5 && !round.caught;++tick) round.step();
        if (round.caught || !capture("flag")) return fail("test_032_extra_flag_error");
        while (round.ticks<1800 && !round.caught) round.step();
        if (!round.caught || !capture("loss",0,1000)) return fail("test_032_extra_loss_error");
        logEvent(log,"test_032_extra_loss",round.game);
        round.restart();
        if (!changeLanguage(executable,course,config,Language::English) || !capture("hole") ||
            !changeLanguage(executable,course,config,Language::Russian))
            return fail("test_032_extra_locale_error");

        const auto* stage=multiDogStage(index);
        if (!stage || round.dogs.openMask || !round.dogs.activeGate(*stage,dualGateLayers) ||
            !capture("gate-closed")) return fail("test_032_extra_gate_error");
        chillMode=true;
        if (race) {
            if (stage->dogCount!=3 || round.extraGhostCount!=2 || !round.jumpStage ||
                !round.keyStage || !round.switchStage || round.chuckles.spawners()!=1)
                return fail("test_032_extra_race_assets_error");
            round.game.position=ParkSwitchBlocks317[0].center-Vec2{42,0};
            round.game.velocity={850,0};
            for (unsigned tick=0;tick<12 && !round.switches.state;++tick) round.step();
            if (round.switches.state!=1 || !capture("switch-active"))
                return fail("test_032_extra_switch_error");
            round.restart();
            round.game.position={-400,100};
            round.game.velocity={200,0};
            round.step();
            if (!round.inSand || length(round.game.velocity)>=170.0f)
                return fail("test_032_extra_sand_error");
            logEvent(log,"test_032_extra_race_switch_and_sand",round.game);
        } else {
            if (stage->dogCount!=2 || courseHazards(index).skullCount!=6 || round.chuckles.spawners()!=3)
                return fail("test_032_extra_double_assets_error");
            while (round.ticks<300 && !round.caught) round.step();
            if (round.caught || round.chuckles.count!=3 || !capture("spawned"))
                return fail("test_032_extra_spawn_error");
            round.restart();
            round.game.position=ParkRings316[0].center;
            round.step();
            if (!round.dogRings[0].everStarted || !capture("ring-active"))
                return fail("test_032_extra_ring_error");
            logEvent(log,"test_032_extra_double_spawners_and_ring",round.game);
        }
        chillMode=false;
        round.restart();

        constexpr int doubleDog[][2]={{124,45},{-19,106}};
        constexpr int racetrack[][2]={{34,127},{94,54},{-66,-114},{21,-56},{60,-104}};
        const auto* route=race?racetrack:doubleDog;
        const unsigned count=race?std::size(racetrack):std::size(doubleDog);
        unsigned gateWins=0, jumps=0, dogHits=0;
        for (unsigned tick=0;!race && tick<13;++tick) round.step();
        for (unsigned shot=0;shot<count;++shot) {
            const Vec2 aim{route[shot][0]/127.0f,route[shot][1]/127.0f};
            round.controls.update(round.game,course,aim,true,false);
            if (round.controls.update(round.game,course,{},false,false)!=ControlEvent::Shot)
                return fail("test_032_extra_route_shot_error");
            logEvent(log,"test_032_extra_shot",round.game);
            for (unsigned tick=0;tick<1100 && (round.game.moving() || round.jump.airborne()) &&
                 !round.caught && !round.game.won;++tick) {
                const unsigned gate=round.dogs.openMask, door=round.roundKeys.opened;
                const bool beforeJump=round.jump.airborne();
                round.step();
                if (round.game.passiveBounce && !round.jump.airborne()) ++dogHits;
                if (round.jump.airborne() && !beforeJump) ++jumps;
                if (gate!=round.dogs.openMask) {
                    gateWins|=round.dogs.openMask;
                    if (!capture(round.dogs.openMask==3?"gate-open":"one-gate"))
                        return fail("test_032_extra_gate_frame_error");
                    logEvent(log,round.dogs.openMask==3?"test_032_extra_two_gates":"test_032_extra_one_gate",round.game);
                }
                if (door!=round.roundKeys.opened && !capture("keys-open"))
                    return fail("test_032_extra_key_frame_error");
            }
            if (round.caught || round.game.moving() || round.jump.airborne())
                return fail("test_032_extra_route_stop_error");
        }
        if (!round.game.won || gateWins!=3 || !capture("win",0,4700)) return fail("test_032_extra_goal_error");
        if (race) {
            if (round.roundKeys.collected!=3 || round.roundKeys.opened!=3 || round.roundKeys.bonus!=300 ||
                !jumps || !round.stopRun.collected || round.speedRun.collected!=3 || !round.dogRings[0].everStarted)
                return fail("test_032_extra_race_route_error");
            logEvent(log,"test_032_extra_race_keys_jump_and_tokens",round.game);
        } else {
            if (!dogHits || !round.dogRings[0].everStarted) return fail("test_032_extra_double_route_error");
            logEvent(log,"test_032_extra_double_dog_contacts",round.game);
        }
        audio.stopRingTimers();
        logEvent(log,"test_032_extra_goal",round.game);
        const auto outcome=advanceCampaign(executable,course,round,save,log);
        Progress saved{};
        if (outcome!=(race?Advance::EndOfBuild:Advance::NextHole) || progress.next!=index+1 ||
            !save.read(saved) || saved.next!=index+1 || saved.best[index].time==UINT32_MAX)
            return fail("test_032_extra_advance_error");
    }
    return true;
}
