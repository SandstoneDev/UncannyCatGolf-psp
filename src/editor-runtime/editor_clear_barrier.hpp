#pragma once
#include <atomic>
#include <cstdint>
// One main-thread producer, one mixer consumer. Queue clearing and stop writes
// happen before publish(); the mixer captures before its stop/command snapshot
// and acknowledges that captured serial after applying those commands.
class EditorAudioClearBarrier {
public:
    void reset() { request.store(0,std::memory_order_relaxed);ack.store(0,std::memory_order_relaxed); }
    uint32_t publish() { return request.fetch_add(1,std::memory_order_release)+1; }
    uint32_t capture() const { return request.load(std::memory_order_acquire); }
    void acknowledge(uint32_t serial) { ack.store(serial,std::memory_order_release); }
    bool complete(uint32_t serial) const { return ack.load(std::memory_order_acquire)==serial; }
    template<class Running,class Clock,class Yield>
    bool wait(uint32_t serial,const Running&running,const Clock&clock,const Yield&yield,uint64_t timeout)const {
        const uint64_t started=clock();
        while(running()) {
            if(complete(serial))return true;
            if(uint64_t(clock()-started)>=timeout)return false;
            yield();
        }
        return true; // No live consumer; a later open resets all queued state.
    }
private:
    std::atomic<uint32_t> request{0},ack{0};
};
