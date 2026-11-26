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
    static constexpr unsigned LegacyCount = 226;
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
        const unsigned char* cache=nullptr;
        unsigned cacheStart=0, cacheEnd=0;
    };

public:
    enum class Open { Missing, Invalid, Ready };
    static constexpr unsigned MusicInputReserve=48*1024;

    unsigned musicInputReady() const {
        return slice.cache && slice.position>=slice.cacheStart && slice.position<slice.cacheEnd?
               slice.cacheEnd-slice.position:0;
    }

    bool canCacheMusic(unsigned id) const {
        return musicTrack==id && slice.size-slice.position>=MusicInputReserve;
    }

    unsigned cacheMusicInput() {
        slice.cache=nullptr;
        const unsigned count=std::min<unsigned>(sizeof musicCache,slice.size-slice.position);
        const unsigned offset=slice.base+slice.position;
        slice.filePosition=Invalid;
        if (std::fseek(music,offset,SEEK_SET)!=0 || std::fread(musicCache,1,count,music)!=count) {
            std::clearerr(music);
            return 0;
        }
        slice.filePosition=offset+count;
        slice.cache=musicCache;
        slice.cacheStart=slice.position;
        slice.cacheEnd=slice.position+count;
        return count;
    }
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
        blockTrack = blockIndex = Invalid;
        for (auto& table:blockTables) table=BlockTable{};
    }

    bool ready() const { return music != nullptr; }
    bool stream(unsigned id) const { return id<Count && (entries[id].flags & Stream); }
    bool loop(unsigned id) const { return id<Count && (entries[id].flags & Loop); }
    unsigned frames(unsigned id) const {
        return id<Count?outputFrames(entries[id], entries[id].frames):0;
    }
    unsigned loopFrame(unsigned id) const {
        return id<Count?outputFrames(entries[id], entries[id].loopFrame):0;
    }

    unsigned readEffect(unsigned id, unsigned position, int16_t* output, unsigned count) {
        if (id>=Count) return 0;
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
        if (id>=Count || position >= frames(id)) return 0;
        if (id>=LegacyCount && entries[id].codec == Pcm && stream(id))
            return readEffect(id,position,output,count);
        if (entries[id].codec != Vorbis) return 0;
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
    unsigned blockTrack = Invalid, blockIndex = Invalid;
    // Immutable bank metadata. Independent bounded windows prevent interleaved
    // voices from seeking back to the offset table for every PCM page.
    struct BlockTable {
        static constexpr unsigned Capacity=8;
        unsigned first=Invalid,count=0;
        bool checked=false;
        uint32_t locations[Capacity][2]{};
    };
    BlockTable blockTables[Count]{};
    alignas(64) int16_t blockPcm[2 * FramesPerBlock];
    alignas(64) unsigned char encoded[S16Stereo * FramesPerBlock];
    OggVorbis_File decoder{};
    Slice slice{};
    unsigned char musicCache[64*1024];
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
        if (end < static_cast<long>(sizeof(header)) ||
            std::fseek(effects, 0, SEEK_SET) != 0 || std::fread(header, sizeof(header), 1, effects) != 1 ||
            header[0] != 0x41564355 || header[1] != 1 ||
            (header[2] != Count &&
             !((Count==LegacyCount || Count==362 || Count==364 || Count==365 || Count==371) &&
               (header[2]==LegacyCount || header[2]==362 || header[2]==364 || header[2]==365 || header[2]==371))) || header[3] != sizeof(Entry) ||
            static_cast<unsigned long>(end)>UINT32_MAX ||
            header[2]>(static_cast<unsigned>(end)-sizeof(header))/sizeof(Entry)) return false;
        // Legacy metadata remains strict even when the shared bank has appended cues.
        // A standalone Main84 bank still exposes its original 226 entries.
        unsigned offset = sizeof(header) + header[2]*sizeof(Entry);
        const unsigned bytes = static_cast<unsigned>(end);
        for (unsigned id=0;id<header[2];++id) {
            Entry entry{};
            if (std::fread(&entry,sizeof entry,1,effects)!=1) return false;
            const bool legacy=id<LegacyCount;
            if (entry.offset != offset || offset>bytes || !entry.bytes || entry.bytes > bytes - offset ||
                !entry.frames || entry.channels<1 || entry.channels>2 ||
                (legacy && entry.channels!=2) || entry.loopFrame >= entry.frames) return false;
            if (entry.codec == Vorbis) {
                if (!(entry.flags & Stream) || (entry.flags & ~(Stream | Loop | ImportLoop)) ||
                    (entry.rate != 44100 && entry.rate != 48000)) return false;
            } else if (entry.codec != Pcm || entry.channels!=2 ||
                       (entry.flags & ~(legacy?Loop:(Stream|Loop|ImportLoop))) || entry.rate != 44100 ||
                       (legacy && entry.loopFrame) || entry.frames > UINT32_MAX / S16Stereo) {
                return false;
            }
            if (id<Count) entries[id]=entry;
            offset += entry.bytes;
        }
        if (header[2]<Count) std::fill(entries+header[2],entries+Count,Entry{});
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
        auto& table=blockTables[id];
        if (!table.checked) {
            uint32_t header[3];
            const IoScope io(measuredIo);
            if (std::fseek(effects, entry.offset, SEEK_SET) != 0 ||
                std::fread(header, sizeof(header), 1, effects) != 1 ||
                header[0] != 0x4d43505a || header[1] != FramesPerBlock || header[2] != blocks) return false;
            table.checked = true;
        }
        if (block<table.first || block-table.first>=table.count) {
            uint32_t incoming[BlockTable::Capacity][2];
            const unsigned first=block/BlockTable::Capacity*BlockTable::Capacity;
            const unsigned count=std::min<unsigned>(BlockTable::Capacity,blocks-first);
            const IoScope io(measuredIo);
            if (std::fseek(effects, entry.offset + 12 + first * 8, SEEK_SET) != 0 ||
                std::fread(incoming, sizeof incoming[0], count, effects) != count) return false;
            std::memcpy(table.locations,incoming,count*sizeof incoming[0]);
            table.first=first;
            table.count=count;
        }
        const auto& loc=table.locations[block-table.first];
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
        if (source.cache && source.position>=source.cacheStart && source.position<source.cacheEnd &&
            count<=(source.cacheEnd-source.position)/size) {
            const auto bytes=count*size;
            std::memcpy(output,source.cache+source.position-source.cacheStart,bytes);
            source.position+=bytes;
            return count;
        }
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
        if (!info || static_cast<uint32_t>(info->rate) != entry.rate || static_cast<unsigned>(info->channels) != entry.channels || ov_streams(&decoder) != 1 ||
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
        if (entries[musicTrack].channels==1) {
            unsigned read=0;
            while (read<count) {
                int link=-1;
                auto* samples=output+read*2;
                const long bytes=ov_read(&decoder,reinterpret_cast<char*>(samples),
                                         (count-read)*sizeof(int16_t),0,2,1,&link);
                if (bytes<=0 || link!=0 || bytes%sizeof(int16_t)) break;
                const unsigned frames=bytes/sizeof(int16_t);
                for (unsigned i=frames;i>0;--i) {
                    const int16_t sample=samples[i-1];
                    samples[2*i-2]=samples[2*i-1]=sample;
                }
                read+=frames;
            }
            return read;
        }
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
