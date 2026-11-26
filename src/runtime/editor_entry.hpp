#pragma once
#include "release_paths.hpp"
#include "editor_entry_data.hpp"
#include "locale.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <zlib.h>

namespace EditorEntry {
inline bool validBool(uint32_t value){return value<=1;}
inline bool validBuilderState(uint32_t value){return value<=2;}
inline bool builderUnlocked(const char*mainExecutable,unsigned profile){
 if(!mainExecutable||profile>=8)return false;
 std::string path=ReleasePaths::state(mainExecutable,"editor-builder.cfg");
 return PreferenceFile<0x45424355,validBuilderState>(path).read()==1;
}
}

namespace EditorEntry054 {
inline constexpr unsigned VramStart=0x88000,WorkspaceBytes=65536;
inline constexpr const char* ModuleRelative="data/editor/EBOOT.PBP";
struct InflateArena {uint8_t*memory;size_t bytes,used=0;};
inline voidpf allocate(voidpf opaque,uInt count,uInt size){
 auto&arena=*static_cast<InflateArena*>(opaque);
 if(!count||!size||count>WorkspaceBytes/size)return nullptr;
 const size_t bytes=size_t(count)*size;
 const uintptr_t base=reinterpret_cast<uintptr_t>(arena.memory);
 const size_t at=size_t(((base+arena.used+63u)&~uintptr_t(63u))-base);
 if(at>arena.bytes||bytes>arena.bytes-at)return nullptr;
 auto*result=arena.memory+at;std::memset(result,0,bytes);arena.used=at+bytes;return result;
}
inline void release(voidpf,voidpf){}
inline bool inflateFrame(FILE*file,const EditorEntry::Frame&record,void*output,size_t outputBytes,void*workspace,size_t workspaceBytes){
 if(!file||!output||!workspace||outputBytes<EditorEntry::RawBytes||workspaceBytes<WorkspaceBytes||!record.bytes||std::fseek(file,long(record.offset),SEEK_SET)!=0)return false;
 InflateArena arena{static_cast<uint8_t*>(workspace),WorkspaceBytes};z_stream stream{};
 stream.zalloc=allocate;stream.zfree=release;stream.opaque=&arena;
 if(inflateInit(&stream)!=Z_OK)return false;
 alignas(64) uint8_t input[4096],decoded[4096];unsigned remaining=record.bytes,written=0;uLong crc=crc32(0,nullptr,0);bool good=false;
 for(;;){
  if(!stream.avail_in&&remaining){const unsigned count=std::min<unsigned>(sizeof input,remaining);if(std::fread(input,1,count,file)!=count)break;remaining-=count;stream.next_in=input;stream.avail_in=count;}
  stream.next_out=decoded;stream.avail_out=sizeof decoded;const int status=inflate(&stream,Z_NO_FLUSH);const unsigned count=sizeof decoded-stream.avail_out;
  if((status!=Z_OK&&status!=Z_STREAM_END)||written>EditorEntry::RawBytes||count>EditorEntry::RawBytes-written)break;
  crc=crc32(crc,decoded,count);std::memcpy(static_cast<uint8_t*>(output)+written,decoded,count);written+=count;
  if(status==Z_STREAM_END){good=written==EditorEntry::RawBytes&&!remaining&&!stream.avail_in&&crc==record.crc;break;}
  if(!count&&!stream.avail_in&&!remaining)break;
 }
 inflateEnd(&stream);return good;
}
inline bool isReturn(int argc,char**argv){return argc>1&&argv&&argv[1]&&std::strcmp(argv[1],"editor-return")==0;}
inline unsigned returnProfile(int argc,char**argv,unsigned count){
 if(argc<3||!argv||!argv[2]||!*argv[2])return 0;
 unsigned value=0;
 for(const unsigned char*p=reinterpret_cast<const unsigned char*>(argv[2]);*p;++p){if(*p<'0'||*p>'9'||value>UINT32_MAX/10)return 0;const unsigned digit=*p-'0';if(value==UINT32_MAX/10&&digit>UINT32_MAX%10)return 0;value=value*10+digit;}
 return value<count?value:0;
}
inline std::string arguments(const std::string&mainExecutable,unsigned profile){
 std::string value=mainExecutable;value.push_back('\0');value+="editor";value.push_back('\0');value+=std::to_string(profile);value.push_back('\0');return value;
}
// Peak Hunter Editor handoff: one checked, consumed record; never profile data.
struct PeakReturnRecord {uint32_t magic,version,enabled,check;};
static_assert(sizeof(PeakReturnRecord)==16);
inline uint32_t peakReturnCheck(const PeakReturnRecord&r){
 return ((r.magic^r.version)*16777619u^r.enabled)*16777619u;
}
inline bool clearPeakReturn(const std::string&path){
 return std::remove(path.c_str())==0||errno==ENOENT;
}
inline bool readPeakReturn(const std::string&path,bool&enabled){
 auto*file=std::fopen(path.c_str(),"rb");if(!file)return false;
 PeakReturnRecord record{};
 const bool valid=std::fread(&record,sizeof record,1,file)==1&&std::fgetc(file)==EOF&&!std::ferror(file)&&
  record.magic==0x50524355u&&record.version==1&&record.enabled<=1&&record.check==peakReturnCheck(record);
 const bool closed=std::fclose(file)==0;
 if(valid&&closed)enabled=record.enabled!=0;
 return valid&&closed;
}
inline bool writePeakReturn(const std::string&path,bool enabled){
 if(!clearPeakReturn(path))return false;
 PeakReturnRecord record{0x50524355u,1,uint32_t(enabled),0};record.check=peakReturnCheck(record);
 auto*file=std::fopen(path.c_str(),"wb");if(!file)return false;
 const bool written=std::fwrite(&record,sizeof record,1,file)==1,closed=std::fclose(file)==0;
 bool verified=false;
 if(written&&closed&&readPeakReturn(path,verified)&&verified==enabled)return true;
 clearPeakReturn(path);return false;
}
inline bool consumePeakReturn(const std::string&path,bool editorReturning){
 bool enabled=false;
 // Short-circuit is deliberate: ordinary Main boot never reads session data.
 const bool valid=editorReturning&&readPeakReturn(path,enabled);
 const bool cleared=clearPeakReturn(path);
 return valid&&cleared&&enabled;
}
inline bool validModule(FILE*file){
 uint32_t header[10]{};bool good=file&&std::fread(header,sizeof header,1,file)==1&&header[0]==0x50425000;
 if(good){good=std::fseek(file,0,SEEK_END)==0;const long size=good?std::ftell(file):-1;good=size>=40&&header[2]>=40;for(unsigned i=2;good&&i<10;++i)good=header[i]<=uint64_t(size)&&(i==2||header[i]>=header[i-1]);good=good&&header[8]<uint64_t(size);}
 return good;
}
}

