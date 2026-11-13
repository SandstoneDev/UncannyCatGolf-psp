#pragma once

#include "asset_file.hpp"

#include <pspkernel.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <new>
#include <zlib.h>

inline constexpr unsigned ResultLargeBytes=320*1024,ResultClearBytes=64*1024,ResultConfettiBytes=288*1024;
inline constexpr unsigned ResultVramBytes=ResultLargeBytes+ResultClearBytes+ResultConfettiBytes;

class LiveResultArt {
public:
    static constexpr unsigned MaxSprites=160, MaxTilesPerSprite=4;

    struct Sprite {
        int16_t x,y;
        uint16_t width,height,first,count;
    };

    struct Tile {
        uint16_t x,y,width,height,pitch,rows,format,reference;
        uint32_t offset,compressed,bytes;
    };

    struct CachedTile {
        Tile tile{};
        const void* pixels=nullptr;
    };

    struct Slot {
        const LiveResultArt* owner=nullptr;
        unsigned id=0;
        Sprite sprite{};
        std::array<CachedTile,MaxTilesPerSprite> images{};
    };

    bool open(const std::string& filename,unsigned expectedSprites,unsigned scratchBytes) {
        const auto file=openAssetFile(filename.c_str());
        if (!file || !expectedSprites || expectedSprites>MaxSprites ||
            std::fseek(file.get(),0,SEEK_END)!=0) return false;
        const long fileBytes=std::ftell(file.get());
        uint32_t header[3];
        std::array<Sprite,MaxSprites> sprites;
        if (fileBytes<0 || std::fseek(file.get(),0,SEEK_SET)!=0 ||
            std::fread(header,sizeof header,1,file.get())!=1 || (header[0]!=0x32524355 && header[0]!=0x33524355) ||
            header[1]!=expectedSprites || !header[2] || header[2]>expectedSprites*MaxTilesPerSprite ||
            std::fread(sprites.data(),sizeof(Sprite),expectedSprites,file.get())!=expectedSprites) return false;
        const bool delta=header[0]==0x33524355;
        unsigned total=0;
        const unsigned firstTile=12+expectedSprites*sizeof(Sprite);
        uint32_t end=firstTile+header[2]*sizeof(Tile);
        if (end>static_cast<uint32_t>(fileBytes)) return false;
        for (unsigned id=0;id<expectedSprites;++id) {
            const auto& sprite=sprites[id];
            if (!sprite.width || !sprite.height || sprite.width>768 || sprite.height>512 ||
                sprite.first!=total || sprite.count>MaxTilesPerSprite || sprite.count>header[2]-total) return false;
            for (unsigned index=0;index<sprite.count;++index) {
                Tile tile;
                if (std::fread(&tile,sizeof tile,1,file.get())!=1 || !valid(tile,sprite) || tile.offset!=end ||
                    (tile.reference && (!delta || tile.reference>total+index)) ||
                    !tile.compressed || tile.compressed>scratchBytes || tile.compressed>fileBytes-end) return false;
                end+=tile.compressed;
            }
            total+=sprite.count;
        }
        if (total!=header[2] || end!=static_cast<uint32_t>(fileBytes)) return false;
        path=filename;
        spriteCount=expectedSprites;
        tileOffset=firstTile;
        tileCount=header[2];
        bankBytes=static_cast<unsigned>(fileBytes);
        stream.reset();
        return true;
    }

