#pragma once

#include "course_objects.hpp"
#include <cstring>

class MenuTv {
public:
    bool load(ThoughtPlayer& source) {
        unsigned count=0;
        for (unsigned i=0;i<std::size(Clips);++i) {
            const auto& clip=source.clip(Clips[i]);
            if (!clip.count || clip.count>Capacity-count) return false;
            starts[i]=count;
            timings[i]=clip;
            for (unsigned frame=0;frame<clip.count;++frame) {
                const auto* pixels=source.sourceFrame(clip.first+frame);
                if (!pixels) return false;
                std::memcpy(frames[count++],pixels,ThoughtBank::FrameBytes);
            }
        }
        sceKernelDcacheWritebackRange(frames,count*ThoughtBank::FrameBytes);
        loaded=count;
        return true;
    }

    unsigned cachedFrames() const { return loaded; }

    const uint16_t* frame(unsigned tick) {
        if (!loaded) return nullptr;
        if (!started || tick-started>=duration()) {
            if (started) current=1+current%(std::size(Clips)-1);
            started=tick;
        }
        const unsigned elapsed=tick-started;
        if (elapsed<StaticTicks) return frames[starts[0]+(tick/2)%timings[0].count];
        const auto& clip=timings[current];
        const unsigned number=(elapsed-StaticTicks)*clip.fps/60;
        return frames[starts[current]+number%clip.count];
    }

private:
    // The original idle clips and static bumper fit as complete, lossless 5650 frames.
    static constexpr unsigned Clips[]={ThoughtPlayer::StaticClip,59,111};
    static constexpr unsigned Capacity=10,StaticTicks=12;

    unsigned duration() const {
        const auto& clip=timings[current];
        return StaticTicks+(clip.count*(clip.loops+1)*60+clip.fps-1)/clip.fps;
    }

    ThoughtBank::Entry timings[std::size(Clips)]{};
    unsigned starts[std::size(Clips)]{};
    unsigned loaded=0,started=0,current=1;
    alignas(64) static uint16_t frames[Capacity][ThoughtBank::FrameBytes/sizeof(uint16_t)];
};

alignas(64) inline uint16_t MenuTv::frames[Capacity][ThoughtBank::FrameBytes/sizeof(uint16_t)]{};
