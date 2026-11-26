#pragma once

#include <cstdint>

enum class Screen : unsigned { Loading, Title, Main, GameMenu, Hole, WorldResults, Credits, Settings, LevelSelect, ProfileSelect, ProfileCreate, ElysianEnding, CosmicEnding, Villa, VillaChecklist, Club, Phone, EndlessMenu, EndlessHole, EndlessResult, CatArtist=20, Meowls=21, MeowlsFilm=22, Dash=23, Bort=24, SoundTest=25, StartIntro=26, AttractDemo=27 };
enum class MainOption : unsigned { Play, Settings, Exit, Villa };

struct Flow {
    Screen screen = Screen::Loading;
    MainOption mainOption = MainOption::Play;
    bool quit = false;
    bool endlessSelected = false;
    unsigned gameMenuChoice = 0;
    unsigned loadingMs = 650;
    unsigned loadingPromptMs = 2600;

    void update(unsigned elapsedMs, bool confirm, bool back, bool up, bool down, bool credits=false, bool villa=false, bool endless=false, bool editor=false) {
        switch (screen) {
        case Screen::StartIntro: break;
        case Screen::Loading:
            if (elapsedMs >= loadingMs || (confirm && elapsedMs >= loadingPromptMs)) screen = Screen::Title;
            break;
        case Screen::Title:
            if (confirm) screen = Screen::Main;
            break;
        case Screen::Main:
            if (up!=down) {
                constexpr MainOption order[]={MainOption::Play,MainOption::Villa,MainOption::Settings,MainOption::Exit};
                unsigned item=0;
                while (order[item]!=mainOption) ++item;
                do {
                    item=(item+(down?1:3))%4;
                    mainOption=order[item];
                } while (!villa && mainOption==MainOption::Villa);
            }
            if (!villa && mainOption==MainOption::Villa) mainOption=MainOption::Play;
            if (back) screen = Screen::Title;
            else if (credits) screen = Screen::Credits;
            else if (confirm && mainOption==MainOption::Villa) screen=Screen::Villa;
            else if (confirm && mainOption==MainOption::Exit) quit = true;
            else if (confirm && mainOption==MainOption::Settings) screen = Screen::Settings;
            else if (confirm) screen = Screen::GameMenu;
            break;
        case Screen::GameMenu:
            if (editor) {
                if (gameMenuChoice>2 || (!endless && gameMenuChoice==1)) gameMenuChoice=0;
                if (up!=down) {
                    do { gameMenuChoice=(gameMenuChoice+(down?1:2))%3; }
                    while (!endless && gameMenuChoice==1);
                }
                endlessSelected=endless && gameMenuChoice==1;
                if (back) screen=Screen::Main;
                else if (confirm && gameMenuChoice<2)
                    screen=endlessSelected?Screen::EndlessMenu:Screen::ProfileSelect;
            } else {
                if (!endless) endlessSelected=false;
                else if (up!=down) endlessSelected=!endlessSelected;
                gameMenuChoice=endlessSelected?1:0;
                if (back) screen = Screen::Main;
                else if (confirm) screen = endlessSelected?Screen::EndlessMenu:Screen::ProfileSelect;
            }
            break;
        case Screen::ProfileSelect:
        case Screen::ProfileCreate:
            break;
        case Screen::LevelSelect:
            if (back) screen = Screen::ProfileSelect;
            else if (confirm) screen = Screen::Hole;
            break;
        case Screen::Settings:
        case Screen::Credits:
            if (back) screen = Screen::Main;
            break;
        case Screen::WorldResults:
        case Screen::ElysianEnding:
        case Screen::CosmicEnding:
        case Screen::Villa:
        case Screen::VillaChecklist:
        case Screen::Club:
        case Screen::EndlessMenu:
        case Screen::EndlessHole:
        case Screen::EndlessResult:
        case Screen::Phone:
        case Screen::CatArtist:
        case Screen::Meowls:
        case Screen::MeowlsFilm:
        case Screen::Dash:
        case Screen::Bort:
        case Screen::SoundTest:
        case Screen::AttractDemo:
            break;
        case Screen::Hole:
            if (back) screen = Screen::LevelSelect;
            break;
        }
    }
};

struct Patch { int16_t u, v, width, height, x, y; };
constexpr int ScreenPages = 5;
constexpr int MaxSpinner = 16;
constexpr int ClockDigits = 14;
constexpr int VisualWidth = 512;
constexpr int VisualHeight = 512;
constexpr int HudHeight = 256;
constexpr int ActorsWidth = 64;
constexpr int ActorsHeight = 32;

struct VisualHeader {
    uint32_t magic, spinnerCount;
    float referenceGhostX, referenceGhostY;
    uint32_t referenceClock, referenceTotal;
    Patch cat, ghost;
    Patch spinner[MaxSpinner];
    Patch strokes[10];
    Patch clock[ClockDigits][10];
};
static_assert(sizeof(VisualHeader) == 2040);
constexpr long VisualBytes = sizeof(VisualHeader) + 2L * VisualWidth * (ScreenPages * VisualHeight + HudHeight) +
                             4L * ActorsWidth * ActorsHeight;
