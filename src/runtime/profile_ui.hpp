enum class ProfileFocus { Cards, New, Load };
enum class ProfileField { Title, Player, Chill, Confirm };

Profiles profiles{""};
ProfileKeyboard profileKeyboard;
ProfileName profileTitle{},profilePlayer{};
ProfileFocus profileFocus=ProfileFocus::New;
ProfileField profileField=ProfileField::Title;
unsigned profileRow=0,profileFirst=0,activeProfile=Profiles::Legacy;
bool profileError=false,profileChill=false;

constexpr unsigned ProfileRows=4;
bool chillAvailable() { return gifts.opened(GiftReward::Chill); }

void openProfiles(Flow& flow) {
    profiles.refresh();
    profileRow=profileFirst=0;
    for (unsigned row=0;row<profiles.size();++row) {
        if (profiles.slot(row)==activeProfile) profileRow=row;
    }
    if (profileRow>=ProfileRows) profileFirst=profileRow-ProfileRows+1;
    profileFocus=profiles.size()?ProfileFocus::Cards:ProfileFocus::New;
    profileError=false;
    flow.screen=Screen::ProfileSelect;
}

bool activateProfile(Flow& flow,SaveFile& save,unsigned slot,const char* log,const Game& game) {
    if (slot>=Profiles::Count) return false;
    auto selected=profiles.save(slot);
    Progress loaded{};
    if (!selected.read(loaded)) return false;
    save=std::move(selected);
    progress=loaded;
    canSwim=earnedSwim045();
    activeProfile=slot;
    chillMode=profiles.entry(slot).chill;
    openLevelSelect(flow);
    logEvent(log,"profile_loaded",game);
    return true;
}

void updateProfiles(Flow& flow,uint32_t pressed,SaveFile& save,const char* log,const Game& game) {
    if (pressed&PSP_CTRL_CIRCLE) {
        if (flow.screen==Screen::ProfileCreate) openProfiles(flow);
        else flow.screen=Screen::GameMenu;
        return;
    }
    const bool up=pressed&PSP_CTRL_UP,down=pressed&PSP_CTRL_DOWN,confirm=pressed&PSP_CTRL_CROSS;
    if (flow.screen==Screen::ProfileSelect) {
        if ((pressed&PSP_CTRL_LEFT) && profiles.size()) profileFocus=ProfileFocus::Cards;
        if ((pressed&PSP_CTRL_RIGHT) && profileFocus==ProfileFocus::Cards) profileFocus=ProfileFocus::New;
        if (profileFocus==ProfileFocus::Cards) {
            if (up && profileRow) --profileRow;
            if (down && profileRow+1<profiles.size()) ++profileRow;
            if (profileRow<profileFirst) profileFirst=profileRow;
            if (profileRow>=profileFirst+ProfileRows) profileFirst=profileRow-ProfileRows+1;
        } else if (profiles.size()) {
            if (up) profileFocus=ProfileFocus::New;
            if (down) profileFocus=ProfileFocus::Load;
        }
        if (!confirm) return;
        if (profileFocus==ProfileFocus::New) {
            profileTitle={};
            profilePlayer=profiles.size()?profiles.entry(profiles.slot(0)).player:ProfileName{};
            profileField=ProfileField::Title;
            profileChill=profileError=false;
            flow.screen=Screen::ProfileCreate;
        } else {
            profileError=!activateProfile(flow,save,profiles.slot(profileRow),log,game);
        }
        return;
    }

    if (up!=down) {
        const unsigned count=chillAvailable()?4:3;
        unsigned field=static_cast<unsigned>(profileField);
        if (!chillAvailable() && profileField==ProfileField::Confirm) field=2;
        field=(field+(down?1:count-1))%count;
        profileField=!chillAvailable() && field==2 ? ProfileField::Confirm : static_cast<ProfileField>(field);
        profileError=false;
        audio.play(Sound::Select);
        return;
    }
    if (!confirm) return;
    if (profileField==ProfileField::Title || profileField==ProfileField::Player) {
        const bool title=profileField==ProfileField::Title;
        profileError=!profileKeyboard.begin(title?profileTitle:profilePlayer,
                           title?lang("Playthrough title","Название прохождения"):lang("Your name","Ваше имя"),
                           russian());
    } else if (profileField==ProfileField::Chill && chillAvailable()) {
        profileChill=!profileChill;
    } else if (Profiles::validName(profileTitle) && Profiles::validName(profilePlayer)) {
        const unsigned slot=profiles.create(profileTitle,profilePlayer,profileChill);
        profileError=!activateProfile(flow,save,slot,log,game);
        if (!profileError) logEvent(log,"profile_created",game);
    }
}

