template<unsigned Width,unsigned Height,unsigned Count>
class RideAtlas033 {
public:
    static constexpr unsigned TextureWidth=Width, TextureHeight=Height;

    bool load(const std::string& path) {
        if (pixels) return true;
        const auto file=openAssetFile(path.c_str());
        if (!file || !readHeader(file.get())) return false;
        pixels.reset(static_cast<uint8_t*>(memalign(64,TextureBytes)));
        if (!pixels || std::fread(pixels.get(),TextureBytes,1,file.get())!=1 ||
            std::fgetc(file.get())!=EOF || std::ferror(file.get())) {
            unload();
            return false;
        }
        sceKernelDcacheWritebackRange(pixels.get(),TextureBytes);
        return true;
    }

    bool openShared(const std::string& path) {
        if (sharedFile) return true;
        auto file=openAssetFile(path.c_str());
        if (!file || !readHeader(file.get()) || std::fseek(file.get(),0,SEEK_END)!=0 ||
            std::ftell(file.get())!=static_cast<long>(PayloadOffset+TextureBytes))
            return false;
        sharedFile=std::move(file);
        return true;
    }

    bool useShared(uint8_t* workspace,unsigned generation) {
        if (sharedPixels==workspace && sharedGeneration==generation) return true;
        sharedPixels=nullptr;
        if (!sharedFile || std::fseek(sharedFile.get(),PayloadOffset,SEEK_SET)!=0 ||
            std::fread(workspace,TextureBytes,1,sharedFile.get())!=1) return false;
        sceKernelDcacheWritebackRange(workspace,TextureBytes);
        sharedPixels=workspace;
        sharedGeneration=generation;
        return true;
    }

    void unload() {
        pixels.reset();
        sharedFile.reset();
        sharedPixels=nullptr;
    }
    const uint8_t* texture() const { return pixels?pixels.get():sharedPixels; }
    const uint32_t* clut() const { return palette; }
    const Patch& patch(unsigned index) const { return patches[index]; }

private:
    static constexpr unsigned TextureBytes=Width*Height;
    static constexpr unsigned PayloadOffset=8+Count*sizeof(Patch)+256*sizeof(uint32_t);

    bool readHeader(std::FILE* file) {
        uint32_t header[2]{};
        if (std::fread(header,sizeof header,1,file)!=1 || header[0]!=0x38504355 || header[1]!=Count ||
            std::fread(patches,sizeof patches,1,file)!=1 ||
            std::fread(palette,sizeof palette,1,file)!=1) return false;
        for (const auto& patch:patches) {
            if (patch.u<0 || patch.v<0 || patch.width<=0 || patch.height<=0 ||
                patch.u+patch.width>static_cast<int>(Width) || patch.v+patch.height>static_cast<int>(Height))
                return false;
        }
        sceKernelDcacheWritebackRange(palette,sizeof palette);
        return true;
    }

    alignas(64) uint32_t palette[256]{};
    Patch patches[Count]{};
    std::unique_ptr<uint8_t,ParkFreePixels> pixels;
    AssetFile sharedFile;
    uint8_t* sharedPixels=nullptr;
    unsigned sharedGeneration=0;
};
