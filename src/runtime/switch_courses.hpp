#pragma once

#include "park_late031.hpp"
#include "dog_courses032.hpp"
#include "park_late032_data.hpp"
#include <cstring>
#include <string>

inline constexpr SolidBlock SwitchBlocks303[]{
    {{208,128},{16,16}}, {{208,160},{16,16}}, {{-208,160},{16,16}}, {{-208,128},{16,16}},
    {{-96,-224},{16,16}}, {{-64,-224},{16,16}}, {{48,-224},{16,16}}, {{80,-224},{16,16}}
};
inline constexpr SolidBlock SwitchBlocks304[]{
    {{-224,224},{16,16}}, {{-224,-224},{16,16}}, {{-128,-224},{16,16}}, {{16,0},{16,16}},
    {{64,80},{16,16}}, {{64,112},{16,16}}, {{224,-224},{16,16}}
};

inline constexpr SolidBlock SwitchBlocks307[]{
    {{-16,64},{16,16}}, {{16,64},{16,16}}
};

template<class AssetPath>
bool loadSwitchCourse(SwitchStage& stage,const char* courseId,AssetPath path) {
    const SolidBlock* blocks=nullptr;
    unsigned count=0;
    if (std::strcmp(courseId,"3_03")==0) {
        blocks=SwitchBlocks303;
        count=std::size(SwitchBlocks303);
    } else if (std::strcmp(courseId,"3_04")==0) {
        blocks=SwitchBlocks304;
        count=std::size(SwitchBlocks304);
    } else if (std::strcmp(courseId,"3_07")==0) {
        blocks=SwitchBlocks307;
        count=std::size(SwitchBlocks307);
    } else if (std::strcmp(courseId,"3_12")==0) {
        blocks=ParkPursuitSwitchBlocks;
        count=std::size(ParkPursuitSwitchBlocks);
    } else if (std::strcmp(courseId,"3_13")==0) {
        blocks=ParkDogSwitchBlocks;
        count=std::size(ParkDogSwitchBlocks);
    } else if (std::strcmp(courseId,"3_17")==0) {
        blocks=ParkSwitchBlocks317;
        count=std::size(ParkSwitchBlocks317);
    } else if (const auto* dog=dogCourse032(courseId)) {
        blocks=dog->switchBlocks;
        count=dog->switchBlockCount;
    } else {
        return stage.load(nullptr,0,nullptr,0);
    }
    const auto first=path((std::string(courseId)+"-switch-0.bin").c_str());
    const auto second=path((std::string(courseId)+"-switch-1.bin").c_str());
    const char* layers[]{first.c_str(),second.c_str()};
    return stage.load(layers,static_cast<unsigned>(std::size(layers)),blocks,count);
}
