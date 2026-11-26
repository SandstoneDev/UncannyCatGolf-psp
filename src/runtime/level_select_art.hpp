#pragma once

#include "level_select.hpp"
#include "visual.hpp"
#include "asset_file.hpp"
#include "storage_hooks.hpp"

#include <cstddef>
#include <string>

#include <cstdio>
#include <pspkernel.h>
#include <pspiofilemgr.h>

class LevelSelectArt {
public:
    static constexpr unsigned Worlds=LevelSelectState::WorldCount, Cards=18, States=13;
    static constexpr unsigned Size=512, Rows=272;
    static constexpr unsigned Locked=6, FramesPerLanguage=Worlds*(Worlds+1)/2;
    static constexpr unsigned FirstFocused=7;

    bool open(const char* filename) {
        const auto input=openAssetFile(filename);
        auto* file=input.get();
        uint32_t magic=0;
        if (!file || std::fread(&magic,sizeof magic,1,file)!=1 || (magic!=0x324c4355 && magic!=0x334c4355) ||
            std::fread(cards,sizeof cards,1,file)!=1 ||
            std::fseek(file,0,SEEK_END)!=0 ||
            std::ftell(file)!=static_cast<long>(FramesOffset+2*FramesPerLanguage*FrameBytes)) return false;
        for (unsigned w=0;w<Worlds;++w) {
            unsigned normalRows=0,focusRows=0;
            for (unsigned i=0;i<LevelSelectState::cells(w);++i) {
                for (unsigned state=0;state<States;++state) {
                    const auto& p=cards[w][i][state];
                    if (p.u<0 || p.v<0 || p.width<=0 || p.height<=0 || p.u+p.width>static_cast<int>(Size) ||
                        p.v+p.height>static_cast<int>(Size) || p.x<136 || p.y<0 ||
                        p.x+p.width>408 || p.y+p.height>static_cast<int>(Rows)) return false;
                    auto& used=state<FirstFocused?normalRows:focusRows;
                    used=std::max(used,static_cast<unsigned>(p.v+p.height));
                }
            }
            normalBytes[w]=((normalRows+7)&~7u)*Size*2;
            focusBytes[w]=((focusRows+7)&~7u)*Size*2;
        }
        virtualSource=ucg_storage::isVirtual(file);
        cleanBack=magic==0x334c4355;
        path=filename;
        panelTags[0]=panelTags[1]=0;
        for (auto& atlas:atlases) atlas.tag=0;
        return true;
    }

    struct CardAtlas {
        alignas(64) uint16_t normal[Size*Size]{};
        alignas(64) uint16_t focus[Size*Rows]{};
        unsigned tag=0;
    };

    static constexpr unsigned VictoryWorkspaceBytes=Size*(Size+Rows)*sizeof(uint16_t);
    uint8_t* cardWorkspace(unsigned slot) {
        static_assert(offsetof(CardAtlas,tag)==VictoryWorkspaceBytes);
        atlases[slot].tag=0;
        return reinterpret_cast<uint8_t*>(&atlases[slot]);
    }

    uint8_t* victoryWorkspace() { return cardWorkspace(0); }

    uint8_t* panelWorkspace(unsigned slot=0) {
        panelTags[slot]=0;
        return reinterpret_cast<uint8_t*>(panelPixels(slot));
    }
    static constexpr unsigned PanelBytes=Size*Rows*sizeof(uint16_t);
    alignas(64) uint16_t base[Size*Rows]{};
    void invalidateBase() { panelTags[0]=0; }

    uint8_t* victoryPixels() {
        atlases[1].tag=0;
        return reinterpret_cast<uint8_t*>(atlases[1].normal);
    }

    template<unsigned Bytes>
    uint32_t* rankWorkspace() {
        static_assert(Bytes<=sizeof(atlases[1].focus));
        atlases[1].tag=0;
        return reinterpret_cast<uint32_t*>(atlases[1].focus);
    }

