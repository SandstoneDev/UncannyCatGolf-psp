#pragma once

struct FrontierShot034 { unsigned wait; int x,y; };
struct FrontierRoute034 { const FrontierShot034* shots; unsigned count; };

inline constexpr FrontierShot034 Shots404[]={{0,82,98},{0,92,77}};
inline constexpr FrontierShot034 Shots405[]={
    {0,-118,21},{0,64,-92},{0,-111,-64},{0,-91,91},{0,79,79},
};
inline constexpr FrontierRoute034 Route404{Shots404,std::size(Shots404)};
inline constexpr FrontierRoute034 Route405{Shots405,std::size(Shots405)};
