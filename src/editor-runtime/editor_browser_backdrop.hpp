#pragma once
#include <memory>
namespace EditorBrowserRender {
// Original full-resolution floor RGBA is retained. Only the normal final
// framebuffer raster is sampled here, following earthbound.gdshader in float32.
inline std::unique_ptr<uint32_t[]> floorPixels;
inline unsigned floorId=~0u,floorWidth=0,floorHeight=0;
inline float sourceU[272],sourceV[272],waveY[272],waveX[272];
inline std::string error;
inline void release(){floorPixels.reset();floorId=~0u;floorWidth=floorHeight=0;}
inline bool load(unsigned id){
 if(floorPixels&&floorId==id)return true;
 flushBatch();
 if(!EditorRender::floorRow(id,0)){error=EditorRender::getError();return false;}
 const auto&record=EditorRender::floorIndex[id];
 if(!record[1]||!record[2]||record[1]>1024||record[2]>1024){error="Invalid original browser floor dimensions";return false;}
 const unsigned count=record[1]*record[2];
 std::unique_ptr<uint32_t[]> next(new(std::nothrow) uint32_t[count]);
 if(!next||std::fseek(EditorRender::floorFile,long(record[0]),SEEK_SET)||std::fread(next.get(),4,count,EditorRender::floorFile)!=count){error="Cannot read original browser floor";return false;}
 floorPixels=std::move(next);floorId=id;floorWidth=record[1];floorHeight=record[2];error.clear();return true;
}
inline uint32_t sample(float u,float v){
 u-=std::floor(u);v-=std::floor(v);
 const unsigned x=std::min(floorWidth-1,unsigned(u*floorWidth)),y=std::min(floorHeight-1,unsigned(v*floorHeight));
 return floorPixels[y*floorWidth+x];
}
inline bool backdrop(bool results,float time){
 if(!load(results?6:0))return false;
 time=std::fmod(time,3600.f);
 for(unsigned i=0;i<272;++i){
  sourceU[i]=(i+.5f)*2/(EditorArt::Scale*floorWidth);
  sourceV[i]=(i+.5f)*2/(EditorArt::Scale*floorHeight);
  waveY[i]=(results?2.f:.01f)*std::sin(.01f*sourceU[i]+time);
  waveX[i]=(results?.1f:1.f)*std::sin(2*sourceV[i]+time*.06f);
 }
 for(unsigned by=0;by<272;by+=256)for(unsigned bx=0;bx<272;bx+=256){
  const unsigned width=std::min(256u,272-bx),height=std::min(256u,272-by);
  for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
   const float u=sourceU[bx+x],v=sourceV[by+y]+waveY[bx+x],wave=waveX[by+y];
   const uint32_t a=sample(u+wave,v),b=sample(u-wave,v);
   const unsigned r=((a&255)+(b&255)+1)/2,g=((a>>8&255)+(b>>8&255)+1)/2,blue=((a>>16&255)+(b>>16&255)+1)/2;
   EditorRender::scratch[y*256+x]=0xff000000u|r|(g<<8)|(blue<<16);
  }
  EditorRender::nativeImage(EditorRender::scratch,ViewX+136+bx,by,width,height,White);
 }
 return true;
}
} // namespace EditorBrowserRender
