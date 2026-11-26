#pragma once
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
namespace EditorSamplePlan {
// Same affine inverse and nearest texel decisions as shadeTexture. A map entry
// identifies an original texel; UINT_MAX preserves the untouched clear border.
template<class Quad>
bool build(const Quad&q,int left,int top,unsigned w,unsigned h,
           unsigned width,unsigned height,uint32_t*out){
  if(!out||!w||!h||w>256||h>256||!width||!height||width>256||height>256)return false;
  const float ax=q[1].x-q[0].x,ay=q[1].y-q[0].y,bx=q[3].x-q[0].x,
              by=q[3].y-q[0].y,det=ax*by-ay*bx;
  if(std::abs(det)<.000001f)return false;
  const float inv=1/det,du=q[1].u-q[0].u,dv=q[3].v-q[0].v;
  for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
    float dx=left+x+.5f-q[0].x,dy=top+y+.5f-q[0].y,
          a=(dx*by-dy*bx)*inv,b2=(dy*ax-dx*ay)*inv;
    if(a<0||b2<0||a>=1||b2>=1){out[y*w+x]=UINT_MAX;continue;}
    unsigned u=unsigned(std::clamp(int(std::floor(q[0].u+a*du)),0,int(width)-1)),
             v=unsigned(std::clamp(int(std::floor(q[0].v+b2*dv)),0,int(height)-1));
    out[y*w+x]=v*width+u;
  }
  return true;
}
}
