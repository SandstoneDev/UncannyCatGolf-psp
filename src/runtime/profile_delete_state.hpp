#pragma once
namespace ProfileDelete {
enum class Action { None, Cancel, Confirm };
struct Prompt {
    bool shown=false,yes=false,armed=false;
    void open(){shown=true;yes=armed=false;}
    void close(){shown=yes=armed=false;}
    Action input(bool confirm,bool heldConfirm,bool cancel,bool left,bool right) {
        if(!shown)return Action::None;
        if(cancel){close();return Action::Cancel;}
        if(!armed){if(!heldConfirm)armed=true;return Action::None;}
        if(left!=right)yes=left;
        if(!confirm)return Action::None;
        const auto action=yes?Action::Confirm:Action::Cancel;
        close();return action;
    }
};
}
