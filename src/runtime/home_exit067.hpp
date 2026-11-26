#pragma once
#include <atomic>
#include <pspkernel.h>

// HOME belongs to firmware. Register before asset/config initialization in each
// executable: loadexec does not carry the previous executable's callbacks.
namespace HomeExit067 {
inline std::atomic<int> registration{1};
inline int callback(int,int,void*) {
    // Do not depend on the render/load loop or perform new save writes here.
    sceKernelExitGame();
    return 0;
}
inline int worker(SceSize,void*) {
    const int cb=sceKernelCreateCallback("UCG HOME exit",callback,nullptr);
    if(cb<0){registration.store(cb,std::memory_order_release);return 0;}
    const int result=sceKernelRegisterExitCallback(cb);
    if(result<0){sceKernelDeleteCallback(cb);registration.store(result,std::memory_order_release);return 0;}
    registration.store(0,std::memory_order_release);
    for(;;) sceKernelSleepThreadCB();
}
inline int install() {
    registration.store(1,std::memory_order_relaxed);
    const int thread=sceKernelCreateThread("UCG HOME callback",worker,0x11,0x1000,PSP_THREAD_ATTR_USER,nullptr);
    if(thread<0)return thread;
    const int started=sceKernelStartThread(thread,0,nullptr);
    if(started<0){sceKernelDeleteThread(thread);return started;}
    // The callback thread has higher priority than main. Yield until its checked
    // registration is published; no game assets or worker threads exist yet.
    int result=1;
    while((result=registration.load(std::memory_order_acquire))==1)
        sceKernelDelayThread(1000);
    if(result<0){sceKernelWaitThreadEnd(thread,nullptr);sceKernelDeleteThread(thread);}
    return result;
}
}
