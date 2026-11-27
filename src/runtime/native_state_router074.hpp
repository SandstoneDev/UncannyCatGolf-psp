#pragma once
// Exact progress routes. Callers serialize access; retain Router until FILEs close.
#include "native_state_archive072.hpp"
#include <sys/stat.h>
namespace NativeRoute074 {
struct ImportRecord {const char*name;const void*bytes;std::size_t size;};
class Router {
 NativeState072::Archive archive_;
 char root_[1024]{};std::size_t rootSize_=0;
 enum class State { Unconfigured,Blocked,Active } state_=State::Unconfigured;
 static bool side(const char*s){return !std::strcmp(s,".0")||!std::strcmp(s,".1");}
 static bool componentSuffix(const char*s,bool named){
  return !std::strcmp(s,".sav")||!std::strcmp(s,".clicks")||
   (!std::strncmp(s,".sav",4)&&side(s+4))||(!std::strncmp(s,".clicks",7)&&side(s+7))||
   (named&&(!std::strcmp(s,".meta")||!std::strcmp(s,".meta.tmp")||(!std::strncmp(s,".mods",5)&&side(s+5))));
 }
 static bool normalizedRoot(const char*s,std::size_t&n){
  if(!s||!(n=std::strlen(s))||n>=1024||std::strchr(s,'\\'))return false;
  std::size_t start=0;
  if(s[0]=='/')start=1;
  else {while(start<n&&((s[start]>='a'&&s[start]<='z')||(s[start]>='A'&&s[start]<='Z')||(s[start]>='0'&&s[start]<='9')))++start;
   if(!start||start+1>=n||s[start]!=':'||s[start+1]!='/')return false;
   start+=2;}
  std::size_t pos=start;
  while(pos<n){std::size_t end=pos;while(end<n&&s[end]!='/'){if(s[end]==':'||static_cast<unsigned char>(s[end])<32)return false;++end;}
   if(end==pos||(end-pos==1&&s[pos]=='.')||(end-pos==2&&s[pos]=='.'&&s[pos+1]=='.'))return false;
   pos=end+1;}
  if(n>start&&s[n-1]=='/')--n;
  return n>0;
 }
 const char*name(const char*path,bool&recognized,bool directory=false)const{
  recognized=false;if(state_==State::Unconfigured||!path)return nullptr;
  const auto n=std::strlen(path);
  const bool slash=root_[rootSize_-1]=='/';
  if(directory&&(n==rootSize_||(!slash&&n==rootSize_+1&&path[rootSize_]=='/'))&&!std::strncmp(path,root_,rootSize_)){recognized=true;return "";}
  if(n<=rootSize_||std::strncmp(path,root_,rootSize_)||(!slash&&path[rootSize_]!='/'))return nullptr;
  const char*base=path+rootSize_+(slash?0:1);
  if(!acceptsBasename(base))return nullptr;
  recognized=true;return base;
 }
 bool ready()const{if(state_!=State::Active){errno=EIO;return false;}return true;}
 static void componentName(unsigned slot,unsigned index,char*out,std::size_t cap){
  static const char*suffix[]={".sav",".sav.0",".sav.1",".meta",".meta.tmp",".mods.0",".mods.1",".clicks",".clicks.0",".clicks.1"};
  out[0]=0;if(!slot){if(index<3||index>6)std::snprintf(out,cap,"progress%s",suffix[index]);}
  else std::snprintf(out,cap,"profile-%02u%s",slot,suffix[index]);
 }
public:
 static bool acceptsBasename(const char*n){
  if(!n||!NativeState072::validName(n))return false;
  if(!std::strncmp(n,"progress",8)&&componentSuffix(n+8,false))return true;
  if(!std::strncmp(n,"profile-0",9)&&n[9]>='1'&&n[9]<='7'&&componentSuffix(n+10,true))return true;
  static const char*pair[]={"gifts.cfg","editor-mode.cfg","editor-builder.cfg","endless.cfg","endless-name.cfg","costume.cfg"};
  for(const char*p:pair){auto k=std::strlen(p);if(!std::strncmp(n,p,k)&&side(n+k))return true;}
  if(!std::strncmp(n,"world-state.cfg",15)){
   const char*s=n+15;if(side(s)||!std::strcmp(s,".0.prev")||!std::strcmp(s,".1.prev")||!std::strcmp(s,".tmp"))return true;
  }
  if(!std::strncmp(n,".profile-delete-0",17)&&n[17]>='0'&&n[17]<='7'&&!std::strncmp(n+18,".journal",8)&&side(n+26))return true;
  if(std::strlen(n)==33&&!std::strncmp(n,"profile-delete-0",16)&&n[16]>='0'&&n[16]<='7'&&n[17]=='-'&&n[26]=='-'&&n[27]=='0'&&n[28]>='0'&&n[28]<='9'&&!std::strcmp(n+29,".bak")){
   for(unsigned i=18;i<26;++i)if(!((n[i]>='0'&&n[i]<='9')||(n[i]>='a'&&n[i]<='f')))return false;
   return true;
  }
  return false;
 }
 bool configure(const char*root){
  if(archive_.openHandles()){errno=EBUSY;return false;}std::size_t n=0;
  if(!normalizedRoot(root,n)){errno=EINVAL;return false;}
  std::memcpy(root_,root,n);root_[n]=0;rootSize_=n;state_=State::Blocked;return true;
 }
 bool activate(){
  if(state_==State::Unconfigured){errno=EINVAL;return false;}
  if(archive_.writableHandles()){errno=EBUSY;return false;}
  for(std::size_t i=0;i<archive_.size();++i)if(!acceptsBasename(archive_.nameAt(i))){state_=State::Blocked;errno=EINVAL;return false;}
  state_=State::Active;return true;
 }
 void block(){if(state_!=State::Unconfigured)state_=State::Blocked;}
 bool active()const{return state_==State::Active;}
 NativeState072::Archive&archive(){return archive_;}
 const NativeState072::Archive&archive()const{return archive_;}
 bool importRecords(const ImportRecord*records,std::size_t count){
  if(state_!=State::Blocked){errno=EINVAL;return false;}
  if(archive_.openHandles()){errno=EBUSY;return false;}
  if((count&&!records)||count>NativeState072::MaxEntries){errno=count>NativeState072::MaxEntries?ENOSPC:EINVAL;return false;}
  NativeState072::Archive candidate;
  for(std::size_t i=0;i<count;++i){const auto&r=records[i];
   if(!acceptsBasename(r.name)){errno=EINVAL;return false;}
   if(candidate.contains(r.name)){errno=EEXIST;return false;}
   if(r.size>NativeState072::MaxValueBytes){errno=EFBIG;return false;}
   NativeState072::Bytes bytes;if(!bytes.assign(r.bytes,r.size)||!candidate.put(r.name,bytes))return false;
  }
  archive_=std::move(candidate);return true;
 }
 std::FILE*open(const char*path,const char*mode,bool&recognized){const char*n=name(path,recognized);return !recognized||!ready()?nullptr:archive_.open(n,mode);}
 int rename(const char*from,const char*to,bool&recognized){
  bool a=false,b=false;const char*f=name(from,a);const char*t=name(to,b);recognized=a||b;
  if(!recognized)return -1;
  if(!a||!b){errno=EXDEV;return -1;}if(!ready())return -1;
  return archive_.rename(f,t)?0:-1;
 }
 int unlink(const char*path,bool&recognized){const char*n=name(path,recognized);return !recognized||!ready()?-1:(archive_.erase(n)?0:-1);}
 int stat(const char*path,struct stat*out,bool&recognized){
  const char*n=name(path,recognized,true);if(!recognized||!ready())return -1;
  if(!out){errno=EFAULT;return -1;}std::memset(out,0,sizeof *out);
  if(!*n){out->st_mode=S_IFDIR|0700;out->st_nlink=1;return 0;}
  const auto*b=archive_.get(n);if(!b)return -1;out->st_mode=S_IFREG|0600;out->st_nlink=1;out->st_size=static_cast<decltype(out->st_size)>(b->size());return 0;
 }
 // Conservative intermediate-map reservation; no retained backup is evicted.
 bool canArchiveProfile(unsigned slot)const{
  if(!ready())return false;
  if(slot>=8){errno=EINVAL;return false;}if(archive_.writableHandles()){errno=EBUSY;return false;}
  std::size_t extra=0,entries=0;char n[81];
  for(unsigned s=0;s<2;++s){std::snprintf(n,sizeof n,".profile-delete-%02u.journal.%u",slot,s);
   const auto*b=archive_.get(n);if(!b){extra+=8+std::strlen(n)+200;++entries;}else if(b->size()<200)extra+=200-b->size();}
  for(unsigned i=0;i<10;++i){componentName(slot,i,n,sizeof n);if(!*n)continue;const auto*b=archive_.get(n);if(b){const auto oldLength=std::strlen(n);std::snprintf(n,sizeof n,"profile-delete-%02u-ffffffff-%02u.bak",slot,i);extra+=std::strlen(n)-oldLength;}}
  if(entries>NativeState072::MaxEntries-archive_.size()||extra>NativeState072::MaxArchiveBytes-archive_.encodedSize()){errno=ENOSPC;return false;}return true;
 }
};
}
