#pragma once
namespace PauseNavigation060 {
enum class Action { None,Resume,Quit };
inline Action update(unsigned& selected,int& captured,bool up,bool down,
                     bool confirmDown,bool confirmUp,bool back){
    if(back){captured=-1;return Action::Resume;}
    if(up!=down)selected^=1u;
    if(confirmDown)captured=static_cast<int>(selected);
    if(confirmUp){
        const int before=captured;captured=-1;
        if(before==static_cast<int>(selected))return selected?Action::Quit:Action::Resume;
    }
    return Action::None;
}
}
