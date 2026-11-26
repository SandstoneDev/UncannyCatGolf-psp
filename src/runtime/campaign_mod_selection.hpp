#pragma once
#include "campaign_mod_options.hpp"
namespace CampaignMods {
// Original creation menu: three columns of 3/4/4 switches.
constexpr Mod Order[]={Chill,Panic,God,Burst,Telekinesis,Buddy,Parry,Jumpy,Dizzy,Missiles,Sticky};
constexpr const char* Names[11][2]={
    {"Chill","Спокойный"},{"Panic","Паника"},{"Godmode","Бог"},
    {"Burst","Рывок"},{"Telekinesis","Телекинез"},{"Little Buddy","Дружок"},
    {"Parry","Парирование"},{"Jumpy","Прыгун"},{"Dizzy","Головокружение"},
    {"Missiles","Ракеты"},{"Sticky","Липкий"}
};
constexpr const char* Details[11][2]={
    {"No chasing Uncannies; worlds 0-4.","Без погони жутких котов; миры 0-4."},
    {"Uncannies chase faster.","Жуткие коты преследуют быстрее."},
    {"Unlimited shot power; Square teleports.","Неограниченный удар; квадрат — телепорт."},
    {"Aim and hit again while moving.","Можно целиться и бить на ходу."},
    {"Move with the analog stick, without shots.","Двигай кота аналогом без ударов."},
    {"A little companion; keep it from ghosts.","Спутник: береги его от призраков."},
    {"Square: block a ghost at the right moment.","Квадрат: отрази атаку призрака вовремя."},
    {"Jump automatically after travelling.","Автопрыжок после пройденного расстояния."},
    {"Moving cat curves clockwise.","На ходу кот сворачивает по часовой стрелке."},
    {"Missiles lock on; keep moving!","Ракеты наводятся — не стой на месте!"},
    {"Stop on contact with ordinary walls.","Кот останавливается у обычных стен."}
};
struct Selection {
    unsigned index=0;
    Mod mod() const { return Order[index]; }
    bool first(uint16_t available) {
        for(unsigned i=0;i<11;++i) if(available&Order[i]) {index=i;return true;}
        return false;
    }
    bool move(int direction,uint16_t available,bool wrap=false) {
        for(int i=int(index)+direction,seen=0;seen<11;i+=direction,++seen) {
            if(i<0 || i>=11) {if(!wrap)return false;i=i<0?10:0;}
            if(available&Order[i]) {index=unsigned(i);return true;}
        }
        return false;
    }
};
}
