#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <zlib.h>
namespace CostumeScreen056 {
struct Bank {
    static constexpr unsigned Side=128,PageBytes=Side*Side*4,Count=37;
    alignas(64) uint32_t pixels[2][Side*Side];
    unsigned pair[2]={UINT32_MAX,UINT32_MAX},width[2]{},height[2]{};
    bool attempted=false,ready=false;
    bool load(const std::string& path,unsigned player,unsigned ghost,void* scratch,unsigned scratchBytes){
        if(attempted && pair[0]==player && pair[1]==ghost)return ready;
        pair[0]=player;pair[1]=ghost;attempted=true;ready=false;
        if(player>=Count || ghost>=Count)return false;
        const auto file=openAssetFile(path.c_str());
        if(!file || std::fseek(file.get(),0,SEEK_END)!=0)return false;
        const long length=std::ftell(file.get());uint32_t header[4]{};
        if(length<16+37*16 || std::fseek(file.get(),0,SEEK_SET)!=0 || std::fread(header,sizeof header,1,file.get())!=1 || header[0]!=0x50534355 || header[1]!=1 || header[2]!=Count || header[3]!=Side)return false;
        for(unsigned i=0;i<2;++i){
            uint32_t row[4]{};
            if(std::fseek(file.get(),16+pair[i]*16,SEEK_SET)!=0 || std::fread(row,sizeof row,1,file.get())!=1 || row[0]<16+37*16 || row[0]>static_cast<unsigned long>(length) || !row[1] || row[1]>scratchBytes || row[1]>static_cast<unsigned long>(length)-row[0] || row[2]>Side || row[3]>Side)return false;
            if(std::fseek(file.get(),row[0],SEEK_SET)!=0 || std::fread(scratch,row[1],1,file.get())!=1)return false;
            unsigned long written=PageBytes,consumed=row[1];
            if(uncompress2(reinterpret_cast<uint8_t*>(pixels[i]),&written,static_cast<uint8_t*>(scratch),&consumed)!=Z_OK || written!=PageBytes || consumed!=row[1])return false;
            width[i]=row[2];height[i]=row[3];
        }
        // load is called only before beginFrame; previous endFrame fenced GE.
        sceKernelDcacheWritebackRange(pixels,sizeof pixels);ready=true;return true;
    }
};
}
