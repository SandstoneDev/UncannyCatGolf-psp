#pragma once

#include "game.hpp"
#include "jump_pad.hpp"
#include "key_doors.hpp"
#include "cosmic_geometry047.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

inline constexpr JumpPad FrontierJump412[]{{{-184,-272},1.5708f,0.0f}};
inline constexpr uint32_t FrontierKeyColor412=0xffff0000u;
inline constexpr KeyDoorStage FrontierKeys412{
    1,1,{{{-184,-272},FrontierKeyColor412}},{{{88,144},{24,48}}},{FrontierKeyColor412},{},
};

struct FrontierLightColor {
    float red,green,blue;
};

struct FrontierPointLight {
    Vec2 center;
    float diameter,energy;
    FrontierLightColor color;
    int flashIndex=-1;
};

inline constexpr FrontierLightColor FrontierAmbient412{0.245599f,0.326674f,0.491381f};
inline constexpr FrontierLightColor FrontierAmbient413{0.245f,0.245f,0.49f};
inline constexpr FrontierLightColor FrontierWhiteLight{1,1,1};
inline constexpr FrontierLightColor FrontierRedLight{1,0,0};
inline constexpr FrontierLightColor FrontierCyanLight{0.678431f,1,1};
inline constexpr float FrontierMovingLightDiameter=128.0f;
inline constexpr float FrontierPlayerLightEnergy=0.5f, FrontierGhostLightEnergy=1.0f, FrontierSkullLightEnergy=0.2f;
inline constexpr FrontierPointLight FrontierStaticLights412[]{
    {{-184,-272},128.0f,0.2f,FrontierWhiteLight},
    {{5,-64},256.0f,1.0f,FrontierCyanLight,0},
    {{184,144},256.0f,1.0f,FrontierCyanLight,1},
    {{-192,88},256.0f,1.0f,FrontierCyanLight,2},
};
inline constexpr FrontierPointLight FrontierStaticLights413[]{
    {{0,232},256.0f,1.0f,FrontierCyanLight,0},
};

class FrontierFlashRun {
public:
    void restart(unsigned stageSeed) {
        for (unsigned i=0;i<lamps.size();++i) {
            const uint32_t seed=stageSeed*0x9e3779b9u+(i+1)*0x85ebca6bu;
            lamps[i]=Lamp{};
            lamps[i].random=seed?seed:1u;
        }
    }

    void step() {
        for (auto& lamp:lamps) {
            lamp.time+=2;
            if (lamp.time>10) {
                lamp.alpha-=lamp.nextRandom();
                lamp.time=static_cast<int>(lamp.nextRandom()*65.0f-60.0f);
            }
            lamp.alpha+=(1.0f-lamp.alpha)*0.1f;
        }
    }

    float brightness(unsigned index) const {
        return index<lamps.size()?std::clamp(lamps[index].alpha,0.0f,1.0f):1.0f;
    }

private:
    struct Lamp {
        int time=0;
        float alpha=1.0f;
        uint32_t random=1;

        float nextRandom() {
            random^=random<<13;
            random^=random>>17;
            random^=random<<5;
            return static_cast<float>(random>>8)*(1.0f/16777216.0f);
        }
    };
    std::array<Lamp,3> lamps{};
};

inline constexpr unsigned FrontierWindCycle=300;
inline constexpr unsigned FrontierWindStart=120,FrontierWindPeak=150,FrontierWindFade=270;

inline constexpr unsigned FrontierMovieWindStart=150,FrontierMovieWindPeak=180;
inline constexpr unsigned FrontierMovieWindFade=210,FrontierMovieWindEnd=240;

struct FrontierWindRun {
    bool active=false,vertical=false,shotMotion=false,movieOnce=false,starstorm=false;

    void restart(bool alongY) { *this={true,alongY,false,false}; }
    void restartMovieOnce() { *this={true,true,false,true}; }
    void restartStarstorm() { *this={true,false,false,false,true}; }

    Vec2 acceleration(unsigned elapsedTicks) const {
        if (!active) return {};
        if (starstorm) return CosmicWindVelocity605(elapsedTicks);
        const unsigned phase=elapsedTicks%FrontierWindCycle;
        if (movieOnce) {
            if (phase<FrontierMovieWindStart || phase>=FrontierMovieWindEnd) return {};
            const bool peak=phase>=FrontierMovieWindPeak && phase<FrontierMovieWindFade;
            return {0,peak?-15.0f:-5.0f};
        }
        // Godot's player velocity track uses discrete update mode.
        const float amount=phase<FrontierWindStart?0.0f:
                           phase<FrontierWindPeak || phase>=FrontierWindFade?5.0f:25.0f;
        return vertical?Vec2{0,amount}:Vec2{amount,0};
    }

    bool drifting() const { return active && !shotMotion; }
    void beginMove() { if (active) shotMotion=true; }
    void afterStep(const Game& game) { if (active && !game.moving()) shotMotion=false; }
};

inline float frontierWindGust(unsigned ticks) {
    const unsigned phase=ticks%FrontierWindCycle;
    if (phase<FrontierWindStart || phase>=FrontierWindFade) return 0.0f;
    if (phase<FrontierWindPeak)
        return static_cast<float>(phase-FrontierWindStart)/(FrontierWindPeak-FrontierWindStart);
    return static_cast<float>(FrontierWindFade-phase)/(FrontierWindFade-FrontierWindPeak);
}

inline float frontierWindDb(unsigned ticks) { return -10.0f+10.0f*frontierWindGust(ticks); }
inline float frontierWindPitch(unsigned ticks) { return 0.8f+0.4f*frontierWindGust(ticks); }

inline float frontierSnowSeconds(unsigned ticks) {
    constexpr float secondsPerTick=1.0f/60.0f;
    const float cycles=2.0f*(ticks/FrontierWindCycle);
    const unsigned phase=ticks%FrontierWindCycle;
    if (phase<FrontierWindStart) return cycles+0.1f*phase*secondsPerTick;
    if (phase<FrontierWindPeak) {
        const float elapsed=(phase-FrontierWindStart)*secondsPerTick;
        return cycles+0.2f+elapsed*(0.1f+0.6f*elapsed);
    }
    if (phase<FrontierWindFade) return cycles+0.4f+0.7f*(phase-FrontierWindPeak)*secondsPerTick;
    const float elapsed=(phase-FrontierWindFade)*secondsPerTick;
    return cycles+1.8f+elapsed*(0.7f-0.6f*elapsed);
}
