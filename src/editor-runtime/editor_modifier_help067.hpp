#pragma once
#include "editor_readable_labels067.hpp"
#include <algorithm>
#include <cstddef>
namespace EditorModifierHelp067 {
inline constexpr float X=272,Y=64,Width=464,Height=384,Row=32,TextScale=1.75f;
inline constexpr unsigned Rows=12;inline constexpr int FocusBase=5000000;
struct Line {const char*first;unsigned length;};
inline constexpr unsigned lineCount(const char*text){unsigned n=0;bool ink=false;for(;*text;++text){if(*text=='\n'){if(ink)++n;ink=false;}else ink=true;}return n+unsigned(ink);}
inline constexpr unsigned Count=lineCount(EditorReadableLabels067::Labels[11]);
inline constexpr unsigned lastScroll(){return Count>Rows?Count-Rows:0;}
inline constexpr bool contains(float x,float y){return x>=X&&y>=Y&&x<X+Width&&y<Y+Height;}
inline unsigned ensureVisible(unsigned selected,unsigned scroll){selected=std::min(selected,Count-1);scroll=std::min(scroll,lastScroll());if(selected<scroll)scroll=selected;if(selected>=scroll+Rows)scroll=selected-Rows+1;return scroll;}
inline unsigned wheel(unsigned scroll,int amount,float x,float y){return contains(x,y)?unsigned(std::clamp(int(std::min(scroll,lastScroll()))-amount,0,int(lastScroll()))):scroll;}
inline Line line(unsigned index){const char*first=EditorReadableLabels067::Labels[11];for(unsigned i=0;*first&&i<index;++i){while(*first&&*first!='\n')++first;if(*first)++first;}const char*end=first;while(*end&&*end!='\n')++end;return {first,unsigned(end-first)};}
inline float rowY(unsigned index,unsigned scroll){return Y+(int(index)-int(scroll))*Row;}
inline constexpr float baseline(unsigned row){return Y+row*Row+24.f;}
}