    struct ReadCount { unsigned calls=0,bytes=0; };
    ReadCount reads() const { return readCount; }
    ReadCount prefetchReads() const { return prefetched; }

    const CardAtlas* loadCards(unsigned world,unsigned keep=LevelSelectState::NoCourse) {
        if (world>=Worlds) return nullptr;
        if (const auto* cached=findCards(world)) return cached;
        auto& atlas=destination(keep);
        atlas.tag=0;
        const auto input=openAssetFile(path.c_str());
        auto* file=input.get();
        if (!read(file,normalOffset(world),atlas.normal,normalBytes[world]) ||
            !read(file,focusOffset(world),atlas.focus,focusBytes[world])) return nullptr;
        commitCards(world,atlas);
        return &atlas;
    }

    bool ready(unsigned world,unsigned latest,bool russian) const {
        return findCards(world) && findPanel(panelIndex(world,latest,russian));
    }

    // Keep the async request in a staging buffer. Switching profiles or leaving
    // the screen can discard that request without letting I/O touch a live texture.
    bool prefetch(unsigned world,unsigned latest,bool russian,unsigned keep=Worlds,bool allowRead=true) {
        const unsigned panel=panelIndex(world,latest,russian);
        if (prefetchWorld!=world || prefetchPanel!=panel || prefetchKeep!=keep) {
            cancelPrefetch();
            prefetchWorld=world;
            prefetchPanel=panel;
            prefetchKeep=keep;
            prefetchPanelSlot=panelDestination(keep<Worlds?panelIndex(keep,latest,russian)+1:0);
        }
        pollPrefetch();
        if (ready(world,latest,russian)) return true;
        if (!allowRead || operation!=Operation::Idle) return false;

        const auto part=prefetchPart();
        if (asyncFailed || virtualSource) {
            if (failure) return false;
            if (!fallbackStream) fallbackStream=openAssetFile(path.c_str());
            if (!read(fallbackStream.get(),part.offset,part.pixels,part.bytes,true)) {
                failure=true;
                return false;
            }
            acceptPrefetched(part);
        } else if (descriptor<0) {
            descriptor=sceIoOpenAsync(path.c_str(),PSP_O_RDONLY,0);
            if (descriptor<0) failAsync("open",descriptor);
            else operation=Operation::Open;
        } else if (filePosition!=part.offset) {
            const int result=sceIoLseek32Async(descriptor,part.offset,PSP_SEEK_SET);
            if (result<0) failAsync("seek",result);
            else {
                operation=Operation::Seek;
                seekTarget=part.offset;
            }
        } else {
            const int result=sceIoReadAsync(descriptor,staging,part.bytes);
            if (result<0) failAsync("read",result);
            else {
                operation=Operation::Read;
                reading=part;
                discardRead=false;
            }
        }
        return ready(world,latest,russian);
    }

    void pollPrefetch() {
        if (operation==Operation::Idle) return;
        SceInt64 value=0;
        const int status=sceIoPollAsync(descriptor,&value);
        if (status>0) return;
        const auto completed=operation;
        operation=Operation::Idle;
        if (status<0 || value<0) {
            failAsync(completed==Operation::Open?"open":completed==Operation::Seek?"seek":"read",
                      status<0?status:static_cast<int>(value));
            return;
        }
        if (completed==Operation::Open) filePosition=0;
        else if (completed==Operation::Seek) {
            if (value!=seekTarget) failAsync("seek_position",static_cast<int>(value));
            else filePosition=seekTarget;
        }
        else {
            if (value!=reading.bytes) {
                failAsync("short_read",static_cast<int>(value));
                return;
            }
            filePosition+=reading.bytes;
            if (!discardRead) {
                sceKernelDcacheInvalidateRange(staging,reading.bytes);
                std::memcpy(reading.pixels,staging,reading.bytes);
                ++prefetched.calls;
                prefetched.bytes+=reading.bytes;
                acceptPrefetched(reading);
            }
        }
    }

