#pragma once
// Bounded native save archive and RAM-backed stdio. No filesystem or PSP utility calls.
// Host translation units must define _GNU_SOURCE before their first libc include.
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>
#ifndef NATIVE_STATE072_ALLOC
#define NATIVE_STATE072_ALLOC(n) std::malloc(n)
#endif
#ifndef NATIVE_STATE072_FREE
#define NATIVE_STATE072_FREE(p) std::free(p)
#endif
#ifdef __PSP__
extern "C" std::FILE* funopen(const void*,int(*)(void*,char*,int),
 int(*)(void*,const char*,int),fpos_t(*)(void*,fpos_t,int),int(*)(void*));
#endif
namespace NativeState072 {
constexpr std::size_t MaxArchiveBytes=256u*1024u,MaxEntries=256,MaxNameBytes=80,MaxValueBytes=16u*1024u;
class Bytes {
 unsigned char*data_=nullptr;std::size_t size_=0,capacity_=0;
public:
 Bytes()=default;
 ~Bytes(){NATIVE_STATE072_FREE(data_);}
 Bytes(const Bytes&)=delete;Bytes&operator=(const Bytes&)=delete;
 Bytes(Bytes&&b)noexcept{swap(b);}Bytes&operator=(Bytes&&b)noexcept{if(this!=&b){Bytes old;old.swap(*this);swap(b);}return *this;}
 void swap(Bytes&b)noexcept{std::swap(data_,b.data_);std::swap(size_,b.size_);std::swap(capacity_,b.capacity_);}
 const unsigned char*data()const{return data_;}unsigned char*data(){return data_;}
 std::size_t size()const{return size_;}bool empty()const{return size_==0;}
 bool assign(const void*src,std::size_t n){
  if(n>MaxArchiveBytes){errno=EFBIG;return false;}if(n&&!src){errno=EINVAL;return false;}
  Bytes tmp;if(n){tmp.data_=static_cast<unsigned char*>(NATIVE_STATE072_ALLOC(n));if(!tmp.data_){errno=ENOMEM;return false;}std::memcpy(tmp.data_,src,n);tmp.size_=tmp.capacity_=n;}swap(tmp);return true;
 }
 bool resize(std::size_t n){
  if(n>MaxArchiveBytes){errno=EFBIG;return false;}
  if(n<=capacity_){if(n>size_)std::memset(data_+size_,0,n-size_);size_=n;return true;}
  auto*p=static_cast<unsigned char*>(NATIVE_STATE072_ALLOC(n));if(!p){errno=ENOMEM;return false;}
  if(size_){std::memcpy(p,data_,size_);}std::memset(p+size_,0,n-size_);NATIVE_STATE072_FREE(data_);data_=p;capacity_=size_=n;return true;
 }
 bool equals(const Bytes&b)const{return size_==b.size_&&(!size_||!std::memcmp(data_,b.data_,size_));}
};
inline bool validName(const char*n){
 if(!n){errno=EINVAL;return false;}std::size_t len=0;
 for(;n[len];++len){unsigned char c=static_cast<unsigned char>(n[len]);if(len>=MaxNameBytes||!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='.'||c=='-')){errno=EINVAL;return false;}}
 if(!len||!std::strcmp(n,".")||!std::strcmp(n,"..")){errno=EINVAL;return false;}return true;
}
class Archive;
inline bool encode(const Archive&,uint64_t,Bytes&);
inline bool decode(const Bytes&,Archive&,uint64_t&);
class Archive {
 struct Entry {char name[MaxNameBytes+1]{};Bytes bytes;};
 Entry entries_[MaxEntries];std::size_t count_=0,total_=32,openHandles_=0,writableHandles_=0;uint64_t revision_=0,epoch_=0;
 std::size_t lower(const char*n)const{std::size_t a=0,b=count_;while(a<b){auto m=a+(b-a)/2;if(std::strcmp(entries_[m].name,n)<0)a=m+1;else b=m;}return a;}
 bool canMutate()const{if(revision_==UINT64_MAX){errno=EOVERFLOW;return false;}return true;}
 void removeAt(std::size_t i){for(;i+1<count_;++i)entries_[i]=std::move(entries_[i+1]);entries_[--count_]=Entry{};}
 void insertAt(std::size_t i,const char*n,Bytes&&b){for(auto j=count_;j>i;--j)entries_[j]=std::move(entries_[j-1]);std::strcpy(entries_[i].name,n);entries_[i].bytes=std::move(b);++count_;}
 friend bool encode(const Archive&,uint64_t,Bytes&);friend bool decode(const Bytes&,Archive&,uint64_t&);
 struct Cookie {
  Archive*owner;Bytes bytes;char name[MaxNameBytes+1]{};std::size_t position=0;uint64_t epoch;
  bool readable=false,writable=false,append=false,failed=false;int error=0;
 };
 static int readCookie(void*v,char*out,int n){auto&c=*static_cast<Cookie*>(v);if(n<0||!c.readable){errno=EBADF;return -1;}auto got=std::min<std::size_t>(static_cast<std::size_t>(n),c.position<c.bytes.size()?c.bytes.size()-c.position:0);if(got)std::memcpy(out,c.bytes.data()+c.position,got);c.position+=got;return static_cast<int>(got);}
 static int writeCookie(void*v,const char*in,int n){
  auto&c=*static_cast<Cookie*>(v);if(n<0||!c.writable){errno=EBADF;return -1;}if(c.failed){errno=c.error;return -1;}if(!n)return 0;
  const auto pos=c.append?c.bytes.size():c.position;
  if(static_cast<std::size_t>(n)>MaxValueBytes-pos){c.failed=true;c.error=EFBIG;errno=c.error;return -1;}
  auto end=pos+static_cast<std::size_t>(n);if(end>c.bytes.size()&&!c.bytes.resize(end)){c.failed=true;c.error=errno;return -1;}
  std::memcpy(c.bytes.data()+pos,in,static_cast<std::size_t>(n));c.position=end;return n;
 }
 static bool seekCookie(Cookie&c,int64_t off,int origin){
  int64_t base=0;if(origin==SEEK_CUR)base=static_cast<int64_t>(c.position);else if(origin==SEEK_END)base=static_cast<int64_t>(c.bytes.size());else if(origin!=SEEK_SET){errno=EINVAL;return false;}
  if(off < -base||off > static_cast<int64_t>(MaxValueBytes)-base){errno=EINVAL;return false;}c.position=static_cast<std::size_t>(base+off);return true;
 }
 static int closeCookie(void*v){
  auto*c=static_cast<Cookie*>(v);int result=0,saved=0;
  if(c->writable){if(c->failed){result=-1;saved=c->error;}else if(c->owner->epoch_!=c->epoch){result=-1;saved=EAGAIN;}else if(!c->owner->put(c->name,c->bytes)){result=-1;saved=errno;}}
  --c->owner->openHandles_;if(c->writable)--c->owner->writableHandles_;
  c->~Cookie();NATIVE_STATE072_FREE(c);if(result)errno=saved;return result;
 }
#ifdef __PSP__
 static fpos_t seekPSP(void*v,fpos_t off,int origin){auto&c=*static_cast<Cookie*>(v);return seekCookie(c,off,origin)?static_cast<fpos_t>(c.position):static_cast<fpos_t>(-1);}
#else
 static ssize_t readHost(void*v,char*out,std::size_t n){return readCookie(v,out,static_cast<int>(std::min<std::size_t>(n,INT_MAX)));}
 static ssize_t writeHost(void*v,const char*in,std::size_t n){return writeCookie(v,in,static_cast<int>(std::min<std::size_t>(n,INT_MAX)));}
 static int seekHost(void*v,off64_t*off,int origin){auto&c=*static_cast<Cookie*>(v);if(!seekCookie(c,*off,origin))return -1;*off=static_cast<off64_t>(c.position);return 0;}
#endif
public:
 Archive()=default;Archive(const Archive&)=delete;Archive&operator=(const Archive&)=delete;
 Archive(Archive&&a)noexcept{*this=std::move(a);}
 Archive&operator=(Archive&&a)noexcept{if(this!=&a){for(std::size_t i=0;i<MaxEntries;++i)entries_[i]=std::move(a.entries_[i]);count_=a.count_;total_=a.total_;revision_=a.revision_;a.count_=0;a.total_=32;a.revision_=0;++epoch_;++a.epoch_;}return *this;}
 std::size_t size()const{return count_;}std::size_t encodedSize()const{return total_;}uint64_t revision()const{return revision_;}
 std::size_t openHandles()const{return openHandles_;}std::size_t writableHandles()const{return writableHandles_;}
 const char*nameAt(std::size_t i)const{return i<count_?entries_[i].name:nullptr;}
 const Bytes*get(const char*n)const{if(!validName(n))return nullptr;auto i=lower(n);if(i==count_||std::strcmp(entries_[i].name,n)){errno=ENOENT;return nullptr;}return &entries_[i].bytes;}
 bool contains(const char*n)const{return get(n)!=nullptr;}
 bool put(const char*n,const Bytes&b){
  if(!validName(n)){return false;}if(b.size()>MaxValueBytes){errno=EFBIG;return false;}auto i=lower(n);bool exists=i<count_&&!std::strcmp(entries_[i].name,n);
  if(exists&&entries_[i].bytes.equals(b))return true;
  const auto add=8+std::strlen(n)+b.size(),old=exists?8+std::strlen(n)+entries_[i].bytes.size():0;
  if((!exists&&count_==MaxEntries)||add>MaxArchiveBytes-(total_-old)){errno=ENOSPC;return false;}if(!canMutate())return false;
  Bytes copy;if(!copy.assign(b.data(),b.size()))return false;
  if(exists)entries_[i].bytes=std::move(copy);else insertAt(i,n,std::move(copy));total_=total_-old+add;++revision_;++epoch_;return true;
 }
 bool erase(const char*n){if(!validName(n))return false;auto i=lower(n);if(i==count_||std::strcmp(entries_[i].name,n)){errno=ENOENT;return false;}if(!canMutate())return false;total_-=8+std::strlen(n)+entries_[i].bytes.size();removeAt(i);++revision_;++epoch_;return true;}
 bool rename(const char*from,const char*to){
  if(!validName(from)||!validName(to)){return false;}auto i=lower(from);if(i==count_||std::strcmp(entries_[i].name,from)){errno=ENOENT;return false;}if(!std::strcmp(from,to))return true;
  auto j=lower(to);bool replace=j<count_&&!std::strcmp(entries_[j].name,to);auto old=8+std::strlen(from)+entries_[i].bytes.size();if(replace)old+=8+std::strlen(to)+entries_[j].bytes.size();auto add=8+std::strlen(to)+entries_[i].bytes.size();
  if(add>MaxArchiveBytes-(total_-old)){errno=ENOSPC;return false;}if(!canMutate())return false;
  // Copy names before shifting: callers may pass a key pointer from unrelated APIs.
  char destination[MaxNameBytes+1];std::strcpy(destination,to);Bytes value=std::move(entries_[i].bytes);removeAt(i);j=lower(destination);if(replace)removeAt(j);insertAt(lower(destination),destination,std::move(value));total_=total_-old+add;++revision_;++epoch_;return true;
 }
 bool cloneTo(Archive&out)const{if(this==&out)return true;Archive tmp;for(std::size_t i=0;i<count_;++i){if(!tmp.entries_[i].bytes.assign(entries_[i].bytes.data(),entries_[i].bytes.size()))return false;std::strcpy(tmp.entries_[i].name,entries_[i].name);}tmp.count_=count_;tmp.total_=total_;tmp.revision_=revision_;out=std::move(tmp);return true;}
 std::FILE*open(const char*n,const char*mode){
  if(!validName(n)||!mode||!mode[0]){errno=EINVAL;return nullptr;}char base=mode[0];if(base!='r'&&base!='w'&&base!='a'){errno=EINVAL;return nullptr;}bool plus=false,binary=false;
  for(std::size_t i=1;mode[i];++i){if(mode[i]=='+'&&!plus)plus=true;else if(mode[i]=='b'&&!binary)binary=true;else{errno=EINVAL;return nullptr;}}
  const Bytes*existing=get(n);if(!existing&&base=='r')return nullptr;
  auto*mem=NATIVE_STATE072_ALLOC(sizeof(Cookie));if(!mem){errno=ENOMEM;return nullptr;}auto*c=new(mem)Cookie;c->owner=this;c->epoch=epoch_;std::strcpy(c->name,n);c->readable=base=='r'||plus;c->writable=base!='r'||plus;c->append=base=='a';
  if(existing&&base!='w'&&!c->bytes.assign(existing->data(),existing->size())){c->~Cookie();NATIVE_STATE072_FREE(c);return nullptr;}if(c->append)c->position=c->bytes.size();
#ifdef __PSP__
  auto*f=funopen(c,c->readable?readCookie:nullptr,c->writable?writeCookie:nullptr,seekPSP,closeCookie);
#else
  cookie_io_functions_t io{};io.read=c->readable?readHost:nullptr;io.write=c->writable?writeHost:nullptr;io.seek=seekHost;io.close=closeCookie;auto*f=fopencookie(c,mode,io);
#endif
  if(!f){c->~Cookie();NATIVE_STATE072_FREE(c);return nullptr;}
  ++openHandles_;if(c->writable)++writableHandles_;
  if(std::setvbuf(f,nullptr,_IONBF,0)){if(c->writable)--writableHandles_;c->writable=false;std::fclose(f);errno=EIO;return nullptr;}return f;
 }
};
namespace detail {
inline void u16(unsigned char*p,uint16_t v){p[0]=static_cast<unsigned char>(v);p[1]=static_cast<unsigned char>(v>>8);}
inline void u32(unsigned char*p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=static_cast<unsigned char>(v>>(8*i));}
inline uint16_t r16(const unsigned char*p){return static_cast<uint16_t>(p[0]|(uint16_t(p[1])<<8));}
inline uint32_t r32(const unsigned char*p){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(p[i])<<(8*i);return v;}
inline uint32_t checksum(const Bytes&b){uint32_t h=2166136261u;for(std::size_t i=0;i<b.size();++i)if(i<28||i>=32){h^=b.data()[i];h*=16777619u;}return h;}
}
inline bool encode(const Archive&a,uint64_t generation,Bytes&out){
 if(!generation){errno=EINVAL;return false;}Bytes tmp;if(!tmp.resize(a.total_))return false;auto*p=tmp.data();std::memcpy(p,"UCGSAVE1",8);detail::u32(p+8,1);detail::u32(p+12,static_cast<uint32_t>(a.total_));for(unsigned i=0;i<8;++i)p[16+i]=static_cast<unsigned char>(generation>>(8*i));detail::u32(p+24,static_cast<uint32_t>(a.count_));std::size_t pos=32;
 for(std::size_t i=0;i<a.count_;++i){auto&e=a.entries_[i];auto n=std::strlen(e.name);detail::u16(p+pos,static_cast<uint16_t>(n));detail::u16(p+pos+2,0);detail::u32(p+pos+4,static_cast<uint32_t>(e.bytes.size()));pos+=8;std::memcpy(p+pos,e.name,n);pos+=n;if(e.bytes.size())std::memcpy(p+pos,e.bytes.data(),e.bytes.size());pos+=e.bytes.size();}detail::u32(p+28,detail::checksum(tmp));out.swap(tmp);return true;
}
inline bool decode(const Bytes&input,Archive&out,uint64_t&generation){
 const auto*p=input.data();const auto size=input.size();if(size<32||size>MaxArchiveBytes){errno=EINVAL;return false;}if(std::memcmp(p,"UCGSAVE1",8)||detail::r32(p+8)!=1||detail::r32(p+12)!=size||detail::r32(p+24)>MaxEntries||detail::r32(p+28)!=detail::checksum(input)){errno=EINVAL;return false;}
 uint64_t gen=0;for(unsigned i=0;i<8;++i)gen|=uint64_t(p[16+i])<<(8*i);if(!gen){errno=EINVAL;return false;}Archive tmp;auto count=detail::r32(p+24);std::size_t pos=32;
 for(uint32_t i=0;i<count;++i){if(size-pos<8){errno=EINVAL;return false;}auto n=detail::r16(p+pos);auto reserved=detail::r16(p+pos+2);auto len=detail::r32(p+pos+4);pos+=8;if(!n||n>MaxNameBytes||reserved||len>MaxValueBytes||n>size-pos||len>size-pos-n){errno=EINVAL;return false;}char name[MaxNameBytes+1];std::memcpy(name,p+pos,n);name[n]=0;if(std::memchr(name,0,n)||!validName(name)||(i&&std::strcmp(tmp.entries_[i-1].name,name)>=0)){errno=EINVAL;return false;}pos+=n;auto&e=tmp.entries_[i];if(!e.bytes.assign(p+pos,len))return false;std::strcpy(e.name,name);pos+=len;++tmp.count_;}
 if(pos!=size){errno=EINVAL;return false;}tmp.total_=size;out=std::move(tmp);generation=gen;return true;
}
} // namespace NativeState072


