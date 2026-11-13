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

    const uint32_t* frame(unsigned rank,unsigned time,Frame& dimensions) {
        const auto& clip=clips[rank];
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
        const unsigned index=clip.first+i;
        dimensions=frames[index];
        if (index==cached) return pixels;
        cached=MaxFrames;
        if (std::fseek(file,dataOffset+index*sizeof pixels,SEEK_SET)!=0 ||
            std::fread(pixels,sizeof pixels,1,file)!=1) return nullptr;
        sceKernelDcacheWritebackRange(pixels,sizeof pixels);
        cached=index;
        return pixels;
    }

private:
    static constexpr unsigned MaxFrames=128;
    struct Clip { uint32_t first,count,loop; };
    std::FILE* file=nullptr;
    Clip clips[6]{};
    Frame frames[MaxFrames]{};
    unsigned dataOffset=0,cached=MaxFrames;
    alignas(64) uint32_t pixels[128*128]{};
};
