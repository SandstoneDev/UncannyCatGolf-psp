#pragma once
#include "speech_readable059.hpp"
namespace AvatarEditorSpeech059 {
inline AvatarSpeech058::Layout layout;
inline void initialize(){layout=AvatarSpeech058::prepare(4,0);SpeechReadable059::publish();}
inline void draw(const EditorController::Controller& controller,const EditorPlay::Scene& scene){
 if(!scene.idleSpeech059.remaining)return;
 const auto authored=scene.speechScale059();if(authored.x==0||authored.y==0)return;
 EditorRender::View camera(controller,&scene);
 const auto at=camera.vertex({scene.game.position.x,scene.game.position.y});
 // PSP readability: upright screen-space lettering; preserve authored tint.
 EditorArt::scissor(ViewX+136,0,ViewX+408,272);
 SpeechReadable059::draw(at.x,at.y,layout,EditorRender::rgba(scene.speechTint059));
 EditorArt::fullView();
}
}
