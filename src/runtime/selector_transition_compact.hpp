#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <zlib.h>

class SelectorTransitionCompact {
public:
    static constexpr unsigned Worlds = 7;
    static constexpr unsigned Frames = 24;
    static constexpr unsigned Fps = 30;
    static constexpr unsigned ClipsPerLanguage = Worlds * (Worlds - 1);
    static constexpr unsigned Count = 2 * ClipsPerLanguage * Frames;
    static constexpr unsigned FrameBytes = 512 * 272 * 2;

    bool open(std::FILE* file) {
        valid = false;
        tag = 0;
        legacy = false;
        uint32_t header[4]{};
        if (!file || std::fseek(file, 0, SEEK_END) != 0) return false;
        const long size = std::ftell(file);
        if (size < 0 || static_cast<uint64_t>(size) > UINT32_MAX || std::fseek(file, 0, SEEK_SET) != 0 ||
            std::fread(header, sizeof header, 1, file) != 1 || header[1] != 2 * ClipsPerLanguage ||
            header[2] != Frames || header[3] != Fps) return false;

        if (header[0] == 0x314d4355) {
            legacy = true;
            valid = readLegacyIndex(file, static_cast<unsigned>(size));
            return valid;
        }
        if (header[0] != 0x324d4355 || std::fread(index, sizeof index, 1, file) != 1) return false;

        const unsigned dataStart = sizeof header + sizeof index;
        for (unsigned i = 0; i < Count; ++i) {
            const auto& entry = index[i];
            if (entry.offset < dataStart || entry.offset > static_cast<unsigned long>(size) ||
                !entry.bytes || entry.bytes > sizeof compressed ||
                entry.bytes > static_cast<unsigned long>(size) - entry.offset || entry.reserved ||
                (entry.base != NoBase &&
                 (entry.base >= Count || entry.base == i || index[entry.base].base != NoBase))) return false;
        }
        valid = true;
        return true;
    }

    const uint16_t* cached(unsigned id) const {
        if (!valid || id >= Count || !tag) return nullptr;
        return samePayload(index[id], index[tag - 1]) ? pixels : nullptr;
    }

    const uint16_t* frame(std::FILE* file, unsigned id) {
        if (!valid || id >= Count || !file) return nullptr;
        if (const auto* existing = cached(id)) return existing;
        const auto& entry = index[id];
        const bool haveBase = entry.base != NoBase && cached(entry.base);
        tag = 0;
        if (legacy) {
            if (!inflateLegacy(file, entry)) return nullptr;
        } else {
            if (entry.base != NoBase && !haveBase && !inflateFrame(file, index[entry.base], false)) return nullptr;
            if (!inflateFrame(file, entry, entry.base != NoBase)) return nullptr;
        }
        tag = id + 1;
        return pixels;
    }

private:
    static constexpr unsigned Rows = 272 / 8;
    static constexpr unsigned RowBytes = 512 * 8 * 2;
    static constexpr unsigned VisibleRowBytes = 272 * 8 * 2;
    static constexpr unsigned VisibleBytes = Rows * VisibleRowBytes;
    static constexpr uint16_t NoBase = 0xffff;
    struct Entry {
        uint32_t offset, bytes;
        uint16_t base, reserved;
    };
    static_assert(sizeof(Entry) == 12);

    static bool samePayload(const Entry& a, const Entry& b) {
        return a.offset == b.offset && a.bytes == b.bytes && a.base == b.base;
    }

    bool readLegacyIndex(std::FILE* file, unsigned fileBytes) {
        unsigned end = 16 + Count * 8;
        if (fileBytes <= end) return false;
        for (auto& entry : index) {
            uint32_t disk[2]{};
            if (std::fread(disk, sizeof disk, 1, file) != 1 || disk[0] != end ||
                !disk[1] || disk[1] > FrameBytes + 1024 || disk[1] > fileBytes - end) return false;
            entry = {disk[0], disk[1], NoBase, 0};
            end += disk[1];
        }
        return end == fileBytes;
    }

    bool inflateLegacy(std::FILE* file, const Entry& entry) {
        if (std::fseek(file, entry.offset, SEEK_SET) != 0) return false;
        z_stream stream{};
        stream.next_out = reinterpret_cast<Bytef*>(pixels);
        stream.avail_out = FrameBytes;
        if (inflateInit(&stream) != Z_OK) return false;

        unsigned remaining = entry.bytes;
        int status = Z_OK;
        while (remaining && status == Z_OK) {
            const unsigned chunk = remaining < sizeof compressed ? remaining : sizeof compressed;
            if (std::fread(compressed, 1, chunk, file) != chunk) break;
            remaining -= chunk;
            stream.next_in = compressed;
            stream.avail_in = chunk;
            status = inflate(&stream, Z_NO_FLUSH);
            if (stream.avail_in) break;
        }
        if (!remaining && status == Z_OK && !stream.avail_out) {
            Bytef extra = 0;
            stream.next_out = &extra;
            stream.avail_out = 1;
            status = inflate(&stream, Z_FINISH);
        }
        const bool ok = status == Z_STREAM_END && !remaining && !stream.avail_in && stream.total_out == FrameBytes;
        inflateEnd(&stream);
        return ok;
    }

    bool inflateFrame(std::FILE* file, const Entry& entry, bool delta) {
        if (std::fseek(file, entry.offset, SEEK_SET) != 0 ||
            std::fread(compressed, 1, entry.bytes, file) != entry.bytes) return false;
        z_stream stream{};
        stream.next_in = compressed;
        stream.avail_in = entry.bytes;
        if (inflateInit(&stream) != Z_OK) return false;

        auto* output = reinterpret_cast<Bytef*>(pixels);
        bool ok = true;
        int status = Z_OK;
        for (unsigned i = 0; i < Rows; ++i) {
            auto* row = output + i * RowBytes;
            stream.next_out = delta ? reinterpret_cast<Bytef*>(difference) : row;
            stream.avail_out = VisibleRowBytes;
            status = inflate(&stream, Z_NO_FLUSH);
            if (stream.avail_out || (status != Z_OK && status != Z_STREAM_END) ||
                (status == Z_STREAM_END && i + 1 != Rows)) {
                ok = false;
                break;
            }
            if (delta) {
                auto* words = pixels + i * RowBytes / sizeof(uint16_t);
                for (unsigned j = 0; j < VisibleRowBytes / sizeof(uint16_t); ++j)
                    words[j] ^= difference[j];
            } else {
                std::memset(row + VisibleRowBytes, 0, RowBytes - VisibleRowBytes);
            }
        }
        if (ok && status == Z_OK) {
            Bytef extra = 0;
            stream.next_out = &extra;
            stream.avail_out = 1;
            status = inflate(&stream, Z_FINISH);
        }
        ok = ok && status == Z_STREAM_END && stream.total_out == VisibleBytes && !stream.avail_in;
        inflateEnd(&stream);
        return ok;
    }

    Entry index[Count]{};
    alignas(64) uint16_t pixels[FrameBytes / sizeof(uint16_t)]{};
    uint16_t difference[VisibleRowBytes / sizeof(uint16_t)]{};
    unsigned char compressed[VisibleBytes + 1024]{};
    unsigned tag = 0;
    bool valid = false, legacy = false;
};
