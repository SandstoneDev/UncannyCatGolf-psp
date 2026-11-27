#include "campaign_mod_selection.hpp"
#include "profile_delete_state.hpp"
#include "profile_delete_art.hpp"
enum class ProfileFocus { Cards, New, Load };
enum class ProfileField { Title, Player, Chill, Confirm };

Profiles profiles{""};
ProfileKeyboard profileKeyboard;
ProfileName profileTitle{},profilePlayer{};
ProfileFocus profileFocus=ProfileFocus::New;
ProfileField profileField=ProfileField::Title;
unsigned profileRow=0,profileFirst=0,activeProfile=Profiles::Legacy;
bool profileError=false,profileChill=false;
uint16_t profileModifiers=0,profileAvailableModifiers=0;
CampaignMods::Selection profileModSelection;
bool profileMapPending=false,profileMapShown=false;
unsigned profileMapSlot=Profiles::None;
uint64_t profileMapStarted=0;

constexpr unsigned ProfileRows=4;
bool profileDeleteMode=false;
ProfileDelete::Prompt profileDeletePrompt;
Profiles::DeleteToken profileDeleteToken{};
ProfileDeleteArt profileDeleteArt;
bool (*profileDeleteReady)(unsigned)=nullptr;
void (*profileDeleteCompleted)(unsigned)=nullptr;

void closeProfileDelete() {
    profileDeleteMode=false;
    profileDeletePrompt.close();
    profileDeleteToken={};
    profileDeleteArt.release();
}
bool prepareProfileDelete() {
    // Input runs after endFrame's GE fence. Async reads target staging only;
    // cancelPrefetch prevents their eventual poll from committing into card1.
    levelSelectArt.cancelPrefetch();
    if(profileDeleteArt.ready())return true;
    return profileDeleteArt.load(levelSelectArt.cardWorkspace(1),LevelSelectArt::VictoryWorkspaceBytes);
}
void drawProfileDeletePart(unsigned part) {
    if(!profileDeleteArt.ready())return;
    setTexture(GU_PSM_8888,ProfileDeleteArt::Width,ProfileDeleteArt::Height,profileDeleteArt.texture());
    hudPatch(profileDeleteArt.part(part));
}

void openProfiles(Flow& flow) {
    closeProfileDelete();
    profiles.refresh();
    profileRow=profileFirst=0;
    for (unsigned row=0;row<profiles.size();++row) {
        if (profiles.slot(row)==activeProfile) profileRow=row;
    }
    if (profileRow>=ProfileRows) profileFirst=profileRow-ProfileRows+1;
    profileFocus=profiles.size()?ProfileFocus::Cards:ProfileFocus::New;
    profileError=false;
    profileMapPending=false;
    flow.screen=Screen::ProfileSelect;
}

unsigned profileMapWorld(unsigned slot) {
    const auto& entry=profiles.entry(slot);
    const unsigned available=entry.chill?std::min(playableCourseCount,ElysianFieldsCourse):playableCourseCount;
    const unsigned next=available?std::min(entry.next,available-1):0;
    unsigned world=0;
    while (world+1<LevelSelectArt::Worlds && next>=LevelSelectState::firstCourse(world+1)) ++world;
    return world;
}

void prefetchProfileMap() {
    const unsigned slot=profiles.slot(profileRow);
    if (profileDeleteMode || profileDeletePrompt.shown || profileFocus==ProfileFocus::New || slot>=Profiles::Count) {
        levelSelectArt.cancelPrefetch();
        return;
    }
    const unsigned world=profileMapWorld(slot);
    levelSelectArt.prefetch(world,world,russian());
}

bool requestProfileMap(unsigned slot) {
    if (slot>=Profiles::Count || !profiles.entry(slot).found) return false;
    profileMapSlot=slot;
    profileMapStarted=sceKernelGetSystemTimeWide();
    profileMapPending=true;
    profileMapShown=false;
    return true;
}

