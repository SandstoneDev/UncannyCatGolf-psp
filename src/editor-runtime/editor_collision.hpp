#pragma once
#include "asset_file.hpp"
#include <cstdint>
#include <cstdio>
namespace EditorArt { namespace Collision {
struct Point {float x,y;};
struct Info {unsigned polygons,width,height;};
// Original full-texture top-left coordinates. The caller subtracts width/2,height/2.
// Visitor receives one complete convex polygon at a time; no main-heap allocation.
inline bool load(const char* executable,unsigned image,void* context,bool(*visitor)(void*,const Point*,unsigned),Info* info=nullptr){
 const auto path=companion(executable,"data/world/editor-collision.bin");auto file=openAssetFile(path.c_str());uint32_t header[4]{},entry[5]{};
 if(!file||std::fread(header,sizeof header,1,file.get())!=1||header[0]!=0x43454355||header[1]!=1||image>=header[2]||header[2]>4096)return false;
 if(std::fseek(file.get(),16+image*20,SEEK_SET)||std::fread(entry,sizeof entry,1,file.get())!=1||std::fseek(file.get(),0,SEEK_END))return false;
 const long length=std::ftell(file.get());if(length<0||entry[0]>unsigned(length)||entry[1]>unsigned(length)-entry[0]||std::fseek(file.get(),entry[0],SEEK_SET))return false;
 if(info)*info={entry[2],entry[3],entry[4]};
 unsigned read=0;Point points[64];
 for(unsigned i=0;i<entry[2];++i){uint32_t count=0;if(std::fread(&count,4,1,file.get())!=1||count<3||count>64||read>entry[1]||entry[1]-read<4+count*4)return false;read+=4;
  for(unsigned j=0;j<count;++j){uint16_t pair[2];if(std::fread(pair,4,1,file.get())!=1)return false;points[j]={float(pair[0]),float(pair[1])};}read+=count*4;
  if(visitor&&!visitor(context,points,count))return false;
 }
 return read==entry[1];
}
}}
