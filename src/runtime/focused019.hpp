bool runFocused019(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,
                   const BackgroundFile& backgrounds,const char* log) {
    constexpr unsigned first=GlowstickFirstCourse+5;
    constexpr unsigned last=first+1;
    Flow flow;
    Round round(course);
    constexpr unsigned ReserveBytes=512*1024, ProbeStride=4096;
    auto* const reserve=static_cast<unsigned char*>(std::malloc(ReserveBytes));
    if (!reserve) {
        logEvent(log,"test_025_heap_reserve_failed",round.game);
        return false;
    }
    auto* const touch=static_cast<volatile unsigned char*>(reserve);
    for (unsigned offset=0;offset<ReserveBytes;offset+=ProbeStride)
        touch[offset]=static_cast<unsigned char>(offset/ProbeStride);
    bool reserveOk=true;
    for (unsigned offset=0;offset<ReserveBytes;offset+=ProbeStride)
        reserveOk&=touch[offset]==static_cast<unsigned char>(offset/ProbeStride);
    std::free(reserve);
    if (!reserveOk) return false;
    logEvent(log,"test_025_heap_reserve_512k",round.game);
    const auto* initialFallback=fallbackBackdrop(static_cast<unsigned>(Screen::Title));
    if (!initialFallback || initialFallback!=fallbackBackdrop(static_cast<unsigned>(Screen::Title))) return false;
    logEvent(log,"test_025_static_fallback",round.game);
    if (!save.read(progress) || progress.next!=first ||
        !changeLanguage(executable,course,config,Language::Russian)) return false;
    const unsigned originalBackground=activeBackground;
    const uint32_t oldSetting=backgrounds.read();
    const auto settingsInput=[&](uint32_t buttons) {
        updateSettings(flow,buttons,executable,course,config,backgrounds,log,round.game);
    };
    flow.screen=Screen::Settings;
    beginSettingsDraft();
    settingsInput(PSP_CTRL_DOWN);
    settingsInput(PSP_CTRL_RIGHT);
    if (settingsError || settingsPreviewError || activeBackground!=2 ||
        backgrounds.read()!=oldSetting) return false;
    settingsInput(PSP_CTRL_CIRCLE);
    if (activeBackground!=originalBackground || backgrounds.read()!=oldSetting) return false;
    beginSettingsDraft();
    settingsInput(PSP_CTRL_DOWN);
    settingsInput(PSP_CTRL_RIGHT);
    settingsInput(PSP_CTRL_CROSS);
    if (settingsError || flow.screen!=Screen::Main || activeBackground!=2 ||
        backgrounds.read()!=BackgroundPreference{2,2,false}.encode()) return false;
    logEvent(log,"test_background_preview_then_save",round.game);
    hardwareProbe.checkStorage(executable);
    if (hardwareProbe.spareFiles!=4 || hardwareProbe.opened || hardwareProbe.written!=4 ||
        hardwareProbe.closed || hardwareProbe.stdioError || hardwareProbe.removed) {
        logEvent(log,"test_storage_probe_failed",round.game);
        return false;
    }
    logEvent(log,"test_storage_probe",round.game);
    Progress legacy{};
    if (!profiles.save(Profiles::Legacy).read(legacy) || legacy.next!=25 ||
        legacy.totalScore()!=244909 || legacy.totalPrisms()!=118) return false;
    logEvent(log,"test_017_save_loaded",round.game);
    ProfileName title{},player{};
    std::snprintf(title.data(),title.size(),"Тест");
    std::snprintf(player.data(),player.size(),"Кот");
    const unsigned newSlot=profiles.create(title,player);
    if (newSlot==Profiles::None || !profiles.entry(newSlot).found ||
        profiles.entry(newSlot).title!=title || profiles.entry(newSlot).player!=player ||
        !profiles.save(newSlot).read(legacy) || legacy.next!=0) return false;
    logEvent(log,"test_profile_written_and_reopened",round.game);
    flow.screen=Screen::ProfileCreate;
    profileField=ProfileField::Title;
    render(flow,course,round,0);
    if (!dumpFrame(executable,"profile-create-readable.565")) return false;
    flow.screen=Screen::GameMenu;
    render(flow,course,round,0);
    if (!dumpFrame(executable,"game-menu-readable.565")) return false;
    logEvent(log,"test_018_save_loaded",round.game);

    const auto capture=[&](const char* name,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        return dumpFrame(executable,name);
    };
    const auto captureFlag=[&](unsigned index,const char* first,const char* second) {
        Patch area{};
        const unsigned signal=index==BankCourse?1:0;
        const unsigned fieldPhase=index>=LightboxCourse?6:0;
        const auto* a=goalFlags.frame(flagSlot(index),signal,fieldPhase,0,area);
        const auto* b=goalFlags.frame(flagSlot(index),signal,fieldPhase,5,area);
        round.police.triggeredAt=signal;
        round.ticks=signal;
        if (!a || !b || std::memcmp(a,b,GoalFlagArt::Size*GoalFlagArt::Size*sizeof(uint16_t))==0 ||
            !capture(first)) return false;
        round.ticks=signal+5;
        const bool captured=capture(second);
        round.police.triggeredAt=0;
        round.ticks=0;
        return captured;
    };
    thoughts.beginMenu(0,0);
    constexpr Screen menuScreens[]={Screen::Title,Screen::Main,Screen::GameMenu};
    constexpr const char* menuTvFrames[]={"menu-tv-title.565","menu-tv-main.565","menu-tv-game.565"};
    for (unsigned index=0;index<std::size(menuScreens);++index) {
        flow.screen=menuScreens[index];
        if (index==2) thoughts.begin(1000,0);
        render(flow,course,round,1500,0,0,0,18+6*index);
        if (!dumpFrame(executable,menuTvFrames[index])) return false;
    }
    logEvent(log,"test_026_menu_tv_continues",round.game);
    for (const auto screen:{Screen::Main,Screen::GameMenu}) {
        flow.screen=screen;
        const char* page=screen==Screen::Main?"main":"game";
        for (const unsigned ms:{0,2350,2400,39950,40000,40050}) {
            char name[40];
            std::snprintf(name,sizeof name,"menu-scroll-%s-%u.565",page,ms);
            render(flow,course,round,1500,0,0,0,30,ms);
            if (!dumpFrame(executable,name)) return false;
        }
    }
    logEvent(log,"test_026_menu_scroll_period",round.game);
    flow.screen=Screen::Main;
    render(flow,course,round,0,0,0,0,30,2400);
    if (!dumpFrame(executable,"menu-back-0.565")) return false;
    render(flow,course,round,500,0,0,0,30,2400);
    if (!dumpFrame(executable,"menu-back-500.565")) return false;
    menuButtonDown=true;
    render(flow,course,round,500,0,0,0,30,2400);
    menuButtonDown=false;
    if (!dumpFrame(executable,"menu-button-pressed.565")) return false;

    flow.screen=Screen::Hole;
    constexpr unsigned SandCourse=TutorialCount+6;
    if (!enterCourse(executable,SandCourse,course,round) ||
        !captureFlag(SandCourse,"russian-1_07-flag0.565","russian-1_07-flag1.565")) return false;
    round.game.position={-112,-64};
    round.ghost.position={60,-64};
    if (!capture("russian-1_07-actors-before.565")) return false;
    round.game.position={-48,-64};
    round.ghost.position={60,8};
    if (!capture("russian-1_07-actors-after.565")) return false;
    logEvent(log,"test_026_1_07_actors_and_flag",round.game);
    if (!enterCourse(executable,0,course,round)) return false;

    flow.screen=Screen::Hole;
    const auto launchSpeed=[&](uint8_t x,uint8_t y) {
        Game ball(course,&walls);
        Controls controls;
        SceCtrlData pad{};
        pad.Lx=x;
        pad.Ly=y;
        controls.update(ball,course,aimInput(pad),true,false);
        if (controls.update(ball,course,aimInput(pad),false,false)!=ControlEvent::Shot) return 0.0f;
        return length(ball.velocity);
    };
    const float weak=launchSpeed(163,128),medium=launchSpeed(192,128),full=launchSpeed(255,128);
    if (launchSpeed(128,128)!=0 || launchSpeed(162,128)!=0 ||
        weak<100 || weak>110 || medium<=weak || full!=course.maxDrag*course.dragMultiplier ||
        full<=medium) return false;
    {
        Game ball(course,&walls);
        Controls controls;
        controls.update(ball,course,{1,0},true,false);
        controls.update(ball,course,{0,0},true,false);
        if (controls.update(ball,course,{0,0},false,false)!=ControlEvent::None || ball.strokes) return false;
    }
    if (auto* file=std::fopen(log,"a")) {
        std::fprintf(file,"build=028 event=test_024_shot_power weak=%.2f medium=%.2f full=%.2f\n",
                     weak,medium,full);
        std::fclose(file);
    }
    round.togglePause();
    using PauseChoice=PauseMenuState::Choice;
    if (!round.paused || !capture("russian-pause.565") ||
        round.pauseMenu.update(PSP_CTRL_DOWN,0,false)!=PauseChoice::None ||
        round.pauseMenu.choice!=PauseChoice::Settings ||
        round.pauseMenu.update(PSP_CTRL_CROSS,0,false)!=PauseChoice::None ||
        !round.pauseMenu.crossArmed || !capture("russian-pause-settings-pressed.565") ||
        round.pauseMenu.update(0,PSP_CTRL_CROSS,false)!=PauseChoice::Settings) return false;
    round.cleanPause=true;
    round.pauseMenu.elapsedMs=2010;
    if (round.pauseMenu.update(PSP_CTRL_CROSS,0,true)!=PauseChoice::None ||
        round.pauseMenu.crossArmed || !capture("russian-pause-clean.565")) return false;
    round.togglePause();
    if (round.paused || round.cleanPause || round.ticks) return false;
    logEvent(log,"test_024_original_pause",round.game);
    const auto captureTv=[&](const char* category) {
        for (unsigned i=0;i<4;++i) {
            char filename[48];
            std::snprintf(filename,sizeof filename,"russian-tv-%s-%u.565",category,i);
            round.totalTicks=18+18*i;
            if (!capture(filename)) return false;
        }
        return true;
    };
    thoughts.begin(0,0);
    if (!captureTv("idle")) return false;
    thoughts.play(Thought::Moving,0,0);
    if (!captureTv("moving")) return false;
    round.totalTicks=0;
    round.togglePause();
    thoughts.begin(0,0);
    thoughts.play(Thought::Moving,0,0);
    for (unsigned i=0;i<4;++i) {
        char filename[48];
        std::snprintf(filename,sizeof filename,"russian-tv-paused-%u.565",i);
        round.tvPausedTicks=12+3*i;
        if (!capture(filename) || round.ticks || round.totalTicks) return false;
    }
    round.togglePause();
    if (round.tvTick()!=21) return false;
    round.restart();
    logEvent(log,"test_024_tv_idle_and_moving",round.game);
    openLevelSelect(flow);
    if (levelSelect.world()!=2 || menuCourse!=first || levelSelect.unlocked(2,6) ||
        moveLevelSelection(PSP_CTRL_DOWN,0) || !capture("selector-world2-before.565")) return false;

    if (!moveLevelSelection(PSP_CTRL_LTRIGGER,100) || levelSelect.world()!=1 ||
        !selectorTravelling(100) || !capture("selector-world2-to-1-t00.565",100) ||
        !capture("selector-world2-to-1-t04.565",234) ||
        !capture("selector-world2-to-1-t10.565",434) ||
        !capture("selector-world2-to-1-t18.565",700) ||
        updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,450) ||
        updateLevelSelector(0,PSP_CTRL_CROSS,0,500) ||
        !capture("selector-world1.565",900)) return false;
    if (!moveLevelSelection(PSP_CTRL_LTRIGGER,1000) || levelSelect.world()!=0 ||
        !capture("selector-world1-to-0-t04.565",1134) ||
        !moveLevelSelection(PSP_CTRL_RTRIGGER,1900) || levelSelect.world()!=1 ||
        !moveLevelSelection(PSP_CTRL_RTRIGGER,2800) || levelSelect.world()!=2 ||
        !capture("selector-world1-to-2-t04.565",2934) ||
        !capture("selector-world1-to-2-t10.565",3134) ||
        !capture("selector-world1-to-2-t18.565",3400) ||
        !capture("selector-world2-after.565",3600) || menuCourse!=first) return false;
    logEvent(log,"test_selector_pan_both_directions",round.game);

    if (updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3630) ||
        !selectorGoDown || !capture("selector-go-pressed.565",3630) ||
        !updateLevelSelector(0,PSP_CTRL_CROSS,0,3690) || selectorGoDown) return false;
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
        currentCourse!=first) return false;
    logEvent(log,"test_selector_direct_cross_starts_2_06",round.game);

    const auto startShot=[&](Vec2 aim) {
        round.controls.update(round.game,course,aim,true,false);
        if (round.controls.update(round.game,course,{},false,false)!=ControlEvent::Shot) return false;
        round.coins.multiplier=0;
        logEvent(log,"test_route_shot",round.game);
        return true;
    };
    const auto shoot=[&](Vec2 aim,unsigned maxTicks,const char* frame=nullptr,unsigned captureAt=0) {
        if (!startShot(aim)) return false;
        for (unsigned tick=0;tick<maxTicks && round.game.moving() && !round.caught && !round.game.won;++tick) {
            round.step();
            if (frame && tick+1==captureAt && !capture(frame)) return false;
        }
        return !round.caught && (round.game.won || !round.game.moving());
    };
    round.restart();
    if (!capture("russian-2_06.565") ||
        !shoot({122/127.0f,67/127.0f},420,"russian-2_06-skull.565",130) || !round.game.won) {
        logEvent(log,"test_2_06_route_failed",round.game);
        return false;
    }
    if (round.game.strokes!=1 || round.game.bounces!=17 || round.brokenBlocks!=7 ||
        round.speedRun.collected!=3) return false;
    logEvent(log,"test_2_06_blocks_boosts_and_goal",round.game);
    if (!capture("russian-2_06-win.565",0,2500) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole || currentCourse!=last ||
        progress.next!=last) return false;

    openLevelSelect(flow);
    if (menuCourse!=last || !levelSelect.unlocked(2,6) || !capture("selector-2_07-unlocked.565") ||
        updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3630) || !selectorGoDown ||
        !capture("selector-go-2_07-pressed.565",3630) ||
        !updateLevelSelector(0,PSP_CTRL_CROSS,0,3690) || selectorGoDown) return false;
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
        currentCourse!=last) return false;
    logEvent(log,"test_selector_direct_cross_starts_2_07",round.game);
    round.restart();
    if (!capture("russian-2_07.565") || !shoot({43/127.0f,97/127.0f},210) ||
        round.game.won || round.stopRun.collected!=2 || round.game.bounces!=6) {
        logEvent(log,"test_2_07_first_shot_failed",round.game);
        return false;
    }
    if (!capture("russian-2_07-traffic.565") || !shoot({76/127.0f,0},210) ||
        !round.game.won || round.game.strokes!=2 || round.game.bounces!=7) {
        logEvent(log,"test_2_07_second_shot_failed",round.game);
        return false;
    }
    logEvent(log,"test_2_07_goal",round.game);
    if (!capture("russian-2_07-win.565",0,2500) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=DimlitCourse) return false;
    Progress saved{};
    if (!save.read(saved) || saved.next!=DimlitCourse || !saved.best[first].score ||
        !saved.best[last].score || config.read()!=Language::Russian ||
        backgrounds.read()!=backgroundPreference.encode()) return false;
    logEvent(log,"test_019_save_reloaded",round.game);

    if (!round.stopStage || round.stopStage->count!=5 ||
        !round.speedStage || round.speedStage.count!=3 ||
        objects.data.courses[DimlitCourse].count!=14 || courseHazards(DimlitCourse).blockCount!=3 ||
        !capture("russian-2_08.565") || !shoot({21/127.0f,120/127.0f},210) ||
        round.game.won || round.stopRun.collected!=1 || round.brokenBlocks!=3 ||
        !round.baddies.count || !round.baddies.alive(0) || round.baddies.velocities[0].x!=0 ||
        round.baddies.velocities[0].y!=-220) {
        logEvent(log,"test_2_08_first_shot_failed",round.game);
        return false;
    }
    logEvent(log,"test_2_08_blocks_tokens_car",round.game);
    if (!shoot({-96/127.0f,75/127.0f},210,"russian-2_08-traffic.565",30) ||
        !round.game.won || round.game.strokes!=2 || round.brokenBlocks!=7 ||
        round.speedRun.collected!=2 || !round.baddies.defeatedAt[0]) {
        logEvent(log,"test_2_08_second_shot_failed",round.game);
        return false;
    }
    logEvent(log,"test_2_08_goal",round.game);
    if (!capture("russian-2_08-win.565",0,2500) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=MuggingCourse || progress.next!=MuggingCourse) return false;

    if (!round.stopStage || round.stopStage->count!=9 ||
        !round.speedStage || round.speedStage.count!=9 || round.extraGhostCount!=13 ||
        !capture("russian-2_09.565")) return false;
    constexpr float catchRadius=27.9655f;
    const auto withinCatchRange=[&](const GhostMotion& ghost) {
        return length(ghost.position-round.game.position)<=catchRadius;
    };
    if (shoot({55/127.0f,-1},20) || !round.caught || !round.ticks || round.ticks>20 ||
        withinCatchRange(round.ghost) || !withinCatchRange(round.extraGhosts[12])) {
        logEvent(log,"test_2_09_catch_route_failed",round.game);
        return false;
    }
    for (unsigned i=0;i<12;++i)
        if (withinCatchRange(round.extraGhosts[i])) return false;
    logEvent(log,"test_2_09_last_ghost_catch",round.game);
    if (!capture("russian-2_09-caught.565",0,1300)) return false;

    round.restart();
    if (!shoot({118/127.0f,-29/127.0f},210,"russian-2_09-ghosts.565",20) ||
        round.game.won || round.stopRun.collected!=1 ||
        !shoot({-34/127.0f,-41/127.0f},210) || !round.game.won ||
        round.game.strokes!=2 || length(round.ghost.velocity)==0) {
        logEvent(log,"test_2_09_goal_route_failed",round.game);
        return false;
    }
    for (unsigned i=0;i<round.extraGhostCount;++i)
        if (length(round.extraGhosts[i].velocity)==0) return false;
    logEvent(log,"test_2_09_goal",round.game);
    if (!capture("russian-2_09-win.565",0,2500) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=JumpCourse || progress.next!=JumpCourse) return false;
    if (!save.read(saved) || saved.next!=JumpCourse || !saved.best[DimlitCourse].score ||
        !saved.best[MuggingCourse].score || config.read()!=Language::Russian ||
        backgrounds.read()!=backgroundPreference.encode()) return false;
    logEvent(log,"test_022_save_reloaded",round.game);

    if (!captureFlag(JumpCourse,"russian-2_10-flag0.565","russian-2_10-flag1.565")) return false;
    if (!round.jumpStage || round.jumpStage->count!=9 || !round.speedStage || round.speedStage.count!=5 ||
        !capture("russian-2_10.565") || !startShot({0,1})) return false;
    bool crossedWall=false;
    for (unsigned tick=1;tick<=65;++tick) {
        const Vec2 before=round.game.position;
        const bool airborne=round.jump.airborne();
        round.step();
        if (round.caught || round.game.won) return false;
        if (!crossedWall && airborne && round.jump.airborne() &&
            walls.sweep(before,round.game.position-before,course.playerRadius).time<=1.0f) {
            crossedWall=true;
            logEvent(log,"test_2_10_jump_crosses_wall",round.game);
        }
        if (tick==48 && (!round.jump.airborne() || round.jump.scale<=JumpRun::BaseScale ||
            !capture("russian-2_10-jump.565"))) return false;
    }
    if (!crossedWall || round.jump.airborne() || round.jump.deathBuffer!=4 || round.game.bounces!=0)
        return false;
    logEvent(log,"test_2_10_jump_landed",round.game);

    // The winning route starts from the original spawn.
    round.restart();
    if (!shoot({-124/127.0f,19/127.0f},420) || !round.game.won ||
        round.game.strokes!=1 || round.speedRun.collected!=0x1d) {
        logEvent(log,"test_2_10_goal_route_failed",round.game);
        return false;
    }
    logEvent(log,"test_2_10_goal",round.game);
    if (!capture("russian-2_10-win.565",0,2500) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=BankCourse || progress.next!=BankCourse) return false;

    if (!captureFlag(BankCourse,"russian-2_11-flag0.565","russian-2_11-flag1.565")) return false;
    if (!round.jumpStage || round.jumpStage->count!=3 || !round.speedStage || round.speedStage.count!=3 ||
        !round.keyStage || round.keyStage->keyCount!=1 || round.keyStage->doorCount!=1 ||
        objects.data.courses[BankCourse].count!=72 || round.police.triggeredAt || round.baddies.stage ||
        !capture("russian-2_11.565")) return false;
    if (!shoot({-100/127.0f,78/127.0f},400,"russian-2_11-police.565",198) || round.game.won ||
        round.roundKeys.collected!=1 || round.roundKeys.opened!=1 || round.roundKeys.bonus!=200 ||
        !round.police.triggeredAt || !round.baddies.count ||
        round.baddies.doorOpenedAt[0]!=round.police.triggeredAt+10) {
        logEvent(log,"test_2_11_key_door_police_failed",round.game);
        return false;
    }
    logEvent(log,"test_2_11_key_door_police",round.game);
    if (!shoot({37/127.0f,-60/127.0f},400) || round.game.won || round.brokenBlocks!=1) {
        logEvent(log,"test_2_11_block_route_failed",round.game);
        return false;
    }
    logEvent(log,"test_2_11_block_broken",round.game);
    if (!shoot({105/127.0f,71/127.0f},400) || !round.game.won || round.game.strokes!=3) {
        logEvent(log,"test_2_11_goal_route_failed",round.game);
        return false;
    }
    unsigned coins=0;
    for (unsigned i=0;i<objects.data.courses[BankCourse].count;++i) coins+=round.coins.has(i);
    if (coins!=53 || round.game.bounces!=32 || round.speedRun.collected!=3) return false;
    logEvent(log,"test_2_11_goal",round.game);
    if (!capture("russian-2_11-win.565",0,2500) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=HighwayCourse || progress.next!=HighwayCourse) return false;
    if (!save.read(saved) || saved.next!=HighwayCourse || !saved.best[JumpCourse].score ||
        !saved.best[BankCourse].score || config.read()!=Language::Russian ||
        backgrounds.read()!=backgroundPreference.encode()) return false;
    logEvent(log,"test_023_save_reloaded",round.game);
    if (round.police.triggeredAt || round.roundKeys.collected || round.roundKeys.opened ||
        round.brokenBlocks || round.jump.airborne()) return false;
    logEvent(log,"test_2_11_police_reset",round.game);

    if (!captureFlag(HighwayCourse,"russian-2_12-flag0.565","russian-2_12-flag1.565")) return false;
    if (!round.baddies.stage || round.baddies.stage->spawnerCount!=4 ||
        !capture("russian-2_12.565")) return false;
    for (unsigned tick=1;tick<=135;++tick) {
        round.step();
        if (round.caught || round.game.won) return false;
    }
    constexpr unsigned firstCars[]={40,60,80,100};
    constexpr float laneY[]={-256,-160,-64,32};
    if (round.baddies.count!=4) return false;
    for (unsigned i=0;i<std::size(firstCars);++i) {
        const Vec2 velocity=round.baddies.velocities[i];
        if (round.baddies.doorOpenedAt[i]!=firstCars[i] || !round.baddies.alive(i) ||
            round.baddies.center(i).y!=laneY[i] || velocity.x!=130 || velocity.y!=0) return false;
    }
    if (!capture("russian-2_12-traffic.565")) return false;
    logEvent(log,"test_2_12_four_car_lanes",round.game);
    hardwareProbe.checkStorage(executable);
    if (hardwareProbe.spareFiles!=4 || hardwareProbe.opened || hardwareProbe.written!=4 ||
        hardwareProbe.closed || hardwareProbe.stdioError || hardwareProbe.removed) return false;
    logEvent(log,"test_024_ticker_descriptors",round.game);

    round.restart();
    if (!shoot({-119/127.0f,43/127.0f},480) || round.game.won ||
        !shoot({115/127.0f,8/127.0f},480) || !round.game.won ||
        round.game.strokes!=2 || round.baddies.defeatedCount!=1) {
        logEvent(log,"test_2_12_route_failed",round.game);
        return false;
    }
    logEvent(log,"test_2_12_car_hit",round.game);
    logEvent(log,"test_2_12_goal",round.game);
    if (!capture("russian-2_12-win.565",0,2500) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=CityCourse || progress.next!=CityCourse) return false;

    if (!captureFlag(CityCourse,"russian-2_13-flag0.565","russian-2_13-flag1.565")) return false;
    logEvent(log,"test_024_goal_flag_phases",round.game);
    if (!round.jumpStage || round.jumpStage->count!=6 || !round.stopStage ||
        round.stopStage->count!=9 || !round.speedStage || round.speedStage.count!=1 ||
        round.extraGhostCount!=2 || !round.keyStage || round.keyStage->keyCount!=1 ||
        round.keyStage->doorCount!=1 || objects.data.courses[CityCourse].count!=24 ||
        !round.baddies.stage || round.baddies.stage->spawnerCount!=1 ||
        !capture("russian-2_13.565")) return false;
    constexpr Vec2 citySticks[]={
        {1,0},{0,1},{12/127.0f,-1},{12/127.0f,-1},
        {-1,44/127.0f},{1,35/127.0f},{-2/127.0f,1},{-1,0},
    };
    unsigned launches=0;
    bool forcedPad=false,jumpFrame=false;
    for (const Vec2 aim:citySticks) {
        if (!startShot(aim)) return false;
        for (unsigned tick=0;tick<480 && round.game.moving() && !round.caught && !round.game.won;++tick) {
            round.step();
            if (round.jump.timer==1) {
                ++launches;
                if (length(round.game.position-Vec2{224,208})<0.01f) {
                    if (std::abs(length(round.game.velocity)-770.0f)>0.01f) return false;
                    forcedPad=true;
                }
            }
            if (forcedPad && !jumpFrame && round.jump.timer==13) {
                if (!capture("russian-2_13-jump.565")) return false;
                jumpFrame=true;
            }
        }
        if (round.caught || round.game.moving()) {
            logEvent(log,"test_2_13_route_failed",round.game);
            return false;
        }
    }
    unsigned cityCoins=0;
    for (unsigned i=0;i<objects.data.courses[CityCourse].count;++i) cityCoins+=round.coins.has(i);
    if (!round.game.won || round.game.strokes!=std::size(citySticks) ||
        round.roundKeys.collected!=1 || round.roundKeys.opened!=1 || round.roundKeys.bonus!=200 ||
        round.stopRun.collected!=0xf7 || round.speedRun.collected!=1 || cityCoins!=16 ||
        launches!=6 || !forcedPad || !jumpFrame || round.jump.airborne() || !round.baddies.count ||
        round.baddies.velocities[0].x!=-150 || length(round.extraGhosts[0].velocity)==0 ||
        length(round.extraGhosts[1].velocity)==0) return false;
    logEvent(log,"test_2_13_key_door_jump",round.game);
    logEvent(log,"test_2_13_goal",round.game);
    if (!capture("russian-2_13-win.565",0,2500) ||
        advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=LightboxCourse || !enterCourse(executable,CityCourse,course,round)) return false;

    const unsigned previousQuest=progress.quest;
    progress.quest=1;
    round.restart();
    if (round.secret.available) return false;
    progress.quest=std::max(previousQuest,2u);
    round.restart();
    if (!round.secret.available || progress.glowstickKey || !shoot({1,0},480) ||
        !shoot({-30/127.0f,1},480) || round.secret.startedAt!=53 ||
        !shoot({-48/127.0f,-99/127.0f},480) || round.game.won || round.ticks!=168 ||
        progress.glowstickKey || !capture("russian-2_13-secret.565")) return false;
    for (unsigned i=0;i<28 && !progress.glowstickKey && !round.caught;++i) round.step();
    if (round.caught || round.ticks!=196 || !round.secret.collected ||
        progress.glowstickKey!=1 || !round.questKeyTick ||
        !capture("russian-2_13-secret-key.565")) return false;
    logEvent(log,"test_2_13_secret_runner",round.game);
    persistProgress(save,round,log);
    if (!save.read(saved) || saved.next!=LightboxCourse || !saved.best[HighwayCourse].score ||
        !saved.best[CityCourse].score || saved.glowstickKey!=1 || config.read()!=Language::Russian ||
        backgrounds.read()!=backgroundPreference.encode()) return false;
    round.restart();
    if (round.secret.available || round.secret.startedAt || round.secret.collected) return false;
    logEvent(log,"test_024_save_reloaded",round.game);

    openLevelSelect(flow);
    if (menuCourse!=LightboxCourse || !levelSelect.unlocked(2,13) ||
        !capture("selector-2_14-unlocked.565") ||
        updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3630) ||
        !selectorGoDown || !updateLevelSelector(0,PSP_CTRL_CROSS,0,3690)) return false;
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
        currentCourse!=LightboxCourse || !round.keyStage || round.keyStage->doorCount!=3 ||
        !round.jumpStage || round.jumpStage->count!=7) return false;
    if (!captureFlag(LightboxCourse,"russian-2_14-flag0.565","russian-2_14-flag1.565") ||
        !capture("russian-2_14.565")) return false;
    constexpr Vec2 lightboxSticks[]={{82,-27},{38,100},{-55,75},{-5,-80}};
    for (const auto stick:lightboxSticks) {
        if (!shoot(stick*(1.0f/127),500)) {
            logEvent(log,"test_2_14_route_failed",round.game);
            return false;
        }
    }
    if (!round.game.won || round.game.strokes!=std::size(lightboxSticks) ||
        round.roundKeys.collected!=7 || round.roundKeys.opened!=7 || round.roundKeys.bonus!=600 ||
        round.brokenBlocks!=3 || round.speedRun.collected!=7 || round.stopRun.collected!=0x14 ||
        !capture("russian-2_14-win.565",0,4700)) return false;
    logEvent(log,"test_027_2_14_all_keys_and_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
        currentCourse!=AlleyCourse || progress.next!=AlleyCourse) return false;
    if (!captureFlag(AlleyCourse,"russian-2_15-flag0.565","russian-2_15-flag1.565") ||
        !capture("russian-2_15.565") || round.extraGhostCount!=1 || !round.baddies.stage ||
        !round.baddies.stage->skull || round.baddies.stage->spawnerCount!=2) return false;
    if (!shoot({79/127.0f,-12/127.0f},500) ||
        !shoot({90/127.0f,-24/127.0f},500,"russian-2_15-skulls.565",24) ||
        !shoot({35/127.0f,108/127.0f},500)) {
        logEvent(log,"test_2_15_route_failed",round.game);
        return false;
    }
    if (!round.game.won || round.game.strokes!=3 || round.roundKeys.opened!=1 ||
        round.baddies.defeatedCount!=1 || !round.baddies.doorOpenedAt[0] ||
        !round.baddies.doorOpenedAt[1] || round.baddies.bonus ||
        !capture("russian-2_15-win.565",0,4700)) return false;
    logEvent(log,"test_027_2_15_skulls_and_goal",round.game);
    if (advanceCampaign(executable,course,round,save,log)!=Advance::EndOfBuild ||
        !save.read(saved) || saved.next!=CourseCount || !saved.best[LightboxCourse].score ||
        !saved.best[AlleyCourse].score || saved.glowstickKey!=1) return false;
    logEvent(log,"test_027_save_reloaded",round.game);

    for (Language locale:{Language::English,Language::Russian}) {
        if (!changeLanguage(executable,course,config,locale)) return false;
        for (unsigned index=0;index<CourseCount;++index) {
            if (!enterCourse(executable,index,course,round)) {
                logEvent(log,"test_025_course_load_failed",round.game);
                return false;
            }
            render(flow,course,round,0);
        }
        logEvent(log,russian()?"test_025_all_courses_ru":"test_025_all_courses_en",round.game);
    }
    return true;
}
