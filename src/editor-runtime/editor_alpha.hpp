#pragma once
// Marching squares/crossing splitting and polygon reduction adapt Godot 4.4
// scene/resources/bit_map.cpp. See GODOT-LICENSE.txt (MIT) alongside this file.
#include "editor_images.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace EditorAlpha {
struct Point {float x=0,y=0;};
using Polygon=std::vector<Point>;
using Polygons=std::vector<Polygon>;
inline constexpr uint64_t MaxPixels=16u*1024u*1024u;
inline constexpr size_t MaxVertices=65536,MaxComponents=4096,MaxFloodSeeds=262144,MaxTriangulationVertices=8192;

namespace detail {
struct Mask {
 const uint8_t*bits;unsigned width,height;
 bool at(int x,int y)const{
  if(x<0||y<0||unsigned(x)>=width||unsigned(y)>=height)return false;
  const size_t i=size_t(y)*width+unsigned(x);return (bits[i>>3]&(1u<<(i&7)))!=0;
 }
};
inline float perpendicular(Point p,Point first,Point last){
 if(first.x==last.x)return std::abs(p.x-last.x);
 if(first.y==last.y)return std::abs(p.y-last.y);
 const float slope=(last.y-first.y)/(last.x-first.x),intercept=first.y-slope*first.x;
 return float(std::abs(slope*p.x-p.y+intercept)/std::sqrt(std::pow(slope,2.0f)+1.0));
}
inline Polygon reduce(const Polygon&points,unsigned width,unsigned height){
 if(points.size()<3)return {};
 if(points.size()<9)return points;
 const float epsilon=std::min(1.0f,float(std::min(width,height))*0.5f);
 struct Range{size_t begin,end;};
 std::vector<Range> pending{{0,points.size()}};Polygon result;
 result.reserve(points.size());
 while(!pending.empty()){
  const auto r=pending.back();pending.pop_back();
  const size_t count=r.end-r.begin;
  if(count<3){for(size_t i=r.begin;i<r.end;++i)result.push_back(points[i]);continue;}
  float maximum=0;size_t split=r.begin;
  for(size_t i=r.begin+1;i+1<r.end;++i){const float distance=perpendicular(points[i],points[r.begin],points[r.end-1]);if(distance>maximum){maximum=distance;split=i;}}
  if(maximum>epsilon){
   // Godot deliberately splits [begin,split) and [split,end), without overlap.
   pending.push_back({split,r.end});pending.push_back({r.begin,split});
  }else{result.push_back(points[r.begin]);result.push_back(points[r.end-1]);}
 }
 if(result.size()>1){
  const auto last=result.back();auto&first=result.front();const float x=last.x-first.x,y=last.y-first.y;
  if(last.y>first.y&&std::sqrt(x*x+y*y)<epsilon*0.5f){first.y=last.y;result.pop_back();}
 }
 return result;
}
inline bool march(const Mask&mask,unsigned startX,unsigned startY,Polygons&result,size_t&vertexBudget,std::string&error){
 int x=int(startX),y=int(startY),previousX=0,previousY=0;
 uint64_t steps=0;Polygon points;Polygons splitPolygons;
 struct Crossing{size_t point,order;};
 std::unordered_map<uint32_t,Crossing> crossings;std::vector<uint32_t> crossingOrder;
 do{
  const unsigned square=(mask.at(x-1,y-1)?1:0)|(mask.at(x,y-1)?2:0)|(mask.at(x-1,y)?4:0)|(mask.at(x,y)?8:0);
  int dx=0,dy=0;
  switch(square){
   case 1:case 5:case 13:dy=-1;break;
   case 8:case 10:case 11:dy=1;break;
   case 4:case 12:case 14:dx=-1;break;
   case 2:case 3:case 7:dx=1;break;
   case 9:dy=previousX==1?1:-1;break;
   case 6:dx=previousY==-1?1:-1;break;
   default:error="Invalid alpha contour boundary";return false;
  }
  if(square==6||square==9){
   const uint32_t key=(uint32_t(x)<<16)|uint32_t(y);
   const auto found=crossings.find(key);
   if(found!=crossings.end()){
    const auto crossing=found->second;
    if(crossing.point>=points.size()){error="Invalid alpha contour crossing";return false;}
    splitPolygons.emplace_back(points.begin()+crossing.point+1,points.end());
    points.resize(crossing.point+1);
    while(crossingOrder.size()>crossing.order+1){crossings.erase(crossingOrder.back());crossingOrder.pop_back();}
    crossings.erase(key);crossingOrder.pop_back();
   }else{
    if(points.empty()){error="Invalid initial alpha crossing";return false;}
    crossings.emplace(key,Crossing{points.size()-1,crossingOrder.size()});crossingOrder.push_back(key);
   }
  }
  x+=dx;y+=dy;
  if(dx==previousX&&dy==previousY){
   if(points.empty()){error="Invalid alpha contour direction";return false;}
   points.back()={float(x),float(y)};
  }else{
   if(++vertexBudget>MaxVertices){error="Alpha contour exceeds 65536-vertex complexity limit";return false;}
   points.push_back({float(x),float(y)});
  }
  previousX=dx;previousY=dy;
  if(++steps>2*(uint64_t(mask.width)*mask.height+1)){error="Alpha contour did not close";return false;}
 }while(x!=int(startX)||y!=int(startY));
 auto append=[&](const Polygon&p){auto simplified=reduce(p,mask.width,mask.height);if(simplified.size()>=3)result.push_back(std::move(simplified));};
 append(points);for(const auto&p:splitPolygons)append(p);
 return true;
}
inline bool flood(const Mask&mask,unsigned startX,unsigned startY,std::vector<uint8_t>&visited,std::string&error){
 const auto marked=[&](unsigned x,unsigned y){const size_t p=size_t(y)*mask.width+x;return (visited[p>>3]&(1u<<(p&7)))!=0;};
 const auto mark=[&](unsigned x,unsigned y){const size_t p=size_t(y)*mask.width+x;visited[p>>3]|=1u<<(p&7);};
 std::vector<uint32_t> pending;pending.reserve(std::min<size_t>(mask.height,4096));pending.push_back(startY*mask.width+startX);
 while(!pending.empty()){
  const uint32_t seed=pending.back();pending.pop_back();const unsigned y=seed/mask.width,x=seed%mask.width;
  if(marked(x,y)||!mask.at(int(x),int(y)))continue;
  unsigned left=x,right=x;
  while(left&&!marked(left-1,y)&&mask.at(int(left)-1,int(y)))--left;
  while(right+1<mask.width&&!marked(right+1,y)&&mask.at(int(right)+1,int(y)))++right;
  for(unsigned at=left;at<=right;++at)mark(at,y);
  for(int neighbor:std::array<int,2>{int(y)-1,int(y)+1}){
   if(neighbor<0||unsigned(neighbor)>=mask.height)continue;
   unsigned at=left?left-1:0;const unsigned end=std::min(mask.width-1,right+1);
   while(at<=end){
    if(!marked(at,unsigned(neighbor))&&mask.at(int(at),neighbor)){
     if(pending.size()>=MaxFloodSeeds){error="Alpha component exceeds flood-work complexity limit";return false;}
     pending.push_back(unsigned(neighbor)*mask.width+at);
     do{++at;}while(at<=end&&!marked(at,unsigned(neighbor))&&mask.at(int(at),neighbor));
    }else ++at;
   }
  }
 }
 return true;
}
inline double cross(Point a,Point b,Point c){return double(b.x-a.x)*(c.y-a.y)-double(b.y-a.y)*(c.x-a.x);}
inline bool equal(Point a,Point b){return a.x==b.x&&a.y==b.y;}
}