#ifndef EDITOR_ENTRY_HOST_TEST
#include <psploadexec.h>
#include "release_loadexec.hpp"
namespace EditorEntry054 {
inline FILE*archive=nullptr;
inline std::string mainPath,modulePath,artPath,peakReturnPath;
inline int cachedFrame=-1;
inline bool artFailed=false,launchPending=false,requestError=false;
inline unsigned launchProfile=0;
// PC Master owns unlocks globally. Seed the compatibility latch from every
// existing campaign profile once, then retain it when profiles are removed.
inline bool editorUnlocked=false,unlockWriteAttempted=false,unlockWriteFailed=false;
inline bool sharedWorldAuthority=false;
inline bool (*worldOwnsRead)(unsigned)=nullptr;
inline std::string unlockPath;
inline void retainUnlock(){
 editorUnlocked=true;
 if(!unlockWriteAttempted&&!unlockPath.empty()){
  unlockWriteAttempted=true;
  unlockWriteFailed=!PreferenceFile<0x454D4355,EditorEntry::validBool>(unlockPath).write(1);
 }
}
inline bool available(){if(sharedWorldAuthority)return worldOwnsRead && worldOwnsRead(31);if(!editorUnlocked&&EndlessRules::unlocked(progress))retainUnlock();return editorUnlocked;}
inline void refreshUnlock(){
 unlockWriteAttempted=false;unlockWriteFailed=false;
 editorUnlocked=PreferenceFile<0x454D4355,EditorEntry::validBool>(unlockPath).read()==1;
 if(sharedWorldAuthority){unlockWriteAttempted=editorUnlocked;return;}
 if(editorUnlocked){unlockWriteAttempted=true;return;}
 profiles.refresh();
 for(unsigned i=0;i<Profiles::Count;++i)if(profiles.entry(i).found&&EndlessRules::unlocked(profiles.snapshot(i))){retainUnlock();break;}
}
inline void close(){if(archive)std::fclose(archive);archive=nullptr;cachedFrame=-1;artFailed=false;}
inline void init(const char*executable,bool editorReturning=false,bool sharedWorld=false){sharedWorldAuthority=sharedWorld;close();mainPath=executable?executable:"EBOOT.PBP";modulePath=ReleasePaths::moduleCompanion(executable,ModuleRelative);artPath=assetPath(executable,"editor-entry.bin");launchPending=requestError=false;launchProfile=0;unlockPath=ReleasePaths::state(executable,"editor-mode.cfg");peakReturnPath=ReleasePaths::state(executable,"campaign-peak-return.tmp");campaignOptions.peakHunter=consumePeakReturn(peakReturnPath,editorReturning);refreshUnlock();}
inline void restoreProfile(int argc,char**argv,SaveFile&save){
 profiles.refresh();const unsigned slot=returnProfile(argc,argv,Profiles::Count);
 if(profiles.entry(slot).found){save=profiles.save(slot);activeProfile=slot;progress=profiles.snapshot(slot);chillMode=profiles.entry(slot).chill;campaignOptions.modifiers=profiles.entry(slot).modifiers;}
}
inline bool openArtwork(){
 if(archive)return true;
 if(artFailed)return false;
 archive=std::fopen(artPath.c_str(),"rb");bool good=archive!=nullptr;uint32_t header[4]{};
 if(good)good=std::fread(header,sizeof header,1,archive)==1&&header[0]==0x45454355&&header[1]==1&&header[2]==EditorEntry::Count&&header[3]==EditorEntry::RawBytes;
 for(unsigned i=0;good&&i<EditorEntry::Count;++i){EditorEntry::Frame record{};good=std::fread(&record,sizeof record,1,archive)==1&&std::memcmp(&record,&EditorEntry::Frames[i],sizeof record)==0&&record.w<=256&&record.h<=256;}
 if(good){good=std::fseek(archive,0,SEEK_END)==0;const long bytes=good?std::ftell(archive):-1;const auto&last=EditorEntry::Frames[EditorEntry::Count-1];good=bytes>=0&&uint64_t(bytes)==uint64_t(last.offset)+last.bytes;}
 if(!good){close();artFailed=true;}
 return good;
}
inline bool prepare(unsigned choice,bool pressed){
 if(!available())return false;
 const unsigned index=EditorEntry::index(russian(),choice,pressed?2:1);
 if(cachedFrame==int(index))return true;
 if(!openArtwork())return false;
 static_assert(VramStart+EditorEntry::RawBytes<=2*1024*1024);
 static_assert(sizeof fallbackPage>=WorkspaceBytes);
 sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
 titleArt=nullptr;titleReusedCourseVram=true;fallbackPageIndex=ScreenPages;
 auto*target=reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(sceGeEdramGetAddr())+VramStart)|0x40000000u);
 cachedFrame=-1;
 if(!inflateFrame(archive,EditorEntry::Frames[index],target,EditorEntry::RawBytes,fallbackPage,sizeof fallbackPage)){close();artFailed=true;return false;}
 cachedFrame=int(index);return true;
}
inline void draw(unsigned choice){
 if(cachedFrame>=0){
  const auto&frame=EditorEntry::Frames[unsigned(cachedFrame)];sceGuTexFlush();
  setTexture(GU_PSM_8888,256,256,static_cast<uint8_t*>(sceGeEdramGetAddr())+VramStart);
  patch({0,0,int16_t(frame.w),int16_t(frame.h),0,0},ViewX+frame.x,frame.y);
 }else{
  rectangle(ViewX+165,91,215,153,GU_RGBA(24,15,40,255));
  for(unsigned row=0;row<3;++row){rectangle(ViewX+170,98+49*row,205,43,GU_RGBA(row==choice?92:45,57,119,255));localeText(row==0?lang("Main Game","ÑIÑsÑÇÑpÑÑÑé"):row==1?lang("Endless Shuffle","ÑAÑuÑÉÑ{ÑÄÑ~ÑuÑâÑ~ÑçÑz ÑÇÑpÑxÑqÑÇÑÄÑÉ"):lang("Custom Levels","ÑQÑuÑtÑpÑ{ÑÑÑÄÑÇ ÑÖÑÇÑÄÑrÑ~ÑuÑz"),272,111+49*row,.76f,row==choice?Gold:White,true,196,true);}
 }
 if(choice!=2)requestError=false;
 if(requestError)localeText(lang("Could not open editor module","ÑNÑu ÑÖÑtÑpÑ|ÑÄÑÉÑé ÑÄÑÑÑ{ÑÇÑçÑÑÑé ÑÇÑuÑtÑpÑ{ÑÑÑÄÑÇ"),272,252,.58f,Gold,true,230,true);
}
inline bool request(unsigned profile,const char*log){
 if(!available()||profile>=Profiles::Count)return false;
 auto*file=std::fopen(modulePath.c_str(),"rb");const bool good=validModule(file);
 if(file)std::fclose(file);
 requestError=!good;
 if(good){launchPending=true;launchProfile=profile;}
 if(log)if(auto*out=std::fopen(log,"a")){std::fprintf(out,"build=053 event=editor_entry_%s profile=%u\n",good?"requested":"missing",profile);std::fclose(out);}
 return good;
}
inline int launch(const char*log){
 if(!launchPending)return -1;
 auto args=arguments(mainPath,launchProfile);SceKernelLoadExecParam parameters{};parameters.size=sizeof parameters;parameters.args=args.size();parameters.argp=args.data();parameters.key=nullptr;
 if(!writePeakReturn(peakReturnPath,campaignOptions.peakHunter)){
  if(log)if(auto*out=std::fopen(log,"a")){std::fprintf(out,"build=053 event=editor_peak_handoff_failed\n");std::fclose(out);}
  return -2;
 }
 if(log)if(auto*out=std::fopen(log,"a")){std::fprintf(out,"build=065 event=editor_loadexec_begin request=%s target=%s profile=%u\n",modulePath.c_str(),ReleaseLoadExec::cfwTarget(modulePath.c_str()),launchProfile);std::fclose(out);}
 const int result=ReleaseLoadExec::execute(modulePath.c_str(),&parameters);
 clearPeakReturn(peakReturnPath);
 if(log)if(auto*out=std::fopen(log,"a")){std::fprintf(out,"build=053 event=editor_loadexec_failed code=0x%08x\n",unsigned(result));std::fclose(out);}
 return result;
}
inline void launchFailure(int code){
 initializeGraphics();bool released=false;
 while(running.load(std::memory_order_relaxed)){
  beginFrame();rectangle(0,0,Width,Height,GU_RGBA(24,15,40,255));
  localeText(lang("Could not open the level editor","ÑNÑu ÑÖÑtÑpÑ|ÑÄÑÉÑé ÑÄÑÑÑ{ÑÇÑçÑÑÑé ÑÇÑuÑtÑpÑ{ÑÑÑÄÑÇ ÑÖÑÇÑÄÑrÑ~ÑuÑz"),204,91,.88f,Gold,true,430,true);
  char value[40];std::snprintf(value,sizeof value,"0x%08X",unsigned(code));localeText(value,204,133,.8f,White,true);
  localeText(lang("Circle: exit","ÑKÑÇÑÖÑs: ÑrÑçÑáÑÄÑt"),204,185,.8f,White,true);
  endFrame();sceDisplayWaitVblankStart();sceGuSwapBuffers();
  SceCtrlData pad{};sceCtrlPeekBufferPositive(&pad,1);if(released&&(pad.Buttons&(PSP_CTRL_CIRCLE|PSP_CTRL_TRIANGLE)))break;if(!pad.Buttons)released=true;
 }
 sceGuTerm();
}
}
#endif
