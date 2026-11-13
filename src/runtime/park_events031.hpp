#pragma once

#include "smiley_ring.hpp"

// World-space geometry from 3_10.tscn.
inline constexpr Vec2 LightsOutZone[]={
    {144,-80},{80,-80},{80,-64},{144,-64},{144,0},
    {80,0},{80,16},{80,80},{192,80},{192,-80}
};
inline constexpr Vec2 AshBabyZone[]={{-56,144},{-48,168},{-56,192},{0,192},{0,144}};
inline constexpr uint32_t ParkScareTint=0xff696969;

struct ParkScareUpdate {
    bool lightsOut=false,ashBaby=false;
};

struct ParkScareRun {
    bool lightsOut=false,ashTriggered=false;
    uint8_t ashAlpha=0;

    ParkScareUpdate step(Vec2 player,float playerRadius,bool scareEnabled=true) {
        ParkScareUpdate event;
        if (ashAlpha) ashAlpha-=2;
        if (!lightsOut && overlaps(LightsOutZone,player,playerRadius)) {
            lightsOut=true;
            event.lightsOut=true;
        }
        if (!ashTriggered && overlaps(AshBabyZone,player,playerRadius)) {
            ashTriggered=true;
            if (scareEnabled) {
                ashAlpha=254;
                event.ashBaby=true;
            }
        }
        return event;
    }

private:
    template <std::size_t N>
    static bool overlaps(const Vec2 (&polygon)[N],Vec2 point,float radius) {
        bool inside=false;
        const float radiusSquared=radius*radius;
        for (std::size_t i=0;i<N;++i) {
            const Vec2 a=polygon[i],b=polygon[(i+1)%N];
            const Vec2 edge=b-a,offset=point-a;
            const float projection=std::clamp(dot(offset,edge)/dot(edge,edge),0.0f,1.0f);
            const Vec2 delta=offset-edge*projection;
            if (dot(delta,delta)<=radiusSquared) return true;
            if ((a.y>point.y)!=(b.y>point.y) && point.x<a.x+edge.x*(point.y-a.y)/edge.y)
                inside=!inside;
        }
        return inside;
    }
};

// Red, green, blue, final. The three keys spawn on the matching jump pads.
inline constexpr uint32_t ParkRingTints[]={0xff8080ff,0xff95ff80,0xffffd480,0xff8ceeff};
inline constexpr bool ParkRingBells[]={true,false,false,false};
inline constexpr SmileyStage ParkRingStages[]={
    {{-32,0},24,10,5,600,2000,110,83,0,
     {{-352,-448},{-320,-448},{-288,-448},{-256,-448},{-224,-448}}},
    {{0,-32},24,10,5,1200,2000,110,83,0,
     {{448,-352},{448,-320},{448,-288},{448,-256},{448,-224}}},
    {{32,0},24,10,5,1800,2000,110,83,0,
     {{352,448},{320,448},{288,448},{256,448},{224,448}}},
    {{0,32},24,10,5,2400,2000,110,83,0,
     {{-448,352},{-448,320},{-448,288},{-448,256},{-448,224}}}
};
static_assert(std::size(ParkRingStages)==std::size(ParkRingTints));
static_assert(std::size(ParkRingStages)==std::size(ParkRingBells));

inline constexpr float ParkChuckleDespawn=640.0f;
inline constexpr Vec2 ParkChuckles[]={
    {88,88},{360,88},{368,368},{192,368},{56,472},{472,472},{-472,472},{-368,368},
    {-80,368},{-88,88},{-472,56},{-88,-88},{88,-88},{472,-56},{472,-472},{368,-368},
    {368,-192},{88,-360},{-56,-472},{-192,-368},{-360,-88},{-472,-472}
};
