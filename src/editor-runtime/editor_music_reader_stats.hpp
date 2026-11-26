#pragma once
#include <atomic>
#include <climits>

// Reader-boundary observations, not event/request counts. UI reads atomic
// snapshots; fields may come from adjacent boundaries. No PCM/queue ownership.
struct EditorMusicReaderStats {
    struct Snapshot {
        unsigned musicSlices,maxMusicSliceUsec,serviceCalls,maxServiceUsec;
        unsigned pendingAdmissions,pendingObservations,lowReserveObservations;
        unsigned primingObservations,startedObservations,minQueuedPending,maxQueuedPending;
    };
    std::atomic<unsigned> musicSlices{0},maxMusicSliceUsec{0},serviceCalls{0},maxServiceUsec{0};
    std::atomic<unsigned> pendingAdmissions{0},pendingObservations{0},lowReserveObservations{0};
    std::atomic<unsigned> primingObservations{0},startedObservations{0},minQueuedPending{UINT_MAX},maxQueuedPending{0};
    void reset() {
        musicSlices=0;maxMusicSliceUsec=0;serviceCalls=0;maxServiceUsec=0;
        pendingAdmissions=0;pendingObservations=0;lowReserveObservations=0;
        primingObservations=0;startedObservations=0;minQueuedPending=UINT_MAX;maxQueuedPending=0;
    }
    // Each field has one writer (reader thread), so a relaxed load/store is
    // sufficient for extrema. They are instrumentation, never synchronization.
    static void maximum(std::atomic<unsigned>& field,unsigned value) {
        if(value>field.load(std::memory_order_relaxed))field.store(value,std::memory_order_relaxed);
    }
    void observe(unsigned queued,unsigned lowWater,bool started) {
        pendingObservations.fetch_add(1,std::memory_order_relaxed);
        if(queued<lowWater)lowReserveObservations.fetch_add(1,std::memory_order_relaxed);
        (started?startedObservations:primingObservations).fetch_add(1,std::memory_order_relaxed);
        if(queued<minQueuedPending.load(std::memory_order_relaxed))minQueuedPending.store(queued,std::memory_order_relaxed);
        maximum(maxQueuedPending,queued);
    }
    void musicRead(unsigned usec) {
        musicSlices.fetch_add(1,std::memory_order_relaxed);maximum(maxMusicSliceUsec,usec);
    }
    void service(unsigned usec,bool pending,bool worked) {
        if(!worked)return;
        serviceCalls.fetch_add(1,std::memory_order_relaxed);maximum(maxServiceUsec,usec);
        if(pending)pendingAdmissions.fetch_add(1,std::memory_order_relaxed);
    }
    Snapshot snapshot()const {
        return {musicSlices.load(std::memory_order_relaxed),maxMusicSliceUsec.load(std::memory_order_relaxed),
            serviceCalls.load(std::memory_order_relaxed),maxServiceUsec.load(std::memory_order_relaxed),
            pendingAdmissions.load(std::memory_order_relaxed),pendingObservations.load(std::memory_order_relaxed),
            lowReserveObservations.load(std::memory_order_relaxed),primingObservations.load(std::memory_order_relaxed),
            startedObservations.load(std::memory_order_relaxed),minQueuedPending.load(std::memory_order_relaxed),
            maxQueuedPending.load(std::memory_order_relaxed)};
    }
};
