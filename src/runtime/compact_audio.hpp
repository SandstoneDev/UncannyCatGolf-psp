#pragma once

#include <vorbis/vorbisfile.h>
#include <zlib.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <pspkernel.h>
#include <cstring>
#include <iterator>
#include <string>

#include "audio_src_48k.inc"

namespace ucg::packed_audio {

template<unsigned Count>
class Bank {
    static constexpr unsigned S16Stereo = 4;
    static constexpr unsigned FramesPerBlock = 4096;
    static constexpr unsigned Invalid = UINT32_MAX;
    static constexpr unsigned RawBlock = 1u << 31;
    static constexpr unsigned Vorbis = 1;
    static constexpr unsigned Pcm = 2;
    static constexpr unsigned Stream = 1;
    static constexpr unsigned Loop = 2;
    static constexpr unsigned ImportLoop = 4;
    static constexpr unsigned NativeWindow = 128;
    static constexpr unsigned FilterTaps = std::size(Resample48k[0]);
    static constexpr unsigned FilterLeft = FilterTaps / 2 - 1;
    static constexpr unsigned FilterRight = FilterTaps - FilterLeft - 1;
    static constexpr unsigned OutputStep = 160;
    static constexpr unsigned OutputPhases = 147;
    static_assert(FilterTaps < NativeWindow && std::size(Resample48k) == OutputPhases);

    struct Entry {
        uint32_t offset, bytes, codec, frames, rate, channels, flags, loopFrame;
    };
public:
    struct IoTiming { unsigned calls=0,microseconds=0; };
private:
    struct Slice {
        std::FILE* file = nullptr;
        unsigned base = 0, size = 0, position = 0, filePosition = Invalid;
        IoTiming* timing=nullptr;
    };

public:
    enum class Open { Missing, Invalid, Ready };
    void measureIo(IoTiming* target) { measuredIo=target; slice.timing=target; }

    Bank() = default;
    Bank(const Bank&) = delete;
    Bank& operator=(const Bank&) = delete;
    ~Bank() { close(); }

    Open open(const char* legacyPath) {
#ifdef UCG_DISC_BUILD
        const std::string path(legacyPath);
#else
        std::string path(legacyPath);
        const auto slash = path.find_last_of("/\\");
        path.resize(slash == std::string::npos ? 0 : slash + 1);
        path += "audio.uca";
#endif
        effects = std::fopen(path.c_str(), "rb");
        if (!effects) return Open::Missing;
        std::setvbuf(effects, nullptr, _IOFBF, 16384);
        if (!readIndex()) return Open::Invalid;
        music = std::fopen(path.c_str(), "rb");
        if (!music) return Open::Invalid;
        std::setvbuf(music, nullptr, _IOFBF, 32768);
        return Open::Ready;
    }

    void close() {
        endMusic();
        if (effects) std::fclose(effects);
        if (music) std::fclose(music);
        effects = music = nullptr;
        blockTrack = blockIndex = tableTrack = Invalid;
    }

    bool ready() const { return music != nullptr; }
    bool stream(unsigned id) const { return entries[id].flags & Stream; }
    bool loop(unsigned id) const { return entries[id].flags & Loop; }
    unsigned frames(unsigned id) const {
        return outputFrames(entries[id], entries[id].frames);
    }
    unsigned loopFrame(unsigned id) const {
        return outputFrames(entries[id], entries[id].loopFrame);
    }

    unsigned readEffect(unsigned id, unsigned position, int16_t* output, unsigned count) {
        const Entry& entry = entries[id];
        if (entry.codec != Pcm || position >= entry.frames) return 0;
        count = std::min<unsigned>(count, entry.frames - position);
        unsigned read = 0;
        while (read < count) {
            const unsigned block = position / FramesPerBlock;
            const unsigned index = position % FramesPerBlock;
            if (!loadBlock(id, block)) break;
            const unsigned n = std::min(count - read, FramesPerBlock - index);
            std::memcpy(output + read * 2, blockPcm + index * 2, n * S16Stereo);
            read += n;
            position += n;
        }
        if (read != count) std::clearerr(effects);
        return read;
    }

    unsigned readMusic(unsigned id, unsigned position, int16_t* output, unsigned count) {
        if (entries[id].codec != Vorbis || position >= frames(id)) return 0;
        if (musicTrack != id && !beginMusic(id)) return 0;
        if (musicPosition != position && !seekMusic(position)) {
            endMusic();
            return 0;
        }
        count = std::min(count, frames(id) - position);
        const unsigned read = entries[id].rate == 44100 ? readNative(output, count) : resample(output, count);
        if (entries[id].rate == 44100) musicPosition += read;
        if (read != count) endMusic();
        return read;
    }

