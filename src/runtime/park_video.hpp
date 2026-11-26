#pragma once

#include "asset_file.hpp"
#include "io_audit.hpp"
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <malloc.h>
#include <new>
#include <zlib.h>
#ifdef __PSP__
#include <pspkernel.h>
#endif

namespace ParkVideoDetail {

struct FreePixels {
    void operator()(void* pixels) const noexcept { std::free(pixels); }
};

template<class Pixel, unsigned Texels, unsigned Frames, uint32_t Magic>
class Layers {
public:
    bool open(const char* path,Pixel* video=nullptr) {
        close();
        const char* const kind=Frames==2?"tiles_open":"video_open";
        LoadIoAudit::setPhase(kind);
        errno=0;
        auto input = openAssetFile(path);
        if (!input) return failed(kind);
        LoadIoAudit::setPhase("pack_length");
        errno=0;
        if (std::fseek(input.get(), 0, SEEK_END) != 0) return failed("pack_length");
        const long length = std::ftell(input.get());
        if (length < 0 || std::fseek(input.get(), 0, SEEK_SET) != 0) return failed("pack_length");

        uint32_t header[3]{};
        LoadIoAudit::setPhase("pack_header");
        errno=0;
        if (std::fread(header, sizeof header, 1, input.get()) != 1) return failed("pack_header",sizeof header);
        if (header[0] != Magic || header[1] != Frames || header[2] != Bytes) return failed("pack_header_validate");
        LoadIoAudit::setPhase("pack_records");
        errno=0;
        if (std::fread(records, sizeof records, 1, input.get()) != 1) return failed("pack_records",sizeof records);

        const auto fileSize = static_cast<unsigned long>(length);
        unsigned long end = sizeof header + sizeof records;
        uint32_t maxLength = 0;
        for (unsigned i = 0; i < Frames; ++i) {
            const auto& record = records[i];
            if (i && record.offset == records[i-1].offset && record.length == records[i-1].length) continue;
            if (record.offset != end || end > fileSize || !record.length ||
                record.length > fileSize - end || record.length > compressBound(Bytes)) return failed("pack_record_validate",i);
            end += record.length;
            if (record.length > maxLength) maxLength = record.length;
        }
        if (end != fileSize) return failed("pack_size_validate",end,static_cast<unsigned>(fileSize));

        std::unique_ptr<Pixel, FreePixels> texture;
        LoadIoAudit::setPhase("pack_texture_alloc");
        errno=0;
        if (!video) texture.reset(static_cast<Pixel*>(memalign(64, Bytes)));
        if (!video && !texture) return failed("pack_texture_alloc",Bytes);
        LoadIoAudit::setPhase("pack_compressed_alloc");
        errno=0;
        std::unique_ptr<Bytef[]> buffer(new (std::nothrow) Bytef[maxLength]);
        if (!buffer) return failed("pack_compressed_alloc",maxLength);
        output = video ? video : texture.get();
        pixels = std::move(texture);
        compressed = std::move(buffer);
        file = std::move(input);
        return true;
    }

    void close() {
        currentOffset = 0;
        output = nullptr;
        file.reset();
        pixels.reset();
        compressed.reset();
    }

    const Pixel* frame(unsigned index) {
        if (!file || index >= Frames) return nullptr;
        const auto& record = records[index];
        if (record.offset == currentOffset) return output;
        LoadIoAudit::setPhase(Frames==2?"tiles_frame":"video_frame");
        errno=0;
        if (std::fseek(file.get(), record.offset, SEEK_SET) != 0 ||
            std::fread(compressed.get(), record.length, 1, file.get()) != 1) {
            failed("pack_frame_read",record.length);
            return nullptr;
        }

        currentOffset = 0;
        unsigned long size = Bytes;
        auto* target = reinterpret_cast<Bytef*>(output);
#ifdef __PSP__
        if (!pixels) target = reinterpret_cast<Bytef*>(reinterpret_cast<uintptr_t>(target) | 0x40000000u);
#endif
        const int decoded=uncompress(target, &size, compressed.get(), record.length);
        if (decoded != Z_OK || size != Bytes) {
            LoadIoAudit::failure("pack_frame_decode",decoded,Bytes,static_cast<unsigned>(size));
            return nullptr;
        }
#ifdef __PSP__
        if (pixels) sceKernelDcacheWritebackRange(output, Bytes);
#endif
        currentOffset = record.offset;
        return output;
    }

private:
    static bool failed(const char* phase,unsigned wanted=0,unsigned got=0) {
        LoadIoAudit::failure(phase,errno,wanted,got);
        return false;
    }
    static constexpr unsigned Bytes = Texels * sizeof(Pixel);
    struct Record { uint32_t offset, length; };
    static_assert(sizeof(Record) == 8);
    AssetFile file;
    std::unique_ptr<Pixel, FreePixels> pixels;
    Pixel* output = nullptr;
    std::unique_ptr<Bytef[]> compressed;
    Record records[Frames]{};
    uint32_t currentOffset = 0;
};

}

class ParkVideo {
public:
    static constexpr unsigned Side = 256;
    static constexpr unsigned FrameCount = 417;
    static constexpr unsigned Bytes = Side * Side * sizeof(uint16_t);
    static constexpr uint64_t LoopUs = 27837710;
    static constexpr float WorldSizeX = 640;
    static constexpr float WorldSizeY = 639;
    static constexpr float NativeHeight = WorldSizeY * (17.0f / 40.0f);

    bool open(const char* path, uint16_t* video) { return layers.open(path, video); }
    void close() { layers.close(); }

    // Scene elapsed time is independent of the switch state. Call after the
    // previous GE list has finished, then bind the returned swizzled 5650 texture.
    const uint16_t* frame(uint64_t elapsedUs) { return layers.frame(frameIndex(elapsedUs)); }
    static constexpr unsigned frameIndex(uint64_t elapsedUs) {
        // Preserve the original stream duration, including the short final hold.
        const auto index = static_cast<unsigned>((elapsedUs % LoopUs) * 3 / 200200);
        return index < FrameCount ? index : FrameCount - 1;
    }
    static constexpr uint32_t tint(unsigned state) {
        return state == 0 ? 0xff264d00 : 0xff400080;
    }

private:
    ParkVideoDetail::Layers<uint16_t, Side * Side, FrameCount, 0x56504355> layers;
};

class ParkSwitchTiles {
public:
    static constexpr unsigned Side = 512;
    static constexpr unsigned Field = 272;
    static constexpr unsigned Bytes = Side * Field * sizeof(uint32_t);

    // The caller supplies VRAM reserved for this layer.
    bool open(const char* path, uint32_t* video) { return layers.open(path, video); }
    void close() { layers.close(); }

    // Swizzled 8888, straight alpha. Sample only the first Field rows and columns.
    // The padded texture uses Side as the GU buffer width.
    const uint32_t* pixels(unsigned state) { return layers.frame(state); }

private:
    ParkVideoDetail::Layers<uint32_t, Side * Field, 2, 0x4c544355> layers;
};
