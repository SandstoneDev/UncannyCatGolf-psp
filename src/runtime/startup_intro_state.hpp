#pragma once
#include "startup_settings.hpp"
namespace StartupIntroState {
enum class Action { None, Beep, Music, Language, Start };
enum class Error { None, Save, Assets, Language };
struct State {
    StartupSettings::Values values{};
    unsigned row=4,elapsedMs=0;
    Error error=Error::None;
    bool armed=false;
    void begin(const StartupSettings::Values& initial) {values=initial;row=4;elapsedMs=0;error=Error::None;armed=false;}
    // Entry must observe a neutral held-button sample. Edges alone cannot
    // distinguish a held launcher Cross from a fresh deliberate confirmation.
    bool accept(bool anyHeld) {if (!armed) {armed=!anyHeld;return false;}return true;}
    Action input(bool up,bool down,bool left,bool right,bool confirm,unsigned ms) {
        elapsedMs+=ms;
        if (up!=down) {row=(row+(down?1:4))%5;return Action::None;}
        if (row==4) return confirm?Action::Start:Action::None;
        if (row==3 && ((left!=right) || confirm)) {
            values.language=values.language==Language::English?Language::Russian:Language::English;
            error=Error::None;return Action::Language;
        }
        if (left==right || row>=3) return Action::None;
        auto& step=row==0?values.master:row==1?values.music:values.sounds;
        const unsigned next=right?std::min(step+1,20u):(step?step-1:0);
        if (next==step) return Action::None;
        step=next;error=Error::None;return row==1?Action::Music:Action::Beep;
    }
};
}
