#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
namespace AvatarClicks058 {
struct Input {bool clicked=false,consume=false;};
struct Context {
 bool campaign=false,profile=false,paused=false,caught=false,won=false,waiting=false;
 bool demo=false,endless=false,horse=false,cinematic=false,conflict=false;
 bool eligible()const{return campaign&&profile&&!paused&&!caught&&!won&&!waiting&&!demo&&!endless&&!horse&&!cinematic&&!conflict;}
};
constexpr uint64_t Maximum=9223372036854775807ull;
class Store {
 struct Record {uint32_t magic,sequence,low,high,check;};
 static_assert(sizeof(Record)==20);
 static constexpr uint32_t Magic=0x31434355; // UCC1, optional extension only
 std::string path;
 static uint64_t value(const Record&r){return uint64_t(r.low)|(uint64_t(r.high)<<32);}
 static uint32_t hash(const Record&r){
  const auto*p=reinterpret_cast<const uint8_t*>(&r);uint32_t h=2166136261u;
  for(unsigned i=0;i<16;++i)h=(h^p[i])*16777619u;
  return h;
 }
 static bool newer(uint32_t a,uint32_t b){return a!=b&&uint32_t(a-b)<0x80000000u;}
 bool load(unsigned index,Record&r)const{
  auto*f=std::fopen((path+"."+char('0'+index)).c_str(),"rb");if(!f)return false;
  const bool ok=std::fread(&r,sizeof r,1,f)==1&&std::fgetc(f)==EOF&&!std::ferror(f);
  std::fclose(f);return ok&&r.magic==Magic&&value(r)<=Maximum&&r.check==hash(r);
 }
 int latest(Record&r)const{
  Record a{},b{};const bool va=load(0,a),vb=load(1,b);
  if(!va&&!vb)return -1;
  if(vb&&(!va||newer(b.sequence,a.sequence))){r=b;return 1;}
  r=a;return 0;
 }
public:
 explicit Store(std::string name):path(std::move(name)){}
 uint64_t read()const{Record r{};return latest(r)<0?0:value(r);}
 bool write(uint64_t count)const{
  if(count>Maximum)return false;
  Record previous{};const int old=latest(previous);const unsigned index=old<0?0:unsigned(old)^1u;
  Record r{Magic,old<0?1u:previous.sequence+1,uint32_t(count),uint32_t(count>>32),0};r.check=hash(r);
  auto*f=std::fopen((path+"."+char('0'+index)).c_str(),"wb");if(!f)return false;
  const bool wrote=std::fwrite(&r,sizeof r,1,f)==1;const bool closed=std::fclose(f)==0;
  Record check{};return wrote&&closed&&load(index,check)&&check.sequence==r.sequence&&value(check)==count;
 }
};
struct Session {
 uint64_t count=0,speechCount=0,remaining=0;
 unsigned slot=8,speechCode=0;
 bool dirty=false,suppressed=false;
 std::string root;
 static constexpr unsigned Slots=8;
 std::string path(unsigned value)const{
  if(!value)return root+"progress.clicks";
  char name[32];std::snprintf(name,sizeof name,"profile-%02u.clicks",value);return root+name;
 }
 bool bind(unsigned value){
  if(value>=Slots)value=Slots;
  if(slot==value)return true;
  if(!flush())return false; // never discard dirty previous profile on failure
  slot=value;count=slot<Slots?Store(path(slot)).read():0;dirty=false;clearSpeech();return true;
 }
 bool flush(){
  if(!dirty)return true;
  if(slot>=Slots||!Store(path(slot)).write(count))return false;
  dirty=false;return true;
 }
 void forget(unsigned value){if(slot==value){slot=Slots;count=0;dirty=false;clearSpeech();}}
 Input input(bool held,bool pressed,bool left,bool allowed){
  if(!held){const bool consumed=suppressed;suppressed=false;return {false,consumed};}
  if(pressed&&left&&allowed&&!suppressed&&slot<Slots){suppressed=true;click();return {true,true};}
  return {false,suppressed};
 }
 void click(){
  if(slot>=Slots||count==Maximum)return;
  ++count;dirty=true;
  const unsigned code=count==10?1:count==35?2:count%100==0?3:0;
  if(code&&!remaining){speechCode=code;speechCount=count;remaining=5000000;}
 }
 void tick(unsigned micros){if(remaining){if(micros>=remaining)clearSpeech();else remaining-=micros;}}
 void clearSpeech(){speechCode=0;remaining=0;}
};
}
