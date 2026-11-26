#pragma once
#include "costume_gradient056.hpp"
#include <algorithm>
#include <cmath>
namespace CostumeShader056 {
inline float phase=0;
inline unsigned used=0;
inline uint32_t colorAt(float x){const int band=static_cast<int>(std::floor(((x-ViewX)/408.f-phase)*64));return Gradient[band&63];}
inline TextureVertex interpolate(const TextureVertex& a,const TextureVertex& b,float t){return {a.u+(b.u-a.u)*t,a.v+(b.v-a.v)*t,0,a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,0};}
inline unsigned clip(const TextureVertex* source,unsigned count,TextureVertex* output,bool yAxis,float edge,bool greater){
    if(!count)return 0;
    unsigned written=0;auto previous=source[count-1];
    auto coordinate=[yAxis](const TextureVertex& p){return yAxis?p.y:p.x;};
    bool wasInside=greater?coordinate(previous)>=edge:coordinate(previous)<=edge;
    for(unsigned i=0;i<count;++i){
        const auto current=source[i];const bool inside=greater?coordinate(current)>=edge:coordinate(current)<=edge;
        if(inside!=wasInside){const float t=(edge-coordinate(previous))/(coordinate(current)-coordinate(previous));auto p=interpolate(previous,current,t);if(yAxis)p.y=edge;else p.x=edge;output[written++]=p;}
        if(inside)output[written++]=current;
        previous=current;wasInside=inside;
    }
    return written;
}
inline void drawQuad(const TextureVertex (&quad)[4]){
    float left=quad[0].x,right=left;
    for(const auto& v:quad){left=std::min(left,v.x);right=std::max(right,v.x);}
    left=std::max(left,0.f);right=std::min(right,480.f);used=0;
    if(left>=right)return;
    const int first=static_cast<int>(std::floor(((left-ViewX)/408.f-phase)*64));
    const int last=static_cast<int>(std::floor(((right-ViewX)/408.f-phase)*64));
    const bool rectangle=quad[0].x==quad[3].x && quad[1].x==quad[2].x && quad[0].y==quad[1].y && quad[2].y==quad[3].y && quad[1].x>quad[0].x && quad[3].y>quad[0].y;
    if(rectangle){
        const float top=std::max(0.f,quad[0].y),bottom=std::min(272.f,quad[3].y);
        if(top>=bottom)return;
        const float uScale=(quad[1].u-quad[0].u)/(quad[1].x-quad[0].x),vScale=(quad[3].v-quad[0].v)/(quad[3].y-quad[0].y);
        const auto visitRectangle=[&](TextureVertex* vertices){
            unsigned count=0;
            for(int band=first;band<=last;++band){
                const float lo=std::max(left,ViewX+(phase+band/64.f)*408),hi=std::min(right,ViewX+(phase+(band+1)/64.f)*408);
                if(lo>=hi)continue;
                if(vertices){
                    vertices[count]={quad[0].u+(lo-quad[0].x)*uScale,quad[0].v+(top-quad[0].y)*vScale,Gradient[band&63],lo,top,0};
                    vertices[count+1]={quad[0].u+(hi-quad[0].x)*uScale,quad[0].v+(bottom-quad[0].y)*vScale,Gradient[band&63],hi,bottom,0};
                }
                count+=2;
            }
            return count;
        };
        used=visitRectangle(nullptr);if(!used)return;
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(used*sizeof(TextureVertex)));
        visitRectangle(vertices);
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,used,nullptr,vertices);
        return;
    }
    // Count exact clipped triangles before reserving command-list memory.
    // Worst-case18vertices/stripe over-reserved axis-aligned actors by3x;
    // Panic's81actors could exhaust Main's128KiB list with that reservation.
    const auto visit=[&](TextureVertex* vertices){
      unsigned countVertices=0;
      for(int band=first;band<=last;++band){
        const float lo=std::max(left,ViewX+(phase+band/64.f)*408),hi=std::min(right,ViewX+(phase+(band+1)/64.f)*408);
        if(lo>=hi)continue;
        TextureVertex a[12],b[12];unsigned count=clip(quad,4,a,false,lo,true);
        count=clip(a,count,b,false,hi,false);count=clip(b,count,a,true,0,true);count=clip(a,count,b,true,272,false);
        // Original fragment explicitly replaces COLOR, including incoming
        // modulate alpha/RGB; its unshaded material uses texture*gradient only.
        const uint32_t color=Gradient[band&63];
        for(unsigned i=1;i+1<count;++i){
            if(vertices){vertices[countVertices]=b[0];vertices[countVertices++].color=color;vertices[countVertices]=b[i];vertices[countVertices++].color=color;vertices[countVertices]=b[i+1];vertices[countVertices++].color=color;}
            else countVertices+=3;
        }
      }
      return countVertices;
    };
    used=visit(nullptr);if(!used)return;
    // Arena storage is unique per draw and remains live until endFrame fences GE.
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(used*sizeof(TextureVertex)));
    visit(vertices);
    sceGuDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,used,nullptr,vertices);
}
}
