#pragma once

#include "cat_artist_book.hpp"

namespace ArtistInk {

alignas(64) constexpr uint32_t Palette[24]={
    0,
    GU_RGBA(0x00,0x00,0x00,255u),GU_RGBA(0xff,0xff,0xff,255u),
    GU_RGBA(0xa3,0xa3,0xa3,255u),GU_RGBA(0x66,0x66,0x66,255u),
    GU_RGBA(0xff,0x00,0x00,255u),GU_RGBA(0xff,0x70,0x00,255u),
    GU_RGBA(0xff,0x9f,0x00,255u),GU_RGBA(0xff,0xef,0x00,255u),
    GU_RGBA(0xae,0xff,0x00,255u),GU_RGBA(0x10,0xff,0x00,255u),
    GU_RGBA(0x00,0xff,0x9f,255u),GU_RGBA(0x00,0xff,0xff,255u),
    GU_RGBA(0x00,0x9f,0xff,255u),GU_RGBA(0x00,0x10,0xff,255u),
    GU_RGBA(0x70,0x00,0xff,255u),GU_RGBA(0x9f,0x00,0xff,255u),
    GU_RGBA(0xef,0x00,0xff,255u),GU_RGBA(0xff,0x00,0x51,255u),
    GU_RGBA(0x6e,0x30,0x00,255u),GU_RGBA(0x09,0x8d,0x00,255u),
};

void swizzle(const ArtistBook::Canvas& canvas,void* dest) {
    auto* pixels=static_cast<uint8_t*>(dest);
    for (unsigned y=0;y<ArtistBook::Side;y+=8) {
        for (unsigned x=0;x<ArtistBook::Side;x+=16) {
            for (unsigned line=0;line<8;++line) {
                std::memcpy(pixels,canvas.data()+ArtistBook::index(x,y+line),16);
                pixels+=16;
            }
        }
    }
}

void bind(const void* pixels) {
    sceGuClutMode(GU_PSM_8888,0,255,0);
    sceGuClutLoad(3,Palette);
    setTexture(GU_PSM_T8,ArtistBook::Side,ArtistBook::Side,pixels);
}

constexpr Patch Full{0,0,128,128,-64,-64};

} // namespace ArtistInk