void drawProfiles(bool create,unsigned elapsedMs,unsigned scrollMs) {
    if (!profileArt.load(russian(),create)) return;
    drawFull(profileArt.base);
    MenuScroll::Textures menu{};
    if (!create && clips.menu(menuScrollPack.c_str(),2*static_cast<unsigned>(russian())+1,menu)) {
        drawMenuTile(menu.tile,scrollMs);
        drawProfileGradient();
        localeText(lang("Select File to Play","Выбери сохранение"),272,1,0.96f,White,true);
        setTexture(GU_PSM_5650,ProfileArt::Size,512,profileArt.base);
        hudPatch({340,102,51,51,340,102});
        hudPatch({340,170,51,68,340,170});
        drawAnimatedBack(elapsedMs,true);
    }
    if (create) drawAnimatedBack(elapsedMs,profileArt.hasBackArt());
    const auto draw=[](ProfilePart part) {
        const Patch& art=profileArt.part(russian(),part);
        if (!art.width) return;
        setTexture(GU_PSM_5650,ProfileArt::Size,ProfileArt::Size,profileArt.atlas);
        hudPatch(art);
    };
    if (!create) {
        draw(ProfilePart::Load);
        const unsigned end=std::min(profiles.size(),profileFirst+ProfileRows);
        for (unsigned row=profileFirst;row<end;++row) {
            const unsigned slot=profiles.slot(row);
            const auto& entry=profiles.entry(slot);
            // The stock PC completion formula uses 113 holes and 565 prisms.
            const unsigned completion=(entry.completed*40+entry.prisms*12)/113;
            const bool focus=row==profileRow && profileFocus==ProfileFocus::Cards;
            const auto part=completion>=100 ? (focus?ProfilePart::CardPeakFocus:ProfilePart::CardPeak) :
                                              (focus?ProfilePart::CardFocus:ProfilePart::Card);
            const auto& card=profileArt.part(russian(),part);
            const int offset=static_cast<int>(std::lround((row-profileFirst)*44.625f));
            setTexture(GU_PSM_5650,ProfileArt::Size,ProfileArt::Size,profileArt.atlas);
            patch(card,ViewX+card.x,card.y+offset);
            char label[64];
            const char* title=entry.title[0]?entry.title.data():lang("Previous progress","Предыдущий сейв");
            std::snprintf(label,sizeof label,"\"%s\"",title);
            localeText(label,162,55+offset,1,White,false,149,true);
            if (entry.player[0]) {
                std::snprintf(label,sizeof label,"\"%s\"",entry.player.data());
                localeText(label,162,68+offset,1,White,false,100,true);
            }
            std::snprintf(label,sizeof label,"%u%%",completion);
            localeText(label,288,68+offset,1,White,true,43,true);
        }
        if (profileFocus!=ProfileFocus::Cards) {
            setTexture(GU_PSM_8888,256,256,settingsWidgets);
            patch({188,44,17,17,0,0},ViewX+332,profileFocus==ProfileFocus::New?126:197);
        }
    } else {
        const bool titleFocus=profileField==ProfileField::Title,playerFocus=profileField==ProfileField::Player;
        if (profileTitle[0]) {
            draw(titleFocus?ProfilePart::TitleFilledFocus:ProfilePart::TitleFilled);
            localeText(profileTitle.data(),242,83,1,White,false,126,true);
        } else if (titleFocus) draw(ProfilePart::TitleFocus);
        if (profilePlayer[0]) {
            draw(playerFocus?ProfilePart::PlayerFilledFocus:ProfilePart::PlayerFilled);
            localeText(profilePlayer.data(),242,108,1,White,false,126,true);
        } else if (playerFocus) draw(ProfilePart::PlayerFocus);
        if (!profileArt.hasBackArt()) rectangle(ViewX+144,135,257,99,GU_RGBA(6,61,63,255));
        localeText(lang("Modifiers","Модификаторы"),272,135,0.82f,White,true);
        const auto locked=[](int x,int y) {
            localeText("???",x,y,0.87f);
            constexpr uint32_t shackle=GU_RGBA(156,107,30,255);
            rectangle(ViewX+x+30,y+3,5,1,shackle);
            rectangle(ViewX+x+29,y+4,1,5,shackle);
            rectangle(ViewX+x+35,y+4,1,5,shackle);
            rectangle(ViewX+x+27,y+8,11,8,Gold);
            rectangle(ViewX+x+31,y+10,2,4,GU_RGBA(54,35,12,255));
        };
        for (unsigned column=0;column<3;++column)
            for (unsigned row=0;row<(column?4u:3u);++row) {
                if (column==0 && row==0 && chillAvailable()) continue;
                locked(162+column*81,151+row*16);
            }
        if (chillAvailable()) {
            const bool focus=profileField==ProfileField::Chill;
            draw(profileChill ? (focus?ProfilePart::ChillOnFocus:ProfilePart::ChillOn) :
                                (focus?ProfilePart::ChillOffFocus:ProfilePart::ChillOff));
        }
        localeText(lang("Select a modifier for details","Выбери модификатор"),272,216,0.72f,White,true,250,true);
        if (Profiles::validName(profileTitle) && Profiles::validName(profilePlayer))
            draw(profileField==ProfileField::Confirm?ProfilePart::ConfirmFocus:ProfilePart::Confirm);
    }
    if (profileError) localeText(lang("Unable to open or save profile","Не удалось открыть или сохранить профиль"),
                                 272,227,1,White,true,254,true);
}
