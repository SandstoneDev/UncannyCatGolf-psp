#pragma once

struct Shot036 { unsigned wait; int x,y; };
struct Route036 { const Shot036* shots; unsigned count; };
inline constexpr Shot036 Shots412[]{{0,51,-122},{0,-127,-32}};
inline constexpr Shot036 Shots413[]{{0,86,-78},{0,93,-93}};
inline constexpr Route036 Route412{Shots412,std::size(Shots412)};
inline constexpr Route036 Route413{Shots413,std::size(Shots413)};
