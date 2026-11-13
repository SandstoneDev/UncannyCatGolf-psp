#pragma once

#include "visual.hpp"
#include "menu_scroll.hpp"
#include "asset_file.hpp"

#include <string>

#include <cstdio>
#include <pspkernel.h>

enum class ProfilePart : unsigned {
    Card, CardFocus, CardPeak, CardPeakFocus,
    TitleFocus, PlayerFocus, TitleFilled, PlayerFilled, TitleFilledFocus, PlayerFilledFocus,
    Confirm, ConfirmFocus, Load, ChillOff, ChillOn, ChillOffFocus, ChillOnFocus, Count
};

class ProfileArt {
public:
    static constexpr unsigned Size=512, Rows=272;

    bool open(const char* filename) {
        const auto input=openAssetFile(filename);
        auto* file=input.get();
        uint32_t magic=0;
        if (!file || std::fread(&magic,sizeof magic,1,file)!=1 || (magic!=0x31464355 && magic!=0x32464355 && magic!=0x33464355) ||
            std::fread(parts,sizeof parts,1,file)!=1 || std::fread(atlas,sizeof atlas,1,file)!=1 ||
            std::fseek(file,0,SEEK_END)!=0 || std::ftell(file)!=static_cast<long>(FrameOffset+4*FrameBytes))
            return false;
        for (const auto& locale:parts) {
            for (const auto& p:locale) {
                if (p.u<0 || p.v<0 || p.width<0 || p.height<0 || p.u+p.width>static_cast<int>(Size) ||
                    p.v+p.height>static_cast<int>(Size) || p.x<0 || p.y<0 ||
                    p.x+p.width>408 || p.y+p.height>static_cast<int>(Rows)) return false;
            }
        }
        iconSize=0;
        if (magic!=0x31464355) {
            std::memcpy(&backSize,backData(),sizeof backSize);
            if (backSize<MenuScroll::BackBytes+MenuScroll::BackTableBytes ||
                backSize>(BackLabelRow-BackDataRow)*Size*2-sizeof backSize) return false;
        } else backSize=0;
        if (magic==0x33464355) {
            std::memcpy(&iconSize,iconData(),sizeof iconSize);
            if (!iconSize || iconSize>(BackLabelRow-IconRow)*Size*2-sizeof iconSize ||
                backSize>(IconRow-BackDataRow)*Size*2-sizeof backSize) return false;
        }
        path=filename;
        current=0;
        sceKernelDcacheWritebackRange(atlas,sizeof atlas);
        return true;
    }

    bool load(bool russian,bool create) {
        const unsigned frame=2*static_cast<unsigned>(russian)+static_cast<unsigned>(create);
        if (frame+1!=current) {
            current=0;
            const auto input=openAssetFile(path.c_str());
            auto* file=input.get();
            if (!file || std::fseek(file,FrameOffset+frame*FrameBytes,SEEK_SET)!=0 ||
                std::fread(base,FrameBytes,1,file)!=1) return false;
            sceKernelDcacheWritebackRange(base,FrameBytes);
            current=frame+1;
        }
        return true;
    }

    const Patch& part(bool russian,ProfilePart id) const { return parts[russian][static_cast<unsigned>(id)]; }

    bool hasBackArt() const { return backSize!=0; }
    unsigned saveIconSize() const { return iconSize; }
    const unsigned char* saveIcon() const { return iconData()+sizeof iconSize; }

    bool backFrame(unsigned elapsedMs,void* pixels) const {
        return backSize && MenuScroll::decodeBackFrames(backData()+sizeof backSize,backSize,elapsedMs,pixels);
    }

    const void* backLabel(bool russian) const {
        return atlas+Size*BackLabelRow+russian*(MenuScroll::BackStride*32*2);
    }

    alignas(64) uint16_t atlas[Size*Size]{},base[Size*Rows]{};

private:
    Patch parts[2][static_cast<unsigned>(ProfilePart::Count)]{};
    static constexpr unsigned BackDataRow=280, IconRow=400, BackLabelRow=496;
    unsigned backSize=0, iconSize=0;

    const unsigned char* iconData() const {
        return reinterpret_cast<const unsigned char*>(atlas+Size*IconRow);
    }

    const unsigned char* backData() const {
        return reinterpret_cast<const unsigned char*>(atlas+Size*BackDataRow);
    }

    static constexpr unsigned FrameOffset=4+sizeof parts+sizeof atlas;
    static constexpr unsigned FrameBytes=Size*Rows*2;
    std::string path;
    unsigned current=0;
};

class MenuButtonArt {
public:
    bool open(const char* filename) {
        if (!MenuScroll::valid(filename)) return false;
        path=filename;
        current=2;
        return true;
    }

    bool load(bool russian) {
        using namespace MenuScroll;
        const unsigned locale=russian;
        if (current==locale) return true;
        current=2;
        const auto file=openAssetFile(path.c_str());
        if (!file || std::fseek(file.get(),buttonOffset(locale),SEEK_SET)!=0 ||
            std::fread(patches,sizeof patches,1,file.get())!=1 ||
            std::fread(atlas,sizeof atlas,1,file.get())!=1) return false;
        for (const auto& button:patches) {
            for (const auto& p:button) {
                if (p.u<0 || p.v<0 || p.width<=0 || p.height<=0 ||
                    p.u+p.width>static_cast<int>(ButtonWidth) || p.v+p.height>static_cast<int>(ButtonRows) ||
                    p.x<static_cast<int>(HudWidth) || p.y<0 ||
                    p.x+p.width>static_cast<int>(ViewWidth) || p.y+p.height>static_cast<int>(Height)) return false;
            }
        }
        sceKernelDcacheWritebackRange(atlas,sizeof atlas);
        current=locale;
        return true;
    }

    const Patch& part(unsigned button,bool pressed) const { return patches[button][pressed]; }

    alignas(64) uint32_t atlas[MenuScroll::ButtonWidth*MenuScroll::ButtonRows]{};

private:
    Patch patches[MenuScroll::ButtonCount][2]{};
    std::string path;
    unsigned current=2;
};
