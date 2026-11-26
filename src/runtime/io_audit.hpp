#pragma once

// Include in one translation unit. Link with --wrap for the exported wrappers.
// No allocation, filesystem output, or calls to wrapped I/O while locked.
#include <atomic>
#include <cstdint>
#ifdef __PSP__
#include <pspkernel.h>
#include <pspiofilemgr.h>
#endif

namespace LoadIoAudit {
struct Error {
    bool valid=false;
    bool observation=false;
    const char* op=nullptr;
    const char* phase=nullptr;
    int uid=-1;
    int64_t raw=0,offset=-1;
    unsigned wanted=0,got=0,pathHash=0;
    char path[24]{};
};
struct Handle {
    int uid=-1;
    int64_t offset=-1;
    unsigned pathHash=0;
    char path[24]{};
    unsigned flags=0;
};
struct Snapshot {
    unsigned openCount=0,highWater=0,opens=0,closes=0,errors=0,dropped=0,untracked=0,shortReads=0;
    int64_t lastRawForPhase=0;
    bool hasRawForPhase=false;
};
namespace Detail {
struct Slot { int64_t offset=0; int uid=-1; unsigned hash=0; char path[24]{}; };
struct StoredError {
    int64_t raw=0,offset=-1;
    const char* phase=nullptr;
    const char* op=nullptr;
    int uid=-1;
    unsigned wanted=0,got=0,hash=0;
    char path[24]{};
};
enum Pending : unsigned char { None,Opening,Closing,Reading,Seeking };
struct State {
    Slot slots[64]{};
    StoredError ring[16]{},first{};
    unsigned char generations[64]{};
    unsigned arguments[64]{};
    unsigned char pending[64]{};
    Snapshot stats{};
    const char* phase="boot";
    unsigned head=0,count=0;
    std::atomic_flag lock=ATOMIC_FLAG_INIT;
};
inline State state;
#if defined(UCG_FILE_PRESSURE_TEST) || defined(UCG_TILE_OPEN_FAULT_TEST)
// Fault injection belongs to test builds only; no state in an ordinary build.
struct OpenFaults {
#ifdef UCG_FILE_PRESSURE_TEST
    unsigned limit=0;
    bool limited=false;
#endif
#ifdef UCG_TILE_OPEN_FAULT_TEST
    const char* basename=nullptr;
    int raw=0;
    bool once=false;
#endif
};
inline OpenFaults openFaults;
#endif
#ifdef __PSP__
static_assert(sizeof(State)<=4096,"PSP audit storage budget");
#endif
struct Guard {
    Guard(){while(state.lock.test_and_set(std::memory_order_acquire)) {
#ifdef __PSP__
        sceKernelDelayThread(1);
#endif
    }}
    ~Guard(){state.lock.clear(std::memory_order_release);}
};
#if defined(UCG_FILE_PRESSURE_TEST) || defined(UCG_TILE_OPEN_FAULT_TEST)
inline bool injectedOpen(const char* path,int& rawResult){
    Guard g;
#ifdef UCG_TILE_OPEN_FAULT_TEST
    if(openFaults.once && path){
        const char* basename=path;
        for(const char* p=path;*p;++p)if(*p=='/' || *p=='\\')basename=p+1;
        const char* a=basename;const char* b=openFaults.basename;
        while(b && *a && *a==*b){++a;++b;}
        if(b && !*a && !*b){openFaults.once=false;rawResult=openFaults.raw;return true;}
    }
#else
    (void)path;
#endif
#ifdef UCG_FILE_PRESSURE_TEST
    if(openFaults.limited && state.stats.openCount>=openFaults.limit){rawResult=static_cast<int>(0x80020320u);return true;}
#endif
    return false;
}
#endif
inline void copy(char (&out)[24],const char* path){
    unsigned n=0; while(path && path[n])++n;
    unsigned start=0; for(unsigned i=0;i<n;++i)if(path[i]=='/' || path[i]=='\\')start=i+1;
    if(n-start>23)start=n-23;
    unsigned i=0;for(;i<n-start;++i)out[i]=path[start+i];
    for(;i<24;++i)out[i]=0;
}
inline unsigned hash(const char* path){unsigned h=2166136261u;while(path && *path){h^=static_cast<unsigned char>(*path++);h*=16777619u;}return h;}
inline int find(int uid){if(uid<0)return -1;for(unsigned i=0;i<64;++i)if(state.slots[i].uid==uid)return static_cast<int>(i);return -1;}
inline Error expand(const StoredError& e,bool observation=false){Error out;out.valid=e.op!=nullptr;out.observation=observation;out.op=e.op;out.phase=e.phase;out.uid=e.uid;out.raw=e.raw;out.offset=e.offset;out.wanted=e.wanted;out.got=e.got;out.pathHash=e.hash;copy(out.path,e.path);return out;}
inline void add(const StoredError& e,bool latch=true){
    if(latch){++state.stats.errors;if(!state.first.op)state.first=e;}
    if(state.count<16)++state.count;else ++state.stats.dropped;
    const unsigned index=state.head&15u,bit=1u<<(index+16);
    state.ring[index]=e;state.head=latch?(state.head&~bit):(state.head|bit);
    state.head=(state.head&~15u)|((index+1)%16);
}
struct Call {
    const char* phase;
    int uid,slot;
    unsigned char generation;
    int64_t offset;
    unsigned hash;
    char path[24];
};
inline Call begin(int uid){Guard g;const int i=find(uid);Call c{state.phase,uid,i,0,-1,0,{}};if(i>=0){const auto& s=state.slots[i];c.generation=state.generations[i];c.offset=s.offset;c.hash=s.hash;copy(c.path,s.path);}return c;}
inline bool same(const Call& c){return c.slot>=0 && state.slots[c.slot].uid==c.uid && state.generations[c.slot]==c.generation;}
inline void raw(const Call& c,int64_t value){if(c.phase==state.phase){state.stats.lastRawForPhase=value;state.stats.hasRawForPhase=true;}}
inline void error(const Call& c,const char* op,int64_t value,unsigned wanted=0,unsigned got=0,bool latch=true){StoredError e;e.raw=value;e.offset=c.offset;e.phase=c.phase;e.op=op;e.uid=c.uid;e.wanted=wanted;e.got=got;e.hash=c.hash;copy(e.path,c.path);add(e,latch);}
inline void remove(int i){state.slots[i].uid=-1;state.pending[i]=None;++state.generations[i];}
inline int opened(const Call& c,const char* path,int uid,bool async,int flags){
    Guard g;raw(c,uid);
    if(uid<0){Call failure=c;failure.hash=hash(path);copy(failure.path,path);error(failure,async?"open_async":"open",uid);return uid;}
    ++state.stats.opens;++state.stats.openCount;
    if(state.stats.openCount>state.stats.highWater)state.stats.highWater=state.stats.openCount;
    int i=find(uid);
    if(i<0)for(unsigned n=0;n<64;++n)if(state.slots[n].uid<0){i=static_cast<int>(n);break;}
    if(i<0){++state.stats.untracked;return uid;}
    // APPEND writes use the kernel's end position, unknown without extra I/O.
    auto& s=state.slots[i];s.uid=uid;s.offset=(flags&0x100)?-1:0;s.hash=hash(path);copy(s.path,path);++state.generations[i];
    state.pending[i]=async?Opening:None;return uid;
}
inline int closed(const Call& c,int value,bool async){Guard g;raw(c,value);if(value<0)error(c,async?"close_async":"close",value);else if(async){if(same(c))state.pending[c.slot]=Closing;}else{++state.stats.closes;if(state.stats.openCount)--state.stats.openCount;if(same(c))remove(c.slot);}return value;}
inline int transferred(const Call& c,int value,unsigned wanted,bool write){Guard g;raw(c,value);if(value<0 || static_cast<unsigned>(value)!=wanted){error(c,write?"write":"read",value,wanted,value>0?static_cast<unsigned>(value):0,value<0);if(value>=0 && !write)++state.stats.shortReads;}if(value>0 && same(c) && c.offset>=0)state.slots[c.slot].offset=c.offset+value;return value;}
inline int64_t sought(const Call& c,int64_t value){Guard g;raw(c,value);if(value<0)error(c,"seek",value);else if(same(c))state.slots[c.slot].offset=value;return value;}
inline int queued(const Call& c,int result,Pending op,unsigned arg){Guard g;raw(c,result);if(result<0)error(c,op==Reading?"read_async_start":"seek_async_start",result,arg,0);else if(same(c)){state.pending[c.slot]=op;state.arguments[c.slot]=arg;}return result;}
inline int completed(const Call& c,int result,const SceInt64* value){
    Guard g;raw(c,result);
    if(result<0){error(c,"async_wait",result);return result;}
    if(result>0 || !value || !same(c))return result;
    raw(c,*value);const auto op=state.pending[c.slot];const unsigned wanted=state.arguments[c.slot];state.pending[c.slot]=None;
    if(*value<0 || (op==Reading && static_cast<uint64_t>(*value)!=wanted)){error(c,op==Opening?"open_async_result":op==Closing?"close_async_result":op==Reading?"read_async":"seek_async_result",*value,op==Reading?wanted:0,*value>0 && op==Reading?static_cast<unsigned>(*value):0,*value<0);if(*value>=0 && op==Reading)++state.stats.shortReads;}
    if((op==Opening && *value<0) || (op==Closing && *value>=0)){if(state.stats.openCount)--state.stats.openCount;++state.stats.closes;remove(c.slot);}
    else if(op==Seeking && *value>=0)state.slots[c.slot].offset=*value;
    else if(op==Reading && *value>0 && c.offset>=0)state.slots[c.slot].offset=c.offset+*value;
    return result;
}
}
inline void setPhase(const char* value){Detail::Guard g;Detail::state.phase=value;Detail::state.stats.hasRawForPhase=false;Detail::state.stats.lastRawForPhase=0;}
inline const char* phase(){Detail::Guard g;return Detail::state.phase;}
inline Snapshot snapshot(){Detail::Guard g;return Detail::state.stats;}
inline Error first(){Detail::Guard g;return Detail::expand(Detail::state.first);}
inline Error last(){Detail::Guard g;const auto& s=Detail::state;const unsigned index=((s.head&15u)+15)%16;return s.count?Detail::expand(s.ring[index],(s.head&(1u<<(index+16)))!=0):Error{};}
inline bool errorAt(unsigned newest,Error& out){Detail::Guard g;const auto& s=Detail::state;if(newest>=s.count){out={};return false;}const unsigned index=((s.head&15u)+15-newest)%16;out=Detail::expand(s.ring[index],(s.head&(1u<<(index+16)))!=0);return true;}
inline bool handleAt(unsigned slot,Handle& out){Detail::Guard g;const auto& s=Detail::state;if(slot>=64 || s.slots[slot].uid<0){out={};return false;}const auto& h=s.slots[slot];out.uid=h.uid;out.offset=h.offset;out.pathHash=h.hash;Detail::copy(out.path,h.path);out.flags=s.pending[slot];return true;}
inline void resetErrors(){Detail::Guard g;auto& s=Detail::state;s.first={};s.count=s.head=0;s.stats.lastRawForPhase=0;s.stats.hasRawForPhase=false;}
inline void failure(const char* stage,int err,unsigned wanted=0,unsigned got=0){auto c=Detail::begin(-1);Detail::Guard g;Detail::error(c,stage,err,wanted,got);}
#ifdef UCG_FILE_PRESSURE_TEST
inline void testOpenLimit(unsigned count){Detail::Guard g;Detail::openFaults.limit=count;Detail::openFaults.limited=true;}
inline void testDisableOpenLimit(){Detail::Guard g;Detail::openFaults.limited=false;}
#endif
#ifdef UCG_TILE_OPEN_FAULT_TEST
inline void testFailOpenOnce(const char* basename,int raw){Detail::Guard g;Detail::openFaults.basename=basename;Detail::openFaults.raw=raw;Detail::openFaults.once=true;}
#endif
}