inline bool fromMask(const uint8_t*bits,unsigned width,unsigned height,Polygons&out,std::string&error){
 out.clear();error.clear();
 if(!bits||!width||!height||width>EditorImages::MaxDimension||height>EditorImages::MaxDimension||uint64_t(width)*height>MaxPixels){error="Invalid alpha bitmap or image exceeds 16M-pixel limit";return false;}
 const detail::Mask mask{bits,width,height};std::vector<uint8_t> visited(size_t((uint64_t(width)*height+7)/8),0);
 size_t components=0,vertexBudget=0;Polygons result;
 for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
  const size_t p=size_t(y)*width+x;
  if((visited[p>>3]&(1u<<(p&7)))||!mask.at(int(x),int(y)))continue;
  if(++components>MaxComponents){error="Alpha image exceeds 4096-component complexity limit";return false;}
  if(!detail::march(mask,x,y,result,vertexBudget,error)||!detail::flood(mask,x,y,visited,error))return false;
 }
 const float centerX=float(width)*0.5f,centerY=float(height)*0.5f;
 for(auto&polygon:result)for(auto&point:polygon){point.x-=centerX;point.y-=centerY;}
 out=std::move(result);return true;
}
inline bool contours(const char*path,Polygons&out,std::string&error){
 out.clear();error.clear();EditorImages::AlphaMask mask;const char*decodeError=nullptr;
 if(!EditorImages::readAlpha(path,mask,decodeError)){error=decodeError?decodeError:"Cannot decode alpha image";return false;}
 return fromMask(mask.bits,mask.info.width,mask.info.height,out,error);
}
inline bool contours(const std::string&path,Polygons&out,std::string&error){return contours(path.c_str(),out,error);}
inline bool triangulate(const Polygons&polygons,Polygons&out,std::string&error){
 out.clear();error.clear();Polygons triangles;size_t total=0;
 for(const auto&source:polygons){
  Polygon points;
  for(auto p:source){if(!std::isfinite(p.x)||!std::isfinite(p.y)){error="Non-finite collision point";return false;}if(points.empty()||!detail::equal(points.back(),p))points.push_back(p);}
  if(points.size()>1&&detail::equal(points.front(),points.back()))points.pop_back();
  bool changed=true;
  while(changed&&points.size()>=3){changed=false;for(size_t i=0;i<points.size();++i){if(std::abs(detail::cross(points[(i+points.size()-1)%points.size()],points[i],points[(i+1)%points.size()]))<1e-9){points.erase(points.begin()+i);changed=true;break;}}}
  if(points.size()<3)continue;
  if(points.size()>MaxTriangulationVertices||total+points.size()>MaxVertices){error="Alpha polygon exceeds triangulation complexity limit";return false;}
  total+=points.size();double area=0;
  for(size_t i=0;i<points.size();++i){const auto a=points[i],b=points[(i+1)%points.size()];area+=double(a.x)*b.y-double(a.y)*b.x;}
  if(std::abs(area)<1e-9)continue;
  if(area<0)std::reverse(points.begin(),points.end());
  std::vector<size_t> indices(points.size());for(size_t i=0;i<indices.size();++i)indices[i]=i;
  while(indices.size()>3){
   bool found=false;
   for(size_t i=0;i<indices.size();++i){
    const auto a=points[indices[(i+indices.size()-1)%indices.size()]],b=points[indices[i]],c=points[indices[(i+1)%indices.size()]];
    if(detail::cross(a,b,c)<=1e-9)continue;
    bool blocked=false;
    for(size_t j=0;j<indices.size();++j){if(j==i||j==(i+1)%indices.size()||j==(i+indices.size()-1)%indices.size())continue;const auto p=points[indices[j]];if(detail::cross(a,b,p)>=-1e-9&&detail::cross(b,c,p)>=-1e-9&&detail::cross(c,a,p)>=-1e-9){blocked=true;break;}}
    if(blocked)continue;
    triangles.push_back({a,b,c});indices.erase(indices.begin()+i);found=true;break;
   }
   if(!found){error="Cannot triangulate self-intersecting alpha contour";return false;}
  }
  triangles.push_back({points[indices[0]],points[indices[1]],points[indices[2]]});
 }
 out=std::move(triangles);return true;
}
}
namespace Alpha=EditorAlpha;
