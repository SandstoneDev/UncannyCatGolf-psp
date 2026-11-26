#pragma once

// PSP pixels. Text uses the same locale scale as the campaign live stats.
namespace EndlessHudLayout066 {
constexpr float Left=5, Width=126, ScoreHeaderY=63, ScoreY=81;
constexpr float ClearedY=99, Row=18, NameLeading=17;
constexpr float NameCenter=74, NameWidth=120, NameBottom=170;
constexpr float SkipY=204, Right=134;
struct Rows { bool inlineCleared; float clearedValueY,nextY,nameY; };
inline Rows rows(unsigned labelWidth,unsigned valueWidth,unsigned spaceWidth,float scale) {
    const bool fits=(labelWidth+spaceWidth+valueWidth)*scale<=Width;
    return {fits,fits?ClearedY:ClearedY+Row,fits?117.f:135.f,fits?135.f:153.f};
}
inline float skipCenter(float width,float originalCenter) {
    return originalCenter+width*.5f>Right?Right-width*.5f:originalCenter;
}
}
