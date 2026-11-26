#pragma once

#include "asset_file.hpp"
#include "smiley_ring.hpp"
#include "visual.hpp"
#include "park_resource_io.hpp"
#include <cstddef>
#include <cstdlib>
#include <malloc.h>
#include <new>
#include <pspkernel.h>
#include <string>

struct ParkBalloon {
    Vec2 anchor;
    float length,width;
    uint32_t color,stringColor;
};
static_assert(sizeof(ParkBalloon)==24);

struct ParkStage {
    static constexpr unsigned MaxBalloons=6;
    SmileyStage smiley;
    uint32_t tint,balloonCount;
    Vec2 ufo;
    float ufoSpeed;
    ParkBalloon balloons[MaxBalloons];
};

class ParkArt {
public:
    static constexpr unsigned Side=256, RingFrames=10, CoinFrames=10, UfoFrames=3;
    static constexpr unsigned MotionFrames=3600, Crossfade=60;
    static constexpr unsigned MotionTrackBytes=MotionFrames*sizeof(Vec2);

    bool open(const char* art,const char* legacyStage) {
        const auto pictures=openAssetFile(art);
        if (!pictures) return false;
        uint32_t header[2]{};
        if (std::fread(header,sizeof header,1,pictures.get())!=1 || header[0]!=0x4b504355 ||
            header[1]!=std::size(patches) || std::fread(patches,sizeof patches,1,pictures.get())!=1 ||
            std::fseek(pictures.get(),0,SEEK_END)!=0 ||
            std::ftell(pictures.get())!=static_cast<long>(sizeof header+sizeof patches+TextureBytes)) return false;
        for (const auto& patch:patches) {
            if (patch.u<0 || patch.v<0 || patch.width<=0 || patch.height<=0 ||
                patch.u+patch.width>static_cast<int>(Side) || patch.v+patch.height>static_cast<int>(Side)) return false;
        }
        artPath=art;
        legacyPath=legacyStage;
        return true;
    }

    bool load(const char* path=nullptr,bool extraRings=false,void* video=nullptr,void* scratch=nullptr,
              void* motionStorage=nullptr,unsigned motionBytes=0,const char** reason=nullptr) {
        io={};
        const std::string name=path?std::string(path):legacyPath;
        if (loadedPath==name && borrowed==video &&
            motionView==(motionStorage?motionStorage:motion.get())) return true;
        if (reason) *reason="park_stage_header";
        auto config=openAssetFile(name.c_str());
        ParkStage stage{};
        if (!config || !readStage(config.get(),stage)) return false;
        if (!extraRings && !stage.balloonCount && !stage.smiley.coinCount && !stage.ufoSpeed) {
            if (std::fgetc(config.get())!=EOF || std::ferror(config.get())) return false;
            unload();
            data=stage;
            loadedPath=name;
            return true;
        }
        unload();
        std::unique_ptr<Cache,FreeCache> memory;
        if (!video) {
            if (reason) *reason="park_art_memory";
            auto* const storage=memalign(64,sizeof(Cache));
            if (!storage) return false;
            memory.reset(new (storage) Cache);
        }
        auto* pixels=video?scratch:memory->texture;
        if (reason) *reason="park_art_pixels";
        if (!pixels) return false;
        std::unique_ptr<BalloonTrack[]> tracks;
        BalloonTrack* trackPixels=nullptr;
        if (stage.balloonCount) {
            if (reason) *reason="park_motion_memory";
            if (motionStorage) {
                if (reinterpret_cast<uintptr_t>(motionStorage)%alignof(BalloonTrack) ||
                    stage.balloonCount>motionBytes/sizeof(BalloonTrack)) return false;
                trackPixels=static_cast<BalloonTrack*>(motionStorage);
            } else {
                tracks.reset(new (std::nothrow) BalloonTrack[stage.balloonCount]);
                if (!tracks) return false;
                trackPixels=tracks.get();
            }
        }
        if (reason) *reason="park_motion_data";
        for (unsigned i=0;i<stage.balloonCount;++i) {
            auto& samples=trackPixels[i].samples;
            if (motionStorage) {
                // Kernel I/O always targets RAM, including borrowed VRAM tracks.
                alignas(64) uint8_t chunk[4096];
                auto* destination=reinterpret_cast<uint8_t*>(samples);
                for (unsigned offset=0;offset<sizeof samples;) {
                    const unsigned bytes=std::min<unsigned>(sizeof chunk,sizeof samples-offset);
                    if (std::fread(chunk,1,bytes,config.get())!=bytes) return false;
                    std::memcpy(destination+offset,chunk,bytes);
                    offset+=bytes;
                }
            } else if (std::fread(samples,sizeof samples,1,config.get())!=1) return false;
            for (const auto point:samples) if (!finite(point)) return false;
        }
        if (std::fgetc(config.get())!=EOF || std::ferror(config.get())) return false;
        config.reset();
        {
            ParkResourceReader pictures(io);
            if (!pictures.open(artPath.c_str()) ||
                !pictures.seek(2*sizeof(uint32_t)+sizeof patches) ||
                !pictures.read(pixels,TextureBytes) || !pictures.finish()) {
                if (reason) *reason=io.phase;
                return false;
            }
        }
        if (video) {
            auto* uncached=reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(video)|0x40000000u);
            std::memcpy(uncached,pixels,TextureBytes);
        } else sceKernelDcacheWritebackRange(pixels,TextureBytes);
        borrowed=video;
        cache=std::move(memory);
        motion=std::move(tracks);
        motionView=trackPixels;
        loadedPath=name;
        data=stage;
        return true;
    }

    void unload() {
        cache.reset();
        borrowed=nullptr;
        motion.reset();
        motionView=nullptr;
        loadedPath.clear();
        data={};
    }

    const uint32_t* pixels() const { return borrowed?static_cast<const uint32_t*>(borrowed):cache?cache->texture:nullptr; }
    void invalidateRingTexture() { ringPixels=nullptr; }
    const uint32_t* ringTexture(uint8_t* storage,unsigned generation) {
        if (ringGeneration!=generation || ringPixels!=storage) {
            ringPixels=nullptr;
            const auto pictures=openAssetFile(artPath.c_str());
            if (!pictures || std::fseek(pictures.get(),2*sizeof(uint32_t)+sizeof patches,SEEK_SET)!=0 ||
                std::fread(storage,TextureBytes,1,pictures.get())!=1) return nullptr;
            sceKernelDcacheWritebackRange(storage,TextureBytes);
            ringPixels=storage;
            ringGeneration=generation;
        }
        return reinterpret_cast<const uint32_t*>(ringPixels);
    }
    const Patch& ring(unsigned tick) const { return patches[tick/4%RingFrames]; }
    const Patch& coin(unsigned tick) const { return patches[RingFrames+tick/4%CoinFrames]; }
    const Patch& ufo(unsigned tick) const { return patches[RingFrames+CoinFrames+tick/6%UfoFrames]; }
    const Patch& balloon() const { return patches[RingFrames+CoinFrames+UfoFrames]; }

    Vec2 balloonMotion(unsigned index,unsigned tick) const {
        constexpr unsigned period=MotionFrames-Crossfade;
        const unsigned phase=tick%period;
        const auto& samples=motionView[index].samples;
        if (tick<period || phase>=Crossfade) return samples[phase];
        const float t=static_cast<float>(phase)/Crossfade;
        const float weight=t*t*(3.0f-2.0f*t);
        return samples[period+phase]*(1.0f-weight)+samples[phase]*weight;
    }

    ParkStage data{};
    ParkReadStatus io{};
    const std::string& atlasPath() const { return artPath; }

