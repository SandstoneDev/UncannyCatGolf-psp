#pragma once

inline constexpr unsigned StarstormCycle047=270;

inline float starstormGust047(unsigned tick) {
    const unsigned phase=tick%StarstormCycle047;
    if (phase<120 || phase>=180) return 0;
    const float t=(phase<150?phase-120:180-phase)/30.0f;
    return t*t*(3-2*t);
}

inline float starstormParticleTime047(unsigned tick) {
    // Integral of the speed-scale track, starting with two seconds of preprocess.
    const unsigned phase=tick%StarstormCycle047;
    const float cycles=0.9375f*(tick/StarstormCycle047);
    if (phase<120) return 2+cycles+phase/600.0f;
    if (phase<150) {
        const float seconds=(phase-120)/60.0f;
        return 2+cycles+0.2f+seconds*(0.1f+0.6f*seconds);
    }
    if (phase<180) return 2+cycles+0.4f+(phase-150)*0.7f/60;
    const float seconds=(phase-180)/60.0f;
    if (seconds<0.125f) return 2+cycles+0.75f+seconds*(0.7f-2.4f*seconds);
    return 2+cycles+0.8f+(seconds-0.125f)*0.1f;
}

inline uint32_t starstormTint047(uint32_t color) {
    return (color&0xff000000u) | (((color&0xffu)*204/255)) |
           ((((color>>8)&0xffu)*204/255)<<8) | ((((color>>16)&0xffu)*204/255)<<16);
}
