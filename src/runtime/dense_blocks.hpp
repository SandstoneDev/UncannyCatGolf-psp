#pragma once

#include "asset_file.hpp"
#include "game.hpp"

class DenseBlocks {
public:
    static constexpr unsigned Capacity=96;

    bool load(const char* path) {
        const auto file=openAssetFile(path);
        uint32_t header[2]{};
        SolidBlock next[Capacity]{};
        if (!file || std::fread(header,sizeof header,1,file.get())!=1 ||
            header[0]!=0x42444355 || !header[1] || header[1]>Capacity ||
            std::fread(next,sizeof(SolidBlock),header[1],file.get())!=header[1] ||
            std::fgetc(file.get())!=EOF || std::ferror(file.get())) return false;
        for (unsigned i=0;i<header[1];++i) {
            const auto& block=next[i];
            if (!std::isfinite(block.center.x) || !std::isfinite(block.center.y) ||
                !std::isfinite(block.halfSize.x) || !std::isfinite(block.halfSize.y) ||
                block.halfSize.x<=0 || block.halfSize.y<=0) return false;
        }
        std::copy_n(next,header[1],blocks);
        count=header[1];
        return true;
    }

    void unload() { count=0; }
    const SolidBlock* data() const { return blocks; }
    unsigned size() const { return count; }

private:
    SolidBlock blocks[Capacity]{};
    unsigned count=0;
};
