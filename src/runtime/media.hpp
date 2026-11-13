#pragma once

#include "menu_scroll.hpp"
#include "live_result_ids.hpp"
#include "ring_timer_audio.hpp"
#include "compact_audio.hpp"
#include "ride_assets033.hpp"
#include "elysian_wind044_audio.hpp"

#include <pspaudio.h>
#include <pspkernel.h>
#include <zlib.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <memory>
#include <new>

enum class Clip : unsigned { Loading, Title, Main, GameMenu, Win, Loss, Win02, Loss02, Win03, Loss03, Win04, Loss04, Win05, Loss05, WorldResults, Win11, Loss11, Win12, Loss12, Win13, Loss13, Win14, Loss14, Win15, Loss15, Win16, Loss16, Win17, Loss17, Win18, Loss18, Win19, Loss19, Win110, Loss110, Win111, Loss111, Win112, Loss112, Win113, Loss113, Win114, Loss114, Win115, Loss115, Win116, Loss116, Win117, Loss117, Win118, Loss118, Credits, WorldResultsGolf, Win201, Loss201, Win202, Loss202, Win203, Loss203, Win204, Loss204, Win205, Loss205, SettingsBackdrop, Win206, Loss206, Win207, Loss207, Win208, Loss208, Win209, Loss209, Win210, Loss210, Win211, Loss211, Win212, Loss212, Win213, Loss213, Win214, Loss214, Win215, Loss215, Win216, Loss216, Win217, Loss217, Win218, Loss218,
    Win301, Loss301, WorldResultsGlowstick, WorldResultsGlowstickReplay, Win302, Loss302, Win303, Loss303, Win304, Loss304, Win305, Loss305, Win306, Loss306, Win307, Loss307, Win308, Loss308, Win309, Loss309, Win310, Loss310, Win311, Loss311, Loss310Dark, Win312, Loss312, Win313, Loss313, Win314, Loss314, Win315, Loss315, Win316, Loss316, Win317, Loss317, Win318, Loss318, Win401, Loss401, WorldResultsPark, WorldResultsParkReplay, Win402, Loss402, Win403, Loss403, Win404, Loss404, Win405, Loss405, Win406, Loss406, Win407, Loss407, Win408, Loss408, Win409, Loss409, Win410, Loss410, Win411, Loss411, Win412, Loss412, Win413, Loss413, Win414, Loss414, Win415, Loss415, Win416, Loss416, Win417, Loss417, Win418, Loss418, Win501, Loss501, WorldResultsFrontier, WorldResultsFrontierReplay, EasyEnding, Win502, Loss502, FakeWin503, Loss503, Win504, Loss504, Win505, Loss505, Win506, Loss506, Win507, Loss507, Win508, Loss508, Win509, Loss509, Win510, Loss510, Win511, Loss511, Win512, Loss512, Win513, Loss513, Win514, Loss514, Win515, Loss515, Win516, Loss516, Win517, Loss517, Win518, Loss518, Win601, Loss601, Win602, Loss602, Win603, Loss603, Win604, Loss604, Win605, Loss605, Win606, Loss606, Win607, Loss607, Win608, Loss608, Win609, Loss609, Win610, Loss610, Win611, Loss611, Win612, Loss612, Win613, Loss613, Win614, Loss614, Win615, Loss615, Win616, Loss616, Win617, Loss617, Win618, Loss618, Count };
enum class Sound : unsigned {
    Startup, Menu, Course, Select, Back, Join, Grab, Cancel, Shot, Bounce, Win, Loss,
    TitleHit, TitleImpact, TitleCheer, ResultsHit, ResultsBang, RankUncanny, RankAtrocious, RankStinky, RankOk, RankSwag, RankPeak, Coin, TerryClick, SecretKey, WorldClear, Onto1, GolfCentral, Pause, Skip, BlockBreak, WaterSplash, WaterDeath, SkullDeath, KeyCollect, DoorOpen, WaterSplash2, WaterSplash3,
    WorldClearGolf, Onto2, WorldUnlock, WorldHop, WorldJump, GlowstickCity, StopToken, GiftAward, GiftFly, GiftBlast, CarDeath, CarHorn1, CarHorn2, JumpPad, Spotlight, PoliceSiren, SecretRun, Push, OutOfBounds,
    WorldClearCity, Onto3, ChucklePark, WorldWobble,
    SmileyRing, SmileyCoin, SmileyTimer, SmileyPass, SmileyFail, ParkUfo,
    SlotJackpot, SlotHappy, SlotOk, SlotQuestion, SlotPhone, SlotCherry, SlotEye,
    SlotBlue, SlotSad, SlotSkull, SlotGood, SlotLose, ParkSwitch, ParkBlockBreak, SmileyTimerShort, ChuckleDeath, CoasterFlyby, MarcyIntercom, MarcySlurp, MarcyMusic, CoasterFlyby308, ParkLightsOut, ParkAshBaby, CoasterFlyby312, ParkDog, WorldClearPark, Onto4, FinalFrontier, RideCoaster, RideWind,
    RideCoinWarning, RideCoinWarning2, RideCoinWarning3, RideGhostWarning, WorldPhwing, WorldDrumroll, Portal, BigCoin, Nuke, FakeGoalReveal, DangerSiren, FinalGhostWake, FrontierUfo, PrismGateShatter, WorldClearFrontier, WallMusic, ElysianMusic, EasyLullaby, HorseAlarm,
    HorseChargeUp, HorseCharge, HorseBounce, HorseGoalLaunch, HorseHurt1, HorseHurt2, HorseHurt3,
    HorseJumpscare, HorseImpact, HorseFlightBurst, HorseMusic, UncannyGraze, WizardMagic1, WizardMagic2, WizardMagic3, WizardDeath,
    HorseDeathSupreme, HorseMissileBoom, HorseSurprise, HorseKeyHoly, HorseKeyProverb,
    GoldenClubThwack, WallDeath, GlassClink, MoonMoo1, MoonMoo2, MoonMoo3, MoonMoo4, MoonMoo5,
    GoldenTeeMusic, GoodEndingMusic, HorseKeyProverbDelay, CosmicChampionship, StarstormWind, MetronomeBeat, RhythmBlock, PolyrhythmBlock, PolyrhythmBlockOrange, GoalRun, GoalLaugh, GoalGasp, SantaBegin, CosmicEndingMusic, CosmicEndingKiss, CosmicEndingSparkle, HurrahExplosion, HurrahSiren, HurrahUfo, Count
};
enum class StageCue : unsigned { None, Police, SecretRun, Park, ParkShort, Marcy, RideWind, FrontierAlarm, StarstormWind, MovingGoalRun, MovingGoalLaugh };

class ClipPlayer {
public:
    ~ClipPlayer() { if (file) std::fclose(file); }

