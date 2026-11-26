#pragma once
#include <algorithm>
namespace EditorHotkeyHelp068 {
inline constexpr float X=24,Y=104,Width=720,Height=360,Row=30,TextScale=1.75f;
inline constexpr unsigned Rows=12;inline constexpr int FocusBase=6000000;
// Existing PSP mapping strings, moved without abbreviating any control chord.
inline constexpr const char*Lines[]={
"Stick: pointer   D-pad: focus","X: left click   Square: right click",
"L: Shift   R: Ctrl   L+R: Alt","Triangle: middle mouse","Select: Tab",
"Select+Square / X: Copy / Paste","Select+Triangle / Start: Undo / Redo",
"Select+Circle: Delete","Select+Up / Down: Wheel","Select+Left / Right: X / Z",
"L+R+Triangle+X: replace player","Playtesting saves your level!"};
inline constexpr unsigned Count=sizeof(Lines)/sizeof(*Lines);
inline constexpr unsigned lastScroll(){return Count-Rows;}
inline constexpr bool contains(float x,float y){return x>=X&&y>=Y&&x<X+Width&&y<Y+Height;}
inline unsigned wheel(unsigned scroll,int amount){return unsigned(std::clamp(int(scroll)-amount,0,int(lastScroll())));}
inline unsigned ensureVisible(unsigned selected,unsigned scroll){scroll=std::min(scroll,lastScroll());if(selected<scroll)scroll=selected;if(selected>=scroll+Rows)scroll=selected-Rows+1;return scroll;}
inline float rowY(unsigned i,unsigned scroll){return Y+(int(i)-int(scroll))*Row;}
inline float baseline(unsigned row){return Y+row*Row+24;}
}
