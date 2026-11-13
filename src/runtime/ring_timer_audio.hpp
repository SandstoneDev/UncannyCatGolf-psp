#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <pspsysmem.h>

class RingTimerAudio {
public:
    static constexpr unsigned Voices=5;
    ~RingTimerAudio() { close(); }
    struct Voice { unsigned command=0,elapsed=0,phase=0; };

    bool open(const char* path) {
        std::FILE* file=std::fopen(path,"rb");
        if (!file) return false;
        uint32_t header[2]{};
        bool ok=std::fread(header,sizeof header,1,file)==1 &&
                header[0]==0x54524355 && header[1]>1 && header[1]<65535;
        SceUID block=-1;
        int16_t* next=nullptr;
        if (ok) {
            block=sceKernelAllocPartitionMemory(2,"ucg-ring-timer",PSP_SMEM_Low,header[1]*4,nullptr);
            if (block>=0) next=static_cast<int16_t*>(sceKernelGetBlockHeadAddr(block));
            ok=next && std::fread(next,2*sizeof(int16_t),header[1],file)==header[1] &&
               std::fgetc(file)==EOF && !std::ferror(file);
        }
        std::fclose(file);
        if (!ok) {
            if (block>=0) sceKernelFreePartitionMemory(block);
            return false;
        }
        close();
        pcm=next;
        partitionBlock=block;
        frames=header[1];
        for (auto& command:commands) command.store(0,std::memory_order_relaxed);
        return true;
    }

    void close() {
        if (partitionBlock>=0) sceKernelFreePartitionMemory(partitionBlock);
        partitionBlock=-1;
        pcm=nullptr;
        frames=0;
    }

    void play(unsigned slot,unsigned ticks) {
        if (slot>=Voices || ticks>TickMask) return;
        auto& command=commands[slot];
        const unsigned old=command.load(std::memory_order_relaxed);
        command.store(((old&~TickMask)+TickMask+1)|ticks,std::memory_order_release);
    }

    void stop() {
        for (unsigned slot=0;slot<Voices;++slot) play(slot,0);
    }

    void mix(Voice (&state)[Voices],int32_t* output,unsigned size,bool paused) const {
        if (!frames) return;
        for (unsigned slot=0;slot<Voices;++slot) {
            auto& voice=state[slot];
            const unsigned command=commands[slot].load(std::memory_order_acquire);
            if (voice.command!=command) voice={command,0,0};
            const unsigned ticks=command&TickMask;
            const unsigned total=ticks*SamplesPerTick;
            if (paused || voice.elapsed>=total) continue;
            mixVoice(voice,output,std::min(size,total-voice.elapsed),ticks);
        }
    }

private:
    static constexpr unsigned TickMask=4095, SamplesPerTick=735;
    static constexpr unsigned FractionBits=16;
    int16_t* pcm=nullptr;
    SceUID partitionBlock=-1;
    unsigned frames=0;
    std::atomic<unsigned> commands[Voices]{};

    void mixVoice(Voice& voice,int32_t* output,unsigned size,unsigned ticks) const {
        const unsigned period=frames<<FractionBits;
        unsigned left=size;
        while (left) {
            const unsigned tick=voice.elapsed/SamplesPerTick;
            const unsigned segment=std::min(left,(tick+1)*SamplesPerTick-voice.elapsed);
            // 22050 Hz input, 44100 Hz mixer, original pitch ramp 0.5 to 3.0.
            const unsigned rate=16384+81920*tick/ticks;
            for (unsigned i=0;i<segment;++i) {
                const unsigned sample=voice.phase>>FractionBits;
                const unsigned next=sample+1==frames?0:sample+1;
                const int fraction=(voice.phase>>8)&255;
                for (unsigned channel=0;channel<2;++channel) {
                    const int current=pcm[sample*2+channel];
                    const int delta=pcm[next*2+channel]-current;
                    *output++ += current+(delta*fraction>>8);
                }
                voice.phase+=rate;
                if (voice.phase>=period) voice.phase-=period;
            }
            voice.elapsed+=segment;
            left-=segment;
        }
    }
};
