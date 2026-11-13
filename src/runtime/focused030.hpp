bool runFocused030(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,const char* log) {
    Flow flow;
    Round round(course);
    const auto fail=[&](const char* stage) {
        logEvent(log,stage,round.game);
        return false;
    };
    if (!save.read(progress) || progress.next!=MartCourse || progress.parkKey ||
        progress.best[CoasterCourse].time==UINT32_MAX ||
        !changeLanguage(executable,course,config,Language::Russian) ||
        !enterCourse(executable,CoasterCourse,course,round) || round.chuckles.count!=2)
        return fail("test_030_seed_error");
    logEvent(log,"test_030_loaded_029",round.game);
    const auto capture=[&](const char* suffix,unsigned elapsed=0,unsigned result=0) {
        render(flow,course,round,elapsed,round.ticks*5/3,round.totalTicks*5/3,result);
        char name[72];
        std::snprintf(name,sizeof name,"%s-%s-%s.565",russian()?"ru":"en",CourseIds[menuCourse],suffix);
        return dumpFrame(executable,name);
    };
    for (unsigned index=MartCourse;index<=MirrorCourse;++index) {
        openLevelSelect(flow);
        const unsigned cell=index-ChuckleParkFirstCourse;
        if (menuCourse!=index || levelSelect.world()!=3 || levelSelect.cell()!=cell ||
            !levelSelect.unlocked(3,cell) || levelSelect.unlocked(3,cell+1) || !capture("map",3000) ||
            updateLevelSelector(PSP_CTRL_CROSS,0,PSP_CTRL_CROSS,3030) || !selectorGoDown ||
            !capture("go",3000) || !updateLevelSelector(0,PSP_CTRL_CROSS,0,3090) || selectorGoDown)
            return fail("test_030_selector_error");
        flow.update(0,true,false,false,false);
        if (flow.screen!=Screen::Hole || !enterCourse(executable,menuCourse,course,round) ||
            currentCourse!=index || !capture("hole"))
            return fail("test_030_enter_error");
        logEvent(log,"test_030_map_cross",round.game);
        for (unsigned tick=0;tick<5 && !round.caught;++tick) round.step();
        if (round.caught || !capture("flag")) return fail("test_030_flag_error");
        while (round.ticks<1800 && !round.caught) round.step();
        if (!round.caught || !capture("loss",0,1000)) return fail("test_030_loss_error");
        logEvent(log,"test_030_loss",round.game);

        round.restart();
        if (!changeLanguage(executable,course,config,Language::English) || !capture("hole") ||
            !changeLanguage(executable,course,config,Language::Russian))
            return fail("test_030_locale_error");
        const auto& hazards=courseHazards(index);
        if (index==MartCourse) {
            if (round.chuckles.count!=13 || hazards.blockCount!=1 || parkArt.data.smiley.coinCount)
                return fail("test_030_mart_assets_error");
            const auto seed=progress;
            progress.quest=1;
            round.restart();
            if (round.marcy.hintVisible()) return fail("test_030_marcy_gating_error");
            progress.quest=2;
            chillMode=true;
            round.restart();
            if (!round.marcy.hintVisible()) return fail("test_030_marcy_hint_error");
            round.game.position=MarcyRun::Trigger;
            round.step();
            if (!round.marcy.startedAt) return fail("test_030_marcy_trigger_error");
            const unsigned initialAudio=audio.blocks();
            for (unsigned tick=0;tick<120 && audio.blocks()-initialAudio<8;++tick) sceDisplayWaitVblankStart();
            if (audio.blocks()-initialAudio<8) return fail("test_030_marcy_audio_error");
            round.game.position=course.spawn;
            for (unsigned tick=0;tick<120;++tick) round.step();
            if (!capture("marcy-intercom")) return fail("test_030_marcy_frame_error");
            for (unsigned i=0;i<11;++i) {
                round.game.position=MarcyRun::Trigger;
                round.step();
                round.game.position=course.spawn;
                round.step();
            }
            if (round.marcy.cleans!=10 || !round.marcy.finishedAt || progress.parkKey)
                return fail("test_030_marcy_spill_error");
            for (unsigned tick=0;tick<179 && !progress.parkKey;++tick) round.step();
            if (!progress.parkKey || !round.questKeyTick || !capture("marcy-key"))
                return fail("test_030_marcy_reward_error");
            SaveFile keySave(companion(executable,"park-key-test.sav"));
            Progress savedKey{};
            if (!keySave.write(progress) || !keySave.read(savedKey) || !savedKey.parkKey)
                return fail("test_030_marcy_save_error");
            round.restart();
            if (round.marcy.hintVisible()) return fail("test_030_marcy_replay_error");
            progress=seed;
            chillMode=false;
            round.restart();
            logEvent(log,"test_030_marcy_11_entries_saved_pcm",round.game);
        } else if (index==FactoryCourse) {
            const auto& ring=parkArt.data.smiley;
            if (!round.switchStage || round.switchStage->blockCount!=2 || round.switchStage->layerCount!=2 ||
                round.switches.state!=0 || ring.timeTicks!=600 || ring.coinCount!=8 ||
                parkArt.data.balloonCount!=3 || !round.stopStage || round.stopStage->count!=4 ||
                !round.speedStage || round.speedStage.count!=2 || !round.keyStage ||
                round.keyStage->keyCount!=2 || round.keyStage->doorCount!=2 || !round.jumpStage ||
                round.jumpStage->count!=1 || hazards.sandCount!=1 || round.chuckles.count)
                return fail("test_030_factory_assets_error");
            chillMode=true;
            for (unsigned tick=0;tick<10;++tick) round.step();
            if (round.chuckles.count!=1 || !round.chuckles.spawnerFrame(round.ticks) || !capture("spawner"))
                return fail("test_030_factory_spawn_error");

            round.game.position=round.stopStage->positions[0];
            round.step();
            if (!round.stopRun.collected) return fail("test_030_factory_stop_error");
            round.game.position=round.jumpStage->pads[0].center;
            round.game.velocity={};
            round.step();
            if (!round.jump.airborne() || !capture("jump")) return fail("test_030_factory_jump_error");
            round.restart();
            round.game.position=ring.center;
            round.step();
            if (round.smiley.phase!=SmileyPhase::Active || !capture("ring-active"))
                return fail("test_030_factory_ring_error");
            for (unsigned i=0;i<ring.coinCount;++i) {
                round.game.position=ring.coins[i];
                round.step();
            }
            if (round.smiley.phase!=SmileyPhase::Passed || round.smiley.bonus!=ring.bonus || !capture("ring-pass"))
                return fail("test_030_factory_ring_pass_error");
            chillMode=false;
            round.restart();
            logEvent(log,"test_030_factory_spawn_stop_jump_ring",round.game);
        }

        constexpr int mart[][2]={{-67,-100},{84,96}};
        constexpr int factory[][2]={{23,85},{-108,-53},{72,0}};
        constexpr int dense[][2]={{86,-72},{-124,33},{11,-127},{-119,-29}};
        constexpr int mirrored[][2]={{116,-54},{51,61}};
        const int (*routes[])[2]={mart,factory,dense,mirrored};
        constexpr unsigned shotCounts[]={std::size(mart),std::size(factory),std::size(dense),std::size(mirrored)};
        const auto* route=routes[index-MartCourse];
        const unsigned shotCount=shotCounts[index-MartCourse];
        unsigned flips=0,sandTicks=0;
        for (unsigned shot=0;shot<shotCount;++shot) {
            const Vec2 aim{route[shot][0]/127.0f,route[shot][1]/127.0f};
            round.controls.update(round.game,course,aim,true,false);
            if (round.controls.update(round.game,course,{},false,false)!=ControlEvent::Shot)
                return fail("test_030_route_shot_error");
            logEvent(log,"test_030_route_shot",round.game);
            for (unsigned tick=0;tick<700 && round.game.moving() && !round.caught && !round.game.won;++tick) {
                const unsigned previous=round.switches.state;
                round.step();
                sandTicks+=round.inSand;
                if (index==DenseCourse && round.ticks==230 && !capture("coaster"))
                    return fail("test_030_coaster_frame_error");
                if (previous!=round.switches.state) {
                    ++flips;
                    if (!capture(round.switches.state?"switch-active":"switch-return"))
                        return fail("test_030_switch_frame_error");
                }
            }
            if (round.caught || round.game.moving()) return fail("test_030_route_stop_error");
        }
        if (!round.game.won || !capture("win",0,4700)) return fail("test_030_win_error");
        if (index==MartCourse) {
            if (round.brokenBlocks!=1 || round.chuckles.defeatedCount!=5 || round.coins.collected[0]!=0xff)
                return fail("test_030_mart_route_error");
            logEvent(log,"test_030_mart_block_eight_coins_five_chuckles",round.game);
        } else if (index==FactoryCourse) {
            if (!flips || !sandTicks || !round.speedRun.collected || !round.roundKeys.collected ||
                !round.roundKeys.opened || !round.smiley.everStarted)
                return fail("test_030_factory_route_error");
            logEvent(log,"test_030_factory_switch_sand_speed_door_goal",round.game);
        }
        if (index==DenseCourse) {
            if (denseBlocks.size()!=60 || !(round.brokenBlocks.low>>32) ||
                round.blockMultiplier!=51 || round.blockBonus!=5*round.blockMultiplier*(round.blockMultiplier+1))
                return fail("test_030_dense_route_error");
            logEvent(log,"test_030_dense_51_blocks",round.game);
        }
        if (index==MirrorCourse) {
            if (round.chuckles.count!=6 || !round.chuckles.defeatedCount ||
                !round.brokenBlocks || !round.coins.collected[0])
                return fail("test_030_mirror_route_error");
            logEvent(log,"test_030_mirror_blocks_coins_chuckle",round.game);
        }
        logEvent(log,"test_030_goal",round.game);
        const auto outcome=advanceCampaign(executable,course,round,save,log);
        const auto expected=index==MirrorCourse?Advance::EndOfBuild:Advance::NextHole;
        Progress saved{};
        if (outcome!=expected || progress.next!=index+1 || !save.read(saved) ||
            saved.next!=index+1 || saved.best[index].time==UINT32_MAX)
            return fail("test_030_advance_error");
        if (index==DenseCourse) {
            const GiftFile giftProbe(companion(executable,"burst-test.cfg"));
            GiftSequence test;
            if (saved.best[DenseCourse].rank!=5 || !test.queue(giftProbe,saved))
                return fail("test_030_burst_queue_error");
            test.load(giftProbe);
            if (!(test.pending()&giftBit(GiftReward::Burst))) return fail("test_030_burst_save_error");
            logEvent(log,"test_030_burst_peak_saved",round.game);
        }
    }
    return true;
}
