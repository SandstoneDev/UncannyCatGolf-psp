#pragma once
#include "editor_toolbar_layout066.hpp"
#include "editor_text_flow068.hpp"
#include <algorithm>
namespace EditorTextLayout068 {
using Rect=EditorToolbarLayout066::Rect;
inline float scroll[6]{};inline unsigned revision[6]{};
inline constexpr float Top=96,Bottom=476,Stretch=1;
inline unsigned controlTab(unsigned id){return id<=14?0:id<=19?1:id<=24?2:id<=32?3:5;}
inline bool scrolling(unsigned tab){return tab==0||tab==3||tab==5;}
inline bool sidebar(unsigned id){return id>=1&&id<=46;}
inline float y(unsigned tab,float source){return scrolling(tab)?source-scroll[tab]:source;}
inline float maximum(unsigned tab){return tab==0?416.f:tab==3?240.f:tab==5?208.f:0.f;}
inline void wheel(unsigned tab,int amount){const auto old=scroll[tab];scroll[tab]=std::clamp(scroll[tab]-amount*40.f,0.f,maximum(tab));if(scroll[tab]!=old)++revision[tab];}
inline void ensure(unsigned tab,float top,float height){if(!scrolling(tab))return;if(top<Top)scroll[tab]+=top-Top;else if(top+height>Bottom)scroll[tab]+=top+height-Bottom;scroll[tab]=std::clamp(scroll[tab],0.f,maximum(tab));}
inline Rect rect(unsigned id,Rect r){
 switch(id){
 case 1:r.y=96;break;case 2:r.y=136;break;
 case 3:r={16,176,224,r.h};break;case 4:r={16,216,224,r.h};break;
 case 5:case 6:case 7:case 8:r={16,360.f+40.f*(id-5),224,r.h};break;
 case 9:r={16,556,224,r.h};break;case 10:r={16,600,224,r.h};break;case 11:r={16,640,224,r.h};break;
 case 12:r.y=284;break;case 13:r={16,716,224,112};break;
 case 14:r={208,848,r.w,r.h};break;case 15:r.h=28;break;case 16:case 17:case 18:case 19:r.y=424;break;
 case 20:r.h=128;break;case 25:r.h=252;break;
 case 26:r={16,652,224,36};break;case 28:r={16,372,224,r.h};break;case 27:r={16,412,224,r.h};break;case 31:r={16,452,224,r.h};break;case 30:r={16,492,224,r.h};break;case 29:r={16,532,224,r.h};break;case 32:r={16,572,224,r.h};break;
 case 33:r.y=96;break;case 42:r={16,136,224,r.h};break;case 43:r={16,176,224,r.h};break;
 case 34:r.y=224;break;case 35:r.y=264;break;case 36:r.y=304;break;
 case 37:r.y=344;break;case 38:r.y=384;break;case 39:r.y=424;break;
 case 41:r.y=464;break;case 40:r.y=504;break;
 case 61:r={352,184,320,56};break;case 62:r={352,256,320,56};break;
 case 44:r.y=624;break;case 45:r={16,576,104,r.h};break;case 46:r={128,576,104,r.h};break;
 default:break;
 }
 if((id>=1&&id<=12)||(id==21)||(id==24)||(id>=27&&id<=32)||(id>=33&&id<=46))r.h=std::max(r.h,36.f);
 if(sidebar(id)&&scrolling(controlTab(id)))r.y=y(controlTab(id),r.y);
 return r;
}
inline constexpr unsigned FloorRows=2;
inline constexpr float FloorTop=716,FloorPitch=56;
inline constexpr unsigned FileRows=5;
inline constexpr float FileTop=138,FilePitch=44;
}
