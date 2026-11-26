#pragma once
#include "startup_intro_art_data.hpp"
#include <cstring>
#include <cstdio>
#include <zlib.h>
namespace StartupIntroArt {
inline unsigned pixelOffset(unsigned x,unsigned y) {
    return (y/8)*(Width*8*4)+(x/4)*128+(y%8)*16+(x%4)*4;
}
inline bool decode(void* output,unsigned capacity,void* scratch,unsigned scratchCapacity,
                   const unsigned char* compressed,unsigned compressedBytes) {
    if (!output || !scratch || !compressed || capacity<Bytes || scratchCapacity<ScratchBytes || compressedBytes!=CompressedBytes) return false;
    auto* atlas=static_cast<unsigned char*>(output);auto* raw=static_cast<unsigned char*>(scratch);
    std::memset(atlas,0,Bytes);
    for (const auto& image:Images) {
        if (image.offset+image.bytes>compressedBytes || image.x+image.width>Width || image.y+image.height>Height) return false;
        unsigned long bytes=image.width*image.height*4;
        if (bytes>scratchCapacity || uncompress(raw,&bytes,compressed+image.offset,image.bytes)!=Z_OK ||
            bytes!=image.width*image.height*4 || crc32(0,raw,bytes)!=image.crc) return false;
        for (unsigned y=0;y<image.height;++y) for (unsigned x=0;x<image.width;++x)
            std::memcpy(atlas+pixelOffset(image.x+x,image.y+y),raw+(y*image.width+x)*4,4);
    }
    return true;
}
// The caller lends gameplayPage: front is the largest decoded image, tail is
// the exact immutable compressed file. No heap/static staging buffer is needed.
inline bool load(const char* path,void* output,unsigned capacity,void* scratch,unsigned scratchCapacity) {
    if (!path || !output || !scratch || capacity<Bytes || scratchCapacity<LoadScratchBytes) return false;
    auto* compressed=static_cast<unsigned char*>(scratch)+ScratchBytes;
    auto* file=std::fopen(path,"rb");if (!file) return false;
    const bool read=std::fread(compressed,1,CompressedBytes,file)==CompressedBytes &&
        std::fgetc(file)==EOF && !std::ferror(file);
    const bool closed=std::fclose(file)==0;
    // Close even on short/extra/error; decompression owns no live file descriptor.
    if (!read || !closed) return false;
    return decode(output,capacity,scratch,ScratchBytes,compressed,CompressedBytes);
}
}
