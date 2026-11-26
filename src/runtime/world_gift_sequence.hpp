#pragma once
#include <algorithm>
#include <cstdint>
#include "world_awards.hpp"

namespace WorldGift {
using Read = const WorldState::Data& (*)();
using Open = bool (*)(unsigned);
enum class Phase { Idle, Flying, NeedsRetry, Confirm };
struct Sequence {
    int id=-1;
    unsigned elapsed=0,slot=0;
    uint64_t start=0,fadeStart=0;
    Phase phase=Phase::Idle;
    bool neutral=false,fullFrame=false,queued=false,displayed=false;
    bool flew=false,blast=false,drainRelease=false,darkAlready=false;

    bool active() const {return phase!=Phase::Idle;}
    void drawn(bool complete) {fullFrame=active() && complete && elapsed>=2000;}
    void submitted(bool success) {queued=fullFrame && success;fullFrame=false;}
    void presented() {displayed|=queued;queued=false;}
    template<class Cue>
    bool update(const WorldState::Data& data,Open open,uint64_t now,bool pressed,bool held,Cue cue) {
        const bool consumeRelease=drainRelease;
        if(drainRelease){pressed=false;drainRelease=held;}
        const int next=WorldState::nextPresent(data);
        if(active() && phase!=Phase::Confirm && id!=next) *this={};
        if(!active()) {
            if(next<0){
                if(fadeStart) {
                    elapsed=static_cast<unsigned>(std::min<uint64_t>((now-fadeStart)/1000,500));
                    if(elapsed>=500)fadeStart=0;
                }
                return consumeRelease;
            }
            id=next;slot=data.pendingCount-1;start=now;phase=Phase::Flying;
        }
        elapsed=static_cast<unsigned>(std::min<uint64_t>((now-start)/1000,UINT32_MAX));
        if(!flew) {cue(false);flew=true;}
        if(elapsed>=1500 && !blast) {cue(true);blast=true;}
        if(phase==Phase::Confirm) {
            if(neutral && pressed){
                *this={};drainRelease=true;darkAlready=next>=0;
                if(next<0)fadeStart=now;
                return true;
            }
            if(!held)neutral=true;
            return true;
        }
        if(!displayed)return true;
        if(phase==Phase::NeedsRetry) {
            if(!held)neutral=true;
            if(!neutral || !pressed)return true;
        }
        // A button used to retry the save cannot acknowledge this reward.
        phase=open && open(static_cast<unsigned>(id))?Phase::Confirm:Phase::NeedsRetry;
        neutral=!held;
        return true;
    }
};
}

