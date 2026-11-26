#pragma once

#include "loss_probe.hpp"
#include <atomic>
#include <cstdint>
#ifdef __PSP__
#include <pspkernel.h>
#else
#include <thread>
#endif

namespace LossProbe {
// Metadata only. All supplied timestamps and I/O durations must be acquired
// outside this collector. The application's single reader owns readBegin/End;
// main and mixer may concurrently update/reset/snapshot their own metadata.
class AudioStats {
public:
    static constexpr unsigned TargetId=11;
    static constexpr unsigned InflightPresent=1,InflightMusic=2,InflightFinished=4;

    AudioStats(){ resetSnapshot(); }
    AudioStats(const AudioStats&)=delete;
    AudioStats& operator=(const AudioStats&)=delete;

    __attribute__((noinline)) unsigned begin(unsigned caughtLow) {
        Guard guard(lock_);
        if(!++generation_) ++generation_; // Zero always means inactive.
        caught_=caughtLow;
        resetSnapshot();
        carryEpoch_=0;
        if(live_) {
            data_.inflightId=liveId_;
            data_.inflightStart=liveStart_;
            data_.inflightFlags=InflightPresent|(liveMusic_?InflightMusic:0u);
            carryEpoch_=liveEpoch_;
        }
        activeEpoch_.store(generation_,std::memory_order_release);
        return generation_;
    }

    __attribute__((noinline)) AudioSnapshot end() {
        Guard guard(lock_);
        activeEpoch_.store(0,std::memory_order_release);
        return data_;
    }

    __attribute__((noinline)) AudioSnapshot snapshot() const { Guard guard(lock_);return data_; }
    unsigned token() const {return activeEpoch_.load(std::memory_order_acquire);}

    void command(unsigned epoch,unsigned now) {
        Guard guard(lock_);if(accepts(epoch)) stamp(Command,data_.command,now);
    }
    void started(unsigned epoch,unsigned now) {
        Guard guard(lock_);if(!accepts(epoch))return;
        ++data_.targetStarted;stamp(MixerStart,data_.mixerStart,now);
    }
    void prepared(unsigned epoch,unsigned pages,unsigned capacity) {
        Guard guard(lock_);if(!accepts(epoch))return;
        if(data_.startReadyPages==UINT32_MAX) {
            data_.startReadyPages=pages;data_.startCapacity=capacity;
        }
    }
    void stopped(unsigned epoch,unsigned now) {
        Guard guard(lock_);if(accepts(epoch))stamp(Stopped,data_.stopped,now);
    }
    void mixed(unsigned epoch,unsigned now) {
        Guard guard(lock_);if(accepts(epoch)) stamp(FirstMixed,data_.firstMixed,now);
    }
    void submitted(unsigned epoch,unsigned now) {
        Guard guard(lock_);
        if(accepts(epoch) && (data_.stampMask&FirstMixed)) stamp(Submitted,data_.submitted,now);
    }
    // Position reached the end of the mixed PCM; this is not a DAC timestamp.
    void completed(unsigned epoch,unsigned now) {
        Guard guard(lock_);if(accepts(epoch)) stamp(Completed,data_.completed,now);
    }
    void missing(unsigned epoch,unsigned now,unsigned frame,unsigned pages) {
        Guard guard(lock_);if(!accepts(epoch))return;
        ++data_.targetMissing;
        if(!(data_.stampMask&FirstMissing)) {
            stamp(FirstMissing,data_.firstMissing,now);
            data_.targetMissingFrame=frame;
            data_.readyPagesAtMissing=pages;
        }
    }

    unsigned readBegin(unsigned id,bool music,unsigned now) {
        Guard guard(lock_);
        liveId_=id;liveMusic_=music;liveStart_=now;live_=true;
        liveEpoch_=activeEpoch_.load(std::memory_order_relaxed);
        return liveEpoch_;
    }

    void readEnd(unsigned epoch,unsigned id,bool music,unsigned start,unsigned end,
                 unsigned ioUs,bool error) {
        Guard guard(lock_);
        const bool matches=live_ && liveEpoch_==epoch && liveId_==id && liveMusic_==music && liveStart_==start;
        if(matches) live_=false;
        const unsigned active=activeEpoch_.load(std::memory_order_relaxed);
        // A read already in flight at begin belongs to its original epoch,
        // including epoch zero. Finish its carried markers, not new counters.
        if(active && (data_.inflightFlags&InflightPresent) && !(data_.inflightFlags&InflightFinished) &&
           carryEpoch_==epoch && data_.inflightId==id && data_.inflightStart==start &&
           !!(data_.inflightFlags&InflightMusic)==music) {
            data_.inflightEnd=end;
            data_.inflightFlags|=InflightFinished;
        }
        if(!matches || !accepts(epoch)) return;
        const unsigned elapsed=static_cast<uint32_t>(end-start);
        const unsigned work=elapsed>ioUs?elapsed-ioUs:0;
        if(music) {
            maximum(data_.musicIoMax,ioUs);maximum(data_.musicWorkMax,work);
        } else {
            maximum(data_.effectIoMax,ioUs);maximum(data_.effectWorkMax,work);
        }
        if(error) ++data_.readErrors;
        if(!music && id==TargetId) {
            ++data_.targetReadCalls;
            maximum(data_.targetIoMax,ioUs);maximum(data_.targetWorkMax,work);
            if(error) ++data_.targetReadErrors;
        }
    }

    void block(unsigned epoch,unsigned gap,bool late,bool musicMissing,bool anySfxMissing) {
        Guard guard(lock_);if(!accepts(epoch))return;
        ++data_.blocks;maximum(data_.gapMax,gap);
        data_.late+=late;data_.musicMissing+=musicMissing;data_.sfxMissing+=anySfxMissing;
    }
    void outputError(unsigned epoch,int error) {
        Guard guard(lock_);
        if(accepts(epoch) && error && !data_.outputError) data_.outputError=error;
    }

private:
    struct Guard {
        std::atomic_flag& flag;
        explicit Guard(std::atomic_flag& value):flag(value) {
            while(flag.test_and_set(std::memory_order_acquire)) {
#ifdef __PSP__
                sceKernelDelayThread(1);
#else
                std::this_thread::yield();
#endif
            }
        }
        ~Guard(){flag.clear(std::memory_order_release);}
    };
    bool accepts(unsigned epoch) const {return epoch && activeEpoch_.load(std::memory_order_relaxed)==epoch;}
    void resetSnapshot() {
        data_={};data_.targetId=TargetId;
        data_.targetMissingFrame=data_.readyPagesAtMissing=data_.inflightId=UINT32_MAX;
        data_.startReadyPages=data_.startCapacity=UINT32_MAX;
    }
    void stamp(unsigned mask,unsigned& field,unsigned now) {
        if(!(data_.stampMask&mask)){field=now;data_.stampMask|=mask;}
    }
    static void maximum(unsigned& field,unsigned value){if(value>field)field=value;}

    AudioSnapshot data_{};
    std::atomic<unsigned> activeEpoch_{0};
    unsigned generation_=0,caught_=0,liveId_=UINT32_MAX,liveStart_=0,liveEpoch_=0,carryEpoch_=0;
    bool live_=false,liveMusic_=false;
    mutable std::atomic_flag lock_=ATOMIC_FLAG_INIT;
};
static_assert(sizeof(AudioStats)<=192,"Loss audio metadata budget");
}
