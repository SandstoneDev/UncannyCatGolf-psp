#pragma once

#include <cstdio>
#include <memory>

struct CloseAssetFile {
    void operator()(std::FILE* file) const noexcept { std::fclose(file); }
};

using AssetFile=std::unique_ptr<std::FILE,CloseAssetFile>;

inline AssetFile openAssetFile(const char* path) {
    return AssetFile(std::fopen(path,"rb"));
}