private:
    struct LegacyStage {
        SmileyStage smiley;
        uint32_t tint;
        ParkBalloon balloons[2];
        Vec2 ufo;
        float ufoSpeed;
    };
    static_assert(sizeof(LegacyStage)==232);

    static bool finite(Vec2 point) {
        return std::isfinite(point.x) && std::isfinite(point.y);
    }

    static bool readStage(std::FILE* file,ParkStage& stage) {
        uint32_t magic=0;
        if (std::fread(&magic,sizeof magic,1,file)!=1) return false;
        if (magic==0x53504355) { // UCPS from 028
            LegacyStage legacy{};
            if (std::fread(&legacy,sizeof legacy,1,file)!=1) return false;
            stage.smiley=legacy.smiley;
            stage.tint=legacy.tint;
            stage.balloonCount=2;
            stage.ufo=legacy.ufo;
            stage.ufoSpeed=legacy.ufoSpeed;
            std::copy(std::begin(legacy.balloons),std::end(legacy.balloons),stage.balloons);
        } else if (magic==0x32504355) { // UCP2
            if (std::fread(&stage,offsetof(ParkStage,balloons),1,file)!=1 ||
                stage.balloonCount>ParkStage::MaxBalloons ||
                std::fread(stage.balloons,sizeof(ParkBalloon),stage.balloonCount,file)!=stage.balloonCount)
                return false;
        } else {
            return false;
        }

        const auto& ring=stage.smiley;
        if (ring.coinCount>SmileyStage::MaxCoins || ring.redoable>1 ||
            !finite(stage.ufo) || !std::isfinite(stage.ufoSpeed) || stage.ufoSpeed<0) return false;
        if (ring.coinCount) {
            if (!ring.timeTicks || !finite(ring.center) ||
                !std::isfinite(ring.ringRadius) || ring.ringRadius<=0 ||
                !std::isfinite(ring.coinRadius) || ring.coinRadius<=0) return false;
            for (unsigned i=0;i<ring.coinCount;++i) if (!finite(ring.coins[i])) return false;
        }
        for (unsigned i=0;i<stage.balloonCount;++i) {
            const auto& balloon=stage.balloons[i];
            if (!finite(balloon.anchor) || !std::isfinite(balloon.length) || balloon.length<=0 ||
                !std::isfinite(balloon.width) || balloon.width<=0) return false;
        }
        return true;
    }

    static constexpr unsigned TextureBytes=Side*Side*sizeof(uint32_t);
    struct alignas(64) Cache {
        uint32_t texture[Side*Side];
    };
    struct BalloonTrack { Vec2 samples[MotionFrames]; };
    struct FreeCache { void operator()(Cache* value) const { std::free(value); } };
    std::unique_ptr<Cache,FreeCache> cache;
    void* borrowed=nullptr;
    std::unique_ptr<BalloonTrack[]> motion;
    BalloonTrack* motionView=nullptr;
    Patch patches[RingFrames+CoinFrames+UfoFrames+1]{};
    std::string artPath,legacyPath,loadedPath;
    uint8_t* ringPixels=nullptr;
    unsigned ringGeneration=0;
};
static_assert(offsetof(ParkStage,balloons)==188);
