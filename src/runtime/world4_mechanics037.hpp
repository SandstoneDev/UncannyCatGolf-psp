#pragma once

#include "baddie_run.hpp"
#include "frontier1415_data037.hpp"
#include "golf.hpp"
#include "world4_mechanics036.hpp"

inline float frontierWindGust414(unsigned ticks) {
    const unsigned phase=ticks%FrontierWindCycle;
    if (phase<FrontierMovieWindStart || phase>=FrontierMovieWindFade) return 0.0f;
    if (phase<FrontierMovieWindPeak)
        return static_cast<float>(phase-FrontierMovieWindStart)/(FrontierMovieWindPeak-FrontierMovieWindStart);
    return static_cast<float>(FrontierMovieWindFade-phase)/(FrontierMovieWindFade-FrontierMovieWindPeak);
}

inline float frontierWindDb414(unsigned ticks) { return -10.0f+10.0f*frontierWindGust414(ticks); }
inline float frontierWindPitch414(unsigned ticks) { return 0.8f+0.4f*frontierWindGust414(ticks); }

inline float frontierSnowSeconds414(unsigned ticks) {
    const float cycles=static_cast<float>(ticks/FrontierWindCycle);
    const unsigned phase=ticks%FrontierWindCycle;
    const auto seconds=[](unsigned ticks) { return ticks*(1.0f/60.0f); };
    if (phase<FrontierMovieWindStart) return cycles+0.1f*seconds(phase);
    if (phase<FrontierMovieWindPeak) {
        const float elapsed=seconds(phase-FrontierMovieWindStart);
        return cycles+0.25f+elapsed*(0.1f+0.4f*elapsed);
    }
    if (phase<FrontierMovieWindFade) {
        const float elapsed=seconds(phase-FrontierMovieWindPeak);
        return cycles+0.4f+elapsed*(0.5f+0.2f*elapsed);
    }
    if (phase<FrontierMovieWindEnd) {
        const float elapsed=seconds(phase-FrontierMovieWindFade);
        return cycles+0.7f+elapsed*(0.7f-0.6f*elapsed);
    }
    return cycles+0.9f+0.1f*seconds(phase-FrontierMovieWindEnd);
}

inline Vec2 frontierSpotlight415(unsigned ticks) {
    constexpr unsigned half=120;
    const unsigned phase=ticks%(2*half);
    const float time=static_cast<float>(phase%half)/half;
    // Godot's cubic value track has zero tangents at the two extrema.
    const float blend=time*time*(3.0f-2.0f*time);
    const float y=192.0f-384.0f*blend;
    return {192.0f,phase<half?y:-y};
}

inline constexpr BaddieStage FrontierCar415{0,0,FrontierBaddieDespawn415,0xffffffffu,{}};

inline void restartFrontierCar415(BaddieRun& run) {
    run.restart(&FrontierCar415);
    run.cars[0]={FrontierCarCenter415,{8,8}};
    run.count=run.liveCount[0]=1;
}

inline void stepFrontierSkulls415(SkullRun& skulls,const TileField& walls,unsigned count,
                                  const BaddieRun& car,uint32_t openedDoors) {
    skulls.step(walls,count,nullptr,0,FrontierBaddieDespawn415,
                car.alive(0)?car.cars:nullptr,nullptr,
                FrontierKeys415.doors,FrontierKeys415.doorCount,openedDoors);
}
