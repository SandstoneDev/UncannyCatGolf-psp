#pragma once
#include "editor_audio.hpp"
namespace ucg::editor_audio {
struct BuilderCue {unsigned id,frames,maxPolyphony;float gainDb,pitch;};
inline constexpr BuilderCue BuilderGet{BuilderPresentGet,52920,4,2.f,1.f},BuilderFly{BuilderPresentFly,105840,1,0.f,1.f},BuilderExplode{BuilderPresentExplode,44034,1,0.f,1.f};
}
