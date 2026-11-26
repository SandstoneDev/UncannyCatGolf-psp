#pragma once
#include <string>
#include <string_view>
namespace ReleasePaths {
inline bool disc(std::string_view path){return path.rfind("disc0:",0)==0;}
inline std::string assetDirectory(const char* executable){
 std::string path=executable?executable:"";
 if(disc(path))return "disc0:/PSP_GAME/SYSDIR/";
 constexpr std::string_view suffix="/data/editor/EBOOT.PBP";
 if(path.size()>=suffix.size()&&path.compare(path.size()-suffix.size(),suffix.size(),suffix)==0)
  path.resize(path.size()-suffix.size()),path+="/EBOOT.PBP";
 const auto slash=path.find_last_of("/\\");return slash==std::string::npos?"":path.substr(0,slash+1);
}
inline std::string stateFromDirectory(const std::string& directory,const char* file){
 return (disc(directory)?std::string("ms0:/PSP/GAME/UCG054/"):directory)+(file?file:"");
}
inline std::string state(const char* executable,const char* file){return stateFromDirectory(assetDirectory(executable),file);}
inline std::string moduleCompanion(const char* executable,const char* filename){
 const std::string file=filename?filename:"";
 if(disc(executable?executable:"")&&file=="EBOOT.PBP")return executable;
 if(disc(executable?executable:"")&&file=="data/editor/EBOOT.PBP")return "disc0:/PSP_GAME/SYSDIR/EDITOR.PBP";
 const auto directory=assetDirectory(executable);
 if(file.empty()||file=="data"||file.rfind("data/",0)==0)return directory+file;
 return stateFromDirectory(directory,filename);
}
}
