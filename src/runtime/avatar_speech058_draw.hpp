#pragma once
#include "speech_readable059.hpp"
namespace AvatarSpeech058 {
inline void publish(){SpeechReadable059::publish();}
inline void draw(const GameplayView& view,Vec2 world,const Layout& layout){
 const auto at=view.position(world);SpeechReadable059::draw(at.x,at.y,layout,White);
}
}
