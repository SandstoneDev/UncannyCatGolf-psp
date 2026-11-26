#pragma once

#include <vorbis/vorbisfile.h>
#include <zlib.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <pspkernel.h>
#include <cstring>
#include <iterator>
#include <memory>
#include <new>
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
    static constexpr unsigned GenericPhases = 256;
    static_assert(FilterTaps < NativeWindow && std::size(Resample48k) == OutputPhases);
    static_assert(GenericPhases*FilterTaps*sizeof(float)==65536);

    struct Entry {
        uint32_t offset, bytes, codec, frames, rate, channels, flags, loopFrame;
    };
#ifdef UCG_EDITOR_MODULE
    static constexpr unsigned EffectCacheSlots=4;
    struct EffectCachePage {
        unsigned id=Invalid,block=Invalid,rank=0;
        int16_t pcm[2*FramesPerBlock];
    };
    struct EffectBlockCache {
        EffectCachePage pages[EffectCacheSlots];
        // Header validation belongs to this open-file epoch, not the most
        // recently decoded source. Never borrow decoded PCM as header input.
        bool headerValid[Count]{};
    };
#endif
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

#ifdef UCG_EDITOR_MODULE
    static constexpr unsigned ModuleEffectCacheSlots=EffectCacheSlots;
    static constexpr unsigned moduleEffectCacheAllocationBytes(){return sizeof(EffectBlockCache);}
    unsigned moduleEffectCacheBytes()const{return effectCache?sizeof(EffectBlockCache):0;}
#endif
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
#ifdef UCG_EDITOR_MODULE
        // Allocate once before prefix loading/audio threads; cache misses must
        // never allocate another slab under renderer memory pressure.
        effectCache.reset(new(std::nothrow) EffectBlockCache);
        if(!effectCache){close();return Open::Invalid;}
#endif
        return Open::Ready;
    }

    void close() {
        endMusic();
        if (external) std::fclose(external);
        if (effects) std::fclose(effects);
        if (music) std::fclose(music);
        effects = music = nullptr;
        external = nullptr;
        externalTrack = Invalid;
        genericFilter.reset();
        blockTrack = blockIndex = tableTrack = Invalid;
#ifdef UCG_EDITOR_MODULE
        effectCache.reset();
#endif
    }

    bool ready() const { return music != nullptr; }

    // Call only after open(), before the reader/mixer threads start. A one-frame
    // PCM placeholder is reserved for external Ogg; later cues may follow it.
    bool overrideMusic(unsigned id, const char* path, double loopSeconds) {
        if (!ready() || id >= Count || (externalTrack != Invalid && id != externalTrack) || !path || !*path ||
            !std::isfinite(loopSeconds) || loopSeconds < 0.0) return false;
        const Entry& previous = entries[id];
        if (externalTrack == Invalid &&
            (previous.codec != Pcm || previous.frames != 1 || previous.rate != 44100 ||
             previous.channels != 2 || previous.flags != (Stream | Loop) || previous.loopFrame)) return false;
        std::FILE* input = std::fopen(path, "rb");
        if (!input) return false;
        std::setvbuf(input, nullptr, _IOFBF, 32768);
        if (std::fseek(input, 0, SEEK_END) != 0) { std::fclose(input); return false; }
        const long bytes = std::ftell(input);
        if (bytes <= 0) { std::fclose(input); return false; }
        Slice probeSlice{input, 0, static_cast<unsigned>(bytes), 0, Invalid, nullptr};
        OggVorbis_File probe{};
        constexpr ov_callbacks callbacks{sliceRead, sliceSeek, nullptr, sliceTell};
        if (ov_open_callbacks(&probeSlice, &probe, nullptr, 0, callbacks) != 0) {
            std::fclose(input);
            return false;
        }
        const vorbis_info* info = ov_info(&probe, 0);
        const ogg_int64_t total = ov_pcm_total(&probe, 0);
        const bool valid = info && (info->channels == 1 || info->channels == 2) &&
            info->rate >= 8000 && info->rate <= 192000 && ov_streams(&probe) == 1 &&
            ov_seekable(&probe) && total > 0 && total <= UINT32_MAX &&
            uint64_t(total) * 44100 <= uint64_t(UINT32_MAX) * static_cast<unsigned>(info->rate);
        const unsigned rate = valid ? static_cast<unsigned>(info->rate) : 0;
        const unsigned channels = valid ? static_cast<unsigned>(info->channels) : 0;
        // Correct a floating-point representation just below an authored
        // integral sample (18.9 * 44100 is a typical example).
        const double nativeLoop = std::floor(loopSeconds * rate + 1e-7);
        const bool validLoop = valid && nativeLoop >= 0.0 && nativeLoop < static_cast<double>(total);
        ov_clear(&probe);
        if (!validLoop) { std::fclose(input); return false; }
        std::unique_ptr<float[]> replacementFilter;
        if (rate != 44100 && rate != 48000 && !makeGenericFilter(rate,replacementFilter)) {
            std::fclose(input);
            return false;
        }
        endMusic();
        if (external) std::fclose(external);
        external = input;
        externalTrack = id;
        genericFilter = std::move(replacementFilter);
        entries[id] = {0, static_cast<unsigned>(bytes), Vorbis, static_cast<unsigned>(total),
                       rate, channels, Stream | Loop, static_cast<unsigned>(nativeLoop)};
        return true;
    }
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
#ifdef UCG_EDITOR_MODULE
            const int16_t* decoded=cachedEffectBlock(id,block);
            if(!decoded)break;
            const unsigned n = std::min(count - read, FramesPerBlock - index);
            std::memcpy(output + read * 2, decoded + index * 2, n * S16Stereo);
