#pragma once
namespace ArleRandomizer056 {
inline constexpr unsigned First=365,Count=6;
inline constexpr float Pitch=1.f,VolumeDb=0.f;
// One shared State belongs to the shared original randomizer resource, not
// each voice/caller. Invoke select only for an actual playback request.
struct State {
    unsigned previous=Count;
    template<class Uniform> unsigned select(Uniform&& uniform) {
        unsigned next=uniform(previous<Count?Count-1:Count);
        if(previous<Count && next>=previous)++next;
        previous=next;
        return First+next;
    }
};
}
