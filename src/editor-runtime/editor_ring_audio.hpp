#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>

// Module-only independent SmileyRing AudioStreamPlayers. Prepare/clear with
// audio threads stopped; setVoice/setPaused/stopAll are main-thread commands.
// mix owns all cursors and performs no allocation, locks, or file access.
class EditorRingAudio {
public:
    template<class Reader>
    bool prepare(Reader& reader,unsigned id,unsigned count,unsigned blockFrames=1024) {
        if (!count) { clear();return true; }
        const unsigned length=reader.frames(id),loop=reader.loopFrame(id);
        if (!length||loop>=length||!reader.loop(id)||!blockFrames||
            length>SIZE_MAX/(2*sizeof(int16_t))||count>SIZE_MAX/sizeof(Voice)||
            blockFrames>SIZE_MAX/(2*sizeof(int64_t))) return false;
        std::unique_ptr<int16_t[]> samples(new(std::nothrow) int16_t[size_t(length)*2]);
        std::unique_ptr<Voice[]> next(new(std::nothrow) Voice[count]);
        std::unique_ptr<int64_t[]> accumulation(new(std::nothrow) int64_t[size_t(blockFrames)*2]);
        if (!samples||!next||!accumulation) return false;
        for (unsigned at=0;at<length;) {
            const unsigned n=std::min(4096u,length-at);
            if (reader.readEffect(id,at,samples.get()+size_t(at)*2,n)!=n) return false;
            at+=n;
        }
        pcm=std::move(samples);voices=std::move(next);scratch=std::move(accumulation);
        frames=length;loopFrame=loop;voiceCount=count;scratchFrames=blockFrames;
        paused.store(false,std::memory_order_relaxed);return true;
    }
    void clear() {
        pcm.reset();voices.reset();scratch.reset();frames=loopFrame=voiceCount=scratchFrames=0;
        paused.store(false,std::memory_order_relaxed);
    }
    unsigned count() const { return voiceCount; }
    unsigned sourceFrames() const { return frames; }
    void setPaused(bool enabled) { paused.store(enabled,std::memory_order_relaxed); }
    void setVoice(unsigned index,bool enabled,float pitch) {
        if (index>=voiceCount||!std::isfinite(pitch)) return;
        auto& v=voices[index];
        const unsigned step=unsigned(std::lround(std::clamp(pitch,.125f,4.f)*65536.f));
        v.step.store(step,std::memory_order_relaxed);
        if (enabled!=v.requested) {
            v.requested=enabled;
            if (enabled) v.generation=(v.generation+1)&0x7fffffffu;
            v.command.store((v.generation<<1)|unsigned(enabled),std::memory_order_release);
        }
    }
    void stopAll() { for(unsigned i=0;i<voiceCount;++i)setVoice(i,false,.5f); }
    void mix(int32_t* out,unsigned size,unsigned busGain=256) {
        if (!out||!pcm||!voices||!scratchFrames) return;
        for (unsigned offset=0;offset<size;) {
            const unsigned n=std::min(size-offset,scratchFrames);
            mixBlock(out+size_t(offset)*2,n,std::min(busGain,256u));offset+=n;
        }
    }
private:
    struct Voice {
        std::atomic<uint32_t> command{0},step{32768};
        // Main-thread-only fields.
        bool requested=false;
        uint32_t generation=0;
        // Mixer-thread-only fields.
        uint32_t seen=0;
        uint64_t phase=0;
    };
    void mixBlock(int32_t* out,unsigned size,unsigned gain) {
        std::fill_n(scratch.get(),size_t(size)*2,int64_t(0));
        const bool held=paused.load(std::memory_order_relaxed);
        const uint64_t end=uint64_t(frames)<<16,start=uint64_t(loopFrame)<<16;
        for(unsigned voice=0;voice<voiceCount;++voice) {
            auto& v=voices[voice];const unsigned command=v.command.load(std::memory_order_acquire);
            if (command!=v.seen) {
                if ((command>>1)!=(v.seen>>1))v.phase=0;
                v.seen=command;
            }
            if (!(command&1)||held)continue;
            const unsigned step=v.step.load(std::memory_order_relaxed);
            for(unsigned i=0;i<size;++i) {
                const unsigned at=unsigned(v.phase>>16),fraction=unsigned(v.phase&65535);
                const unsigned next=at+1<frames?at+1:loopFrame;
                if(gain)for(unsigned channel=0;channel<2;++channel) {
                    const int32_t a=pcm[size_t(at)*2+channel],b=pcm[size_t(next)*2+channel];
                    const int64_t sample=int64_t(a)+(int64_t(b-a)*fraction)/65536;
                    scratch[size_t(i)*2+channel]+=sample*gain/256;
                }
                v.phase+=step;
                if(v.phase>=end)v.phase=start+(v.phase-end)%(end-start);
            }
        }
        for(unsigned i=0;i<size*2;++i)out[i]=int32_t(std::clamp(int64_t(out[i])+scratch[i],int64_t(INT32_MIN),int64_t(INT32_MAX)));
    }
    std::unique_ptr<int16_t[]> pcm;
    std::unique_ptr<Voice[]> voices;
    std::unique_ptr<int64_t[]> scratch;
    unsigned frames=0,loopFrame=0,voiceCount=0,scratchFrames=0;
    std::atomic<bool> paused{false};
};
