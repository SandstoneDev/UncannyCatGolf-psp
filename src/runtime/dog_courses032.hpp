#pragma once

#include "park_late031.hpp"

#include <cstring>
#include <iterator>

// World coordinates and initial velocity from the two level scenes.
// The DogGolf and DogGoal instances share the original scene defaults.
inline constexpr DogGolfScene DogGolf314{
    {0, -192}, {-150, 0}, {64, 128},
    15.8f, 16.0f, 0.064f, 0.05f, 0.001f, true,
};
inline constexpr DogGolfScene DogGolf315{
    {112, 176}, {282.842712f, 282.842712f}, {-16, 176},
    15.8f, 16.0f, 0.064f, 0.05f, 0.001f, true,
};

inline constexpr SolidBlock DogSwitchBlocks314[]{
    {{-112, 112}, {16, 16}},
    {{-80, 112}, {16, 16}},
    {{112, 176}, {16, 16}},
    {{144, 176}, {16, 16}},
};
inline constexpr SolidBlock DogSwitchBlocks315[]{
    {{208, 144}, {16, 16}},
    {{208, 176}, {16, 16}},
    {{208, 208}, {16, 16}},
    {{208, -208}, {16, 16}},
};

struct DogCourse032 {
    const DogGolfScene& dog;
    const SolidBlock* switchBlocks;
    unsigned switchBlockCount;
};
inline constexpr DogCourse032 DogCourse314{DogGolf314, DogSwitchBlocks314, std::size(DogSwitchBlocks314)};
inline constexpr DogCourse032 DogCourse315{DogGolf315, DogSwitchBlocks315, std::size(DogSwitchBlocks315)};

inline const DogCourse032* dogCourse032(const char* courseId) {
    if (std::strcmp(courseId, "3_14") == 0) return &DogCourse314;
    if (std::strcmp(courseId, "3_15") == 0) return &DogCourse315;
    return nullptr;
}
