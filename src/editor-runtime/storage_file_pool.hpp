#pragma once
// Immutable asset channels: logical FILE cursors are independent; only a
// bounded set of real read-only FILEs consumes PSP kernel file descriptors.
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#ifdef __PSP__
extern "C" std::FILE* funopen(const void*,int(*)(void*,char*,int),
    int(*)(void*,const char*,int),fpos_t(*)(void*,fpos_t,int),int(*)(void*));
#endif
namespace ucg_storage { namespace rawfile {
struct Sync {
 bool(*lock)(void*)=nullptr;void(*unlock)(void*)=nullptr;void*context=nullptr;
 bool complete()const{return lock&&unlock;}
};
template<unsigned Channels=4> class Pool {
 static_assert(Channels>0 && Channels<=8,"bounded immutable raw handles");
 static constexpr unsigned PathBytes=512;
 struct Cookie {Pool*pool;Cookie*next=nullptr;int64_t position=0;long length=0;char path[PathBytes]{};};
 struct Slot {std::FILE*file=nullptr;uint64_t age=0;char path[PathBytes]{};long length=-1;};
 Slot slots[Channels]{};Cookie*head=nullptr;Sync sync{};
 using Opener=std::FILE*(*)(const char*,const char*);
 Opener opener=nullptr;uint64_t clock=0;unsigned live_=0,peak_=0;
 bool ready=false,poisoned=false;
 struct Guard {
  Pool&pool;bool held;
  explicit Guard(Pool&p):pool(p),held(p.ready&&p.sync.lock(p.sync.context)){}
  ~Guard(){if(held)pool.sync.unlock(pool.sync.context);}
 };
 bool closeSlot(Slot&s){
  auto*f=s.file;s.file=nullptr;s.path[0]=0;s.age=0;s.length=-1;
  if(!f)return true;
  if(std::fclose(f)){
   // fclose invalidates FILE even on error; never retry it or assume its raw
   // handle was released. Charge that uncertain handle until process exit.
   poisoned=true;if(!errno)errno=EIO;return false;
  }
#ifdef UCG_STREAM_PROFILE
  ++stats_.closes;
#endif
  --live_;return true;
 }
 Slot* channel(const char*path){
  if(poisoned){errno=EIO;return nullptr;}
  for(auto&s:slots)if(s.file && !std::strcmp(s.path,path)){
#ifdef UCG_STREAM_PROFILE
   ++stats_.reuses;
#endif
   s.age=++clock;return &s;}
  Slot*chosen=&slots[0];
  for(auto&s:slots){if(!s.file){chosen=&s;break;}if(s.age<chosen->age)chosen=&s;}
  if(!closeSlot(*chosen))return nullptr;
  auto*f=opener(path,"rb");if(!f)return nullptr;
  chosen->file=f;std::memcpy(chosen->path,path,std::strlen(path)+1);chosen->age=++clock;
#ifdef UCG_STREAM_PROFILE
  ++stats_.opens;
#endif
  ++live_;peak_=std::max(peak_,live_);return chosen;
 }
 bool forgetIfUnused(const char*path){
  for(auto*c=head;c;c=c->next)if(!std::strcmp(c->path,path))return true;
  for(auto&s:slots)if(s.file && !std::strcmp(s.path,path))return closeSlot(s);
  return true;
 }
 int read(Cookie&c,void*output,unsigned bytes){
  if(!bytes || c.position>=c.length)return 0;
  bytes=std::min<unsigned>(bytes,static_cast<unsigned>(c.length-c.position));
  Guard guard(*this);if(!guard.held){errno=EIO;return -1;}
  auto*s=channel(c.path);if(!s)return -1;
#ifdef UCG_STREAM_PROFILE
  ++stats_.readCalls;++stats_.readSeeks;
#endif
  std::clearerr(s->file);
  if(std::fseek(s->file,static_cast<long>(c.position),SEEK_SET))return -1;
  const auto got=std::fread(output,1,bytes,s->file);
  if(std::ferror(s->file)){if(!errno)errno=EIO;return -1;}
#ifdef UCG_STREAM_PROFILE
  stats_.readBytes+=got;
#endif
  c.position+=got;return static_cast<int>(got);
 }
 static int closeCookie(void*value){
  auto*c=static_cast<Cookie*>(value);auto&pool=*c->pool;
  bool okay=true;
  {Guard guard(pool);if(!guard.held){errno=EIO;return -1;}
   auto**at=&pool.head;while(*at && *at!=c)at=&(*at)->next;
   if(*at)*at=c->next;
   okay=pool.forgetIfUnused(c->path);
  }
  delete c;return okay?0:-1;
 }
 bool seek(Cookie&c,int64_t offset,int origin){
  int64_t base=0;
  if(origin==SEEK_CUR)base=c.position;else if(origin==SEEK_END)base=c.length;
  else if(origin!=SEEK_SET){errno=EINVAL;return false;}
  if(offset< -base || offset>LONG_MAX-base){errno=EINVAL;return false;}
  c.position=base+offset;return true;
 }
#ifdef __PSP__
 static int readCookie(void*v,char*out,int bytes){if(bytes<0){errno=EINVAL;return -1;}auto&c=*static_cast<Cookie*>(v);return c.pool->read(c,out,bytes);}
 static fpos_t seekCookie(void*v,fpos_t off,int origin){auto&c=*static_cast<Cookie*>(v);return c.pool->seek(c,off,origin)?static_cast<fpos_t>(c.position):static_cast<fpos_t>(-1);}
#else
 static ssize_t readCookie(void*v,char*out,size_t bytes){auto&c=*static_cast<Cookie*>(v);return c.pool->read(c,out,std::min<size_t>(bytes,INT_MAX));}
 static int seekCookie(void*v,off64_t*off,int origin){auto&c=*static_cast<Cookie*>(v);if(!c.pool->seek(c,*off,origin))return -1;*off=c.position;return 0;}
#endif
public:
#ifdef UCG_STREAM_PROFILE
 struct Stats {unsigned opens=0,closes=0,logical=0,reuses=0,endSeeks=0,readCalls=0,readBytes=0,readSeeks=0;};
private:
 Stats stats_{};
public:
 Stats stats(){Guard guard(*this);return stats_;}
#endif
 bool initialize(Sync hooks,Opener open){
  if(!hooks.complete() || !open)return false;
  if(!ready){sync=hooks;opener=open;ready=true;}
  Guard guard(*this);if(!guard.held || head || poisoned)return false;
  for(auto&s:slots)closeSlot(s);
  if(poisoned)return false;
  clock=0;peak_=0;return true;
 }
 std::FILE* open(const char*path){
  if(!path || !ready){errno=EINVAL;return nullptr;}
  const auto length=std::strlen(path);if(length>=PathBytes){errno=ENAMETOOLONG;return nullptr;}
  auto*c=new(std::nothrow) Cookie;if(!c){errno=ENOMEM;return nullptr;}
  c->pool=this;std::memcpy(c->path,path,length+1);
  {Guard guard(*this);if(!guard.held){delete c;errno=EIO;return nullptr;}
   auto*s=channel(path);
   if(!s){delete c;return nullptr;}
   std::clearerr(s->file);
   if(s->length<0){
#ifdef UCG_STREAM_PROFILE
    ++stats_.endSeeks;
#endif
    if(std::fseek(s->file,0,SEEK_END)){
     const int error=errno;forgetIfUnused(path);delete c;errno=error;return nullptr;
    }
    const long length=std::ftell(s->file);
    if(length<0){const int error=errno;forgetIfUnused(path);delete c;errno=error;return nullptr;}
    s->length=length;
   }
   c->length=s->length;
#ifdef UCG_STREAM_PROFILE
   ++stats_.logical;
#endif
   c->next=head;head=c;
  }
#ifdef __PSP__
  auto*f=funopen(c,readCookie,nullptr,seekCookie,closeCookie);
#else
  cookie_io_functions_t functions{};functions.read=readCookie;functions.seek=seekCookie;functions.close=closeCookie;
  auto*f=fopencookie(c,"rb",functions);
#endif
  if(!f){const int error=errno;closeCookie(c);errno=error;return nullptr;}
  // The shared raw FILE already buffers reads; a second per-cookie buffer
  // would duplicate memory and obscure the independent logical cursor.
  if(std::setvbuf(f,nullptr,_IONBF,0)){const int error=errno;std::fclose(f);errno=error;return nullptr;}
  return f;
 }
 unsigned live(){Guard guard(*this);return guard.held?live_:0;}
 unsigned peak(){Guard guard(*this);return guard.held?peak_:0;}
};
} }
