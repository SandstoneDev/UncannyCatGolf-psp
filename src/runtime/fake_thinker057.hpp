#pragma once
namespace FakeThinker057 {
// true means the selected actor handled the draw, including authored Nothing
// and fully clipped sprites. false retains the original Canny/not-ready path.
inline bool drawSelected(float x,float y) {
    if (!Costumes054::screenActorReady056()) return false;
    const unsigned width=Costumes054::screenBank056.width[0];
    const unsigned height=Costumes054::screenBank056.height[0];
    if (!width || !height) return true;
    constexpr float Scale=2.75f,Raster=Scale*17.f/64;
    const float halfWidth=width*Raster*.5f,halfHeight=height*Raster*.5f;
    // Same native248-source-pixel clip used by both authored FakeThinkers.
    if (x+halfWidth<=ViewX || x-halfWidth>=ViewX+132 ||
        y+halfHeight<=0 || y-halfHeight>=Height) return true;
    return Costumes054::drawScreenActor056(false,x,y,Scale);
}
}
