#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
namespace CreditsComposite057 {
inline unsigned swizzle(unsigned x,unsigned y,unsigned width,unsigned bytes){return (y/8)*width*8*bytes+(x*bytes/16)*128+(y%8)*16+(x*bytes)%16;}
inline uint32_t rgba565(uint16_t p){return (p&31)*255/31|((p>>5&63)*255/63<<8)|((p>>11)*255/31<<16)|0xff000000u;}
inline uint16_t rgb565(uint32_t p){return (p&255)>>3|((p>>8&255)>>2)<<5|((p>>16&255)>>3)<<11;}
inline uint32_t over(uint32_t dest,uint32_t source,unsigned alpha,bool premult=false){
 uint32_t out=0;for(unsigned c=0;c<3;++c){const unsigned s=source>>(c*8)&255,d=dest>>(c*8)&255;out|=std::min(255u,((premult?s*255:s*alpha)+d*(255-alpha)+127u)/255)<<(c*8);}return out|0xff000000u;
}
struct Image{const uint8_t* pixels=nullptr;unsigned width=0,height=0,stride=0;float left=0,top=0;bool swizzled=false;const uint32_t* palette=nullptr;};
struct Actor{uint32_t kind;float a,b,c,d,x,y,alpha;};
inline bool bounds(const Actor& a,const Image& im,int& l,int& t,int& r,int& b){
 const float determinant=a.a*a.d-a.b*a.c;if(!im.pixels||!im.width||!im.height||!std::isfinite(determinant)||std::abs(determinant)<1e-10f)return false;
 float loX=100000,loY=100000,hiX=-100000,hiY=-100000;
 for(unsigned i=0;i<4;++i){const float x=im.left+(i&1?im.width:0),y=im.top+(i&2?im.height:0);const float px=a.x+a.a*x+a.c*y,py=a.y+a.b*x+a.d*y;loX=std::min(loX,px);loY=std::min(loY,py);hiX=std::max(hiX,px);hiY=std::max(hiY,py);}
 if(!std::isfinite(loX)||!std::isfinite(loY)||!std::isfinite(hiX)||!std::isfinite(hiY)||loX>272||loY>272||hiX<0||hiY<0)return false;
 l=std::max(0,int(std::floor(loX)));t=std::max(0,int(std::floor(loY)));r=std::min(272,int(std::ceil(hiX)));b=std::min(272,int(std::ceil(hiY)));return l<r&&t<b;
}
template<class Gradient>void draw(uint16_t* movie,const Actor& a,const Image& image,bool rainbow,Gradient gradient){
 int l,t,r,b;if(!bounds(a,image,l,t,r,b))return;
 const float inv=1/(a.a*a.d-a.b*a.c);
 for(int y=t;y<b;++y)for(int x=l;x<r;++x){
  const float dx=x+.5f-a.x,dy=y+.5f-a.y;
  const int u=int(std::floor((a.d*dx-a.c*dy)*inv-image.left)),v=int(std::floor((-a.b*dx+a.a*dy)*inv-image.top));
  if(u<0||v<0||unsigned(u)>=image.width||unsigned(v)>=image.height)continue;
  uint32_t p;if(image.palette){const unsigned offset=image.swizzled?swizzle(u,v,image.stride,1):v*image.stride+u;p=image.palette[image.pixels[offset]%24];}
  else std::memcpy(&p,image.pixels+(image.swizzled?swizzle(u,v,image.stride,4):4*(v*image.stride+u)),4);
  unsigned alpha=unsigned((p>>24)*std::clamp(a.alpha,0.f,1.f)+.5f);
  if(rainbow){const uint32_t g=gradient(x);uint32_t c=0;for(unsigned j=0;j<4;++j)c|=((p>>(j*8)&255)*(g>>(j*8)&255)/255)<<(j*8);p=c;alpha=p>>24;}
  auto& dest=movie[swizzle(x,y,512,2)/2];dest=rgb565(over(rgba565(dest),p,alpha));
 }
}
}