    bool open(const char* path) {
        if (file) std::fclose(file);
        file=nullptr;
        current=MaxFrames;
        currentMenu=MenuScroll::RecordCount;
        liveResults=false;
        commonResults=false;
        auto incoming=openAssetFile(path);
        auto* source=incoming.get();
        uint32_t header[3]{};
        if (!source || std::fseek(source,0,SEEK_END)!=0) return false;
        const long size=std::ftell(source);
        if (size<0 || static_cast<unsigned long>(size)>UINT32_MAX ||
            std::fseek(source,0,SEEK_SET)!=0 ||
            std::fread(header,sizeof header,1,source)!=1 ||
            (header[0]!=LegacyMagic && header[0]!=DynamicMagic && header[0]!=DeltaMagic) ||
            header[1]!=ClipCount || !header[2] || header[2]>MaxFrames ||
            std::fread(clips,sizeof clips,1,source)!=1 ||
            std::fread(frames,sizeof(Frame),header[2],source)!=header[2]) return false;

        const uint32_t total=header[2], bytes=static_cast<uint32_t>(size);
        uint32_t first=0,end=sizeof header+sizeof clips+total*sizeof(Frame);
        bool dynamic=false,commonDynamic=false;
        for (unsigned id=0;id<ClipCount;++id) {
            const auto& clip=clips[id];
            if (clip.first!=first || clip.count>total-first) return false;
            if (!clip.count) {
                const bool pilot=isPilotResultClip040(id) || isNativeLossClip041(id),common=!pilot && isCommonWinClip040(id);
                if (header[0]==LegacyMagic || (!pilot && !common)) return false;
                dynamic=true;
                commonDynamic|=common;
            }
            for (uint32_t index=0;index<clip.count;++index) {
                const Frame& record=frames[first+index];
                const unsigned length=record.length(),distance=record.distance();
                if (record.offset!=end || length>sizeof compressed || (!length && !distance) ||
                    record.offset>bytes || length>bytes-record.offset) return false;
                if (distance && (header[0]!=DeltaMagic || distance>MaxDistance || distance>index ||
                    frames[first+index-distance].distance())) return false;
                end+=length;
            }
            first+=clip.count;
        }
        if (first!=total || end!=bytes) return false;
        liveResults=dynamic;
        commonResults=commonDynamic;
        file=incoming.release();
        return true;
    }

    template<unsigned Bytes>
    uint8_t* textureWorkspace(unsigned& generation) {
        static_assert(Bytes<=sizeof pixels);
        current=MaxFrames;
        currentMenu=MenuScroll::RecordCount;
        workspaceActive=true;
        generation=workspaceGeneration;
        return reinterpret_cast<uint8_t*>(pixels);
    }

    template<unsigned Bytes>
    uint8_t* compressedTextureWorkspace(unsigned& generation) {
        static_assert(Bytes<=sizeof compressed);
        textureWorkspace<0>(generation);
        return compressed;
    }

    void useRideWorkspace(RideArt033& ride) {
        static_assert(RideArt033::BackgroundBytes+RideArt033::CompressedBytes<=sizeof compressed);
        unsigned generation=0;
        auto* memory=textureWorkspace<RideArt033::TrackBytes>(generation);
        ride.useWorkspace(reinterpret_cast<uint16_t*>(memory),compressed,generation);
    }

    unsigned duration(Clip id) const { return count(id)*50; }
    unsigned count(Clip id) const {
        const unsigned index=static_cast<unsigned>(id);
        return file && index<ClipCount?clips[index].count:0;
    }
    bool hasLiveResults() const { return liveResults; }
    bool hasCommonResults() const { return commonResults; }
    void resetTextureWorkspace() { leaveTextureWorkspace(); }

    bool menu(const char* path,unsigned index,MenuScroll::Textures& art) {
        leaveTextureWorkspace();
        using namespace MenuScroll;
        static_assert(TileBytes+HudBytes+StripBytes+BackBytes<=FrameBytes && ForegroundBytes<=sizeof compressed);
        if (index>=RecordCount) return false;
        auto* tile=reinterpret_cast<unsigned char*>(pixels);
        auto* hud=tile+TileBytes;
        auto* strip=hud+HudBytes;
        if (currentMenu!=index) {
            current=MaxFrames;
            currentMenu=RecordCount;
            const auto input=openAssetFile(path);
            auto* bank=input.get();
            if (!bank || std::fseek(bank,4,SEEK_SET)!=0 || std::fread(tile,TileBytes,1,bank)!=1 ||
                std::fseek(bank,recordOffset(index),SEEK_SET)!=0 ||
                std::fread(hud,HudBytes,1,bank)!=1 || std::fread(compressed,ForegroundBytes,1,bank)!=1 ||
                std::fread(strip,StripBytes,1,bank)!=1) return false;
            sceKernelDcacheWritebackRange(tile,TileBytes+HudBytes+StripBytes);
            sceKernelDcacheWritebackRange(compressed,ForegroundBytes);
            currentMenu=index;
        }
        art={tile,hud,compressed,strip,strip+StripBytes};
        return true;
    }

    const uint16_t* frame(Clip id,unsigned timeMs,bool repeat=false) {
        leaveTextureWorkspace();
        currentMenu=MenuScroll::RecordCount;
        if (!count(id)) return nullptr;
        const Section& s=clips[static_cast<unsigned>(id)];
        unsigned i=timeMs/50;
        if (i>=s.count) {
            const unsigned loopStart=s.count>20?s.count-20:0;
            i=repeat ? loopStart+(i-loopStart)%(s.count-loopStart) : s.count-1;
        }
        const unsigned index=s.first+i;
        if (index==current) return pixels;
        const Frame& record=frames[index];
        const unsigned distance=record.distance(),key=index-distance;
        const bool reuseKey=distance && current==key;
        current=MaxFrames;
        if (!reuseKey && !decodeKey(frames[key])) return nullptr;
        if (distance && record.length() && !applyDelta(record)) return nullptr;
        sceKernelDcacheWritebackRange(pixels,FrameBytes);
        current=index;
        return pixels;
    }

private:
    static constexpr uint32_t LegacyMagic=0x33414355, DynamicMagic=0x34414355, DeltaMagic=0x35414355;
    static constexpr unsigned ClipCount=static_cast<unsigned>(Clip::Count);
    static constexpr unsigned MaxFrames=14400;
    static constexpr unsigned FrameBytes=512*272*2;
    static constexpr unsigned SizeBits=20, SizeMask=(1u<<SizeBits)-1, MaxDistance=9;
    static constexpr unsigned DeltaScratchBytes=4096, InflateAllocationLimit=48*1024;
    struct Section { uint32_t first,count; };
    struct Frame {
        uint32_t offset,packedLength;
        unsigned length() const { return packedLength&SizeMask; }
        unsigned distance() const { return packedLength>>SizeBits; }
    };
    static_assert(sizeof(Section)==8 && sizeof(Frame)==8);
    static_assert(FrameBytes+1024<SizeMask);

    struct Inflater {
        z_stream stream{};
        size_t available=InflateAllocationLimit;
        bool ready=false;

        Inflater(Bytef* input,unsigned bytes) {
            stream.zalloc=allocate;
            stream.zfree=release;
            stream.opaque=&available;
            ready=inflateInit(&stream)==Z_OK;
            if (ready) {
                stream.next_in=input;
                stream.avail_in=bytes;
            }
        }

        ~Inflater() { if (ready) inflateEnd(&stream); }

        bool complete(int result,unsigned bytes) const {
            return result==Z_STREAM_END && stream.total_out==FrameBytes && stream.total_in==bytes;
        }

    private:
        static voidpf allocate(voidpf opaque,uInt count,uInt size) {
            auto& remaining=*static_cast<size_t*>(opaque);
            if (!count || size>remaining/count) return nullptr;
            auto* memory=std::calloc(count,size);
            if (memory) remaining-=static_cast<size_t>(count)*size;
            return memory;
        }