    bool preloadFrames(unsigned first,unsigned count,uint8_t* storage,unsigned capacity) {
        dropPreloadedFrames();
        if (first>=spriteCount || count>spriteCount-first || count>59 || capacity<sizeof(FrameCache)) return false;
        auto& cache=*new(storage) FrameCache{};
        storage+=sizeof cache;
        capacity-=sizeof cache;
        const auto input=openAssetFile(path.c_str());
        auto* file=input.get();
        if (!file || std::fseek(file,12+first*sizeof(Sprite),SEEK_SET)!=0 ||
            std::fread(cache.sprites.data(),sizeof(Sprite),count,file)!=count) return false;
        const unsigned tileStart=cache.sprites[0].first;
        const auto& last=cache.sprites[count-1];
        const unsigned tileEnd=last.first+last.count;
        if (tileStart>=tileEnd || tileEnd>tileCount || tileEnd-tileStart>cache.tiles.size() ||
            std::fseek(file,tileOffset+tileStart*sizeof(Tile),SEEK_SET)!=0 ||
            std::fread(cache.tiles.data(),sizeof(Tile),tileEnd-tileStart,file)!=tileEnd-tileStart) return false;
        const unsigned offset=cache.tiles[0].offset;
        if (offset>bankBytes || offset<tileOffset+tileCount*sizeof(Tile)) return false;
        const unsigned limit=std::min(capacity,bankBytes-offset);
        unsigned bytes=0;
        for (unsigned i=0;i<count;++i) {
            const auto& sprite=cache.sprites[i];
            if (sprite.first<tileStart || sprite.first>tileEnd || sprite.count>tileEnd-sprite.first ||
                sprite.count>MaxTilesPerSprite) return false;
            for (unsigned j=0;j<sprite.count;++j) {
                const auto& tile=cache.tiles[sprite.first-tileStart+j];
                if (!valid(tile,sprite) || tile.reference || tile.offset!=offset+bytes ||
                    !tile.compressed || tile.compressed>limit-bytes) return false;
                bytes+=tile.compressed;
            }
        }
        if (std::fseek(file,offset,SEEK_SET)!=0 || std::fread(storage,1,bytes,file)!=bytes) return false;
        cachedFirst=first;
        cachedCount=count;
        cachedTileStart=tileStart;
        cachedOffset=offset;
        cachedData=storage;
        cachedMetadata=&cache;
        return true;
    }

    void dropPreloadedFrames() { cachedCount=0; }
    bool hasPreloadedFrames() const { return cachedCount!=0; }

    bool prepare(unsigned id,Slot& slot,uint8_t* vram,unsigned capacity,uint8_t* scratch,unsigned scratchBytes,bool cached=false) {
        if (id>=spriteCount || path.empty()) return false;
        if (slot.owner==this && slot.id==id && (!slot.sprite.count || slot.images[0].pixels==vram)) return true;
        slot.owner=nullptr;
        const bool inMemory=id>=cachedFirst && id-cachedFirst<cachedCount;
        Sprite sprite;
        std::array<Tile,MaxTilesPerSprite> tiles;
        std::FILE* file=nullptr;
        if (inMemory) {
            sprite=cachedMetadata->sprites[id-cachedFirst];
            std::copy_n(cachedMetadata->tiles.data()+sprite.first-cachedTileStart,sprite.count,tiles.data());
        } else {
            if (!stream) stream=openAssetFile(path.c_str());
            file=stream.get();
            if (!file || std::fseek(file,12+id*sizeof(Sprite),SEEK_SET)!=0 ||
                std::fread(&sprite,sizeof sprite,1,file)!=1 || sprite.count>MaxTilesPerSprite ||
                std::fseek(file,tileOffset+sprite.first*sizeof(Tile),SEEK_SET)!=0 ||
                std::fread(tiles.data(),sizeof(Tile),sprite.count,file)!=sprite.count) return false;
        }
        const bool prefetched=!inMemory && readSpritePayload(file,sprite,tiles,scratch,scratchBytes);
        unsigned used=0;
        for (unsigned i=0;i<sprite.count;++i) {
            const auto& tile=tiles[i];
            used=(used+63)&~63u;
            if (!valid(tile,sprite) || used>capacity || tile.bytes>capacity-used) return false;
            auto* pixels=vram+used;
            auto* target=cached?pixels:reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(pixels)|0x40000000u);
            if (inMemory || prefetched) {
                unsigned long size=tile.bytes;
                const auto* source=inMemory?cachedData+(tile.offset-cachedOffset):scratch+(tile.offset-tiles[0].offset);
                if (uncompress(target,&size,source,tile.compressed)!=Z_OK || size!=tile.bytes) return false;
            } else if (!decodeTile(file,tile,sprite.first+i,target,scratch,scratchBytes)) return false;
            if (cached) sceKernelDcacheWritebackRange(pixels,tile.bytes);
            slot.images[i]={tile,pixels};
            used+=tile.bytes;
        }
        slot.id=id;
        slot.sprite=sprite;
        slot.owner=this;
        return true;
    }

    void closeStream() { stream.reset(); }

