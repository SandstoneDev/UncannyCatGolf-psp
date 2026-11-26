#pragma once
#include "editor_tooltips068.hpp"
namespace EditorReadableLabels067 {
inline constexpr const char* Labels[]={
"Song:",
"Rank values:",
"-",
"Level Bounds:",
"Floor:",
"Lighting:",
"Layer:",
"Color palette:",
"",
"If you have graphics in your world folder, you can use them here!",
"Shift + Middle Mouse to move the camera\nScrollwheel selected object to scale it\n- Hold shift for finer tuning\n- Hold Z for X scale only,  X for Y scale only\nRight click removes object\nAlt click copies object\nHold shift to un-snap\nMiddle mouse button switches between select and place mode. In select mode, you can select and drag around objects.\nEscape to deselect.\nCTRL + Scrollwheel changes camera zoom.\nDeselect any selected objects before selecting new ones.\nCTRL+Z to undo, CTRL+Y to redo\nPlaytesting saves your level!",
"(seperate with commas)\nModifier list:\n- hard_mode\n- burst\n- telekinesis\n- buddy\n- parry\n- jumpy\n- dizzy\n- missiles\n- sticky\n- god\n- bort\n- bees\n",
"Grid Size:",
"Mode: PLACEMENT",
"Welcome to the LEVEL EDITOR!\nSelect an option...",
};
struct Placement {float x,baseline;};
inline Placement placement(unsigned id,const EditorArt::Packed::Label&l){
 float x=id==2?124.f:id==7?16.f:l.x,baseline=l.y+17;
 switch(id){case 0:baseline=276;break;case 1:baseline=352;break;case 2:x=0;baseline=0;break;case 3:baseline=548;break;case 4:baseline=708;break;case 5:baseline=864;break;case 6:baseline=108;break;case 7:baseline=408;break;case 12:baseline=568;break;case 13:x=272;baseline=504;break;default:break;}
 return {x,baseline};
}
inline bool selection=false;
inline bool draw(unsigned id,const EditorArt::Packed::Label&l){
 if(id==2||id>=std::size(Labels)||l.baked||l.welcome||!*Labels[id])return false;
 const bool oldOutline=EditorTextFlow068::outline;EditorTextFlow068::outline=true;struct RestoreOutline{bool v;~RestoreOutline(){EditorTextFlow068::outline=v;}}restoreOutline{oldOutline};
 if(id==9){const auto lines=EditorTooltips068::lines(Labels[id],464,1.75f);for(unsigned row=0;row<lines.size();++row)EditorArt::textScaled(3,lines[row].c_str(),272,400+row*32,White,464,1.75f);return true;}
 const auto old=EditorArt::uiScissor;const bool scrolling=l.tab==0||l.tab==5;if(scrolling)EditorArt::scissor(old[0],std::max(old[1],int(EditorTextLayout068::Top*EditorArt::Scale)),old[2],std::min(old[3],int(EditorTextLayout068::Bottom*EditorArt::Scale)));
 auto p=placement(id,l);if(l.tab==0||l.tab==5)p.baseline=EditorTextLayout068::y(unsigned(l.tab),p.baseline);EditorArt::textScaled(3,id==13?(selection?"Mode: SELECTION":"Mode: PLACEMENT"):Labels[id],p.x,p.baseline,White,id==13?760.f:id==5?184.f:240-p.x,id==6?1.5f:1.75f);if(scrolling)EditorArt::scissor(old[0],old[1],old[2],old[3]);return true;
}
}
