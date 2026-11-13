#pragma once

struct Polyrhythm049 {
    static constexpr unsigned Pink=1,Cyan=2,Orange=4,All=Pink|Cyan,Triple=All|Orange;
    unsigned mask=All;
    float particleSeconds=0,particleSpeed=0.8f;
    float orangeSeconds=0,orangeSpeed=0.8f;

    unsigned step(unsigned beats,bool beatChanged,bool threeLayers=false) {
        particleSpeed+=(0.1f-particleSpeed)*0.05f;
        particleSeconds+=particleSpeed/60;
        if (threeLayers) {
            orangeSpeed+=(0.1f-orangeSpeed)*0.05f;
            orangeSeconds+=orangeSpeed/60;
        }
        if (!beatChanged) return 0;
        if (beats%2==0) {
            mask&=~Pink;
            if (beats%4==2) mask|=Pink;
        }
        unsigned events=0;
        if (beats%3==0) {
            mask&=~Cyan;
            if (beats%6==(threeLayers?3u:0u)) mask|=Cyan;
            particleSpeed=1;
            events|=Cyan;
        }
        if (threeLayers && beats%5==0) {
            mask&=~Orange;
            if (beats%10==5) mask|=Orange;
            orangeSpeed=1;
            events|=Orange;
        }
        return events;
    }

    unsigned collisionState(bool threeLayers=false) const {
        return mask?mask-1:threeLayers?7:3;
    }
};
