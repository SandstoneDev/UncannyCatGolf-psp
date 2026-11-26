#pragma once

#include <atomic>
#include <pspkernel.h>

class MenuStreaming {
public:
    using PrepareArt=bool (*)(bool);
    enum class ArtState : unsigned { Idle, Pending, Ready, Failed };

    bool start(PrepareArt prepare) {
        stopping.store(false,std::memory_order_relaxed);
        prepareArt=prepare;
        thread=sceKernelCreateThread("ucg-menu-reader",run,0x34,0x4000,0,nullptr);
        if (thread<0) return false;
        MenuStreaming* self=this;
        if (sceKernelStartThread(thread,sizeof self,&self)>=0) return true;
        sceKernelDeleteThread(thread);
        thread=-1;
        return false;
    }

    void stop() {
        if (thread<0) return;
        stopping.store(true,std::memory_order_release);
        sceKernelWaitThreadEnd(thread,nullptr);
        sceKernelDeleteThread(thread);
        thread=-1;
    }

    bool available() const { return thread>=0; }

    void requestArt(bool russian) {
        artLocale=russian;
        artState.store(ArtState::Pending,std::memory_order_release);
    }

    ArtState artStatus() const { return artState.load(std::memory_order_acquire); }
    unsigned artTime() const { return artElapsed; }
    void clearArtResult() { artState.store(ArtState::Idle,std::memory_order_relaxed); }

private:
    static int run(SceSize,void* args) { return (*static_cast<MenuStreaming**>(args))->work(); }

    int work() {
        while (!stopping.load(std::memory_order_acquire)) {
            if (artStatus()==ArtState::Pending) {
                const auto now=sceKernelGetSystemTimeLow();
                const bool ready=prepareArt(artLocale);
                artElapsed=sceKernelGetSystemTimeLow()-now;
                artState.store(ready?ArtState::Ready:ArtState::Failed,std::memory_order_release);
            }
            sceKernelDelayThread(4000);
        }
        return 0;
    }

    PrepareArt prepareArt=nullptr;
    int thread=-1;
    bool artLocale=false;
    unsigned artElapsed=0;
    std::atomic<ArtState> artState{ArtState::Idle};
    std::atomic<bool> stopping{false};
};