extern "C" {
int __real_sceIoOpen(const char*,int,int);
int __real_sceIoClose(SceUID);
int __real_sceIoRead(SceUID,void*,SceSize);
int __real_sceIoWrite(SceUID,const void*,SceSize);
SceOff __real_sceIoLseek(SceUID,SceOff,int);
int __real_sceIoLseek32(SceUID,int,int);
int __real_sceIoOpenAsync(const char*,int,int);
int __real_sceIoCloseAsync(SceUID);
int __real_sceIoReadAsync(SceUID,void*,SceSize);
int __real_sceIoLseek32Async(SceUID,int,int);
int __real_sceIoPollAsync(SceUID,SceInt64*);
int __real_sceIoWaitAsync(SceUID,SceInt64*);
int __real_sceIoWaitAsyncCB(SceUID,SceInt64*);
int __wrap_sceIoOpen(const char* p,int f,int m){
    const auto c=LoadIoAudit::Detail::begin(-1);
#if defined(UCG_FILE_PRESSURE_TEST) || defined(UCG_TILE_OPEN_FAULT_TEST)
    int injected;
    if(LoadIoAudit::Detail::injectedOpen(p,injected))return LoadIoAudit::Detail::opened(c,p,injected,false,f);
#endif
    const int r=__real_sceIoOpen(p,f,m);return LoadIoAudit::Detail::opened(c,p,r,false,f);
}
int __wrap_sceIoClose(SceUID d){const auto c=LoadIoAudit::Detail::begin(d);return LoadIoAudit::Detail::closed(c,__real_sceIoClose(d),false);}
int __wrap_sceIoRead(SceUID d,void* p,SceSize n){const auto c=LoadIoAudit::Detail::begin(d);return LoadIoAudit::Detail::transferred(c,__real_sceIoRead(d,p,n),n,false);}
int __wrap_sceIoWrite(SceUID d,const void* p,SceSize n){const auto c=LoadIoAudit::Detail::begin(d);return LoadIoAudit::Detail::transferred(c,__real_sceIoWrite(d,p,n),n,true);}
SceOff __wrap_sceIoLseek(SceUID d,SceOff p,int w){const auto c=LoadIoAudit::Detail::begin(d);return LoadIoAudit::Detail::sought(c,__real_sceIoLseek(d,p,w));}
int __wrap_sceIoLseek32(SceUID d,int p,int w){const auto c=LoadIoAudit::Detail::begin(d);return static_cast<int>(LoadIoAudit::Detail::sought(c,__real_sceIoLseek32(d,p,w)));}
int __wrap_sceIoOpenAsync(const char* p,int f,int m){const auto c=LoadIoAudit::Detail::begin(-1);const int r=__real_sceIoOpenAsync(p,f,m);return LoadIoAudit::Detail::opened(c,p,r,true,f);}
int __wrap_sceIoCloseAsync(SceUID d){const auto c=LoadIoAudit::Detail::begin(d);return LoadIoAudit::Detail::closed(c,__real_sceIoCloseAsync(d),true);}
int __wrap_sceIoReadAsync(SceUID d,void* p,SceSize n){const auto c=LoadIoAudit::Detail::begin(d);return LoadIoAudit::Detail::queued(c,__real_sceIoReadAsync(d,p,n),LoadIoAudit::Detail::Reading,n);}
int __wrap_sceIoLseek32Async(SceUID d,int p,int w){const auto c=LoadIoAudit::Detail::begin(d);return LoadIoAudit::Detail::queued(c,__real_sceIoLseek32Async(d,p,w),LoadIoAudit::Detail::Seeking,static_cast<unsigned>(p));}
int __wrap_sceIoPollAsync(SceUID d,SceInt64* p){const auto c=LoadIoAudit::Detail::begin(d);return LoadIoAudit::Detail::completed(c,__real_sceIoPollAsync(d,p),p);}
int __wrap_sceIoWaitAsync(SceUID d,SceInt64* p){const auto c=LoadIoAudit::Detail::begin(d);return LoadIoAudit::Detail::completed(c,__real_sceIoWaitAsync(d,p),p);}
int __wrap_sceIoWaitAsyncCB(SceUID d,SceInt64* p){const auto c=LoadIoAudit::Detail::begin(d);return LoadIoAudit::Detail::completed(c,__real_sceIoWaitAsyncCB(d,p),p);}
}