    void cancelPrefetch() {
        discardRead=true;
        fallbackStream.reset();
        prefetchWorld=Worlds;
        position=0;
        failure=false;
    }

    void closePrefetch() {
        cancelPrefetch();
        if (descriptor<0) return;
        if (operation!=Operation::Idle) {
            SceInt64 ignored;
            sceIoWaitAsync(descriptor,&ignored);
            operation=Operation::Idle;
        }
        sceIoClose(descriptor);
        descriptor=-1;
    }

    bool prefetchInFlight() const { return operation!=Operation::Idle; }
    bool prefetchFailed() const { return failure; }
    const char* prefetchErrorStep() const { return asyncErrorStep; }
    int prefetchError() const { return asyncError; }

    static unsigned panelIndex(unsigned world,unsigned latest,bool russian) {
        return static_cast<unsigned>(russian)*FramesPerLanguage+latest*(latest+1)/2+world;
    }

    bool hasCleanBack() const { return cleanBack; }

    const uint16_t* loadBase(unsigned world,unsigned latest,bool russian) {
        if (world>=Worlds || latest>=Worlds || world>latest) return nullptr;
        const unsigned frame=panelIndex(world,latest,russian);
        if (const auto* cached=findPanel(frame)) return cached;
        const auto slot=panelDestination(0);
        auto* pixels=panelWorkspace(slot);
        const auto input=openAssetFile(path.c_str());
        if (!read(input.get(),FramesOffset+frame*FrameBytes,pixels,FrameBytes)) return nullptr;
        sceKernelDcacheWritebackRange(pixels,FrameBytes);
        panelTags[slot]=frame+1;
        return panelPixels(slot);
    }

    bool openGo(const char* filename) {
        const auto input=openAssetFile(filename);
        auto* file=input.get();
        if (!file || std::fseek(file,0,SEEK_END)!=0 ||
            std::ftell(file)!=static_cast<long>(2*FramesPerLanguage*sizeof goPixels)) return false;
        goPath=filename;
        goTag=0;
        return true;
    }

    const uint16_t* goFrame(unsigned world,unsigned latest,bool russian) {
        if (latest>=Worlds || world>latest) return nullptr;
        const auto index=panelIndex(world,latest,russian);
        if (goTag!=index+1) {
            goTag=0;
            const auto input=openAssetFile(goPath.c_str());
            auto* file=input.get();
            if (!read(file,index*sizeof goPixels,goPixels,sizeof goPixels)) return nullptr;
            sceKernelDcacheWritebackRange(goPixels,sizeof goPixels);
            goTag=index+1;
        }
        return goPixels;
    }

    Patch cards[Worlds][Cards][States]{};

private:
    static constexpr unsigned AtlasOffset=4+sizeof cards;
    static constexpr unsigned AtlasBytes=Size*Size*2, FrameBytes=Size*Rows*2;
    static constexpr unsigned FocusOffset=AtlasOffset+Worlds*AtlasBytes;
    static constexpr unsigned FramesOffset=FocusOffset+Worlds*FrameBytes;
    static unsigned normalOffset(unsigned world) { return AtlasOffset+world*AtlasBytes; }
    static unsigned focusOffset(unsigned world) { return FocusOffset+world*FrameBytes; }

    const CardAtlas* findCards(unsigned world) const {
        for (const auto& atlas:atlases) if (atlas.tag==world+1) return &atlas;
        return nullptr;
    }

    CardAtlas& destination(unsigned keep) {
        return atlases[keep<Worlds && atlases[0].tag==keep+1 ? 1 : 0];
    }

    uint16_t* panelPixels(unsigned slot) { return slot?neighbour:base; }
    const uint16_t* panelPixels(unsigned slot) const { return slot?neighbour:base; }