private:
    static_assert(sizeof(Sprite)==12 && sizeof(Tile)==28);
    unsigned spriteCount=0,tileOffset=0,tileCount=0,bankBytes=0;
    std::string path;
    AssetFile stream;
    struct FrameCache {
        std::array<Sprite,59> sprites;
        std::array<Tile,59*MaxTilesPerSprite> tiles;
    };
    const FrameCache* cachedMetadata=nullptr;
    const uint8_t* cachedData=nullptr;
    unsigned cachedFirst=0,cachedCount=0,cachedTileStart=0,cachedOffset=0;

    bool readSpritePayload(std::FILE* file,const Sprite& sprite,
                           const std::array<Tile,MaxTilesPerSprite>& tiles,
                           uint8_t* scratch,unsigned capacity) const {
        if (sprite.count<2) return false;
        const unsigned start=tiles[0].offset;
        if (start<tileOffset+tileCount*sizeof(Tile) || start>bankBytes) return false;
        const unsigned limit=std::min(capacity,bankBytes-start);
        unsigned length=0;
        for (unsigned i=0;i<sprite.count;++i) {
            const auto& tile=tiles[i];
            if (tile.reference || !valid(tile,sprite) || tile.offset!=start+length ||
                !tile.compressed || tile.compressed>limit-length) return false;
            length+=tile.compressed;
        }
        return std::fseek(file,start,SEEK_SET)==0 && std::fread(scratch,1,length,file)==length;
    }

    bool readPayload(std::FILE* file,const Tile& tile,uint8_t* scratch,unsigned scratchBytes) const {
        return tile.offset>=tileOffset+tileCount*sizeof(Tile) && tile.offset<=bankBytes &&
               tile.compressed && tile.compressed<=scratchBytes && tile.compressed<=bankBytes-tile.offset &&
               std::fseek(file,tile.offset,SEEK_SET)==0 && std::fread(scratch,tile.compressed,1,file)==1;
    }

    bool decodeTile(std::FILE* file,const Tile& tile,unsigned index,uint8_t* output,
                    uint8_t* scratch,unsigned scratchBytes) const {
        if (tile.reference) {
            Tile reference;
            if (tile.reference>index ||
                std::fseek(file,tileOffset+(tile.reference-1)*sizeof(Tile),SEEK_SET)!=0 ||
                std::fread(&reference,sizeof reference,1,file)!=1 || reference.reference ||
                reference.bytes!=tile.bytes || reference.pitch!=tile.pitch || reference.rows!=tile.rows ||
                reference.format!=tile.format || !readPayload(file,reference,scratch,scratchBytes)) return false;
            unsigned long bytes=reference.bytes;
            if (uncompress(output,&bytes,scratch,reference.compressed)!=Z_OK || bytes!=reference.bytes)
                return false;
            if (!readPayload(file,tile,scratch,scratchBytes)) return false;
            if (tile.bytes<=scratchBytes-tile.compressed) {
                auto* difference=scratch+tile.compressed;
                unsigned long bytes=tile.bytes,packedBytes=tile.compressed;
                if (uncompress2(difference,&bytes,scratch,&packedBytes)!=Z_OK ||
                    bytes!=tile.bytes || packedBytes!=tile.compressed) return false;
                for (unsigned i=0;i<tile.bytes;++i) output[i]+=difference[i];
                return true;
            }
            return addDelta(output,tile.bytes,scratch,tile.compressed);
        }
        if (!readPayload(file,tile,scratch,scratchBytes)) return false;
        unsigned long bytes=tile.bytes;
        return uncompress(output,&bytes,scratch,tile.compressed)==Z_OK && bytes==tile.bytes;
    }

    static bool addDelta(uint8_t* output,unsigned size,uint8_t* packed,unsigned bytes) {
        z_stream stream{};
        if (inflateInit(&stream)!=Z_OK) return false;
        stream.next_in=packed;
        stream.avail_in=bytes;
        uint8_t buffer[2048];
        unsigned used=0;
        int status=Z_OK;
        while (status==Z_OK) {
            stream.next_out=buffer;
            stream.avail_out=sizeof buffer;
            status=inflate(&stream,Z_NO_FLUSH);
            const unsigned count=sizeof buffer-stream.avail_out;
            if (count>size-used || (!count && status==Z_OK)) break;
            for (unsigned i=0;i<count;++i) output[used+i]+=buffer[i];
            used+=count;
        }
        const bool ok=status==Z_STREAM_END && used==size && !stream.avail_in;
        inflateEnd(&stream);
        return ok;
    }

    static bool valid(const Tile& tile,const Sprite& sprite) {
        if (tile.format>1 || !tile.width || !tile.height || tile.width>256 || tile.height>256 ||
            tile.x+tile.width>sprite.width || tile.y+tile.height>sprite.height) return false;
        const unsigned bpp=tile.format?2:4;
        return tile.pitch>=tile.width && tile.pitch<=256 && (tile.pitch*bpp)%16==0 &&
               tile.rows>=tile.height && tile.rows<=256 && tile.rows%8==0 &&
               tile.bytes==static_cast<uint32_t>(tile.pitch*tile.rows*bpp);
    }
};
