#pragma once

#include <cstdint>

// Main-thread-owned RAM collector. Audio workers publish their own snapshot;
// the caller passes a consistent copy here. No clock, allocation, I/O or audio
// commands are performed by this helper. All timestamps are low32 microseconds.
namespace LossProbe {
enum Stamp : unsigned {
    Command=1u<<0, MixerStart=1u<<1, FirstMixed=1u<<2,
    Submitted=1u<<3, Completed=1u<<4, FirstMissing=1u<<5, Stopped=1u<<6
};

struct AudioSnapshot {
    unsigned command,mixerStart,firstMixed,submitted,completed,firstMissing,stampMask;
    unsigned targetId,targetStarted,targetMissing,targetMissingFrame,readyPagesAtMissing;
    unsigned targetReadCalls,targetIoMax,targetWorkMax,targetReadErrors;
    unsigned musicMissing,sfxMissing,readErrors;
    int outputError;
    unsigned blocks,gapMax,late,musicIoMax,musicWorkMax,effectIoMax,effectWorkMax;
    unsigned inflightId,inflightStart,inflightEnd,inflightFlags;
    unsigned startReadyPages,startCapacity,stopped;
};
static_assert(sizeof(AudioSnapshot)==136,"AudioSnapshot must remain a flat 32-bit POD");

enum class EndReason : unsigned { None,RetrySettled,NextCaught,Menu,Exit,Manual };
enum RecordFlags : unsigned {
    HasRetryBegin=1u<<0,HasRetryEnd=1u<<1,HasFirstGameplayFrame=1u<<2,
    HasRetryPersistence=1u<<3,HasRetryReset=1u<<4,HasRetryJournal=1u<<5,RetryWrote=1u<<6
};

struct Record {
    uint32_t caught,retryBegin,retryEnd,firstGameplayFrame,ended,durationUs;
    unsigned flags,frames,renderMaxUs,sequence;
    EndReason reason;
    unsigned preRetryBlocks,preRetryTargetMissing,preRetryMusicMissing,preRetrySfxMissing;
    uint32_t retryPersistBegin,retryPersistEnd,retryResetEnd,retryJournalEnd;
    AudioSnapshot audio;
};
static_assert(sizeof(Record)==212,"Measurement record layout");

class Recorder {
public:
    static constexpr unsigned Capacity=8;
    static constexpr uint32_t RetrySettleUs=1000000;

    __attribute__((noinline)) void caught(uint32_t now,const AudioSnapshot& audio) {
        if(active_) finish(now,EndReason::NextCaught);
        current_={};
        current_.caught=now;
        current_.sequence=++sequence_;
        current_.audio=audio;
        active_=true;
    }

    __attribute__((noinline)) void retryBegin(uint32_t now,const AudioSnapshot& audio) {
        if(!active_ || (current_.flags&HasRetryBegin)) return;
        current_.retryBegin=now;
        current_.flags|=HasRetryBegin;
        current_.preRetryBlocks=audio.blocks;
        current_.preRetryTargetMissing=audio.targetMissing;
        current_.preRetryMusicMissing=audio.musicMissing;
        current_.preRetrySfxMissing=audio.sfxMissing;
        current_.audio=audio;
    }

    __attribute__((noinline)) void retryEnd(uint32_t now) {
        if(!active_ || !(current_.flags&HasRetryBegin) || (current_.flags&HasRetryEnd)) return;
        current_.retryEnd=now;
        current_.flags|=HasRetryEnd;
    }

    void retryPersistence(uint32_t started,uint32_t ended,bool wrote) {
        if(!active_ || !(current_.flags&HasRetryBegin) || (current_.flags&HasRetryPersistence)) return;
        current_.retryPersistBegin=started;current_.retryPersistEnd=ended;
        current_.flags|=HasRetryPersistence|(wrote?RetryWrote:0u);
    }
    void retryResetDone(uint32_t now) {
        if(!active_ || !(current_.flags&HasRetryPersistence) || (current_.flags&HasRetryReset)) return;
        current_.retryResetEnd=now;current_.flags|=HasRetryReset;
    }
    void retryJournalDone(uint32_t now) {
        if(!active_ || !(current_.flags&HasRetryReset) || (current_.flags&HasRetryJournal)) return;
        current_.retryJournalEnd=now;current_.flags|=HasRetryJournal;
    }

    void gameplayFrame(uint32_t now) {
        if(!active_ || !(current_.flags&HasRetryEnd) || (current_.flags&HasFirstGameplayFrame)) return;
        current_.firstGameplayFrame=now;
        current_.flags|=HasFirstGameplayFrame;
    }

    void render(unsigned durationUs) {
        if(!active_) return;
        ++current_.frames;
        if(durationUs>current_.renderMaxUs) current_.renderMaxUs=durationUs;
    }

    __attribute__((noinline)) void sample(const AudioSnapshot& audio) {
        if(active_) current_.audio=audio;
    }

    bool retrySettled(uint32_t now) const {
        return active_ && (current_.flags&HasFirstGameplayFrame) &&
            static_cast<uint32_t>(now-current_.firstGameplayFrame)>=RetrySettleUs;
    }

    void tick(uint32_t now) {
        if(retrySettled(now)) finish(now,EndReason::RetrySettled);
    }

    __attribute__((noinline)) void finish(uint32_t now,EndReason reason) {
        if(!active_) return;
        current_.ended=now;
        current_.durationUs=static_cast<uint32_t>(now-current_.caught);
        current_.reason=reason;
        if(count_==Capacity) {
            first_=(first_+1)%Capacity;
            --count_;
            ++dropped_;
        }
        queue_[(first_+count_)%Capacity]=current_;
        ++count_;
        active_=false;
    }

    bool pop(Record& output) {
        if(!count_) return false;
        output=queue_[first_];
        first_=(first_+1)%Capacity;
        --count_;
        return true;
    }

    __attribute__((noinline)) bool peek(unsigned oldestIndex,Record& output) const {
        if(oldestIndex>=count_) return false;
        output=queue_[(first_+oldestIndex)%Capacity];
        return true;
    }

    __attribute__((noinline)) void discard(unsigned count) {
        const unsigned available=count<count_?count:count_;
        first_=(first_+available)%Capacity;
        count_-=available;
    }

    unsigned pending() const { return count_; }
    unsigned dropped() const { return dropped_; }
    bool active() const { return active_; }

private:
    Record queue_[Capacity]{},current_{};
    unsigned first_=0,count_=0,dropped_=0,sequence_=0;
    bool active_=false;
};
static_assert(sizeof(Recorder)<=2048,"Loss measurements must fit the RAM budget");
}
