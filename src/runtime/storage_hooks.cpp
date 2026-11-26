#include "storage_hooks.hpp"
#include "storage_stdio.hpp"
#include "storage_pool.hpp"
#include "storage_file_pool.hpp"
#include "release_paths.hpp"
#include "release_iso_map.hpp"
#ifdef __PSP__
#include <pspkernel.h>
#else
#include <mutex>
#include <condition_variable>
#endif
#include <cerrno>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <type_traits>

extern "C" std::FILE* __real_fopen(const char*,const char*);
namespace {
std::FILE* physicalOpen(const char* path,const char* mode){
    const auto physical=ReleaseIsoMap::physical(path);
    return __real_fopen(physical.c_str(),mode);
}
}
namespace ucg_storage {
namespace {
struct Slot { std::FILE* file=nullptr;Reader* reader=nullptr;bool reserved=false; };
Slot slots[MaxFiles];
#ifdef __PSP__
SceUID registryMutex=-1;
struct Guard { Guard() { if(registryMutex>=0) sceKernelWaitSema(registryMutex,1,nullptr); }
    ~Guard() { if(registryMutex>=0) sceKernelSignalSema(registryMutex,1); } };
#else
// Placement storage intentionally has no registered destructor. A FILE owned
// by another TU may close after this TU's ordinary globals are destroyed.
std::mutex& registryMutex(){alignas(std::mutex) static unsigned char memory[sizeof(std::mutex)];static auto* value=new(memory)std::mutex;return *value;}
struct Guard { Guard() { registryMutex().lock(); } ~Guard() { registryMutex().unlock(); } };
#endif
static_assert(std::is_trivially_destructible<WindowPool<1>>::value,"pool survives global FILE destruction");
WindowPool<1> decodedPool;
bool poolReady=false;
rawfile::Pool<4> rawFiles;
static_assert(std::is_trivially_destructible<rawfile::Pool<4>>::value,"raw pool survives global FILE destruction");
#ifdef __PSP__
SceUID windowMutex=-1,windowAvailable=-1;
SceUID rawFileMutex=-1;
bool rawFileLock(void*){return rawFileMutex>=0&&sceKernelWaitSema(rawFileMutex,1,nullptr)>=0;}
void rawFileUnlock(void*){sceKernelSignalSema(rawFileMutex,1);}
bool windowLock(void*){return windowMutex>=0&&sceKernelWaitSema(windowMutex,1,nullptr)>=0;}
void windowUnlock(void*){sceKernelSignalSema(windowMutex,1);}
bool windowWait(void*){return windowAvailable>=0&&sceKernelWaitSema(windowAvailable,1,nullptr)>=0;}
void windowSignal(void*){sceKernelSignalSema(windowAvailable,1);}
#else
struct HostWindows {std::mutex metadata,availability;std::condition_variable changed;unsigned tokens=1;};
std::mutex& rawFileMutex(){alignas(std::mutex) static unsigned char memory[sizeof(std::mutex)];static auto* value=new(memory)std::mutex;return *value;}
bool rawFileLock(void*){rawFileMutex().lock();return true;}
void rawFileUnlock(void*){rawFileMutex().unlock();}
HostWindows& hostWindows(){alignas(HostWindows) static unsigned char memory[sizeof(HostWindows)];static auto* value=new(memory)HostWindows;return *value;}
bool windowLock(void* context){static_cast<HostWindows*>(context)->metadata.lock();return true;}
void windowUnlock(void* context){static_cast<HostWindows*>(context)->metadata.unlock();}
bool windowWait(void* context){auto& state=*static_cast<HostWindows*>(context);std::unique_lock<std::mutex> lock(state.availability);state.changed.wait(lock,[&](){return state.tokens!=0;});--state.tokens;return true;}
void windowSignal(void* context){auto& state=*static_cast<HostWindows*>(context);{std::lock_guard<std::mutex> lock(state.availability);++state.tokens;}state.changed.notify_one();}
#endif
bool prepareWindows(){
 if(poolReady)return decodedPool.clear();
 void* context=nullptr;
#ifndef __PSP__
 context=&hostWindows();
#endif
 poolReady=decodedPool.initialize({windowLock,windowUnlock,windowWait,windowSignal,context});return poolReady;
}
std::FILE* openPooled(const char*path,const char*mode){
 if(!mode || std::strcmp(mode,"rb")){errno=EINVAL;return nullptr;}
 return rawFiles.open(path);
}
std::string root,payload;
bool configured=false;
unsigned peak=0;
bool insideData(const char* path,bool directory=false) {
    if (!configured || !path) return false;
    const std::string prefix=root+"data";
    const size_t length=std::strlen(path);
    if (length<prefix.size() || std::strncmp(path,prefix.c_str(),prefix.size()) ||
        (length==prefix.size()?!directory:path[prefix.size()]!='/')) return false;
    // No alternate traversal spellings can turn an asset open into a save open.
    for (size_t at=0;at<length;) {
        size_t end=at;while(end<length && path[end]!='/') ++end;
        if ((end-at==2 && path[at]=='.' && path[at+1]=='.') ||
            (end-at==1 && path[at]=='.')) return false;
        at=end+1;
    }
    return !std::strchr(path,'\\');
}
void release(unsigned slot) { Guard guard;slots[slot]={}; }
int closeReader(void* value) {
    auto* reader=static_cast<Reader*>(value);
    {
        Guard guard;
        for (auto& slot:slots) if (slot.reader==reader) { slot={};break; }
    }
    delete reader;return 0;
}
std::FILE* virtualOpen(const char* path,bool& recognized) {
    recognized=false;
    if (!eligible(path)) return nullptr;
    const std::string descriptor=std::string(path)+".ucb";
    auto* input=openPooled(descriptor.c_str(),"rb");
    if (!input) {
        recognized=errno!=ENOENT;
        if(recognized)diagnostic::record("descriptor_open",errno);
        return nullptr;
    }
    recognized=true;
    unsigned selected=MaxFiles;
    { Guard guard;for(unsigned i=0;i<MaxFiles;++i) if(!slots[i].reserved) { slots[i].reserved=true;selected=i;break; } }
    if (selected==MaxFiles) { diagnostic::record("virtual_slots",EMFILE,MaxFiles,MaxFiles);std::fclose(input);errno=EMFILE;return nullptr; }
    auto* reader=new(std::nothrow) Reader;
    if (!reader) { diagnostic::record("reader_alloc",ENOMEM,sizeof(Reader),0);std::fclose(input);release(selected);errno=ENOMEM;return nullptr; }
    if (!reader->setWindowHooks(decodedPool.hooks())) { std::fclose(input);delete reader;release(selected);errno=EIO;return nullptr; }
    if (!reader->openOwned(input,payload.c_str(),openPooled)) { delete reader;release(selected);errno=EIO;return nullptr; }
#ifdef __PSP__
    auto* file=funopen(reader,detail::read,nullptr,detail::seek,closeReader);
#else
    cookie_io_functions_t functions{};functions.read=detail::read;functions.seek=detail::seek;functions.close=closeReader;
    auto* file=fopencookie(reader,"rb",functions);
#endif
    if (!file) { diagnostic::record("funopen",errno);delete reader;release(selected);return nullptr; }
    // Keep stdio's read-ahead small; Reader already retains one decoded block.
    std::setvbuf(file,nullptr,_IOFBF,4096);
    { Guard guard;slots[selected].file=file;slots[selected].reader=reader;
      unsigned count=0;for(const auto& slot:slots) count+=slot.file!=nullptr;peak=std::max(peak,count); }
    return file;
}
std::FILE* wrap(const char* path,const char* mode) {
    // Only exact read modes: writes, append, update and user files stay ordinary.
    if (!path || !mode || (std::strcmp(mode,"rb") && std::strcmp(mode,"r"))) return __real_fopen(path,mode);
    bool recognized=false;
    auto* result=virtualOpen(path,recognized);
    if(recognized)return result;
    auto* plain=physicalOpen(path,mode);
    if(!plain)diagnostic::record("plain_open",errno);
    return plain;
}
}
bool configure(const char* executable) {
#ifdef __PSP__
    // Created before worker threads. A sleeping kernel semaphore avoids the
    // single-core priority inversion/deadlock of an atomic spin lock.
    if(registryMutex<0) registryMutex=sceKernelCreateSema("ucg_storage",0,1,1,nullptr);
    if(registryMutex<0) return false;
    if(windowMutex<0) windowMutex=sceKernelCreateSema("ucg_windows_meta",0,1,1,nullptr);
    if(windowAvailable<0) windowAvailable=sceKernelCreateSema("ucg_windows_free",0,1,1,nullptr);
    if(windowMutex<0||windowAvailable<0) return false;
    if(rawFileMutex<0)rawFileMutex=sceKernelCreateSema("ucg_storage_raw",0,1,1,nullptr);
    if(rawFileMutex<0)return false;
#endif
    Guard guard;
    for (const auto& slot:slots) if (slot.reserved) return false;
    if(!rawFiles.initialize({rawFileLock,rawFileUnlock,nullptr},physicalOpen))return false;
    configured=false;root.clear();payload.clear();peak=0;
    if (!executable || !prepareWindows()) return false;
    root=ReleasePaths::assetDirectory(executable);payload=root+"storage.ucd";
    configured=true;return true;
}
bool eligible(const char* path) {
    if (!insideData(path)) return false;
    const size_t size=std::strlen(path);
    return size<4 || (std::strcmp(path+size-4,".PBP") && std::strcmp(path+size-4,".pbp"));
}
bool virtualDirectory(const char* path) { return insideData(path,true); }
std::FILE* openVirtualOnly(const char* path,bool& recognized) { return virtualOpen(path,recognized); }
bool virtualExists(const char* path) {
    if (!eligible(path)) return false;
    struct stat status{};const auto descriptor=std::string(path)+".ucb";
    const auto physical=ReleaseIsoMap::physical(descriptor.c_str());
    return ::stat(physical.c_str(),&status)==0 && S_ISREG(status.st_mode);
}
bool isVirtual(std::FILE* file) { Guard guard;for(const auto& slot:slots) if(file && slot.file==file) return true;return false; }
unsigned liveFiles() { Guard guard;unsigned result=0;for(const auto& slot:slots) result+=slot.file!=nullptr;return result; }
unsigned peakFiles() { Guard guard;return peak; }
unsigned rawOpenFiles(){return rawFiles.live();}
unsigned rawPeakOpenFiles(){return rawFiles.peak();}
unsigned poolResidentBytes(){return decodedPool.residentBytes();}
unsigned readerShellBytes(){return sizeof(Reader);}
unsigned poolPeakPinned(){return decodedPool.stats().peakPinned;}
unsigned poolPinned(){return decodedPool.stats().pinned;}
unsigned poolInvalidReleases(){return static_cast<unsigned>(decodedPool.stats().invalidReleases); }
} // namespace ucg_storage
extern "C" std::FILE* __wrap_fopen(const char* path,const char* mode) { return ucg_storage::wrap(path,mode); }
