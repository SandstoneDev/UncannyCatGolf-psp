#pragma once
#include "game.hpp"
#include <array>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace EndlessPresentation {
struct Trail { Vec2 position{},drift{};float scale=1;unsigned age=20; };
struct State {
    unsigned page=0,previousPage=0,slideUs=250000,flashUs=0,scareUs=0;
    int direction=1;
    uint64_t clockUs=0,introUs=0;
    bool demonSeen=false,introFinished=false;
    uint32_t random=1;
    float originalZoom=1,zoom=1;
    Vec2 originalCenter{},center{},target{};
    std::array<Trail,20> trails{};
    unsigned trailHead=0;

    void menu(bool hasScores) {page=previousPage=hasScores?0:1;slideUs=250000;}
    void turn(int dir) {
        if (slideUs<250000) return;
        previousPage=page;direction=dir<0?-1:1;
        page=(page+(direction<0?6:1))%7;slideUs=0;
    }
    float slide() const {
        const float t=std::min(slideUs/250000.f,1.f);
        return t<.5f?4*t*t*t:1-std::pow(-2*t+2,3.f)*.5f;
    }
    void startIntro(float baseZoom,Vec2 baseCenter,Vec2 demon) {
        demonSeen=true;introUs=1;introFinished=false;
        originalZoom=zoom=baseZoom;originalCenter=center=baseCenter;
        target=demon-Vec2{32,0};trails={};trailHead=0;
        for (auto& trail:trails) trail.age=20;
    }
    bool frozen() const {return introUs && introUs<=1150000;}
    bool camera() const {return introUs && introUs<1650000;}
    float cameraRatio() const {return camera()?zoom/originalZoom:1;}
    float opacity() const {return frozen()?std::min(introUs/1000000.f,1.f):1;}
    uint64_t advance(uint64_t delta,bool paused=false) {
        clockUs+=delta;slideUs=static_cast<unsigned>(std::min<uint64_t>(250000,slideUs+delta));
        scareUs=scareUs>delta?scareUs-static_cast<unsigned>(delta):0;
        if (!paused) flashUs=flashUs>delta?flashUs-static_cast<unsigned>(delta):0;
        introFinished=false;
        const uint64_t old=introUs;
        if (camera()) {
            introUs=std::min<uint64_t>(1650000,introUs+delta);
            if (old<=1150000 && introUs>1150000) introFinished=true;
            const unsigned count=static_cast<unsigned>(std::min<uint64_t>(100,introUs/10000));
            const float amount=1-std::pow(.9f,static_cast<float>(count));
            zoom=originalZoom+(3.5f-originalZoom)*amount;
            center=originalCenter+(target-originalCenter)*amount;
            if (introUs>1150000) {
                const unsigned back=static_cast<unsigned>(std::min<uint64_t>(100,(introUs-1150000)/5000));
                const float remaining=std::pow(.9f,static_cast<float>(back));
                zoom=originalZoom+(zoom-originalZoom)*remaining;
                center=originalCenter+(center-originalCenter)*remaining;
            }
        }
        if (paused) return 0;
        if (old && old<=1150000) return delta>1150001-old?delta-(1150001-old):0;
        return delta;
    }
    unsigned draw(unsigned maximum) {
        random^=random<<13;random^=random>>17;random^=random<<5;
        return random%maximum;
    }
    void trail(Vec2 position) {
        for (auto& entry:trails) if (entry.age<20) ++entry.age;
        const Vec2 drift{(static_cast<int>(draw(2001))-1000)*.00064f,(static_cast<int>(draw(2001))-1000)*.00064f};
        trails[trailHead]={position,drift,.8f+draw(301)*.001f,0};
        trailHead=(trailHead+1)%trails.size();
    }
    void extraSkip() {flashUs=1000000;}
    void caught() {scareUs=700000;}
    float skipScale() const {
        const float age=(1000000-flashUs)/1000000.f;
        return 1+.25f*std::max(0.f,1-std::abs(age-.25f)*4);
    }
    static float hurryPitch(unsigned ticks) {return 1+.4f*(10-std::min(ticks,600u)/60.f);}
};
}
