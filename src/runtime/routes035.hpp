#pragma once

struct Shot035 { unsigned wait; int x,y; };
struct Route035 { const Shot035* shots; unsigned count; };
inline constexpr Shot035 Shots408[]{{0,22,-122},{0,127,23},{0,95,80}};
inline constexpr Shot035 Shots409[]{{0,-52,44},{0,-42,-91},{0,-98,46}};
inline constexpr Route035 Route408{Shots408,std::size(Shots408)};
inline constexpr Route035 Route409{Shots409,std::size(Shots409)};
