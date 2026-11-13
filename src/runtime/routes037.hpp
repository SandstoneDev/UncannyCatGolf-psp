#pragma once

struct Shot037 { unsigned wait; int x,y; };
struct Route037 { const Shot037* shots; unsigned count; };
inline constexpr Shot037 Shots416[]{{0,24,-122},{0,108,0},{0,-67,13},{0,-96,-79}};
inline constexpr Shot037 Shots417[]{{0,-47,-88},{0,79,96},{0,-127,-13},{0,-96,-29},{0,102,84},
    {0,84,-102},{0,38,-126},{0,-69,103},{0,102,84},{0,96,64},{0,-116,62},{0,93,-93},{0,116,0}};
inline constexpr Route037 Route416{Shots416,std::size(Shots416)};
inline constexpr Route037 Route417{Shots417,std::size(Shots417)};
