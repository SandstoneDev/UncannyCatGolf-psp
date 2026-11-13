#pragma once

#include "campaign.hpp"
#include "portal.hpp"

inline constexpr SolidBlock FrontierSwitchBlocks402[]{
    {{368,368},{16,16}},
    {{368,400},{16,16}},
    {{368,432},{16,16}},
    {{368,464},{16,16}},
    {{80,48},{16,16}},
    {{112,48},{16,16}},
    {{144,48},{16,16}},
    {{176,48},{16,16}},
    {{-464,208},{16,16}},
    {{-464,240},{16,16}},
    {{-464,272},{16,16}},
    {{-464,304},{16,16}},
    {{16,-208},{16,16}},
    {{48,-208},{16,16}},
    {{80,-208},{16,16}},
    {{112,-208},{16,16}},
    {{144,-208},{16,16}},
    {{176,-208},{16,16}},
    {{-16,-208},{16,16}},
    {{-48,-208},{16,16}},
    {{-80,-208},{16,16}},
    {{-112,-208},{16,16}},
    {{464,48},{16,16}},
    {{208,-464},{16,16}},
    {{240,-464},{16,16}},
    {{272,-464},{16,16}},
    {{176,-464},{16,16}},
    {{144,-464},{16,16}},
    {{112,-464},{16,16}},
    {{464,16},{16,16}},
    {{464,80},{16,16}},
};

inline constexpr Vec2 FrontierSpeedTokens402[]{
    {128,256},
    {-96,256},
};

inline constexpr PortalNode FrontierPortals403[]{
    {{224,-304},1},
    {{-320,-192},0},
    {{320,56},3},
    {{-288,-96}},
    {{-288,320},5},
    {{-112,160}},
    {{288,288},7},
    {{-160,64}},
};

inline PortalStage frontierPortals(unsigned course) {
    if (course==FrontierPortalCourse) return {FrontierPortals403,std::size(FrontierPortals403)};
    return {};
}

template<class AssetPath>
bool loadFrontierSwitchLayers(SwitchStage& stage,const char* courseId,
                              const SolidBlock* blocks,unsigned count,AssetPath path) {
    constexpr unsigned phaseCount=4;
    std::string files[phaseCount];
    const char* layers[phaseCount];
    for (unsigned phase=0;phase<phaseCount;++phase) {
        const auto name=std::string(courseId)+"-switch-"+std::to_string(phase)+".bin";
        files[phase]=path(name.c_str());
        layers[phase]=files[phase].c_str();
    }
    return stage.load(layers,phaseCount,blocks,count);
}