    const uint16_t* findPanel(unsigned panel) const {
        for (unsigned i=0;i<2;++i)
            if (panelTags[i]==panel+1) return panelPixels(i);
        return nullptr;
    }

    unsigned panelDestination(unsigned keepTag) const {
        if (!panelTags[0]) return 0;
        if (!panelTags[1]) return 1;
        return panelTags[0]==keepTag?1:0;
    }

    void commitCards(unsigned world,CardAtlas& atlas) {
        sceKernelDcacheWritebackRange(atlas.normal,normalBytes[world]);
        sceKernelDcacheWritebackRange(atlas.focus,focusBytes[world]);
        atlas.tag=world+1;
    }

    bool read(std::FILE* file,unsigned offset,void* target,unsigned bytes,bool ahead=false) {
        if (!file || std::fseek(file,offset,SEEK_SET)!=0 || std::fread(target,1,bytes,file)!=bytes) return false;
        auto& count=ahead?prefetched:readCount;
        ++count.calls;
        count.bytes+=bytes;
        return true;
    }

    static constexpr unsigned Chunk=64*1024;
    enum class Operation { Idle, Open, Seek, Read };
    struct PrefetchPart {
        unsigned offset=0,bytes=0;
        uint8_t* pixels=nullptr;
        CardAtlas* atlas=nullptr;
    };

    PrefetchPart prefetchPart() {
        const unsigned world=prefetchWorld;
        if (!findPanel(prefetchPanel)) {
            return {FramesOffset+prefetchPanel*FrameBytes+position,std::min(FrameBytes-position,Chunk),
                    panelWorkspace(prefetchPanelSlot)+position,nullptr};
        }
        auto& atlas=destination(prefetchKeep);
        atlas.tag=0;
        const bool focus=position>=normalBytes[world];
        const unsigned pos=focus?position-normalBytes[world]:position;
        const unsigned offset=(focus?focusOffset(world):normalOffset(world))+pos;
        const unsigned size=(focus?focusBytes[world]:normalBytes[world])-pos;
        return {offset,std::min(size,Chunk),reinterpret_cast<uint8_t*>(focus?atlas.focus:atlas.normal)+pos,&atlas};
    }

    void acceptPrefetched(const PrefetchPart& part) {
        position+=part.bytes;
        if (part.atlas) {
            if (position==normalBytes[prefetchWorld]+focusBytes[prefetchWorld]) {
                commitCards(prefetchWorld,*part.atlas);
                position=0;
            }
        } else if (position==FrameBytes) {
            sceKernelDcacheWritebackRange(panelPixels(prefetchPanelSlot),FrameBytes);
            panelTags[prefetchPanelSlot]=prefetchPanel+1;
            position=0;
        }
    }

    void failAsync(const char* step,int error) {
        if (descriptor>=0) sceIoClose(descriptor);
        descriptor=-1;
        asyncFailed=true;
        asyncErrorStep=step;
        asyncError=error;
    }

    std::string path,goPath;
    AssetFile fallbackStream;
    SceUID descriptor=-1;
    Operation operation=Operation::Idle;
    unsigned filePosition=0,seekTarget=0;
    bool discardRead=false,asyncFailed=false,failure=false,virtualSource=false;
    const char* asyncErrorStep=nullptr;
    int asyncError=0;
    PrefetchPart reading{};
    alignas(64) uint8_t staging[Chunk]{};
    unsigned normalBytes[Worlds]{},focusBytes[Worlds]{};
    unsigned prefetchWorld=Worlds,prefetchPanel=0,position=0;
    ReadCount readCount{},prefetched{};
    CardAtlas atlases[2]{};
    alignas(64) uint16_t goPixels[64*64]{};
    alignas(64) uint16_t neighbour[Size*Rows]{};
    unsigned panelTags[2]{},goTag=0;
    unsigned prefetchKeep=Worlds,prefetchPanelSlot=0;
    bool cleanBack=false;
};