#else
            if (!loadBlock(id, block)) break;
            const unsigned n = std::min(count - read, FramesPerBlock - index);
            std::memcpy(output + read * 2, blockPcm + index * 2, n * S16Stereo);
#endif
            read += n;
            position += n;
        }
        if (read != count) std::clearerr(effects);
        return read;
    }

    unsigned readMusic(unsigned id, unsigned position, int16_t* output, unsigned count) {
        if (entries[id].codec == Pcm) return readEffect(id,position,output,count);
        if (entries[id].codec != Vorbis || position >= frames(id)) return 0;
        if (musicTrack != id && !beginMusic(id)) return 0;
        if (musicPosition != position && !seekMusic(position)) {
            endMusic();
            return 0;
        }
        count = std::min(count, frames(id) - position);
        const unsigned read = entries[id].rate == 44100 ? readNative(output, count) :
            entries[id].rate == 48000 ? resample(output, count) : resampleGeneric(output,count);
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
    std::FILE* external = nullptr;
    unsigned externalTrack = Invalid;
    std::unique_ptr<float[]> genericFilter;
    unsigned blockTrack = Invalid, blockIndex = Invalid, tableTrack = Invalid;
#ifdef UCG_EDITOR_MODULE
    std::unique_ptr<EffectBlockCache> effectCache;
#endif
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
        return (uint64_t(native) * 44100 + entry.rate - 1) / entry.rate;
    }

    bool readIndex() {
        uint32_t header[4];
        if (std::fseek(effects, 0, SEEK_END) != 0) return false;
        const long end = std::ftell(effects);
        if (end < static_cast<long>(sizeof(header)) ||
            std::fseek(effects, 0, SEEK_SET) != 0 || std::fread(header, sizeof(header), 1, effects) != 1 ||
            header[0] != 0x41564355 || header[1] != 1 ||
            (header[2] != Count && !(Count == 362 && (header[2] == 364 || header[2] == 365 || header[2] == 371))) ||
            header[3] != sizeof(Entry) || static_cast<unsigned long>(end) > UINT32_MAX ||
            header[2] > (static_cast<unsigned>(end) - sizeof(header)) / sizeof(Entry)) return false;
        unsigned offset = sizeof(header) + header[2] * sizeof(Entry);
        const unsigned bytes = static_cast<unsigned>(end);
        for (unsigned id = 0; id < header[2]; ++id) {
            Entry entry{};
            if (std::fread(&entry, sizeof entry, 1, effects) != 1) return false;
            if (entry.offset != offset || offset > bytes || !entry.bytes || entry.bytes > bytes - offset ||
                !entry.frames || entry.channels < 1 || entry.channels > 2 || entry.loopFrame >= entry.frames) return false;
            if (entry.codec == Vorbis) {
                if (!(entry.flags & Stream) || (entry.flags & ~(Stream | Loop | ImportLoop)) ||
                    (entry.rate != 44100 && entry.rate != 48000)) return false;
            } else if (entry.codec != Pcm || entry.channels != 2 ||
                       (entry.flags & ~(Stream | Loop | ImportLoop)) || entry.rate != 44100 ||
                       entry.frames > UINT32_MAX / S16Stereo) {
                return false;
            }
            if (id < Count) entries[id] = entry;
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

#ifdef UCG_EDITOR_MODULE
    const int16_t* cachedEffectBlock(unsigned id,unsigned block) {
        if(!effectCache)return nullptr;
        auto& pages=effectCache->pages;
        unsigned chosen=EffectCacheSlots;
        for(unsigned i=0;i<EffectCacheSlots;++i)
            if(pages[i].id==id&&pages[i].block==block){chosen=i;break;}
        if(chosen==EffectCacheSlots){
            for(unsigned i=0;i<EffectCacheSlots;++i)
                if(pages[i].id==Invalid){chosen=i;break;}
            if(chosen==EffectCacheSlots){
                chosen=0;
                for(unsigned i=1;i<EffectCacheSlots;++i)
                    if(pages[i].rank>pages[chosen].rank)chosen=i;
            }
            // Preserve every valid cached page if header/payload/inflate fails.
            if(!loadBlock(id,block))return nullptr;
            const unsigned count=std::min<unsigned>(entries[id].frames-block*FramesPerBlock,FramesPerBlock);
            std::memcpy(pages[chosen].pcm,blockPcm,count*S16Stereo);
            pages[chosen].id=id;pages[chosen].block=block;
            pages[chosen].rank=EffectCacheSlots;
        }
        const unsigned rank=pages[chosen].rank;
        for(unsigned i=0;i<EffectCacheSlots;++i)
            if(i!=chosen&&pages[i].id!=Invalid&&pages[i].rank<rank)++pages[i].rank;
        pages[chosen].rank=0;
        return pages[chosen].pcm;
    }
#endif
    bool loadBlock(unsigned id, unsigned block) {
        if (id == blockTrack && block == blockIndex) return true;
        const Entry& entry = entries[id];
        const unsigned blocks = (entry.frames + FramesPerBlock - 1) / FramesPerBlock;
        const unsigned dataStart = 12 + blocks * 8;
        if (block >= blocks || dataStart > entry.bytes) return false;
#ifdef UCG_EDITOR_MODULE
        if (!effectCache->headerValid[id]) {
#else
        if (id != tableTrack) {
#endif
            uint32_t header[3];
            const IoScope io(measuredIo);
            if (std::fseek(effects, entry.offset, SEEK_SET) != 0 ||
                std::fread(header, sizeof(header), 1, effects) != 1 ||
                header[0] != 0x4d43505a || header[1] != FramesPerBlock || header[2] != blocks) return false;
#ifdef UCG_EDITOR_MODULE
            effectCache->headerValid[id]=true;
#else
            tableTrack = id;
#endif
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
        slice = {id == externalTrack ? external : music, entry.offset, entry.bytes, 0, Invalid, measuredIo};
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
        const unsigned native = entry.rate == 44100 ? position : uint64_t(position) * entry.rate / 44100;
        const unsigned start = entry.rate == 44100 || native < FilterLeft ? native : native - FilterLeft;
        const unsigned seek = entry.rate != 44100 && native < FilterLeft ? 0 : start;
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
            const unsigned channels=entries[musicTrack].channels;
            const unsigned stride=channels*sizeof(int16_t);
            const long bytes = ov_read(&decoder, reinterpret_cast<char*>(output + read * 2),
                                       (count - read) * stride, 0, 2, 1, &link);
            if (bytes <= 0 || link != 0 || bytes % stride) break;
            const unsigned frames=bytes/stride;
            if (channels==1) {
                for (unsigned i=frames;i>0;--i) {
                    const int16_t sample=output[read*2+i-1];
                    output[(read+i-1)*2]=output[(read+i-1)*2+1]=sample;
                }
            }
            read += frames;
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

    static double besselI0(double value) {
        double sum=1.0,term=1.0;
        const double quarter=value*value*0.25;
        for (unsigned k=1;k<40;++k) {
            term*=quarter/(double(k)*k);
            sum+=term;
            if (term<sum*1e-15) break;
        }
        return sum;
    }

    static bool makeGenericFilter(unsigned rate,std::unique_ptr<float[]>& result) {
        // Exactly 64 KiB, allocated only for an external rate other than 44.1/48 kHz.
        std::unique_ptr<float[]> table(new(std::nothrow) float[GenericPhases*FilterTaps]);
        if (!table) return false;
        constexpr double pi=3.14159265358979323846,beta=8.6;
        const double cutoff=0.94*std::min(1.0,44100.0/rate);
        const double windowScale=1.0/besselI0(beta);
        for (unsigned phase=0;phase<GenericPhases;++phase) {
            double weights[FilterTaps],sum=0.0;
            const double fraction=double(phase)/GenericPhases;
            for (unsigned i=0;i<FilterTaps;++i) {
                const double x=double(i)-FilterLeft-fraction;
                const double radius=x/(FilterTaps*0.5);
                const double window=besselI0(beta*std::sqrt(std::max(0.0,1.0-radius*radius)))*windowScale;
                const double argument=pi*cutoff*x;
                const double sinc=std::abs(argument)<1e-12?1.0:std::sin(argument)/argument;
                weights[i]=cutoff*sinc*window;
                sum+=weights[i];
            }
            if (!std::isfinite(sum) || sum<=0.0) return false;
            for (unsigned i=0;i<FilterTaps;++i) table[phase*FilterTaps+i]=float(weights[i]/sum);
        }
        result=std::move(table);
        return true;
    }

    unsigned resampleGeneric(int16_t* output,unsigned count) {
        if (!genericFilter) return 0;
        const unsigned rate=entries[musicTrack].rate,last=entries[musicTrack].frames-1;
        unsigned center=uint64_t(musicPosition)*rate/44100;
        unsigned fraction=uint64_t(musicPosition)*rate%44100;
        unsigned read=0;
        for (;read<count;++read) {
            const unsigned scaled=fraction*GenericPhases;
            const unsigned phase=scaled/44100,next=(phase+1)%GenericPhases;
            const unsigned shift=phase+1==GenericPhases?1:0;
            const float blend=float(scaled%44100)*(1.0f/44100.0f);
            if (!fillSource(std::min(center+FilterRight+shift,last))) break;
            const float* first=genericFilter.get()+phase*FilterTaps;
            const float* second=genericFilter.get()+next*FilterTaps;
            float leftA=0,rightA=0,leftB=0,rightB=0;
            for (unsigned i=0;i<FilterTaps;++i) {
                const unsigned a=std::min<unsigned>(center+i>FilterLeft?center+i-FilterLeft:0,last);
                const unsigned b=std::min<unsigned>(center+i+shift>FilterLeft?center+i+shift-FilterLeft:0,last);
                if (a<sourceBase || b<sourceBase) return read;
                const unsigned atA=(a-sourceBase)*2,atB=(b-sourceBase)*2;
                leftA+=first[i]*sourcePcm[atA];
                rightA+=first[i]*sourcePcm[atA+1];
                leftB+=second[i]*sourcePcm[atB];
                rightB+=second[i]*sourcePcm[atB+1];
            }
            const float left=leftA+(leftB-leftA)*blend,right=rightA+(rightB-rightA)*blend;
            output[read*2]=std::clamp<int32_t>(int32_t(left+(left>=0?0.5f:-0.5f)),-32768,32767);
            output[read*2+1]=std::clamp<int32_t>(int32_t(right+(right>=0?0.5f:-0.5f)),-32768,32767);
            fraction+=rate;
            center+=fraction/44100;
            fraction%=44100;
        }
        musicPosition+=read;
        return read;
    }
};

} // namespace ucg::packed_audio
