#pragma once
#include <string>
#include <cstring>
#include <vector>
namespace EditorTooltips068 {
struct Entry{const char*path;const char*text;};
// Verbatim original level_editor.tscn tooltip strings; no extra instructions.
inline constexpr Entry Entries[]={
{"HUD/tabs/customTab/loopoffset","Loop Point for the song (in seconds)"},
{"HUD/Tools/PlacementMode","PLACEMENT mode"},
{"HUD/Tools/SelectMode","SELECTION mode"},
{"HUD/Tools/MusicToggle","Play music!"},
{"HUD/Tools/switchToggle","Cycle Trough switch tiles"},
{"HUD/Tools/grid","Grid Toggle\n"},
{"HUD/Tools/shiftRight","Shift Level"},{"HUD/Tools/shiftLeft","Shift Level"},
{"HUD/Tools/shiftUp","Shift Level"},{"HUD/Tools/ShiftDown","Shift Level"}};
inline const char*text(const char*path){for(const auto&e:Entries)if(!std::strcmp(e.path,path))return e.text;return nullptr;}
inline std::vector<std::string>lines(const char*text,float width=464,float scale=1.75f,unsigned font=3){
 std::vector<std::string>out;std::string line,word;
 const auto append=[&](){if(word.empty())return;const auto candidate=line.empty()?word:line+" "+word;if(!line.empty()&&EditorArt::textWidth(font,candidate.c_str())*scale>width){out.push_back(line);line=word;}else line=candidate;word.clear();};
 for(;*text;++text){if(*text==' '||*text=='\n'){append();if(*text=='\n'&&!line.empty()){out.push_back(line);line.clear();}}else word+=*text;}
 append();if(!line.empty())out.push_back(line);return out;
}
}
