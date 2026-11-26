#pragma once

bool runFocused054(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,const char* log) {
    using namespace Villa054;
    Round round(course);
    Flow flow;
    const auto fail=[&](const char* message) {
        logEvent(log,message,round.game);
        return false;
    };
    const auto capture=[&](const char* name) {
        render(flow,course,round,1000,0,0,0,0);
        return dumpFrame(executable,name);
    };
    // Diagnostic input uses the same pointer hit testing as the ordinary build.
    const auto clickVilla=[&](Vec2 position) {
        pointer.position=position;
        stepVilla(flow,{},PSP_CTRL_CROSS,PSP_CTRL_CROSS,0,16);
        stepVilla(flow,{},0,0,PSP_CTRL_CROSS,16);
    };
    const auto resetVilla=[&](unsigned questPhase) {
        beginVisit(questPhase);
        pointer.captured=-1;
        pointerName=pointerBackLabel=pointerWhiteOutline=pointerSelectedOutline={};
        pointerBack=pointerLeft=pointerRight=pointerCursor=pointerHover={};
    };
    const auto resetChecklist=[&]() {
        checklistLocale=-1;
        checklistVerticalTrack=checklistVerticalGrabber={};
        checklistHorizontalTrack=checklistHorizontalGrabber={};
        checklistDragging=false;
    };
    const auto holdArrow=[&](Vec2 position) {
        pointer.position=position;
        stepVilla(flow,{},PSP_CTRL_CROSS,PSP_CTRL_CROSS,0,17);
        for (unsigned i=0;i<240;++i) stepVilla(flow,{},PSP_CTRL_CROSS,0,0,17);
        stepVilla(flow,{},0,0,PSP_CTRL_CROSS,0);
    };
    if (!save.read(progress) || progress.next!=CourseCount) return fail("test_054_seed");
    const Progress seed=progress;

    const GiftFile isolated(statePath(executable,"villa-test-gifts.cfg"));
    Costumes054::preferencePath=statePath(executable,"villa-test-costume.cfg");
    gifts.load(isolated);
    flow.screen=Screen::Main;
    flow.update(1000,false,false,false,true,false,available());
    if (available() || flow.mainOption!=MainOption::Settings || !capture("053ex-villa-locked-ru.565")) return fail("test_054_locked");
    const unsigned open=giftBit(GiftReward::Villa)|giftBit(GiftReward::Phone);
    if (!isolated.write(open|(open<<16))) return fail("test_054_gift");
    gifts.load(isolated);
    flow.mainOption=MainOption::Play;
    flow.update(1000,false,false,false,true,false,available());
    if (!available() || flow.mainOption!=MainOption::Villa || !capture("054-entry-ru.565"))
        return fail("test_054_menu_ru");
    flow.update(1000,true,false,false,false,false,available());
    if (flow.screen!=Screen::Villa) return fail("test_054_villa_open");
    resetVilla(0);
    Villa054::camera=0;
    residentMotion.reset(earned,123456u);
    night=false;
    if (!capture("054-villa-day-left-ru.565") || !panoramaReady) return fail("test_054_villa_day");
    holdArrow({736,480});
    if (Villa054::camera!=MaxPan || !capture("054-villa-day-right-ru.565")) return fail("test_054_pan_right");
    holdArrow({288,480});
    if (Villa054::camera!=0) return fail("test_054_pan_left");
    clickVilla({322,350});
    resetChecklist();
    if (flow.screen!=Screen::VillaChecklist || !capture("054-checklist-top-ru.565") || !checklistReady)
        return fail("test_054_checklist");
    layoutChecklist();
    pointer.scroll=checklistMaxVertical();
    if (pointer.scroll!=checklistMaxVertical() || !capture("054-checklist-end-ru.565"))
        return fail("test_054_checklist_last");
    stepChecklist(flow,{},0,PSP_CTRL_CIRCLE,0,0);
    resetVilla(progress.quest);
    if (flow.screen!=Screen::Villa) return fail("test_054_checklist_back");
    if (!changeLanguage(executable,course,config,Language::English)) return fail("test_054_english");
    night=true;
    panoramaReady=false;
    if (!capture("054-villa-night-left-en.565") || !panoramaReady) return fail("test_054_villa_night");
    clickVilla({322,350});
    resetChecklist();
    if (flow.screen!=Screen::VillaChecklist || !capture("054-checklist-top-en.565"))
        return fail("test_054_checklist_en");
    stepChecklist(flow,{},0,PSP_CTRL_CIRCLE,0,0);
    resetVilla(progress.quest);
    const auto uncanny=Costumes054::Id::Uncanny;
    const auto uncannyPosition=residentPosition(uncanny)*(1/Scale);
    Villa054::camera=std::clamp((uncannyPosition.x-512)*Scale,0.0f,float(MaxPan));
    pointer.position={uncannyPosition.x-Villa054::camera/Scale,uncannyPosition.y-24};
    stepVilla(flow,{},0,0,0,0);
    if (pointer.hover!=int(AttractionCount+static_cast<unsigned>(uncanny)))
        return fail("test_054_costume_hover");
    clickVilla(pointer.position);
    Costumes054::restore();
    if (Costumes054::selected!=Costumes054::Id::Uncanny || !Costumes054::gameplayArt() ||
        Costumes054::patchFor(false).width<=0 || Costumes054::patchFor(true).width<=0 ||
        Costumes054::select(Costumes054::Id::Nothing,earned) ||
        !capture("054-costume-uncanny-en.565")) return fail("test_054_costume");
    logEvent(log,"test_054_costume_pass",round.game);
    SaveFile isolatedQuest(statePath(executable,"villa-test-quest.sav"));
    Club054::questSave=&isolatedQuest;
    progress.quest=1;
    Villa054::camera=225;
    clickVilla({927-Villa054::camera/Scale,221});
    if (flow.screen!=Screen::Phone) return fail("test_phone_phase1_entry");
    Phone054::enter();
    for (const char digit:std::string_view(ClubText054::Quest::FriendNumber)) {
        const unsigned key=digit=='0'?10:static_cast<unsigned>(digit-'1');
        while (Phone054::selected/3!=key/3) Phone054::step(flow,PSP_CTRL_DOWN,16);
        while (Phone054::selected%3!=key%3) Phone054::step(flow,PSP_CTRL_RIGHT,16);
        Phone054::step(flow,PSP_CTRL_CROSS,16);
    }
    while (Phone054::selected/3!=3) Phone054::step(flow,PSP_CTRL_DOWN,16);
    while (Phone054::selected%3!=2) Phone054::step(flow,PSP_CTRL_RIGHT,16);
    Phone054::step(flow,PSP_CTRL_CROSS,16);
    Progress phaseTwo{};
    if (Phone054::state!=Phone054::State::Calling || !isolatedQuest.read(phaseTwo) ||
        phaseTwo.quest!=2 || phaseTwo.next!=seed.next || phaseTwo.totalPrisms()!=seed.totalPrisms() ||
        phaseTwo.totalScore()!=seed.totalScore() ||
        Phone054::lines!=&ClubText054::Quest::FriendPhases[1]) return fail("test_phone_phase1_save");
    for (unsigned i=0;i<50;++i) Phone054::step(flow,0,100);
    if (Phone054::state!=Phone054::State::Talking) return fail("test_phone_phase1_talk");
    Phone054::step(flow,PSP_CTRL_CROSS,0);
    if (!capture("053ex-phone-friend-phase1-en.565")) return fail("test_phone_phase1_frame");
    Phone054::step(flow,PSP_CTRL_CIRCLE,0);
    Phone054::leave();
    if (flow.screen!=Screen::Villa) return fail("test_phone_phase1_return");
    logEvent(log,"test_053ex_friend_phase2",round.game);
    resetVilla(progress.quest);
    Villa054::camera=0;
    clickVilla({488,176});
    if (flow.screen!=Screen::Club) return fail("test_054_club_entry");
    Club054::enter(earned);
    Club054::step(flow,{},0,0,100);
    Club054::step(flow,{},0,0,100);
    Club054::step(flow,{},0,0,100);
    if (!capture("054-club-lobby-en.565") || !Club054::ready) return fail("test_054_club_art");
    const Vec2 north{0,-1};
    Club054::step(flow,north,0,0,16);
    Club054::step(flow,north,PSP_CTRL_CROSS,PSP_CTRL_CROSS,16);
    Club054::step(flow,north,0,0,16);
    for (unsigned i=0;i<130 && Club054::roomY==0;++i) Club054::step(flow,north,0,0,16);
    if (Club054::roomX!=0 || Club054::roomY!=-1 ||
        !capture("054-club-crew-en.565") || !Club054::ready) return fail("test_054_club_shot");
    Club054::updateNamedArea("Level/BreakerDoorHitbox/CollisionShape2D");
    if (Club054::physics.breakerDoor || progress.quest!=2 || !Club054::remember(4) ||
        !Club054::updateNamedArea("Level/BreakerDoorHitbox/CollisionShape2D") ||
        !Club054::physics.breakerDoor) return fail("test_054_breaker_gate");
    Club054::updateNamedArea("Level/Breaker/BreakerArea/CollisionShape2D");
    if (!Club054::physics.breakerOn) return fail("test_054_breaker_switch");
    Club054::updateNamedArea("Level/Basement/RobertCutscene/PlayerDetector/CollisionShape2D");
    Club054::cutscene(1000);
    Club054::cutscene(750);
    if (Club054::scene!=Club054::Scene::Speak) return fail("test_054_robert_start");
    const int oldX=Club054::roomX,oldY=Club054::roomY;
    Club054::roomX=7;
    Club054::roomY=4;
    Club054::clockMs+=650;
    if (!capture("054-club-robert-en.565") || !Club054::ready || !Club054::robertArtReady)
        return fail("test_054_robert_art");
    Club054::roomX=oldX;
    Club054::roomY=oldY;
    for (unsigned i=0;i<ClubText054::Robert::LineCount;++i) Club054::cutscene(100000);
    Progress clubProgress{};
    if (Club054::scene!=Club054::Scene::Recital || !isolatedQuest.read(clubProgress) || clubProgress.quest!=5)
        return fail("test_054_robert_reward");
    for (unsigned i=0;i<18 && flow.screen==Screen::Club;++i)
        Club054::step(flow,{},PSP_CTRL_START,0,100);
    if (flow.screen!=Screen::Villa) return fail("test_054_club_back");
    Club054::leave();
    resetVilla(progress.quest);
    if (!VillaUnlocks::unlocked(earned,VillaUnlocks::Id::Nothing)) return fail("test_054_nothing");
    logEvent(log,"test_054_club_pass",round.game);
    Villa054::camera=225;
    clickVilla({927-Villa054::camera/Scale,221});
    if (flow.screen!=Screen::Phone) return fail("test_054_phone_entry");
    Phone054::enter();
    if (!capture("054-phone-en.565") || !Phone054::plateReady || !Phone054::overlayReady)
        return fail("test_054_phone_art");
    for (const char digit:std::string_view(ClubText054::Quest::FriendNumber))
        Phone054::press(digit=='0'?10:static_cast<unsigned>(digit-'1'));
    Phone054::press(11);
    for (unsigned i=0;i<40;++i) Phone054::step(flow,0,100);
    if (Phone054::state!=Phone054::State::Talking || !Phone054::lines ||
        !isolatedQuest.read(clubProgress) || clubProgress.quest!=6 ||
        !capture("054-phone-call-en.565")) return fail("test_054_phone_friend");
    if (!changeLanguage(executable,course,config,Language::Russian)) return fail("test_054_phone_ru");
    Phone054::plateReady=false;
    if (!capture("054-phone-call-ru.565") || !Phone054::plateReady ||
        !changeLanguage(executable,course,config,Language::English)) return fail("test_054_phone_ru");
    Phone054::step(flow,PSP_CTRL_CIRCLE,0);
    Phone054::leave();
    if (flow.screen!=Screen::Villa) return fail("test_054_phone_back");
    logEvent(log,"test_054_phone_pass",round.game);
    stepVilla(flow,{},0,PSP_CTRL_CIRCLE,0,0);
    leaveVisit();
    if (flow.screen!=Screen::Main || !capture("054-entry-en.565")) return fail("test_054_villa_back");
    audio.muteMusic();
    sceKernelDelayThread(100000);
    logEvent(log,"test_054_return_menu",round.game);
    if (!changeLanguage(executable,course,config,Language::Russian) || !save.read(progress) ||
        progress.next!=seed.next || progress.totalScore()!=seed.totalScore() ||
        progress.totalPrisms()!=seed.totalPrisms() || progress.cosmicKey!=seed.cosmicKey)
        return fail("test_054_save");
    logEvent(log,"test_054_seed_kept",round.game);
    if (!enterCourse(executable,CourseCount-1,course,round)) return fail("test_054_costume_course");
    flow.screen=Screen::Hole;
    if (!capture("054-costume-campaign-ru.565")) return fail("test_054_costume_campaign");
    logEvent(log,"test_054_villa_pass",round.game);
    return true;
}








