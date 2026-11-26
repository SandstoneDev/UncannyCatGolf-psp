#pragma once

struct ResultAssets {
    alignas(64) uint16_t board[256*256]{};
    alignas(64) uint32_t bar[256*64]{};
    alignas(64) uint32_t letters[256*256]{};
    uint8_t advance[2][96]{};

    bool load(const char* path) {
        std::FILE* file=std::fopen(path,"rb");
        if (!file) return false;
        const bool ok=std::fread(board,sizeof board,1,file)==1 &&
                      std::fread(bar,sizeof bar,1,file)==1 &&
                      std::fread(letters,sizeof letters,1,file)==1 &&
                      std::fread(advance,sizeof advance,1,file)==1 &&
                      std::fgetc(file)==EOF && !std::ferror(file);
        std::fclose(file);
        if (!ok) return false;
        sceKernelDcacheWritebackRange(board,sizeof board);
        sceKernelDcacheWritebackRange(bar,sizeof bar);
        sceKernelDcacheWritebackRange(letters,sizeof letters);
        return true;
    }
};

class RankPlayer {
public:
    struct Frame { uint32_t duration; int16_t width,height; };
    ~RankPlayer() { if (file) std::fclose(file); }

    bool open(const char* path) {
        if (file) std::fclose(file);
        cached=MaxFrames;
        clearPrefetch();
        file=std::fopen(path,"rb");
        if (!file || std::fseek(file,0,SEEK_END)!=0) return false;
        const long size=std::ftell(file);
        uint32_t header[2]{};
        if (std::fseek(file,0,SEEK_SET)!=0 || std::fread(header,sizeof header,1,file)!=1 ||
            header[0]!=0x36524355 || !header[1] || header[1]>MaxFrames ||
            std::fread(clips,sizeof clips,1,file)!=1 ||
            std::fread(frames,sizeof(Frame),header[1],file)!=header[1]) return false;
        dataOffset=8+sizeof clips+header[1]*sizeof(Frame);
        if (size!=static_cast<long>(dataOffset+header[1]*sizeof pixels)) return false;
        unsigned expected=0;
        for (auto& clip:clips) {
            if (clip.first!=expected || !clip.count || clip.count>header[1]-expected || clip.loop>1) return false;
            expected+=clip.count;
        }
        if (expected!=header[1]) return false;
        for (unsigned i=0;i<expected;++i) {
            const auto& f=frames[i];
            if (!f.duration || f.duration>5000 || f.width<=0 || f.width>128 ||
                f.height<=0 || f.height>128) return false;
        }
        return true;
    }

    static constexpr unsigned PrefetchSlots=4;
    static constexpr unsigned PrefetchBytes=PrefetchSlots*128*128*sizeof(uint32_t);

    void prepare(unsigned rank,uint32_t* workspace) {
        clearPrefetch();
        hits=misses=0;
        preparedRank=rank;
        prefetched=workspace;
    }

    void fill(unsigned time) {
        if (!prefetched || preparedRank>=std::size(clips)) return;
        const auto& clip=clips[preparedRank];
        const unsigned first=frameIndex(clip,time)-clip.first;
        unsigned slots=0;
        for (unsigned ahead=0;ahead<std::min<unsigned>(PrefetchSlots,clip.count);++ahead) {
            unsigned index=first+ahead;
            if (index>=clip.count) {
                if (!clip.loop) break;
                index-=clip.count;
            }
            const unsigned slot=index%PrefetchSlots;
            if (slots&(1u<<slot)) break;
            slots|=1u<<slot;
            index+=clip.first;
            if (ready[slot]==index) continue;
            if (loading!=index) {
                ready[slot]=MaxFrames;
                loading=index;
                loadedBytes=0;
            }
            constexpr unsigned chunk=32*1024;
            auto* output=reinterpret_cast<uint8_t*>(prefetched+slot*(sizeof pixels/sizeof pixels[0]));
            if (!readFrame(index,output+loadedBytes,loadedBytes,chunk)) {
                clearPrefetch();
                return;
            }
            loadedBytes+=chunk;
            if (loadedBytes==sizeof pixels) {
                sceKernelDcacheWritebackRange(output,sizeof pixels);
                ready[slot]=index;
                loading=MaxFrames;
            }
            return;
        }
    }

    void clearPrefetch() {
        prefetched=nullptr;
        loading=MaxFrames;
        std::fill_n(ready,PrefetchSlots,MaxFrames);
    }

    unsigned prefetchedFrames() const { return hits; }
    unsigned streamedFrames() const { return misses; }

    const uint32_t* frame(unsigned rank,unsigned time,Frame& dimensions) {
        if (!file || rank>=std::size(clips)) return nullptr;
        const auto& clip=clips[rank];
        const unsigned index=frameIndex(clip,time);
        dimensions=frames[index];
        if (prefetched && rank==preparedRank) {
            const unsigned slot=(index-clip.first)%PrefetchSlots;
            if (ready[slot]==index) {
                ++hits;
                return prefetched+slot*(sizeof pixels/sizeof pixels[0]);
            }
        }
        if (index==cached) return pixels;
        ++misses;
        cached=MaxFrames;
        if (!readFrame(index,pixels,0,sizeof pixels)) return nullptr;
        sceKernelDcacheWritebackRange(pixels,sizeof pixels);
        cached=index;
        return pixels;
    }

private:
    struct Clip { uint32_t first,count,loop; };

    unsigned frameIndex(const Clip& clip,unsigned time) const {
        if (clip.loop) {
            unsigned duration=0;
            for (unsigned i=0;i<clip.count;++i) duration+=frames[clip.first+i].duration;
            time%=duration;
        }
        unsigned i=0;
        while (i+1<clip.count && time>=frames[clip.first+i].duration) {
            time-=frames[clip.first+i].duration;
            ++i;
        }
        return clip.first+i;
    }

    bool readFrame(unsigned index,void* output,unsigned offset,unsigned bytes) {
        return std::fseek(file,dataOffset+index*sizeof pixels+offset,SEEK_SET)==0 &&
            std::fread(output,1,bytes,file)==bytes;
    }

    static constexpr unsigned MaxFrames=128;
    std::FILE* file=nullptr;
    Clip clips[6]{};
    Frame frames[MaxFrames]{};
    unsigned dataOffset=0,cached=MaxFrames;
    alignas(64) uint32_t pixels[128*128]{};
    uint32_t* prefetched=nullptr;
    unsigned ready[PrefetchSlots]{};
    unsigned loading=MaxFrames,loadedBytes=0,preparedRank=0,hits=0,misses=0;
};
