#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#ifdef __PSP__
#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <malloc.h>
#endif

// Transition milestones only. Failed writes stay in a bounded RAM queue and
// are retried after audio shutdown releases its file descriptors.
class ParkTransitionTrace {
public:
    void setPath(const char* value) {
        if (std::strlen(value)>=sizeof path) return;
        std::strcpy(path,value);
    }
    __attribute__((noinline)) void record(const char* phase,const char* from,const char* to,unsigned a=0,unsigned b=0,unsigned c=0) {
        if (count<Capacity) {
            unsigned heap=0,kernel=0,largest=0;
#ifdef __PSP__
            heap=static_cast<unsigned>(mallinfo().fordblks);
            kernel=sceKernelTotalFreeMemSize();
            largest=sceKernelMaxFreeMemSize();
#endif
            entries[count++]={phase,from,to,a,b,c,++sequence,heap,kernel,largest};
        }
        else ++dropped;
        flush();
    }
    __attribute__((noinline)) void flush() {
        if (!path[0] || !count) return;
#ifdef __PSP__
        const int file=sceIoOpen(path,PSP_O_WRONLY|PSP_O_APPEND|PSP_O_CREAT,0666);
        if (file<0) { lastError=file; return; }
#else
        auto* file=std::fopen(path,"a");
        if (!file) { lastError=-1; return; }
#endif
        if (interruptedLine) {
#ifdef __PSP__
            const int result=sceIoWrite(file,"\n",1);
#else
            const int result=static_cast<int>(std::fwrite("\n",1,1,file));
#endif
            if (result!=1) {
                lastError=result;
#ifdef __PSP__
                sceIoClose(file);
#else
                std::fclose(file);
#endif
                return;
            }
            interruptedLine=false;
        }
        unsigned written=0;
        for (;written<count;++written) {
            const auto& entry=entries[written];
            char text[320];
            const int bytes=std::snprintf(text,sizeof text,
                "build=053 event=park_transition phase=%s from=%s to=%s a=%u b=%u c=%u seq=%u dropped=%u heap_free=%u kernel_free=%u largest=%u\n",
                entry.phase,entry.from,entry.to,entry.a,entry.b,entry.c,entry.sequence,dropped,
                entry.heap,entry.kernel,entry.largest);
            if (bytes<0 || static_cast<unsigned>(bytes)>=sizeof text) break;
            unsigned done=0;
            while (done<static_cast<unsigned>(bytes)) {
#ifdef __PSP__
                const int result=sceIoWrite(file,text+done,bytes-done);
#else
                const int result=static_cast<int>(std::fwrite(text+done,1,bytes-done,file));
#endif
                if (result<=0) { lastError=result; break; }
                done+=static_cast<unsigned>(result);
            }
            if (done!=static_cast<unsigned>(bytes)) { interruptedLine=done>0; break; }
        }
#ifdef __PSP__
        const int closed=sceIoClose(file);
#else
        const int closed=std::fclose(file);
#endif
        if (closed<0) { lastError=closed; return; }
        if (written) {
            for (unsigned i=written;i<count;++i) entries[i-written]=entries[i];
            count-=written;
        }
    }
    unsigned pending() const { return count; }
    int error() const { return lastError; }
private:
    struct Entry { const char* phase; const char* from; const char* to; unsigned a,b,c,sequence,heap,kernel,largest; };
    static constexpr unsigned Capacity=32;
    Entry entries[Capacity]{};
    unsigned count=0,sequence=0,dropped=0;
    int lastError=0;
    bool interruptedLine=false;
    char path[192]{};
};