        static void release(voidpf,voidpf memory) { std::free(memory); }
    };

    std::FILE* file=nullptr;
    Section clips[ClipCount]{};
    Frame frames[MaxFrames]{};
    alignas(64) uint16_t pixels[FrameBytes/sizeof(uint16_t)]{};
    alignas(64) unsigned char compressed[FrameBytes+1024]{};
    unsigned workspaceGeneration=0;
    bool workspaceActive=false;
    bool liveResults=false;
    bool commonResults=false;

    bool readPayload(const Frame& record) {
        return std::fseek(file,record.offset,SEEK_SET)==0 &&
            std::fread(compressed,1,record.length(),file)==record.length();
    }

    bool decodeKey(const Frame& record) {
        if (!readPayload(record)) return false;
        Inflater decoder(compressed,record.length());
        if (!decoder.ready) return false;
        decoder.stream.next_out=reinterpret_cast<Bytef*>(pixels);
        decoder.stream.avail_out=FrameBytes;
        const int result=inflate(&decoder.stream,Z_FINISH);
        return decoder.complete(result,record.length());
    }

    bool applyDelta(const Frame& record) {
        if (!readPayload(record)) return false;
        Inflater decoder(compressed,record.length());
        if (!decoder.ready) return false;
        alignas(4) unsigned char difference[DeltaScratchBytes];
        auto* output=reinterpret_cast<unsigned char*>(pixels);
        unsigned cursor=0;
        int result=Z_OK;
        while (result==Z_OK) {
            const unsigned remaining=FrameBytes-cursor;
            const unsigned capacity=std::min(DeltaScratchBytes,remaining?remaining:1u);
            decoder.stream.next_out=difference;
            decoder.stream.avail_out=capacity;
            result=inflate(&decoder.stream,Z_NO_FLUSH);
            const unsigned produced=capacity-decoder.stream.avail_out;
            if (produced>remaining) return false;
            for (unsigned i=0;i<produced;++i) output[cursor+i]^=difference[i];
            cursor+=produced;
        }
        return decoder.complete(result,record.length());
    }

    void leaveTextureWorkspace() {
        if (workspaceActive) ++workspaceGeneration;
        workspaceActive=false;
    }
    unsigned current=MaxFrames, currentMenu=MenuScroll::RecordCount;
};


enum class AudioProbePhase : unsigned { Off, Profile, Osk60, Osk30, Victory, Title, Count };

struct AudioProbeTiming {
    std::atomic<unsigned> blocks{0},readMax{0},mixMax{0},gapMax{0},late{0};
    std::atomic<unsigned> priming{0},underruns{0},effectUnderruns{0},readErrors{0};
    std::atomic<int> outputError{0};
    std::atomic<unsigned> musicIoUs{0},effectIoUs{0},musicIoCalls{0},effectIoCalls{0};
    std::atomic<unsigned> musicIoMax{0},effectIoMax{0},musicWorkMax{0},effectWorkMax{0};
};

class AudioMixer {
public:
    void probe(AudioProbePhase phase) { probePhase.store(phase,std::memory_order_relaxed); }
    const AudioProbeTiming& timing(AudioProbePhase phase) const {
        return timings[static_cast<unsigned>(phase)];
    }

#ifdef UCG_VICTORY_PROBE
    void resetTiming(AudioProbePhase phase) {
        auto& sample=timings[static_cast<unsigned>(phase)];
        sample.blocks=sample.readMax=sample.mixMax=sample.gapMax=sample.late=0;
        sample.priming=sample.underruns=sample.effectUnderruns=sample.readErrors=0;
        sample.outputError=0;
        sample.musicIoUs=sample.effectIoUs=sample.musicIoCalls=sample.effectIoCalls=0;
        sample.musicIoMax=sample.effectIoMax=sample.musicWorkMax=sample.effectWorkMax=0;
    }
#endif

    ~AudioMixer() { stop(); }

    bool open(const char* path,const char* slotPath,const char* parkPath,const char* ringPath) {
        if (files[0] || compact.ready()) return false;
        openFailure=nullptr;
        if (!loadBanks(path,slotPath,parkPath)) {
            stop();
            return false;
        }
        if (!ringTimers.open(ringPath)) {
            openFailure="ring_timers";
            stop();
            return false;
        }
        channel=sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,Block,PSP_AUDIO_FORMAT_STEREO);
        if (channel<0) {
            openFailure="reserve_channel";
            stop();
            return false;
        }
        requested.store(0,std::memory_order_relaxed);
        stageCueCommand.store(0,std::memory_order_relaxed);
        consumed.store(0,std::memory_order_relaxed);
        produced.store(0,std::memory_order_relaxed);
        for (auto& command:pending) command.store(0,std::memory_order_relaxed);
        for (auto& word:stopping) word.store(0,std::memory_order_relaxed);
        musicTuning.store(UnityTune,std::memory_order_relaxed);
        musicCursorTrack.store(0,std::memory_order_relaxed);
        musicCursor.store(0,std::memory_order_relaxed);
        active.store(true,std::memory_order_release);
        reader=startThread("ucg-pcm-reader",readerMain,0x2e);
        if (reader>=0) thread=startThread("ucg-mixer",threadMain,0x12);
        if (thread>=0) return true;
        openFailure="start_threads";
        stop();
        return false;
    }

    void play(Sound id) {
        const unsigned index=static_cast<unsigned>(id);
        if (index<Count) pending[index].store(PlainEffect,std::memory_order_relaxed);
    }

    void playTuned(Sound id,float gainDb,float pitch) {
        const unsigned index=static_cast<unsigned>(id);
        if (index>=Count) return;
        if (!std::isfinite(gainDb)) gainDb=0;
        if (!std::isfinite(pitch)) pitch=1;
        const float gain=std::pow(10.0f,std::clamp(gainDb,-96.0f,12.0f)/20.0f);
        pending[index].store(packTuning(gain,std::clamp(pitch,0.5f,3.0f)),std::memory_order_relaxed);
    }

    void stop(Sound sound) {
        const unsigned index=static_cast<unsigned>(sound);
        if (index<Count) stopping[index/32].fetch_or(1u<<(index%32),std::memory_order_relaxed);
    }

    void stageCue(StageCue cue) {
        const unsigned current=stageCueCommand.load(std::memory_order_relaxed);
        if ((current&CueMask)==static_cast<unsigned>(cue)) return;
        stageCueCommand.store(((current&~CueStateMask)+CueStateMask+1)|static_cast<unsigned>(cue),
                              std::memory_order_release);
    }

    void restartStageCue(StageCue cue) {
        const unsigned current=stageCueCommand.load(std::memory_order_relaxed);
        stageCueCommand.store(((current&~CueStateMask)+CueStateMask+1)|static_cast<unsigned>(cue),
                              std::memory_order_release);
    }

    void pauseStageCue(bool paused) {
        if (paused) stageCueCommand.fetch_or(CuePause,std::memory_order_relaxed);
        else stageCueCommand.fetch_and(~CuePause,std::memory_order_relaxed);
    }

    void playRingTimer(unsigned slot,unsigned ticks) { ringTimers.play(slot,ticks); }
    void stopRingTimers() { ringTimers.stop(); }

    void music(Sound id) {
        requestTrack(static_cast<unsigned>(id)+1);
        holdMusic.store(false,std::memory_order_release);
    }
    void primeMusic(Sound id) {
        holdMusic.store(true,std::memory_order_release);
        requestTrack(static_cast<unsigned>(id)+1);
    }
    void muteMusic() {
        requestTrack(0);
        holdMusic.store(false,std::memory_order_release);
    }
    void musicTune(float gain,float pitch) {
        if (!std::isfinite(gain)) gain=1;
        if (!std::isfinite(pitch)) pitch=1;
        musicTuning.store(packTuning(std::clamp(gain,0.0f,4.0f),std::clamp(pitch,0.0f,4.0f)),
                          std::memory_order_relaxed);
    }
    float musicPositionSeconds() const { return musicPositionFrames()/44100.0f; }
    unsigned musicPositionFrames(Sound source=Sound::Count) const {
        const unsigned track=requested.load(std::memory_order_acquire);
        const unsigned id=track&TrackMask;
        if (!id || (source!=Sound::Count && id!=static_cast<unsigned>(source)+1) ||
            track!=musicCursorTrack.load(std::memory_order_acquire)) return 0;
        return musicCursor.load(std::memory_order_relaxed);
    }
    unsigned blocks() const { return audible.load(std::memory_order_relaxed); }
    unsigned effectMemoryBytes() const { return effectBytes; }
    const char* failure() const { return openFailure; }

    void rideVolume(float music,float cartDb,float windDb,float windPitch=0) {
        const auto gain=[](float db) { return unsigned(256*std::min(1.0f,std::pow(10.0f,db/20))); };
        const unsigned master=unsigned(256*std::clamp(music,0.0f,1.0f));
        rideGains.store(master|(gain(cartDb)<<9)|(gain(windDb)<<18),std::memory_order_relaxed);
        rideWindStep.store(static_cast<unsigned>(windPitch*WizardWindUnity+0.5f),std::memory_order_relaxed);
    }

    void stop() {
        active.store(false,std::memory_order_release);
        joinThread(thread);
        joinThread(reader);
        if (channel>=0) {
            sceAudioChRelease(channel);
            channel=-1;
        }
        for (auto*& source:files) {
            if (source) std::fclose(source);
            source=nullptr;
        }
        compact.close();
        for (auto& effect:effects) effect={};
        ringTimers.close();
        effectPcm.reset();
        effectStreams.reset();
        effectStreamCount=0;
        effectBytes=0;
        fileOffset=UINT32_MAX;
    }