    void endMusic() {
        if (musicTrack == Invalid) return;
        ov_clear(&decoder);
        musicTrack = Invalid;
        std::clearerr(music);
    }

private:
    Entry entries[Count]{};
    std::FILE* effects = nullptr;
    std::FILE* music = nullptr;
    unsigned blockTrack = Invalid, blockIndex = Invalid, tableTrack = Invalid;
    alignas(64) int16_t blockPcm[2 * FramesPerBlock];
    alignas(64) unsigned char encoded[S16Stereo * FramesPerBlock];
    OggVorbis_File decoder{};
    Slice slice{};
    IoTiming* measuredIo=nullptr;
    unsigned musicTrack = Invalid, musicPosition = 0;
    unsigned sourceBase = 0, sourceCount = 0;
    int16_t sourcePcm[2 * NativeWindow];

    static unsigned outputFrames(const Entry& entry, unsigned native) {
        if (entry.rate == 44100) return native;
        return (uint64_t(native) * OutputPhases + OutputStep - 1) / OutputStep;
    }

    bool readIndex() {
        uint32_t header[4];
        if (std::fseek(effects, 0, SEEK_END) != 0) return false;
        const long end = std::ftell(effects);
        if (end < static_cast<long>(sizeof(header) + sizeof(entries)) ||
            std::fseek(effects, 0, SEEK_SET) != 0 || std::fread(header, sizeof(header), 1, effects) != 1 ||
            header[0] != 0x41564355 || header[1] != 1 || header[2] != Count || header[3] != sizeof(Entry) ||
            std::fread(entries, sizeof(Entry), Count, effects) != Count) return false;
        unsigned offset = sizeof(header) + sizeof(entries);
        const unsigned bytes = static_cast<unsigned>(end);
        for (const Entry& entry : entries) {
            if (entry.offset != offset || !entry.bytes || entry.bytes > bytes - offset ||
                !entry.frames || entry.channels != 2 || entry.loopFrame >= entry.frames) return false;
            if (entry.codec == Vorbis) {
                if (!(entry.flags & Stream) || (entry.flags & ~(Stream | Loop | ImportLoop)) ||
                    (entry.rate != 44100 && entry.rate != 48000)) return false;
            } else if (entry.codec != Pcm || (entry.flags & ~Loop) || entry.rate != 44100 ||
                       entry.loopFrame || entry.frames > UINT32_MAX / S16Stereo) {
                return false;
            }
            offset += entry.bytes;
        }
        return offset == bytes;
    }

    struct IoScope {
        IoTiming* sample;
        unsigned start;
        explicit IoScope(IoTiming* target):sample(target),start(target?sceKernelGetSystemTimeLow():0) {}
        ~IoScope() {
            if (sample) {
                ++sample->calls;
                sample->microseconds+=sceKernelGetSystemTimeLow()-start;
            }
        }
    };

    bool loadBlock(unsigned id, unsigned block) {
        if (id == blockTrack && block == blockIndex) return true;
        const Entry& entry = entries[id];
        const unsigned blocks = (entry.frames + FramesPerBlock - 1) / FramesPerBlock;
        const unsigned dataStart = 12 + blocks * 8;
        if (block >= blocks || dataStart > entry.bytes) return false;
        if (id != tableTrack) {
            uint32_t header[3];
            const IoScope io(measuredIo);
            if (std::fseek(effects, entry.offset, SEEK_SET) != 0 ||
                std::fread(header, sizeof(header), 1, effects) != 1 ||
                header[0] != 0x4d43505a || header[1] != FramesPerBlock || header[2] != blocks) return false;
            tableTrack = id;
        }
        uint32_t loc[2];
        {
            const IoScope io(measuredIo);
            if (std::fseek(effects, entry.offset + 12 + block * 8, SEEK_SET) != 0 ||
                std::fread(loc, sizeof(loc), 1, effects) != 1) return false;
        }
        const bool raw = loc[1] & RawBlock;
        const unsigned length = loc[1] & ~RawBlock;
        if (loc[0] < dataStart || loc[0] > entry.bytes || !length || length > sizeof(encoded) ||
            length > entry.bytes - loc[0]) return false;
        const unsigned bytes = std::min<unsigned>(entry.frames - block * FramesPerBlock, FramesPerBlock) * S16Stereo;
        blockTrack = Invalid;
        {
            const IoScope io(measuredIo);
            if (std::fseek(effects, entry.offset + loc[0], SEEK_SET) != 0 ||
                (raw && length != bytes) ||
                std::fread(raw ? static_cast<void*>(blockPcm) : encoded, 1, length, effects) != length) return false;
        }
        if (!raw) {
            unsigned long output = bytes;
            if (uncompress(reinterpret_cast<unsigned char*>(blockPcm), &output, encoded, length) != Z_OK ||
                output != bytes) return false;
        }
        blockTrack = id;
        blockIndex = block;
        return true;
    }

