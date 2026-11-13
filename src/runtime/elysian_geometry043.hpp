#pragma once

#include "baddie_run.hpp"
#include "chuckle_run.hpp"
#include "elysian_horse039.hpp"
#include "frontier18501_mechanics.hpp"
#include "jump_pad.hpp"
#include "portal.hpp"

inline constexpr HorseCourse039 ElysianHorse514{
    384.0f, {0.0f, -192.0f}, {-1.0f, 1.0f}, 8, 80.0f, 600, {{
        {{-256.0f, -112.0f}, -1, true},
        {{-256.0f, 112.0f}, -1, true},
        {{256.0f, 112.0f}, -1, true},
        {{256.0f, -112.0f}, -1, true},
        {{112.0f, -256.0f}, -1, true},
        {{-112.0f, -256.0f}, -1, true},
        {{112.0f, 256.0f}, -1, true},
        {{-112.0f, 256.0f}, -1, true},
    }}
};

inline constexpr HorseCourse039 ElysianHorse515{
    448.0f, {-320.0f, -320.0f}, {1.0f, 0.3f}, 3, 125.0f, 450, {{
        {{-352.0f, -160.0f}, -1, true},
        {{-352.0f, 288.0f}, -1, true},
        {{352.0f, -288.0f}, -1, true},
    }}
};

inline constexpr SolidBlock ElysianSwitchBlocks514[]={
    {{-256.0f, 256.0f}, {16.0f, 16.0f}},
    {{-256.0f, -256.0f}, {16.0f, 16.0f}},
    {{256.0f, -256.0f}, {16.0f, 16.0f}},
    {{256.0f, 256.0f}, {16.0f, 16.0f}},
};

inline constexpr Vec2 ElysianNukes514[]={
    {-256.0f, 0.0f},
    {0.0f, -256.0f},
    {256.0f, 0.0f},
};

inline constexpr BaddieStage ElysianSkulls514{1, 2, 512.0f, 0xffffd9ccu, {
    {{-64.0f, -64.0f}, {-88.38834764831843f, 88.38834764831843f}, 100, 100, 12},
    {{64.0f, 64.0f}, {-88.38834764831843f, 88.38834764831843f}, 100, 100, 12},
}};

inline constexpr ChuckleSpawner ElysianChuckles514[]={
    {{64.0f, -64.0f}, 10, 60, 9},
    {{-64.0f, 64.0f}, 10, 60, 9},
};

inline constexpr JumpStage ElysianJump515{7, {
    {{160.0f, 160.0f}, 0.0f, 0.0f},
    {{352.0f, 160.0f}, 1.5708f, 0.0f},
    {{352.0f, 352.0f}, 3.14159f, 0.0f},
    {{160.0f, 352.0f}, 4.71239f, 0.0f},
    {{-160.0f, 32.0f}, 4.71239f, 0.0f},
    {{32.0f, -160.0f}, 1.5708f, 0.0f},
    {{-208.0f, 288.0f}, 0.0f, 0.0f},
}};

inline constexpr PortalNode ElysianPortalNodes515[]={
    {{32.0f, 256.0f}, 1},
    {{-32.0f, -352.0f}, 0},
    {{-416.0f, -416.0f}, 3},
    {{-304.0f, 160.0f}, PortalNode::NoTarget},
};


inline float elysianCubicPingPong515(unsigned tick,unsigned period) {
    const unsigned phase=tick%period;
    const unsigned distance=std::min(phase,period-phase);
    const float t=static_cast<float>(distance)/static_cast<float>(period/2);
    return t*t*(3.0f-2.0f*t);
}

inline void updateElysianMotion515(unsigned tick,JumpStage& pads,PortalNode* portals) {
    const float shortTrack=192.0f*elysianCubicPingPong515(tick,60);
    const float longTrack=elysianCubicPingPong515(tick,300);
    const float angle=static_cast<float>(tick%120)/120.0f;
    pads.pads[0].center.x=160.0f+shortTrack;
    pads.pads[1].center.y=160.0f+shortTrack;
    pads.pads[2].center.x=352.0f-shortTrack;
    pads.pads[3].center.y=352.0f-shortTrack;
    pads.pads[4].center.x=-160.0f+448.0f*longTrack;
    pads.pads[5].radians=1.5708f+(-4.71239f-1.5708f)*angle;
    pads.pads[6].radians=-6.28319f*angle;
    portals[1].position.y=-352.0f+704.0f*longTrack;
    portals[2].position.x=160.0f-576.0f*longTrack;
}

inline constexpr Vec2 ElysianRockVertices515_1[]={
    {-192.0f, -44.0f},
    {-172.0f, -44.0f},
    {-152.0f, -24.0f},
    {-144.0f, -4.0f},
    {-140.0f, 16.0f},
    {-140.0f, 28.0f},
    {-132.0f, 44.0f},
    {-136.0f, 60.0f},
    {-148.0f, 72.0f},
    {-168.0f, 72.0f},
    {-184.0f, 76.0f},
    {-220.0f, 76.0f},
    {-236.0f, 68.0f},
    {-244.0f, 68.0f},
    {-256.0f, 64.0f},
    {-276.0f, 64.0f},
    {-280.0f, 60.0f},
    {-280.0f, 36.0f},
    {-272.0f, 16.0f},
    {-260.0f, 0.0f},
    {-256.0f, -8.0f},
    {-248.0f, -16.0f},
    {-236.0f, -24.0f},
    {-220.0f, -32.0f},
};
inline constexpr Vec2 ElysianRockOffsets515_1[]={
    {664.0f, 392.0f},
    {200.0f, 416.0f},
};
inline constexpr Vec2 ElysianRockVertices515_2[]={
    {0.0f, 0.0f},
    {24.0f, 4.0f},
    {64.0f, 20.0f},
    {80.0f, 28.0f},
    {88.0f, 36.0f},
    {92.0f, 52.0f},
    {88.0f, 68.0f},
    {84.0f, 76.0f},
    {68.0f, 92.0f},
    {60.0f, 92.0f},
    {44.0f, 96.0f},
    {36.0f, 96.0f},
    {20.0f, 92.0f},
    {8.0f, 96.0f},
    {-20.0f, 96.0f},
    {-36.0f, 88.0f},
    {-60.0f, 76.0f},
    {-72.0f, 64.0f},
    {-72.0f, 28.0f},
    {-56.0f, 16.0f},
    {-40.0f, 8.0f},
    {-32.0f, 4.0f},
    {-28.0f, 0.0f},
};
inline constexpr Vec2 ElysianRockOffsets515_2[]={
    {456.0f, -64.0f},
    {-464.0f, 16.0f},
};

inline WallContact sweepElysianRocks515(Vec2 position,Vec2 motion,float radius) {
    const auto one=sweepElysianRockGroup<ElysianRockVertices515_1,ElysianRockOffsets515_1>(position,motion,radius);
    const auto two=sweepElysianRockGroup<ElysianRockVertices515_2,ElysianRockOffsets515_2>(position,motion,radius);
    return two.time<one.time?two:one;
}