private:
    static constexpr unsigned Count=static_cast<unsigned>(Sound::Count);
    static constexpr unsigned SlotFirst=static_cast<unsigned>(Sound::SlotJackpot);
    static constexpr unsigned ParkFirst=static_cast<unsigned>(Sound::ParkSwitch);
    static constexpr unsigned MaxEffectBytes=3520*1024;

    static constexpr unsigned effectBank(unsigned id) {
        return id>=ParkFirst?2:id>=SlotFirst?1:0;
    }
    static constexpr unsigned CueMask=15,CuePause=16,CueStateMask=CueMask|CuePause;
    static constexpr unsigned CommandWords=(Count+31)/32;
    static constexpr unsigned GainBits=11,GainMask=(1u<<GainBits)-1,PlainEffect=1;
    static constexpr unsigned UnityTune=(WizardWindUnity<<GainBits)|256;
    static unsigned packTuning(float gain,float pitch) {
        return (static_cast<unsigned>(pitch*WizardWindUnity+0.5f)<<GainBits) |
                static_cast<unsigned>(gain*256+0.5f);
    }
    static constexpr unsigned Block=1024;
    static constexpr unsigned PcmSlots=8,PcmFrames=4096,TrackMask=0xff;
    static constexpr unsigned PrefixFrames=4096, ResidentFrames=12288;
    static constexpr unsigned EffectSlots=2, EffectFrames=2048, MusicLowWater=3;
    static constexpr unsigned MaxActiveEffectStreams=32;
    static_assert(Count<TrackMask && (PcmSlots&(PcmSlots-1))==0);
    static_assert(PrefixFrames%Block==0 && EffectFrames%Block==0);
    struct Entry { uint32_t offset,frames,stream,loop; };
    template<unsigned Frames> struct PcmPage {
        alignas(64) int16_t samples[Frames*2];
        unsigned request,frames;
        bool last;
    };
    struct EffectStream {
        PcmPage<EffectFrames> pcm[EffectSlots];
        // The reader owns produced and the two source fields. The mixer owns consumed and request.
        std::atomic<unsigned> produced{0},consumed{0},request{0};
        unsigned sourceRequest=0,sourceFrame=0;
    };
    struct Effect {
        const int16_t* prefix=nullptr;
        unsigned prefixFrames=0;
        bool streamed=false;
    };
    RingTimerAudio ringTimers;
    ucg::packed_audio::Bank<Count> compact;
    Entry entries[Count]{};
    Effect effects[Count]{};
    std::unique_ptr<int16_t[]> effectPcm;
    std::unique_ptr<EffectStream[]> effectStreams;
    unsigned effectStreamCount=0;
    unsigned voiceSlots[Count]{};
    unsigned effectBytes=0;
    const char* openFailure=nullptr;
    std::FILE* files[3]{};
    unsigned fileIndex=0;
    uint32_t fileOffset=UINT32_MAX;
    int channel=-1,thread=-1,reader=-1;
    std::atomic<bool> active{false};
    std::atomic<unsigned> requested{0},stageCueCommand{0};
    // Each cursor has one writer. Release/acquire also protects the page contents.
    PcmPage<PcmFrames> pcm[PcmSlots]{};
    std::atomic<unsigned> produced{0},consumed{0};
    std::atomic<uint32_t> pending[Count]{};
    std::atomic<unsigned> musicTuning{UnityTune},musicCursorTrack{0},musicCursor{0};
    std::atomic<bool> holdMusic{false};
    std::atomic<uint32_t> stopping[CommandWords]{};
    std::atomic<unsigned> audible{0};
    std::atomic<unsigned> rideGains{256},rideWindStep{0};
    alignas(64) int16_t buffers[2][Block*2]{};
    int32_t work[Block*2]{};
    std::atomic<AudioProbePhase> probePhase{AudioProbePhase::Off};
    AudioProbeTiming timings[static_cast<unsigned>(AudioProbePhase::Count)]{};

    static void recordMax(std::atomic<unsigned>& maximum,unsigned value) {
        if (value>maximum.load(std::memory_order_relaxed))
            maximum.store(value,std::memory_order_relaxed);
    }

    static bool alwaysResident(unsigned index) {
        switch (static_cast<Sound>(index)) {
        case Sound::Shot:
        case Sound::Coin:
        case Sound::StopToken:
        case Sound::CarDeath:
        case Sound::CarHorn1:
        case Sound::CarHorn2:
        case Sound::JumpPad:
            return true;
        default:
            return false;
        }
    }

    static unsigned streamedPrefix(unsigned index) {
        switch (static_cast<Sound>(index)) {
        case Sound::Startup:
        case Sound::TitleHit:
        case Sound::TitleImpact:
        case Sound::TitleCheer:
        case Sound::ResultsHit:
        case Sound::ResultsBang:
        case Sound::RankUncanny:
        case Sound::RankAtrocious:
        case Sound::RankStinky:
        case Sound::RankOk:
        case Sound::RankSwag:
        case Sound::RankPeak:
        case Sound::WorldUnlock:
        case Sound::WorldHop:
        case Sound::WorldJump:
        case Sound::WorldWobble:
            return EffectFrames;
        default:
            return index>=static_cast<unsigned>(Sound::WorldClearCity)?3*Block:PrefixFrames;
        }
    }

    bool openBank(unsigned bank,const char* path,unsigned first,unsigned count) {
        auto*& source=files[bank];
        source=std::fopen(path,"rb");
        if (!source) return false;
        std::setvbuf(source,nullptr,_IOFBF,16384);
        uint32_t header[2];
        if (std::fseek(source,0,SEEK_END)!=0) return false;
        const long size=std::ftell(source);
        if (size<0 || std::fseek(source,0,SEEK_SET)!=0 ||
            std::fread(header,sizeof header,1,source)!=1 || header[0]!=0x33534355 ||
            header[1]!=count || std::fread(entries+first,sizeof(Entry),count,source)!=count) return false;
        uint32_t end=8+count*sizeof(Entry);
        for (unsigned i=first;i<first+count;++i) {
            const auto& entry=entries[i];
            if (entry.offset!=end || !entry.frames || entry.stream>1 || entry.loop>1 ||
                (bank==1 && (entry.stream || entry.loop)) || entry.offset>static_cast<uint32_t>(size) ||
                entry.frames>(static_cast<uint32_t>(size)-entry.offset)/4) return false;
            end+=entry.frames*4;
        }
        return end==static_cast<uint32_t>(size);
    }

    bool loadBanks(const char* path,const char* slotPath,const char* parkPath) {
        using Packed = ucg::packed_audio::Bank<Count>;
        openFailure="compact_audio_bank";
        const auto format=compact.open(path);
        if (format==Packed::Open::Invalid) return false;
        if (format==Packed::Open::Ready) {
            for (unsigned id=0;id<Count;++id)
                entries[id]={0,compact.frames(id),compact.stream(id),compact.loop(id)};
        } else {
            openFailure="audio_bank";
            if (!openBank(0,path,0,SlotFirst)) return false;
            openFailure="slots_audio_bank";
            if (!openBank(1,slotPath,SlotFirst,ParkFirst-SlotFirst)) return false;
            openFailure="park_audio_bank";
            if (!openBank(2,parkPath,ParkFirst,Count-ParkFirst)) return false;
        }
        openFailure="effect_budget";

        constexpr unsigned count=Count, byteLimit=MaxEffectBytes;
        unsigned prefixFrames=0,streamCount=0;
        for (unsigned i=0;i<count;++i) {
            const auto& entry=entries[i];
            if (entry.stream) continue;
            const bool newEffect=i>=static_cast<unsigned>(Sound::RideCoaster);
            const bool resident=entry.frames<=(newEffect?Block:ResidentFrames) || alwaysResident(i);
            const unsigned legacyPrefix=streamedPrefix(i);
            const unsigned prefix=newEffect?Block:legacyPrefix;
            const unsigned frames=resident?entry.frames:std::min<unsigned>(entry.frames,prefix);
            if (frames>byteLimit/4-prefixFrames) return false;
            effects[i].prefixFrames=frames;
            prefixFrames+=frames;
            streamCount+=!resident;
        }
        streamCount=std::min(streamCount,MaxActiveEffectStreams);
        const size_t bytes=prefixFrames*4+streamCount*sizeof(EffectStream);
        if (bytes>byteLimit) return false;

        openFailure="effect_prefix_memory";
        effectPcm.reset(new(std::nothrow) int16_t[prefixFrames*2]);
        if (!effectPcm) return false;
        openFailure="effect_stream_memory";
        effectStreams.reset(new(std::nothrow) EffectStream[streamCount]);
        if (streamCount && !effectStreams) return false;
        openFailure="effect_prefix_read";
        int16_t* prefix=effectPcm.get();
        effectStreamCount=streamCount;
        std::fill_n(voiceSlots,Count,UINT32_MAX);
        for (unsigned i=0;i<count;++i) {
            Effect& effect=effects[i];
            if (!effect.prefixFrames) continue;
            effect.prefix=prefix;
            effect.streamed=effect.prefixFrames<entries[i].frames;
            if (readPcm(i,0,prefix,effect.prefixFrames)!=effect.prefixFrames) return false;
            prefix+=effect.prefixFrames*2;
        }
        effectBytes=static_cast<unsigned>(bytes);
        openFailure=nullptr;
        return true;
    }

    int startThread(const char* name,int (*entry)(SceSize,void*),int priority) {
        const int id=sceKernelCreateThread(name,entry,priority,0x3000,0,nullptr);
        if (id<0) return -1;
        AudioMixer* self=this;
        if (sceKernelStartThread(id,sizeof self,&self)>=0) return id;
        sceKernelDeleteThread(id);
        return -1;
    }

    static void joinThread(int& id) {
        if (id<0) return;
        sceKernelWaitThreadEnd(id,nullptr);
        sceKernelDeleteThread(id);
        id=-1;
    }

    void requestTrack(unsigned track) {
        const unsigned old=requested.load(std::memory_order_relaxed);
        if ((old&TrackMask)==track) return;
        musicTuning.store(UnityTune,std::memory_order_relaxed);
        // The generation distinguishes mute -> restart of the same track from its old PCM.
        requested.store(((old+TrackMask+1)&~TrackMask)|track,std::memory_order_release);
    }

    static int readerMain(SceSize,void* arg) {
        return (*static_cast<AudioMixer**>(arg))->readAudio();
    }

    static int threadMain(SceSize,void* arg) {
        return (*static_cast<AudioMixer**>(arg))->run();
    }

    // Only the reader uses the archive after the effect prefixes are loaded.
    unsigned readPcm(unsigned id,unsigned position,int16_t* samples,unsigned frames) {
        const auto phase=probePhase.load(std::memory_order_relaxed);
        auto* sample=phase==AudioProbePhase::Off?nullptr:&timings[static_cast<unsigned>(phase)];
        const unsigned start=sample?sceKernelGetSystemTimeLow():0;
        unsigned count=0;
        if (compact.ready()) {
            using Bank=ucg::packed_audio::Bank<Count>;
            Bank::IoTiming io{};
            compact.measureIo(sample?&io:nullptr);
            const bool music=entries[id].stream!=0;
            count=music?compact.readMusic(id,position,samples,frames):compact.readEffect(id,position,samples,frames);
            compact.measureIo(nullptr);
            if (sample) {
                const unsigned elapsed=sceKernelGetSystemTimeLow()-start;
                (music?sample->musicIoCalls:sample->effectIoCalls).fetch_add(io.calls,std::memory_order_relaxed);
                (music?sample->musicIoUs:sample->effectIoUs).fetch_add(io.microseconds,std::memory_order_relaxed);
                recordMax(music?sample->musicIoMax:sample->effectIoMax,io.microseconds);
                recordMax(music?sample->musicWorkMax:sample->effectWorkMax,
                          elapsed>io.microseconds?elapsed-io.microseconds:0);
            }
        } else {
            const unsigned bank=effectBank(id);
            const uint32_t offset=entries[id].offset+position*4;
            auto* source=files[bank];
            if ((fileIndex==bank && fileOffset==offset) || std::fseek(source,offset,SEEK_SET)==0)
                count=std::fread(samples,4,frames,source);
            fileIndex=bank;
            fileOffset=count==frames?offset+count*4:UINT32_MAX;
            if (count!=frames) std::clearerr(source);
        }
        if (sample) {
            recordMax(sample->readMax,sceKernelGetSystemTimeLow()-start);
            if (count!=frames) sample->readErrors.fetch_add(1,std::memory_order_relaxed);
        }
        if (count!=frames) sceKernelDelayThread(10000);
        return count;
    }

    void fillMusic(unsigned request,unsigned& position,bool& finished,unsigned write) {
        const unsigned track=(request&TrackMask)-1;
        const Entry& entry=entries[track];
        auto& page=pcm[write&(PcmSlots-1)];
        unsigned filled=0;
        while (filled<PcmFrames) {
            if (position==entry.frames) {
                if (!entry.loop) break;
                position=compact.ready()?compact.loopFrame(track):
                         track==static_cast<unsigned>(Sound::FinalFrontier)?833490:0;
            }
            const unsigned size=std::min<unsigned>(entry.frames-position,PcmFrames-filled);
            const unsigned count=readPcm(track,position,page.samples+filled*2,size);
            filled+=count;
            position+=count;
            if (count!=size || requested.load(std::memory_order_acquire)!=request ||
                !active.load(std::memory_order_acquire)) break;
        }
        finished=!entry.loop && position==entry.frames;
        if (filled && requested.load(std::memory_order_acquire)==request) {
            page.request=request;
            page.frames=filled;
            page.last=finished;
            produced.store(write+1,std::memory_order_release);
        }
    }

    bool fillEffect(unsigned& next,bool emptyOnly) {
        for (unsigned i=0;i<effectStreamCount;++i) {
            const unsigned slot=(next+i)%effectStreamCount;
            auto& stream=effectStreams[slot];
            const unsigned request=stream.request.load(std::memory_order_acquire);
            const unsigned voice=request&TrackMask;
            if (!voice) continue;
            const unsigned index=voice-1;
            const auto& effect=effects[index];
            if (request!=stream.sourceRequest) {
                stream.sourceRequest=request;
                stream.sourceFrame=effect.prefixFrames;
            }
            const auto& entry=entries[index];
            const unsigned write=stream.produced.load(std::memory_order_relaxed);
            const unsigned queued=write-stream.consumed.load(std::memory_order_acquire);
            if (stream.sourceFrame==entry.frames || queued>=EffectSlots || (emptyOnly && queued))
                continue;

            auto& page=stream.pcm[write%EffectSlots];
            const unsigned frames=std::min<unsigned>(entry.frames-stream.sourceFrame,EffectFrames);
            const unsigned count=readPcm(index,stream.sourceFrame,page.samples,frames);
            if (count==frames && stream.request.load(std::memory_order_acquire)==request) {
                page.request=request;
                page.frames=count;
                stream.sourceFrame+=count;
                stream.produced.store(write+1,std::memory_order_release);
            }
            next=(slot+1)%effectStreamCount;
            return true;
        }
        return false;
    }

    int readAudio() {
        unsigned current=0,position=0,nextEffect=0;
        bool finished=false;
        while (active.load(std::memory_order_acquire)) {
            const unsigned request=requested.load(std::memory_order_acquire);
            if (request!=current) {
                compact.endMusic();
                current=request;
                position=0;
                finished=false;
            }
            const unsigned track=request&TrackMask;
            const unsigned write=produced.load(std::memory_order_relaxed);
            const unsigned queued=write-consumed.load(std::memory_order_acquire);
            const bool music=track && entries[track-1].stream && !finished && queued<PcmSlots;
            if (music && queued<MusicLowWater) fillMusic(request,position,finished,write);
            else if (fillEffect(nextEffect,true) || fillEffect(nextEffect,false)) continue;
            else if (music) fillMusic(request,position,finished,write);
            else sceKernelDelayThread(2000);
        }
        return 0;
    }

    unsigned takeMusic(int16_t* mix,unsigned request,unsigned& offset,bool& finished,
                       unsigned& played) {
        musicPage(request,offset,finished);
        unsigned filled=0;
        unsigned read=consumed.load(std::memory_order_relaxed);
        const unsigned write=produced.load(std::memory_order_acquire);
        while (read!=write && filled<Block) {
            const auto& slot=pcm[read&(PcmSlots-1)];
            if (slot.request==request) {
                const unsigned size=std::min(slot.frames-offset,Block-filled);
                std::memcpy(mix+filled*2,slot.samples+offset*2,size*4);
                filled+=size;
                offset+=size;
                if (offset<slot.frames) break;
                finished=slot.last;
            }
            offset=0;
            consumed.store(++read,std::memory_order_release);
        }
        played+=filled;
        return filled;
    }

    const PcmPage<PcmFrames>* musicPage(unsigned request,unsigned& offset,bool& finished) {
        unsigned read=consumed.load(std::memory_order_relaxed);
        const unsigned write=produced.load(std::memory_order_acquire);
        while (read!=write) {
            const auto& page=pcm[read&(PcmSlots-1)];
            if (page.request!=request) {
                offset=0;
            } else if (offset<page.frames) {
                return &page;
            } else {
                offset-=page.frames;
                if (page.last) {
                    offset=0;
                    finished=true;
                }
            }
            consumed.store(++read,std::memory_order_release);
        }
        return nullptr;
    }

    unsigned takePitchedMusic(int16_t* mix,unsigned request,unsigned& offset,bool& finished,
                              unsigned& played,unsigned& fraction,unsigned step) {
        unsigned output=0;
        if (!step) return 0;
        while (output<Block) {
            const auto* page=musicPage(request,offset,finished);
            if (!page) break;
            const auto* samples=page->samples+2*offset;
            const unsigned available=page->frames-offset;
            unsigned input=0;
            for (;output<Block && input<available;++output) {
                const auto* from=samples+2*input;
                const auto* to=from;
                if (fraction) {
                    if (input+1<available) {
                        to=from+2;
                    } else if (!page->last) {
                        const unsigned read=consumed.load(std::memory_order_relaxed);
                        if (produced.load(std::memory_order_acquire)!=read+1) {
                            const auto& next=pcm[(read+1)&(PcmSlots-1)];
                            if (next.request==request) to=next.samples;
                        }
                    }
                }
                for (unsigned ch=0;ch<2;++ch)
                    mix[2*output+ch]=static_cast<int16_t>(wizardWindSample044(from[ch],to[ch],fraction));
                fraction+=step;
                input+=fraction>>WizardWindFractionBits;
                fraction&=WizardWindUnity-1;
            }
            played+=page->last?std::min(input,available):input;
            offset+=input;
        }
        // Retire a page crossed by the final output sample before taking the next block.
        musicPage(request,offset,finished);
        return output;
    }

    static StageCue cueFor(unsigned index) {
        if (index==static_cast<unsigned>(Sound::SmileyTimer) || index==static_cast<unsigned>(Sound::ParkUfo)) return StageCue::Park;
        if (index==static_cast<unsigned>(Sound::SmileyTimerShort)) return StageCue::ParkShort;
        if (index==static_cast<unsigned>(Sound::MarcyMusic)) return StageCue::Marcy;
        if (index==static_cast<unsigned>(Sound::SecretRun)) return StageCue::SecretRun;
        if (index==static_cast<unsigned>(Sound::Spotlight) ||
            index==static_cast<unsigned>(Sound::PoliceSiren)) return StageCue::Police;
        if (index==static_cast<unsigned>(Sound::RideWind)) return StageCue::RideWind;
        if (index==static_cast<unsigned>(Sound::DangerSiren)) return StageCue::FrontierAlarm;
        if (index==static_cast<unsigned>(Sound::StarstormWind)) return StageCue::StarstormWind;
        if (index==static_cast<unsigned>(Sound::GoalRun)) return StageCue::MovingGoalRun;
        if (index==static_cast<unsigned>(Sound::GoalLaugh)) return StageCue::MovingGoalLaugh;
        return StageCue::None;
    }

    // Only the mixer assigns a slot to a voice. The reader sees the voice and generation together.
    static void requestEffect(EffectStream& stream,unsigned voice) {
        const unsigned old=stream.request.load(std::memory_order_relaxed);
        stream.consumed.store(stream.produced.load(std::memory_order_acquire),std::memory_order_release);
        stream.request.store(((old&~TrackMask)+TrackMask+1)|voice,std::memory_order_release);
    }

    EffectStream* streamFor(unsigned index,bool restart=false) {
        if (!effects[index].streamed) return nullptr;
        const unsigned assigned=voiceSlots[index];
        if (assigned<effectStreamCount) {
            auto& stream=effectStreams[assigned];
            if (restart) requestEffect(stream,index+1);
            return &stream;
        }
        for (unsigned slot=0;slot<effectStreamCount;++slot) {
            auto& stream=effectStreams[slot];
            if (stream.request.load(std::memory_order_relaxed)&TrackMask) continue;
            voiceSlots[index]=slot;
            requestEffect(stream,index+1);
            return &stream;
        }
        return nullptr;
    }

    void releaseStream(unsigned index) {
        const unsigned slot=voiceSlots[index];
        if (slot>=effectStreamCount) return;
        requestEffect(effectStreams[slot],0);
        voiceSlots[index]=UINT32_MAX;
    }

    void setEffect(unsigned index,unsigned& position,bool playing) {
        position=playing?0:entries[index].frames;
        if (playing) streamFor(index,true);
        else releaseStream(index);
    }

    const PcmPage<EffectFrames>* effectPage(EffectStream& stream) {
        const unsigned request=stream.request.load(std::memory_order_relaxed);
        unsigned read=stream.consumed.load(std::memory_order_relaxed);
        const unsigned write=stream.produced.load(std::memory_order_acquire);
        while (read!=write) {
            const auto& page=stream.pcm[read%EffectSlots];
            if (page.request==request) return &page;
            stream.consumed.store(++read,std::memory_order_release);
        }
        return nullptr;
    }

    bool effectReady(unsigned index,unsigned position) {
        const Effect& effect=effects[index];
        if (position>=entries[index].frames || position<effect.prefixFrames) return true;
        auto* stream=streamFor(index);
        return stream && effectPage(*stream);
    }

    bool mixEffect(unsigned index,unsigned& position,unsigned gain) {
        const Effect& effect=effects[index];
        if (position>=entries[index].frames) return true;
        const int16_t* samples=nullptr;
        unsigned frames=0;
        if (position<effect.prefixFrames) {
            samples=effect.prefix+position*2;
            frames=std::min(effect.prefixFrames-position,Block);
        } else {
            auto* stream=streamFor(index);
            const auto* page=stream?effectPage(*stream):nullptr;
            if (!page) return false;
            const unsigned offset=(position-effect.prefixFrames)%EffectFrames;
            samples=page->samples+offset*2;
            frames=std::min(page->frames-offset,Block);
        }
        if (gain==256) {
            for (unsigned i=0;i<frames*2;++i) work[i]+=samples[i];
        } else {
            for (unsigned i=0;i<frames*2;++i) work[i]+=int32_t(samples[i])*int32_t(gain)/256;
        }
        position+=frames;
        if (position==entries[index].frames) {
            releaseStream(index);
        } else if (effect.streamed && position>effect.prefixFrames &&
                   (position-effect.prefixFrames)%EffectFrames==0) {
            // The slot has already been assigned when its page is consumed.
            auto& stream=effectStreams[voiceSlots[index]];
            const unsigned read=stream.consumed.load(std::memory_order_relaxed);
            stream.consumed.store(read+1,std::memory_order_release);
        }
        return true;
    }

    bool mixPitchedEffect(unsigned index,unsigned& position,unsigned& fraction,unsigned step,unsigned gain,
                          bool loop) {
        const Effect& effect=effects[index];
        for (unsigned output=0;output<Block;) {
            if (position==entries[index].frames) {
                if (!loop) return true;
                setEffect(index,position,true);
            }
            const int16_t* samples;
            unsigned available;
            EffectStream* stream=nullptr;
            if (position<effect.prefixFrames) {
                samples=effect.prefix+position*2;
                available=effect.prefixFrames-position;
            } else {
                stream=streamFor(index);
                const auto* page=stream?effectPage(*stream):nullptr;
                if (!page) return false;
                const unsigned offset=(position-effect.prefixFrames)%EffectFrames;
                samples=page->samples+offset*2;
                available=page->frames-offset;
            }
            unsigned input=0;
            for (;output<Block && input<available;++output) {
                const unsigned next=std::min(input+1,available-1);
                for (unsigned channel=0;channel<2;++channel)
                    work[2*output+channel]+=wizardWindSample044(samples[2*input+channel],samples[2*next+channel],fraction)*
                                           static_cast<int>(gain)/256;
                fraction+=step;
                input+=fraction>>WizardWindFractionBits;
                fraction&=WizardWindUnity-1;
            }
            position=std::min<unsigned>(position+input,entries[index].frames);
            if (position==entries[index].frames) releaseStream(index);
            else if (stream && input>=available) {
                const unsigned read=stream->consumed.load(std::memory_order_relaxed);
                stream->consumed.store(read+1,std::memory_order_release);
            }
        }
        return true;
    }

    int run() {
        RingTimerAudio::Voice ringVoices[RingTimerAudio::Voices]{};
        unsigned position[Count];
        for (unsigned i=0;i<Count;++i) position[i]=entries[i].frames;
        unsigned current=0,offset=0,buffer=0,previousSubmit=0,previousCue=0;
        bool started=false,finished=false,cartLoop=false;
        unsigned musicFraction=0,musicPlayed=0;
        unsigned effectFraction[Count]{},effectTuning[Count]{};
        AudioProbePhase previousPhase=AudioProbePhase::Off;
        while (active.load(std::memory_order_acquire)) {
            auto& mix=buffers[buffer];
            const AudioProbePhase phase=probePhase.load(std::memory_order_relaxed);
            auto* sample=phase==AudioProbePhase::Off?nullptr:&timings[static_cast<unsigned>(phase)];
            const unsigned mixStart=sample?sceKernelGetSystemTimeLow():0;
            const unsigned request=requested.load(std::memory_order_acquire);
            if (request!=current) {
                current=request;
                started=false;
                finished=false;
                offset=musicFraction=musicPlayed=0;
                musicCursor.store(0,std::memory_order_relaxed);
                musicCursorTrack.store(request,std::memory_order_release);
            }
            const unsigned tuning=musicTuning.load(std::memory_order_relaxed);
            const unsigned step=holdMusic.load(std::memory_order_acquire)?0:tuning>>GainBits;
            const unsigned track=request&TrackMask;
            const unsigned filled=step==WizardWindUnity && !musicFraction?
                takeMusic(mix,request,offset,finished,musicPlayed):
                takePitchedMusic(mix,request,offset,finished,musicPlayed,musicFraction,step);
            if (track && musicPlayed>=entries[track-1].frames) {
                const auto& entry=entries[track-1];
                if (entry.loop) {
                    const unsigned loopStart=compact.ready()?compact.loopFrame(track-1):
                        track-1==static_cast<unsigned>(Sound::FinalFrontier)?833490:0;
                    musicPlayed=loopStart+(musicPlayed-entry.frames)%(entry.frames-loopStart);
                } else musicPlayed=entry.frames;
            }
            musicCursor.store(musicPlayed,std::memory_order_relaxed);
            if (sample && step && track && entries[track-1].stream && filled<Block && !finished) {
                auto& missing=started||filled?sample->underruns:sample->priming;
                missing.fetch_add(1,std::memory_order_relaxed);
            }
            started|=filled!=0;
            std::fill(mix+filled*2,mix+Block*2,0);
            const unsigned ride=rideGains.load(std::memory_order_relaxed);
            const int32_t gain=(ride&511)*(tuning&GainMask)/256;
            if (gain==256) std::copy(mix,mix+Block*2,work);
            else for (unsigned i=0;i<Block*2;++i) work[i]=int32_t(mix[i])*gain/256;
            constexpr unsigned cart=static_cast<unsigned>(Sound::RideCoaster);
            constexpr unsigned wind=static_cast<unsigned>(Sound::RideWind);
            uint32_t stops[CommandWords];
            for (unsigned i=0;i<CommandWords;++i) stops[i]=stopping[i].exchange(0,std::memory_order_relaxed);
            const unsigned cueState=stageCueCommand.load(std::memory_order_acquire);
            const bool cueChanged=(cueState&~CueStateMask)!=(previousCue&~CueStateMask);
            previousCue=cueState;
            const auto activeCue=static_cast<StageCue>(cueState&CueMask);
            const bool cuePaused=cueState&CuePause;
            const unsigned windStep=rideWindStep.load(std::memory_order_relaxed);
            for (unsigned i=0;i<Count;++i) {
                const unsigned command=pending[i].load(std::memory_order_relaxed)?
                    pending[i].exchange(0,std::memory_order_relaxed):0;
                if (!effects[i].prefix) continue;
                const auto cue=cueFor(i);
                if (cue!=StageCue::None && cueChanged) {
                    effectFraction[i]=effectTuning[i]=0;
                    setEffect(i,position[i],cue==activeCue);
                }
                if (stops[i/32]&(1u<<(i%32))) {
                    if (i==cart) cartLoop=false;
                    setEffect(i,position[i],false);
                    continue;
                }
                if (cue!=StageCue::None && cuePaused) continue;
                if (command &&
                    (i!=static_cast<unsigned>(Sound::Push) || position[i]==entries[i].frames)) {
                    if (i==cart) cartLoop=true;
                    effectFraction[i]=0;
                    effectTuning[i]=command==PlainEffect?0:command;
                    setEffect(i,position[i],true);
                }
            }

            constexpr unsigned spot=static_cast<unsigned>(Sound::Spotlight);
            constexpr unsigned siren=static_cast<unsigned>(Sound::PoliceSiren);
            const bool policeWaiting=activeCue==StageCue::Police && !cuePaused &&
                (!effectReady(spot,position[spot]) || !effectReady(siren,position[siren]));
            bool effectMissing=policeWaiting;
            for (unsigned i=0;i<Count;++i) {
                if (!effects[i].prefix) continue;
                const auto cue=cueFor(i);
                if ((cue!=StageCue::None && (cuePaused || (cue==StageCue::Police && policeWaiting))) ||
                    ((i==static_cast<unsigned>(Sound::CoasterFlyby) ||
                     i==static_cast<unsigned>(Sound::CoasterFlyby308) ||
                     i==static_cast<unsigned>(Sound::CoasterFlyby312) ||
                     i==static_cast<unsigned>(Sound::RideCoaster)) && cuePaused)) continue;
                const bool windEffect=i==wind || i==static_cast<unsigned>(Sound::StarstormWind);
                const bool loop=(i==cart && cartLoop) || (windEffect && activeCue==cue) ||
                    (i==static_cast<unsigned>(Sound::DangerSiren) && activeCue==StageCue::FrontierAlarm) ||
                    ((cue==StageCue::MovingGoalRun || cue==StageCue::MovingGoalLaugh) && activeCue==cue);
                if (loop && position[i]==entries[i].frames) setEffect(i,position[i],true);
                constexpr unsigned horn1=static_cast<unsigned>(Sound::CarHorn1);
                constexpr unsigned horn2=static_cast<unsigned>(Sound::CarHorn2);
                constexpr unsigned hornGain=81; // Authored -10 dB in Q8.
                const unsigned tuning=effectTuning[i];
                const unsigned effectGain=tuning?tuning&GainMask:i==cart?(ride>>9)&511:windEffect?(ride>>18)&511:
                    (i==horn1 || i==horn2)?hornGain:cue==StageCue::MovingGoalRun?102:256;
                const unsigned pitch=windEffect && loop && windStep?windStep:tuning>>GainBits;
                if ((windEffect && loop && windStep) || (pitch && pitch!=WizardWindUnity)) {
                    if (!mixPitchedEffect(i,position[i],effectFraction[i],pitch,effectGain,loop))
                        effectMissing=true;
                } else if (!mixEffect(i,position[i],effectGain)) effectMissing=true;
            }
            if (sample && effectMissing) sample->effectUnderruns.fetch_add(1,std::memory_order_relaxed);
            ringTimers.mix(ringVoices,work,Block,cuePaused);
            bool nonzero=false;
            for (unsigned i=0;i<Block*2;++i) {
                mix[i]=static_cast<int16_t>(std::clamp<int32_t>(work[i],-32768,32767));
                nonzero|=mix[i]!=0;
            }
            sceKernelDcacheWritebackRange(mix,sizeof mix);
            if (sample) {
                const unsigned submit=sceKernelGetSystemTimeLow();
                recordMax(sample->mixMax,submit-mixStart);
                if (phase==previousPhase) {
                    const unsigned gap=submit-previousSubmit;
                    recordMax(sample->gapMax,gap);
                    constexpr unsigned blockUs=Block*1000000u/44100u;
                    if (gap>blockUs+2000) sample->late.fetch_add(1,std::memory_order_relaxed);
                }
                previousSubmit=submit;
                sample->blocks.fetch_add(1,std::memory_order_relaxed);
            }
            previousPhase=phase;
            const int result=sceAudioOutputBlocking(channel,PSP_AUDIO_VOLUME_MAX,mix);
            if (result<0) {
                if (sample) sample->outputError.store(result,std::memory_order_relaxed);
                active.store(false,std::memory_order_release);
                return 1;
            }
            // The channel can still be consuming this buffer after the call returns.
            buffer^=1u;
            if (nonzero) audible.fetch_add(1,std::memory_order_relaxed);
        }
        return 0;
    }
};
