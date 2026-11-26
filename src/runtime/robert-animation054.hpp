#pragma once

#include <cstdint>

namespace ucg_club054 {

struct RobertFrame { uint16_t variant, ticks; };
struct RobertAnimation { const char* name; uint16_t first, count; bool loop; };
inline constexpr uint16_t robertEntry = 10, robertClosed = 11;
inline constexpr int robertRoomX = 7, robertRoomY = 4, robertCorridorRoomX = 6;
inline constexpr RobertFrame robertFrames[] = {
    {12, 12},
    {13, 15},
    {12, 15},
    {14, 15},
    {14, 12},
    {15, 12},
    {14, 12},
    {12, 15},
    {13, 15},
    {16, 10},
    {17, 10},
    {18, 10},
    {19, 10},
    {20, 10},
    {21, 10},
    {21, 10},
    {21, 10},
    {21, 10},
    {22, 10},
    {22, 10},
    {22, 10},
    {22, 10},
    {22, 10},
    {22, 10},
    {22, 10},
    {22, 10},
    {22, 10},
    {22, 12},
    {21, 12},
    {20, 12},
    {19, 12},
    {18, 12},
    {23, 12},
    {18, 12},
};
inline constexpr RobertAnimation robertAnimations[] = {
    {"start", 0, 1, false},
    {"turn", 1, 3, false},
    {"talk", 4, 2, true},
    {"wait", 6, 1, false},
    {"play", 7, 2, true},
    {"bow", 9, 18, false},
    {"turn_back", 27, 5, false},
    {"walk", 32, 2, true},
};
} // namespace ucg_club054
