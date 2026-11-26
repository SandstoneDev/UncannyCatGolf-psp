#pragma once
#include <algorithm>
#include <cmath>
namespace AvatarProjection059 {
struct Point {float x,y;};
struct Quad {float x0,y0,x1,y1,u0,v0,u1,v1;};
struct Projection {
 float x,y,sx,sy,left,right,top,bottom;
 bool quad(float a,float b,float c,float d,float u0,float v0,float u1,float v1,Quad& q)const{
  const float ax=x+a*sx,bx=x+c*sx,ay=y+b*sy,by=y+d*sy;
  if(!std::isfinite(ax)||!std::isfinite(bx)||!std::isfinite(ay)||!std::isfinite(by)||ax==bx||ay==by)return false;
  q.x0=std::max(left,std::min(ax,bx));q.x1=std::min(right,std::max(ax,bx));
  q.y0=std::max(top,std::min(ay,by));q.y1=std::min(bottom,std::max(ay,by));
  if(q.x0>=q.x1||q.y0>=q.y1)return false;
  q.u0=u0+(u1-u0)*(q.x0-ax)/(bx-ax);q.u1=u0+(u1-u0)*(q.x1-ax)/(bx-ax);
  q.v0=v0+(v1-v0)*(q.y0-ay)/(by-ay);q.v1=v0+(v1-v0)*(q.y1-ay)/(by-ay);return true;
 }
 Point point(float a,float b)const{return {x+a*sx,y+b*sy};}
 unsigned tail(Point* polygon,unsigned count)const{
  for(unsigned i=0;i<count;++i)if(!std::isfinite(polygon[i].x)||!std::isfinite(polygon[i].y))return 0;
  Point output[8];
  for(unsigned plane=0;plane<4&&count;++plane){
   const auto distance=[&](Point p){return plane==0?p.x-left:plane==1?right-p.x:plane==2?p.y-top:bottom-p.y;};
   unsigned used=0;Point previous=polygon[count-1];float pd=distance(previous);
   for(unsigned i=0;i<count;++i){const Point current=polygon[i];const float cd=distance(current);
    if((pd>=0)!=(cd>=0)){const float t=pd/(pd-cd);Point p{previous.x+(current.x-previous.x)*t,previous.y+(current.y-previous.y)*t};
     if(plane==0)p.x=left;else if(plane==1)p.x=right;else if(plane==2)p.y=top;else p.y=bottom;output[used++]=p;}
    if(cd>=0)output[used++]=current;
    previous=current;pd=cd;
   }
   count=used;for(unsigned i=0;i<count;++i)polygon[i]=output[i];
  }
  return count;
 }
};
}
