#pragma once

#include "elysian_horse039.hpp"
#include "frontier18501_mechanics.hpp"
#include "jump_pad.hpp"

inline constexpr HorseCourse039 ElysianHorse518{
    512.0f, {-192.0f, 192.0f}, {1.0f, 0.0f}, 7, 200.0f, 120, {{
    }}, 30
};

inline constexpr Vec2 ElysianMines518[]={
    {-280.0f, -424.0f},
    {-376.0f, -224.0f},
    {-336.0f, 40.0f},
    {320.0f, 72.0f},
    {424.0f, 432.0f},
    {296.0f, -424.0f},
    {384.0f, -152.0f},
};
inline constexpr Vec2 ElysianMineDetectorOffset518={0.0f, -1.0f};
inline constexpr float ElysianMineRadius518=16.0f;
inline constexpr JumpStage ElysianJump518{1, {{{-176.0f, 56.0f}, 2.35619f, 875.0f}}};
inline constexpr CircleCollider ElysianTee518{{0.0f, -144.0f}, 40.0f};
inline constexpr SolidBlock ElysianCameraZone518{{0.0f, -144.0f}, {240.0f, 240.0f}};
inline constexpr Vec2 ElysianCameraTarget518={-128.0f, -144.0f};
inline constexpr SolidBlock ElysianEndWalls518[]={
    {{0.0f, -408.0f}, {272.0f, 8.0f}},
    {{0.0f, 120.0f}, {272.0f, 8.0f}},
    {{264.0f, -144.0f}, {8.0f, 272.0f}},
    {{-264.0f, -144.0f}, {8.0f, 272.0f}},
};
inline constexpr SolidBlock PrismGate601{{0.0f, 160.0f}, {112.0f, 32.0f}};
inline constexpr unsigned PrismGateRequired601=375;