void finishProfileMap(Flow& flow,SaveFile& save) {
    if (!avatarClicks058.bind(profileMapSlot)) {profileError=true;profileMapPending=false;return;}
    save=profiles.save(profileMapSlot);
    progress=profiles.snapshot(profileMapSlot);
    applySharedWorldProgress();
    canSwim=earnedSwim045();
    activeProfile=profileMapSlot;
    ArtistCopycat::setProfile(activeProfile);
    chillMode=profiles.entry(activeProfile).chill;
    campaignOptions.modifiers=profiles.entry(activeProfile).modifiers;
    openLevelSelect(flow);
    profileMapPending=false;
}

void updateProfiles(Flow& flow,uint32_t pressed,const char* log,const Game& game,SaveFile& save,uint32_t held) {
    if (profileDeletePrompt.shown) {
        const unsigned slot=profileDeleteToken.slot;
        const auto action=profileDeletePrompt.input(pressed&PSP_CTRL_CROSS,held&PSP_CTRL_CROSS,
            pressed&PSP_CTRL_CIRCLE,pressed&PSP_CTRL_LEFT,pressed&PSP_CTRL_RIGHT);
        if (action==ProfileDelete::Action::Cancel) {profileDeleteToken={};audio.play(Sound::Back);}
        if (action==ProfileDelete::Action::Confirm) {
            if(profileDeleteReady && !profileDeleteReady(slot)) {
                profileDeleteToken={};profileError=true;audio.play(Sound::Back);return;
            }
            const auto result=profiles.confirmDelete(profileDeleteToken);
            profileDeleteToken={};
            profileError=result!=Profiles::DeleteResult::Archived;
            if (!profileError) {
                if(profileDeleteCompleted)profileDeleteCompleted(slot);
                profileMapPending=false;profileMapSlot=Profiles::None;
                levelSelectArt.cancelPrefetch();
                if(activeProfile==slot) {
                    activeProfile=Profiles::None;
                    progress={};chillMode=false;campaignOptions.modifiers=0;
                    save=profiles.save(Profiles::Legacy); // Idle binding only; no save write.
                    ArtistCopycat::preferred=ArtistGlobal::NoProfile;
                }
                profileRow=profiles.size()?std::min(profileRow,profiles.size()-1):0;
                profileFirst=profileRow>=ProfileRows?profileRow-ProfileRows+1:0;
                profileFocus=profiles.size()?ProfileFocus::Cards:ProfileFocus::New;
                audio.play(Sound::Select);
                logEvent(log,"profile_deleted",game);
            }
        }
        return;
    }
    if (pressed&PSP_CTRL_CIRCLE) {
        if (profileMapPending) {
            profileMapPending=false;
            levelSelectArt.cancelPrefetch();
            return;
        }
        if (flow.screen==Screen::ProfileCreate) openProfiles(flow);
        else {closeProfileDelete();flow.screen=Screen::GameMenu;}
        return;
    }
    if (profileMapPending) return;
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
            closeProfileDelete();
            profileTitle={};
            profilePlayer=profiles.size()?profiles.entry(profiles.slot(0)).player:ProfileName{};
            profileField=ProfileField::Title;
            profileChill=profileError=false;
            profileModifiers=0;
            profileAvailableModifiers=campaignAvailableMods();
            profileModSelection.first(profileAvailableModifiers);
            flow.screen=Screen::ProfileCreate;
        } else if (profileFocus==ProfileFocus::Load) {
            if(profileDeleteMode) {closeProfileDelete();profileError=false;}
            else {
                profileError=!prepareProfileDelete();
                profileDeleteMode=!profileError;
            }
            audio.play(Sound::Select);
        } else if (profileDeleteMode) {
            profileError=!avatarClicks058.flush() || !prepareProfileDelete() || !profiles.selectDelete(profiles.slot(profileRow),profileDeleteToken);
            if(!profileError) {profileDeletePrompt.open();audio.play(Sound::Select);}
        } else {
            profileError=!requestProfileMap(profiles.slot(profileRow));
        }
        return;
    }

    if (profileField==ProfileField::Chill && (pressed&(PSP_CTRL_LEFT|PSP_CTRL_RIGHT))) {
        profileModSelection.move((pressed&PSP_CTRL_RIGHT)?1:-1,profileAvailableModifiers,true);
        audio.play(Sound::Select);
        return;
    }
    if (up!=down) {
        if (profileField==ProfileField::Chill) {
            if (!profileModSelection.move(down?1:-1,profileAvailableModifiers))
                profileField=down?ProfileField::Confirm:ProfileField::Player;
        } else {
            const unsigned count=profileAvailableModifiers?4:3;
            unsigned field=static_cast<unsigned>(profileField);
            if (!profileAvailableModifiers && profileField==ProfileField::Confirm) field=2;
            field=(field+(down?1:count-1))%count;
            profileField=!profileAvailableModifiers && field==2 ? ProfileField::Confirm : static_cast<ProfileField>(field);
            if (profileField==ProfileField::Chill) {
                profileModSelection.first(profileAvailableModifiers);
                if(up) while(profileModSelection.move(1,profileAvailableModifiers)) {}
            }
        }
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
    } else if (profileField==ProfileField::Chill && profileAvailableModifiers) {
        profileModifiers=CampaignMods::toggle(profileModifiers,profileModSelection.mod());
        profileChill=(profileModifiers&CampaignMods::Chill)!=0;
    } else if (Profiles::validName(profileTitle) && Profiles::validName(profilePlayer)) {
        const unsigned slot=profiles.create(profileTitle,profilePlayer,profileChill,profileModifiers);
        profileError=!requestProfileMap(slot);
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
        if(profileDeleteMode) drawProfileDeletePart(0);
        else localeText(lang("Select File to Play","Выбери сохранение"),272,1,0.96f,White,true);
        setTexture(GU_PSM_5650,ProfileArt::Size,512,profileArt.base);
        hudPatch({340,102,51,51,340,102});
        if(!profileDeleteMode)hudPatch({340,170,51,68,340,170});
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
        if(profileDeleteMode)drawProfileDeletePart(1);
        else draw(ProfilePart::Load);
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
        for(unsigned i=0;i<11;++i) {
            const unsigned column=i<3?0:i<7?1:2,row=i<3?i:i<7?i-3:i-7;
            const int x=162+column*81,y=151+row*16;
            const auto mod=CampaignMods::Order[i];
            const bool locked=!(profileAvailableModifiers&mod);
            const bool focus=!locked && profileField==ProfileField::Chill && profileModSelection.index==i;
            Theme067::modifier(float(x+65),float(y+1),bool(profileModifiers&mod),focus,locked);
            localeText(locked?"???":CampaignMods::Names[i][russian()],x,y,.70f,White,false,61,true);
        }
        const char* detail=profileField==ProfileField::Chill ?
            CampaignMods::Details[profileModSelection.index][russian()] :
            lang("Select a modifier for details","Выбери модификатор");
        localeText(detail,272,216,0.70f,White,true,250,true);
        if (Profiles::validName(profileTitle) && Profiles::validName(profilePlayer))
            draw(profileField==ProfileField::Confirm?ProfilePart::ConfirmFocus:ProfilePart::Confirm);
    }
    if (profileMapPending)
        drawOriginalLoading067(elapsedMs,228,216,lang("Loading...","Загрузка..."));
    if (profileDeletePrompt.shown) {
        rectangle(ViewX+136,0,272,272,GU_RGBA(0,0,0,170));
        drawProfileDeletePart(profileDeleteArt.prompt(russian()));
        drawProfileDeletePart(profileDeleteArt.yes(russian(),profileDeletePrompt.yes));
        drawProfileDeletePart(profileDeleteArt.no(russian(),!profileDeletePrompt.yes));
    }
    if (profileError) localeText(lang("Unable to open or save profile","Не удалось открыть или сохранить профиль"),
                                 272,227,1,White,true,254,true);
}

