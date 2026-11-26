#pragma once
// CFW's public user syscall, without changing privilege or patching memory.
#include <cstdint>
#include <cstring>
#ifndef RELEASE_LOADEXEC_HOST_TEST
#include <psploadexec.h>
#include <psploadexec_kernel.h> // SceKernelLoadExecVSHParam declaration only.
extern "C" int sctrlKernelLoadExecVSHWithApitype(int,const char*,SceKernelLoadExecVSHParam*);
#endif
namespace ReleaseLoadExec {
constexpr int Permission=static_cast<int>(0x80020149u);
using Native=int(*)(const char*,SceKernelLoadExecParam*);
using Cfw=int(*)(int,const char*,SceKernelLoadExecVSHParam*);
inline int api(const char* file) {
 if(!file)return 0;
 if(std::strncmp(file,"ms0:/",5)==0)return 0x141;
 if(std::strncmp(file,"ef0:/",5)==0)return 0x152;
 if(std::strncmp(file,"disc0:/",7)==0)return 0x120;
 return 0;
}
inline bool linkedSyscall(uint32_t first,uint32_t second) {
 // Native loadcore links user imports as JR_RA; SYSCALL. Weak unresolved
 // imports contain SYSCALL_NOT_LINKED (0x54c); NOP, which must not be called.
 return first==0x03e00008u && (second&0xfc00003fu)==0x0000000cu && second!=0x0000054cu;
}
inline const char* cfwTarget(const char* file) {
 // Disc loadexec consumes the raw PRX; keep the PBP for request validation
 // only. Both native and CFW calls use the raw image. argv stays untouched.
 return file&&std::strcmp(file,"disc0:/PSP_GAME/SYSDIR/EDITOR.PBP")==0?
        "disc0:/PSP_GAME/SYSDIR/EDITOR.BIN":file;
}
inline int executeWith(const char*file,SceKernelLoadExecParam*parameters,
                      Native native,Cfw cfw,bool available) {
 const int result=native(cfwTarget(file),parameters);
 if(result!=Permission||!available||!cfw)return result;
 const int apitype=api(file);
 if(!apitype)return result;
 SceKernelLoadExecVSHParam vsh{};
 vsh.size=sizeof vsh;
 if(parameters){vsh.args=parameters->args;vsh.argp=parameters->argp;}
 vsh.key=parameters&&parameters->key?parameters->key:"game";
 return cfw(apitype,cfwTarget(file),&vsh);
}
#ifndef RELEASE_LOADEXEC_HOST_TEST
inline int execute(const char*file,SceKernelLoadExecParam*parameters) {
 const auto*stub=reinterpret_cast<const volatile uint32_t*>(
  reinterpret_cast<uintptr_t>(&sctrlKernelLoadExecVSHWithApitype));
 const bool available=linkedSyscall(stub[0],stub[1]);
 return executeWith(file,parameters,sceKernelLoadExec,sctrlKernelLoadExecVSHWithApitype,available);
}
#endif
}