inline WallContact sweepPrismGate601(Vec2 position,Vec2 motion,float radius) {
    WallContact contact;
    TileField::sweepBlock(PrismGate601,position,motion,radius,contact);
    return contact;
}
inline constexpr Vec2 ElysianRockVertices518_1[]={
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
inline constexpr Vec2 ElysianRockOffsets518_1[]={
    {728.0f, 496.0f},
    {-312.0f, 462.0f},
};
inline constexpr Vec2 ElysianRockVertices518_2[]={
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
inline constexpr Vec2 ElysianRockOffsets518_2[]={
    {-8.0f, -456.0f},
};

inline WallContact sweepElysianRocks518(Vec2 position,Vec2 motion,float radius) {
    const auto one=sweepElysianRockGroup<ElysianRockVertices518_1,ElysianRockOffsets518_1>(position,motion,radius);
    const auto two=sweepElysianRockGroup<ElysianRockVertices518_2,ElysianRockOffsets518_2>(position,motion,radius);
    return two.time<one.time?two:one;
}
inline constexpr Vec2 ElysianSandSouth518[]={
    {512.0f, 368.0f},
    {464.0f, 344.0f},
    {424.0f, 328.0f},
    {384.0f, 336.0f},
    {336.0f, 368.0f},
    {288.0f, 440.0f},
    {160.0f, 432.0f},
    {64.0f, 456.0f},
    {-72.0f, 432.0f},
    {-200.0f, 456.0f},
    {-272.0f, 432.0f},
    {-304.0f, 368.0f},
    {-336.0f, 336.0f},
    {-352.0f, 280.0f},
    {-392.0f, 264.0f},
    {-432.0f, 240.0f},
    {-480.0f, 248.0f},
    {-512.0f, 264.0f},
    {-512.0f, 512.0f},
    {512.0f, 512.0f},
};
inline constexpr Vec2 ElysianSandHole518[]={
    {0.0f, -432.0f},
    {-104.0f, -408.0f},
    {-160.0f, -392.0f},
    {-224.0f, -352.0f},
    {-256.0f, -288.0f},
    {-288.0f, -184.0f},
    {-280.0f, -40.0f},
    {-232.0f, 56.0f},
    {-112.0f, 136.0f},
    {0.0f, 152.0f},
    {160.0f, 112.0f},
    {232.0f, 40.0f},
    {248.0f, -8.0f},
    {272.0f, -88.0f},
    {272.0f, -160.0f},
    {256.0f, -264.0f},
    {208.0f, -368.0f},
    {104.0f, -408.0f},
};
inline constexpr Vec2 ElysianSandOuter518[]={
    {0.0f, -472.0f},
    {96.0f, -464.0f},
    {184.0f, -440.0f},
    {264.0f, -376.0f},
    {312.0f, -280.0f},
    {328.0f, -160.0f},
    {304.0f, -16.0f},
    {248.0f, 96.0f},
    {152.0f, 168.0f},
    {0.0f, 208.0f},
    {-104.0f, 184.0f},
    {-240.0f, 112.0f},
    {-296.0f, 40.0f},
    {-336.0f, -56.0f},
    {-344.0f, -184.0f},
    {-328.0f, -264.0f},
    {-280.0f, -360.0f},
    {-200.0f, -432.0f},
    {-120.0f, -464.0f},
};
inline constexpr Vec2 ElysianWaterHole518[]={
    {0.0f, -496.0f},
    {101.0f, -490.0f},
    {199.0f, -462.0f},
    {289.0f, -394.0f},
    {345.0f, -284.0f},
    {365.0f, -158.0f},
    {340.0f, -4.0f},
    {272.0f, 119.0f},
    {172.0f, 199.0f},
    {0.0f, 242.0f},
    {-116.0f, 215.0f},
    {-259.0f, 134.0f},
    {-320.0f, 50.0f},
    {-366.0f, -58.0f},
    {-371.0f, -187.0f},
    {-352.0f, -278.0f},
    {-299.0f, -379.0f},
    {-216.0f, -450.0f},
    {-125.0f, -489.0f},
};
inline constexpr Vec2 ElysianWaterOuter518[]={
    {0.0f, -512.0f},
    {-512.0f, -512.0f},
    {-512.0f, 240.0f},
    {-481.0f, 224.0f},
    {-427.0f, 218.0f},
    {-383.0f, 245.0f},
    {-334.0f, 265.0f},
    {-315.0f, 329.0f},
    {-285.0f, 357.0f},
    {-253.0f, 416.0f},
    {-194.0f, 427.0f},
    {-72.0f, 403.0f},
    {65.0f, 428.0f},
    {158.0f, 406.0f},
    {279.0f, 416.0f},
    {320.0f, 349.0f},
    {373.0f, 312.0f},
    {428.0f, 304.0f},
    {512.0f, 344.0f},
    {512.0f, -512.0f},
};

template<unsigned Count>
inline bool testElysianArea518(const Vec2 (&points)[Count],Vec2 position,float radiusSquared,bool& inside) {
    for (unsigned i=0;i<Count;++i) {
        const Vec2 start=points[i],end=points[(i+1)%Count];
        const Vec2 offset=position-start,edge=end-start;
        const float edgeSquared=dot(edge,edge),projection=dot(offset,edge);
        const float cross=edge.x*offset.y-edge.y*offset.x;
        if (dot(offset,offset)<=radiusSquared ||
            (projection>0 && projection<edgeSquared && cross*cross<=radiusSquared*edgeSquared)) return true;
        if ((start.y>position.y)!=(end.y>position.y) && (cross>0)==(edge.y>0)) inside=!inside;
    }
    return false;
}

inline bool touchesElysianWater518(Vec2 position,float radius) {
    bool inside=false;
    const float squared=radius*radius;
    return testElysianArea518(ElysianWaterOuter518,position,squared,inside) ||
           testElysianArea518(ElysianWaterHole518,position,squared,inside) || inside;
}

inline bool touchesElysianSand518(Vec2 position,float radius) {
    bool inside=false;
    const float squared=radius*radius;
    if (testElysianArea518(ElysianSandSouth518,position,squared,inside) || inside) return true;
    return testElysianArea518(ElysianSandOuter518,position,squared,inside) ||
           testElysianArea518(ElysianSandHole518,position,squared,inside) || inside;
}
