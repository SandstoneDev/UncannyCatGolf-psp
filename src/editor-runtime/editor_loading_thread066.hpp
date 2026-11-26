#pragma once
#include "editor_loading066.hpp"
#include <atomic>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspge.h>
namespace EditorLoadingThread066 {
inline std::atomic<bool> stopping{false};
inline std::atomic<unsigned> presented{0},changes{0};
inline std::atomic<int> failure{0};
inline int thread=-1;
inline uint16_t*video=nullptr;
inline unsigned videoBytes=0;
inline int present(unsigned buffer){
 __sync_synchronize();
 return sceDisplaySetFrameBuf(static_cast<uint16_t*>(sceGeEdramGetAddr())+buffer*EditorLoading066::FramePixels,512,PSP_DISPLAY_PIXEL_FORMAT_565,PSP_DISPLAY_SETBUF_NEXTFRAME);
}
inline int entry(SceSize,void*){
 const uint64_t start=sceKernelGetSystemTimeWide();unsigned buffer=0,lastFrame=~0u;
 while(!stopping.load(std::memory_order_acquire)){
  const uint64_t elapsed=sceKernelGetSystemTimeWide()-start;
  const unsigned frame=unsigned((elapsed/25000)%10);
  if(!EditorLoading066::draw(video,videoBytes,frame,buffer)){failure.store(-1);break;}
  const int shown=present(buffer);if(shown<0){failure.store(shown);break;}
  sceDisplayWaitVblankStart();++presented;if(frame!=lastFrame){++changes;lastFrame=frame;}buffer^=1;
  const uint64_t now=sceKernelGetSystemTimeWide()-start,next=(elapsed/25000+1)*25000;
  if(next>now)sceKernelDelayThread(unsigned(next-now));
 }
 return 0;
}
struct Result{bool shown=false,spinning=false;};
inline Result start(uint16_t*uncachedVideo,unsigned bytes,void*scratch,unsigned scratchBytes){
 if(thread>=0)return {true,true};
 video=uncachedVideo;videoBytes=bytes;
 if(!EditorLoading066::paint(video,videoBytes,static_cast<uint16_t*>(scratch),scratchBytes)||present(1)<0)return {};
 sceDisplayWaitVblankStart();presented=changes=0;failure=0;stopping.store(false,std::memory_order_release);
 thread=sceKernelCreateThread("UCG loading",entry,24,16*1024,PSP_THREAD_ATTR_USER,nullptr);
 if(thread<0)return {true,false};
 if(sceKernelStartThread(thread,0,nullptr)<0){sceKernelDeleteThread(thread);thread=-1;return {true,false};}
 return {true,true};
}
inline bool stop(){
 if(thread>=0){stopping.store(true,std::memory_order_release);if(sceKernelWaitThreadEnd(thread,nullptr)<0)return false;sceKernelDeleteThread(thread);thread=-1;}
 // Restore the GU's original display1/draw0 pairing before the first GE frame.
 if(video){if(present(1)<0)return false;sceDisplayWaitVblankStart();}
 return failure.load()==0;
}
}
