#pragma once
#include "asset_file.hpp"
#include "visual.hpp"
#include <cstdint>
#include <algorithm>
#include <iterator>
#include <cstdio>
#include <string>
#include <utility>
#include <zlib.h>
#include <pspkernel.h>

class ProfileDeleteArt {
public:
    static constexpr unsigned Width=512,Height=128,Bytes=Width*Height*4,Parts=12;
    void open(std::string filename){path=std::move(filename);release();}
    void release(){pixels=nullptr;}
    bool ready() const{return pixels!=nullptr;}
    bool load(uint8_t* workspace,unsigned workspaceBytes) {
        if(ready())return true;
        if(!workspace || workspaceBytes<Bytes)return false;
        const auto file=openAssetFile(path.c_str());
        uint32_t header[6]{};
        Patch next[Parts]{};
        if(!file || std::fread(header,sizeof header,1,file.get())!=1 || header[0]!=0x31445055 ||
           header[1]!=Width || header[2]!=Height || header[3]!=Parts || !header[4] ||
           header[4]>workspaceBytes-Bytes || std::fread(next,sizeof next,1,file.get())!=1)return false;
        for(const auto& p:next)if(p.u<0 || p.v<0 || p.width<=0 || p.height<=0 || p.u+p.width>int(Width) ||
            p.v+p.height>int(Height) || p.x<136 || p.y<0 || p.x+p.width>408 || p.y+p.height>272)return false;
        auto* encoded=workspace+Bytes;
        if(std::fread(encoded,header[4],1,file.get())!=1 || std::fgetc(file.get())!=EOF || std::ferror(file.get()))return false;
        uLongf bytes=Bytes;
        if(uncompress(workspace,&bytes,encoded,header[4])!=Z_OK || bytes!=Bytes || crc32(0,workspace,Bytes)!=header[5])return false;
        std::copy(std::begin(next),std::end(next),std::begin(parts));
        pixels=workspace;sceKernelDcacheWritebackRange(pixels,Bytes);return true;
    }
    const uint8_t* texture() const{return pixels;}
    const Patch& part(unsigned index) const{return parts[index];}
    unsigned prompt(bool russian) const{return russian?7:2;}
    unsigned yes(bool russian,bool focus) const{return (russian?8:3)+unsigned(focus);}
    unsigned no(bool russian,bool focus) const{return (russian?10:5)+unsigned(focus);}
private:
    std::string path;
    Patch parts[Parts]{};
    uint8_t* pixels=nullptr;
};
