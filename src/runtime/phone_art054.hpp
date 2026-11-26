// Generated from Windows Mystery Phone resources by pack_phone054.py.
#pragma once

#include "visual.hpp"

namespace PhoneArt054 {

constexpr unsigned Width = 512, Height = 512, Side = 272;
constexpr unsigned PlateBytes = 278528;
constexpr unsigned OverlayWidth = 512, OverlayHeight = 256;
constexpr unsigned OverlayBytes = 524288;
constexpr const char* PlateFile[] = {"phone-plate-en054.bin", "phone-plate-ru054.bin"};
constexpr const char* OverlayFile = "phone-overlays054.bin";
struct Rect { int x, y, width, height; };
constexpr Rect Lcd{136, 30, 102, 34};
constexpr Rect Digits{140, 30, 94, 34};
constexpr Rect DialogueText{8, 153, 255, 85};
constexpr Rect Keys[] = {
    {136, 68, 32, 32},
    {170, 68, 32, 32},
    {204, 68, 32, 32},
    {136, 102, 32, 32},
    {170, 102, 32, 32},
    {204, 102, 32, 32},
    {136, 136, 32, 32},
    {170, 136, 32, 32},
    {204, 136, 32, 32},
    {136, 170, 32, 32},
    {170, 170, 32, 32},
    {204, 170, 32, 32},
};
constexpr const char* KeyValues[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "delete", "0", "call"};
constexpr unsigned Delete = 9, Zero = 10, Call = 11;

constexpr Patch Pressed[] = {
    {0, 163, 35, 31, 134, 70},
    {35, 163, 35, 31, 168, 70},
    {70, 163, 35, 31, 202, 70},
    {105, 163, 35, 31, 134, 104},
    {140, 163, 35, 31, 168, 104},
    {175, 163, 35, 31, 202, 104},
    {210, 163, 35, 31, 134, 138},
    {245, 163, 35, 31, 168, 138},
    {280, 163, 35, 31, 202, 138},
    {315, 163, 35, 31, 134, 172},
    {350, 163, 35, 31, 168, 172},
    {385, 163, 35, 31, 202, 172},
};

constexpr Patch Fragment[] = {
    {420, 163, 19, 17, 20, 174},
    {439, 163, 19, 17, 46, 174},
    {458, 163, 19, 17, 71, 174},
    {477, 163, 19, 16, 20, 196},
    {0, 194, 19, 16, 46, 196},
    {19, 194, 19, 16, 71, 196},
    {38, 194, 19, 16, 20, 217},
    {57, 194, 19, 16, 46, 217},
};

constexpr Patch Scanline[] = {
    {269, 0, 102, 34, 136, 30},
    {371, 0, 102, 34, 136, 30},
};

constexpr Patch Dialogue{0, 0, 269, 163, 2, 78};
constexpr Patch Next{473, 0, 28, 29, 241, 240};
constexpr Patch Focus{76, 194, 34, 34, 0, 0};

} // namespace PhoneArt054
