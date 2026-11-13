#pragma once

#include <cstdint>

enum class Screen : unsigned { Loading, Title, Main, GameMenu, Hole, WorldResults, Credits, Settings, LevelSelect, ProfileSelect, ProfileCreate, ElysianEnding, CosmicEnding };
enum class MainOption : unsigned { Play, Settings, Exit };

struct Flow {
    Screen screen = Screen::Loading;
    MainOption mainOption = MainOption::Play;
    bool quit = false;
    unsigned loadingMs = 650;
    unsigned loadingPromptMs = 2600;

    void update(unsigned elapsedMs, bool confirm, bool back, bool up, bool down, bool credits=false) {
        switch (screen) {
        case Screen::Loading:
            if (elapsedMs >= loadingMs || (confirm && elapsedMs >= loadingPromptMs)) screen = Screen::Title;
            break;
        case Screen::Title:
            if (confirm) screen = Screen::Main;
            break;
        case Screen::Main:
            if (up!=down) {
                const auto item=static_cast<unsigned>(mainOption);
                mainOption=static_cast<MainOption>((item+(down?1:2))%3);
            }
            if (back) screen = Screen::Title;
            else if (credits) screen = Screen::Credits;
            else if (confirm && mainOption==MainOption::Exit) quit = true;
            else if (confirm && mainOption==MainOption::Settings) screen = Screen::Settings;
            else if (confirm) screen = Screen::GameMenu;
            break;
        case Screen::GameMenu:
            if (back) screen = Screen::Main;
            else if (confirm) screen = Screen::ProfileSelect;
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
