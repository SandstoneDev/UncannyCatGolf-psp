#pragma once

struct ResultAssets {
    alignas(64) uint16_t board[256*256]{};
    alignas(64) uint32_t bar[256*64]{};
    // Victory labels use the first font bank; localized labels use LocaleFont.
    static constexpr unsigned LettersWidth=256, LettersHeight=128;
    alignas(64) uint32_t letters[LettersWidth*LettersHeight]{};
    uint8_t advance[2][96]{};

    bool load(const char* path) {
        std::FILE* file=std::fopen(path,"rb");
        if (!file) return false;
        const bool ok=std::fread(board,sizeof board,1,file)==1 &&
                      std::fread(bar,sizeof bar,1,file)==1 &&
                      std::fread(letters,sizeof letters,1,file)==1 &&
                      std::fseek(file,sizeof letters,SEEK_CUR)==0 &&
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

    struct OpenFailure {
        const char* phase=nullptr;
        long wanted=0,got=0;
        int error=0,streamError=0,eof=0,clip=-1,frame=-1;
        uint32_t header[2]{};
        int64_t raw=0;
        const char* rawOp=nullptr;
        unsigned rawOpenCount=0;
        bool rawValid=false,virtualFile=false;
    } failure;
    const OpenFailure& openFailure() const {return failure;}

    bool open(const char* path) {
        failure={};
        if (file) std::fclose(file);
        file=nullptr;
        cached=MaxFrames;
        clearPrefetch();
        errno=0;
        auto incoming=openAssetFile(path);
        auto* source=incoming.get();
        uint32_t header[2]{};
        const auto reject=[&](const char* phase,long wanted=0,long got=0,int clip=-1,int frame=-1) {
            failure.phase=phase;failure.wanted=wanted;failure.got=got;failure.error=errno;
            failure.streamError=source?std::ferror(source):0;failure.eof=source?std::feof(source):0;
            failure.clip=clip;failure.frame=frame;std::copy_n(header,2,failure.header);
            const auto raw=LoadIoAudit::first();failure.raw=raw.raw;failure.rawOp=raw.op;failure.rawValid=raw.valid;
            failure.rawOpenCount=LoadIoAudit::snapshot().openCount;
            failure.virtualFile=source && ucg_storage::isVirtual(source);
            return false;
        };
        if (!source) return reject("open");
        errno=0;
        if (std::fseek(source,0,SEEK_END)!=0) return reject("seek_end");
        errno=0;
        const long size=std::ftell(source);
        if (size<0) return reject("tell_size",0,size);
        errno=0;
        if (std::fseek(source,0,SEEK_SET)!=0) return reject("seek_start");
        errno=0;
        const auto headerRead=std::fread(header,sizeof header,1,source);
        if (headerRead!=1) return reject("read_header",1,headerRead);
        if (header[0]!=0x36524355) return reject("magic",0x36524355,header[0]);
        if (!header[1] || header[1]>MaxFrames) return reject("frame_count",MaxFrames,header[1]);
        errno=0;
        const auto clipsRead=std::fread(clips,sizeof clips,1,source);
        if (clipsRead!=1) return reject("read_clips",1,clipsRead);
        errno=0;
        const auto framesRead=std::fread(frames,sizeof(Frame),header[1],source);
        if (framesRead!=header[1]) return reject("read_frames",header[1],framesRead);
        dataOffset=8+sizeof clips+header[1]*sizeof(Frame);
        if (size!=static_cast<long>(dataOffset+header[1]*sizeof pixels)) return reject("file_size",dataOffset+header[1]*sizeof pixels,size);
        unsigned expected=0;
        int clipId=0;
        for (auto& clip:clips) {
            if (clip.first!=expected || !clip.count || clip.count>header[1]-expected || clip.loop>1) return reject("clip",expected,clip.first,clipId);
            expected+=clip.count;++clipId;
        }
        if (expected!=header[1]) return reject("clip_total",header[1],expected);
        for (unsigned i=0;i<expected;++i) {
            const auto& f=frames[i];
            if (!f.duration || f.duration>5000 || f.width<=0 || f.width>128 ||
                f.height<=0 || f.height>128) return reject("frame",0,0,-1,i);
        }
        file=incoming.release();
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