    static size_t sliceRead(void* output, size_t size, size_t count, void* context) {
        auto& source = *static_cast<Slice*>(context);
        if (!size) return 0;
        count = std::min(count, (source.size - source.position) / size);
        const unsigned target = source.base + source.position;
        const IoScope io(source.timing);
        if (source.filePosition != target && std::fseek(source.file, target, SEEK_SET) != 0) return 0;
        const size_t read = std::fread(output, size, count, source.file);
        source.position += read * size;
        source.filePosition = target + read * size;
        return read;
    }

    static int sliceSeek(void* context, ogg_int64_t offset, int whence) {
        auto& source = *static_cast<Slice*>(context);
        ogg_int64_t origin = 0;
        switch (whence) {
        case SEEK_SET: break;
        case SEEK_CUR: origin = source.position; break;
        case SEEK_END: origin = source.size; break;
        default: return -1;
        }
        if (offset < -origin || offset > source.size - origin) return -1;
        source.position = origin + offset;
        return 0;
    }

    static long sliceTell(void* context) {
        return static_cast<Slice*>(context)->position;
    }

    bool beginMusic(unsigned id) {
        endMusic();
        const Entry& entry = entries[id];
        slice = {music, entry.offset, entry.bytes, 0, Invalid, measuredIo};
        constexpr ov_callbacks callbacks{sliceRead, sliceSeek, nullptr, sliceTell};
        if (ov_open_callbacks(&slice, &decoder, nullptr, 0, callbacks) != 0) {
            std::clearerr(music);
            return false;
        }
        musicTrack = id;
        const vorbis_info* info = ov_info(&decoder, 0);
        if (!info || static_cast<uint32_t>(info->rate) != entry.rate || info->channels != 2 || ov_streams(&decoder) != 1 ||
            !ov_seekable(&decoder) || ov_pcm_total(&decoder, 0) != entry.frames) {
            endMusic();
            return false;
        }
        musicPosition = sourceBase = sourceCount = 0;
        return true;
    }

    bool seekMusic(unsigned position) {
        const Entry& entry = entries[musicTrack];
        const unsigned native = entry.rate == 44100 ? position : uint64_t(position) * OutputStep / OutputPhases;
        const unsigned start = entry.rate == 44100 || native < FilterLeft ? native : native - FilterLeft;
        const unsigned seek = entry.rate == 48000 && native < FilterLeft ? 0 : start;
        if (ov_pcm_seek(&decoder, seek) != 0 || ov_pcm_tell(&decoder) != seek) return false;
        sourceBase = seek;
        sourceCount = 0;
        musicPosition = position;
        return true;
    }

    unsigned readNative(int16_t* output, unsigned count) {
        unsigned read = 0;
        while (read < count) {
            int link = -1;
            const long bytes = ov_read(&decoder, reinterpret_cast<char*>(output + read * 2),
                                       (count - read) * S16Stereo, 0, 2, 1, &link);
            if (bytes <= 0 || link != 0 || bytes % S16Stereo) break;
            read += bytes / S16Stereo;
        }
        return read;
    }

    bool fillSource(unsigned through) {
        if (through < sourceBase) return false;
        if (through < sourceBase + sourceCount) return true;
        const unsigned discard = sourceCount > FilterTaps ? sourceCount - FilterTaps : 0;
        if (discard) {
            sourceCount -= discard;
            sourceBase += discard;
            std::memmove(sourcePcm, sourcePcm + discard * 2, sourceCount * S16Stereo);
        }
        const unsigned end = sourceBase + sourceCount;
        const unsigned count = std::min<unsigned>(NativeWindow - sourceCount, entries[musicTrack].frames - end);
        sourceCount += readNative(sourcePcm + sourceCount * 2, count);
        return through < sourceBase + sourceCount;
    }

    unsigned resample(int16_t* output, unsigned count) {
        // Only 48 kHz streams use this 160:147 filter. Decode remains in the source sample domain.
        const unsigned last = entries[musicTrack].frames - 1;
        unsigned center = uint64_t(musicPosition) * OutputStep / OutputPhases;
        unsigned phase = uint64_t(musicPosition) * OutputStep % OutputPhases;
        unsigned read = 0;
        for (; read < count; ++read) {
            if (!fillSource(std::min(center + FilterRight, last))) break;
            const auto& taps = Resample48k[phase];
            int32_t left = 0, right = 0;
            for (unsigned i = 0; i < FilterTaps; ++i) {
                const unsigned at = std::min<unsigned>(center + i > FilterLeft ? center + i - FilterLeft : 0, last);
                if (at < sourceBase) return read;
                const unsigned index = (at - sourceBase) * 2;
                left += taps[i] * int32_t(sourcePcm[index]);
                right += taps[i] * int32_t(sourcePcm[index + 1]);
            }
            output[read * 2] = std::clamp<int32_t>((left + 8192) >> 14, -32768, 32767);
            output[read * 2 + 1] = std::clamp<int32_t>((right + 8192) >> 14, -32768, 32767);
            phase += OutputStep;
            center += phase / OutputPhases;
            phase %= OutputPhases;
        }
        musicPosition += read;
        return read;
    }
};

} // namespace ucg::packed_audio
