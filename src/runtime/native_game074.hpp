#pragma once
#include "native_state_router074.hpp"
#include "native_save_controller074.hpp"
#include "home_exit067.hpp"
#include <psputility.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <pspiofilemgr.h>
#include <pspctrl.h>
#include <pspdebug.h>
namespace NativeGame074 {
inline NativeRoute074::Router router;
inline NativeSave074::Controller controller;
inline uint64_t durableRevision=0;
inline bool nativeExists=false, ready=false, active=false, failed=false, russian=false;
inline char logPath[1024]{};
inline int serializationError=0;
inline void event(const char*name,int error=0){
 if(!*logPath)return;
 const auto trace=controller.diagnostic();
 if(auto*f=std::fopen(logPath,"a")){std::fprintf(f,"build=075 event=native_%s error=%d generation=%llu revision=%llu durable=%llu entries=%u bytes=%u pre_status=%08x init_result=%08x last_status=%d mode=%d multi=%d\n",name,error,(unsigned long long)controller.generation(),(unsigned long long)router.archive().revision(),(unsigned long long)durableRevision,unsigned(router.archive().size()),unsigned(router.archive().encodedSize()),unsigned(trace.beforeStatus),unsigned(trace.initResult),trace.lastStatus,trace.mode,trace.multi);std::fclose(f);}
}
inline bool dirty(){return ready&&(!nativeExists||router.archive().revision()!=durableRevision);}
inline bool flush(){
 if(!ready||active||router.archive().writableHandles()){errno=EBUSY;return false;}
 if(!dirty())return true;
 serializationError=0;active=true;const auto revision=router.archive().revision();
 const bool ok=controller.save(router.archive());active=false;failed=!ok;
 if(ok){nativeExists=true;durableRevision=revision;event("saved");}else event("save_error",controller.error());
 return ok;
}
// Firmware error UI is used only after a failed operation has relinquished the
// utility. A poisoned utility keeps its buffers pinned until process exit.
inline bool retryDialog(bool loading){
 if(controller.poisoned()){
  event("utility_poisoned",controller.error());
  // Never start a second utility against uncertain firmware ownership.
  pspDebugScreenInit();
  pspDebugScreenPrintf("PSP native save error: %08X\nProgress was not confirmed saved.\nTRIANGLE: quit.\n",unsigned(controller.error()));
  for(;;){controller.drain();SceCtrlData pad{};sceCtrlPeekBufferPositive(&pad,1);if(pad.Buttons&PSP_CTRL_TRIANGLE)break;sceDisplayWaitVblankStart();}
  sceKernelExitGame();return false;
 }
 static pspUtilityMsgDialogParams dialog;
 static unsigned __attribute__((aligned(16))) list[4096];
 std::memset(&dialog,0,sizeof dialog);dialog.base.size=sizeof dialog;
 sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&dialog.base.language);
 sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,&dialog.base.buttonSwap);
 dialog.base.graphicsThread=17;dialog.base.accessThread=19;dialog.base.fontThread=18;dialog.base.soundThread=16;
 dialog.mode=PSP_UTILITY_MSGDIALOG_MODE_TEXT;
 dialog.options=PSP_UTILITY_MSGDIALOG_OPTION_TEXT|PSP_UTILITY_MSGDIALOG_OPTION_YESNO_BUTTONS;
 const char*message=loading?(russian?"Не удалось загрузить сохранение PSP.\nДанные не изменены.\nПовторить? Нет: выйти.":"Cannot load PSP save data.\nYour data has not been replaced.\nRetry? No: quit."):(russian?"Не удалось записать сохранение PSP.\nПовторить? Нет: выйти без сохранения.":"Cannot save progress to PSP save data.\nRetry? No: quit without saving.");
 std::snprintf(dialog.message,sizeof dialog.message,"%s\nError: %08X",message,unsigned(serializationError?serializationError:controller.error()));
 if(sceUtilityMsgDialogInitStart(&dialog)!=0){sceKernelExitGame();return false;}
 bool shutdown=false;unsigned progressAt=sceKernelGetSystemTimeLow();
 for(;;){
  sceGuStart(GU_DIRECT,list);sceGuScissor(0,0,480,272);sceGuClearColor(0xff000000);sceGuClear(GU_COLOR_BUFFER_BIT);sceGuFinish();sceGuSync(0,0);
  const int status=sceUtilityMsgDialogGetStatus();
  if(status==PSP_UTILITY_DIALOG_VISIBLE){sceUtilityMsgDialogUpdate(1);progressAt=sceKernelGetSystemTimeLow();}
  else if(status==PSP_UTILITY_DIALOG_QUIT&&!shutdown){shutdown=true;progressAt=sceKernelGetSystemTimeLow();sceUtilityMsgDialogShutdownStart();}
  else if(status==PSP_UTILITY_DIALOG_NONE)break;
  if(status<PSP_UTILITY_DIALOG_NONE||status>PSP_UTILITY_DIALOG_FINISHED||
     (status!=PSP_UTILITY_DIALOG_VISIBLE&&unsigned(sceKernelGetSystemTimeLow()-progressAt)>30000000u)){
    event("message_utility_error",status);sceKernelExitGame();return false;
  }
  sceDisplayWaitVblankStart();sceGuSwapBuffers();
 }
 return dialog.base.result==0&&dialog.buttonPressed==PSP_UTILITY_MSGDIALOG_RESULT_YES;
}
inline void serializationFailure(){
 serializationError=-(errno?errno:EIO);event("serialization_error",serializationError);
 if(!retryDialog(false)){event("quit_without_serialization",serializationError);sceKernelExitGame();}
}
inline bool importLegacy(const char*root){
 const int dir=sceIoDopen(root);if(dir<0){event("import_directory_error",dir);return false;}
 NativeState072::Archive imported;
 SceIoDirent entry{};int scan=0;bool ok=true;
 while((scan=sceIoDread(dir,&entry))>0){
  char logical[81]{};const size_t nameSize=std::strlen(entry.d_name);
  if(nameSize<sizeof logical){for(size_t i=0;i<nameSize;++i){const unsigned char c=entry.d_name[i];logical[i]=char(c>='A'&&c<='Z'?c+('a'-'A'):c);}}
  if(*logical&&NativeRoute074::Router::acceptsBasename(logical)){
   if(!FIO_S_ISREG(entry.d_stat.st_mode)||entry.d_stat.st_size<0||entry.d_stat.st_size>NativeState072::MaxValueBytes){ok=false;break;}
   char path[1200];const int n=std::snprintf(path,sizeof path,"%s%s%s",root,*root&&root[std::strlen(root)-1]=='/'?"":"/",entry.d_name);
   if(n<0||size_t(n)>=sizeof path){ok=false;break;}
   NativeState072::Bytes bytes;
   if(!bytes.resize(size_t(entry.d_stat.st_size))){ok=false;break;}
   // Explicit real fopen is the sole one-time legacy read bypass.
   extern std::FILE* realLegacyOpen(const char*,const char*);
   std::FILE*f=realLegacyOpen(path,"rb");if(!f){ok=false;break;}
   const bool read=std::fread(bytes.data(),1,bytes.size(),f)==bytes.size()&&std::fgetc(f)==EOF&&!std::ferror(f);
   const bool closed=std::fclose(f)==0;
   if(!read||!closed||imported.contains(logical)||!imported.put(logical,bytes)){ok=false;break;}
  }
  std::memset(&entry,0,sizeof entry);
 }
 const int closed=sceIoDclose(dir);
 if(!ok||scan<0||closed!=0){event("import_error",scan<0?scan:-errno);return false;}
 router.archive()=std::move(imported);event("legacy_imported");return true;
}
inline bool boot(const char*root,const char*log){
 std::snprintf(logPath,sizeof logPath,"%s",log?log:"");
 ready=false;failed=false;HomeExit067::defer();
 if(!router.configure(root))return false;
 controller.configure(nullptr,0,nullptr,nullptr);
 const auto loaded=controller.load(router.archive());
 if(loaded==NativeSave074::LoadResult::Loaded){nativeExists=true;durableRevision=router.archive().revision();}
 else if(loaded==NativeSave074::LoadResult::Missing){nativeExists=false;if(!importLegacy(root))return false;}
 else {event("load_error",controller.error());return false;}
 if(!router.activate()){event("invalid_archive",errno);return false;}
 ready=true;event(nativeExists?"loaded":"migration_ready");return true;
}
inline void requireFlush(){
 while(!flush()){
  if(!retryDialog(false)){event("quit_without_save",controller.error());sceKernelExitGame();return;}
  // Controller retry accepts only its exact known or attempted native bytes;
  // a different card never becomes an overwrite baseline here.
 }
}
}
extern "C" std::FILE* __real_fopen(const char*,const char*);
namespace NativeGame074 {inline std::FILE*realLegacyOpen(const char*p,const char*m){return __real_fopen(p,m);}}
