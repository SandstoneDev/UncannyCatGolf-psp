#pragma once
// Included inside EditorRender, after the original shader. This cache stores
// original unlit texels, not lit output: moving lights still run the same shader.
struct SampleSource {
  unsigned page=UINT_MAX;
  const std::string*file=nullptr;
  unsigned x=0,y=0,w=0,h=0;
};
inline const EditorShaderAxes::Axes*shaderAxesFor(const View&view,int left,int top,unsigned w,unsigned h,float step){
 const EditorShaderAxes::Key key{view.cameraX,view.cameraY,view.centerX,view.centerY,step,Scale,ViewX,EditorShaderAxes::nativeMode()};
 return EditorShaderAxes::bind(view.shaderAxes,key,left,top,w,h);
}
inline bool readSampleSource(void*context,const uint32_t*plan,size_t count,uint32_t*out){
  const auto&source=*static_cast<const SampleSource*>(context);
  const Texture texture=source.file?customPage(*source.file,source.x,source.y,source.w,source.h):builtinPage(source.page,true);
  if(!texture)return false;
  if(!source.file)zeroAlphaPages.observe(source.page,texture.palette,texture.colors,
      texture.format==GU_PSM_T8&&texture.width==256&&texture.height==256);
  for(size_t i=0;i<count;++i)out[i]=plan[i]==UINT_MAX?0:texturePixel(texture,plan[i]&255,plan[i]>>8);
  return true;
}
#ifdef UCG_EDITOR_ENABLE_LIT_MEMO
struct LitSampleSource{
  SampleSource source;const View*view;Lighting::FloatColor tint;
  int left,top;unsigned w,h;float step;
};
inline bool readLitSample(void*context,const uint32_t*plan,size_t count,uint32_t*out){
  auto&c=*static_cast<LitSampleSource*>(context);const auto&source=c.source;
  const auto key=source.file?EditorSampleCache::Key::custom(source.file->data(),source.file->size(),source.x,source.y,source.w,source.h):EditorSampleCache::Key::bank(source.page);
  const auto raw=sampleCache.get({key,c.w,c.h,plan,count},readSampleSource,&c.source);
  if(!raw)return false;
  const auto&view=*c.view;
  const bool transparent=std::isfinite(c.tint.a)&&std::isfinite(Lighting::ambient.a);
  const auto*coordinates=shaderAxesFor(view,c.left,c.top,c.w,c.h,c.step);
  for(unsigned y=0;y<c.h;++y)for(unsigned x=0;x<c.w;++x){
    const size_t i=y*c.w+x;if(plan[i]==UINT_MAX||(transparent&&!(raw.rgba[i]>>24))){out[i]=0;continue;}
    float worldX=coordinates?coordinates->x[c.left+x]:float(view.cameraX)+(c.left+x+.5f-ViewX-float(view.centerX)*Scale)*c.step,
          worldY=coordinates?coordinates->y[c.top+y]:float(view.cameraY)+(c.top+y+.5f-float(view.centerY)*Scale)*c.step;
    out[i]=Lighting::shade(raw.rgba[i],worldX,worldY,c.tint);
  }
  return true;
}
#endif
inline bool sampledShade(const View&view,const std::array<Vertex,4>&q,uint32_t tint,
                         const Color*sourceTint,unsigned page,const std::string*file,
                         unsigned sx,unsigned sy,unsigned sw,unsigned sh,EditorWholeCpu::Asset immutableAsset){
  float l=q[0].x,r=l,t=q[0].y,b=t;
  for(const auto&v:q){l=std::min(l,v.x);r=std::max(r,v.x);t=std::min(t,v.y);b=std::max(b,v.y);}
  const int left=std::max(int(clip.x),int(std::floor(l))),right=std::min(int(clip.right),int(std::ceil(r))),
            top=std::max(int(clip.y),int(std::floor(t))),bottom=std::min(int(clip.bottom),int(std::ceil(b)));
  if(left>=right||top>=bottom)return true;
  const unsigned w=unsigned(right-left),h=unsigned(bottom-top);
  if(w>256||h>256||uint64_t(w)*h>EditorSampleCache::MaxPixels)return false;
  float step=1/(float(view.zoom)*Scale),
        wx=float(view.cameraX)+(l-ViewX-float(view.centerX)*Scale)*step,
        wy=float(view.cameraY)+(t-float(view.centerY)*Scale)*step;
  Lighting::select(wx,wy,wx+(r-l)*step,wy+(b-t)*step);
  if(!Lighting::count&&!highColor(sourceTint)&&!highColor(&view.light))return false;
  if(!file&&zeroAlphaPages.canSkip(page,sourceTint?float(sourceTint->a):1.f,
                                Lighting::ambient.a))return true;
  // Only a complete already-owned builtin T8 palette is inspected. Preserve
  // all source RGBA words; the existing finite-alpha shader would emit only0.
  if(!file&&(!sourceTint||std::isfinite(float(sourceTint->a)))&&
     std::isfinite(Lighting::ambient.a))
    for(auto&cached:pages)
      if(cached.pixels&&cached.file.empty()&&cached.id==page){
        if(cached.format==GU_PSM_T8&&cached.bytes>=66560&&
           cached.colors&&cached.colors<=256){
          const Texture hot=texture(cached);
          if(zeroAlphaPages.observe(page,hot.palette,hot.colors,true)){
            cached.age=++clock;return true;
          }
        }
        break;
      }

  // Acquire BEFORE plan generation: allocation/reclaim/IO may borrow scratch.
  auto immutable=file&&EditorWholeCpu::known(immutableAsset)
      ?acquireWholeCpu(immutableAsset):EditorWholeCpu::Cache::Lease{};
  bool planReady=false;
  if(!file&&EditorPlanMemo::admitShape(w,h))
    for(const auto&cached:pages)
      if(cached.pixels&&cached.file.empty()&&cached.id==page){
        if(cached.format==GU_PSM_T8&&cached.bytes>=66560&&
           cached.colors&&cached.colors<=256)
          planReady=planCache.copy(EditorPlanMemo::Key::from(q,left,top,w,h),decodeScratch,65536);
        break;
      }
  // No page pointer survives cache allocation/reclaim. The existing scan below
  // revalidates residency, and generic fallback owns this complete copied plan.
  if(!planReady&&!EditorSamplePlan::build(q,left,top,w,h,256,256,decodeScratch))return false;
  if(immutable){
    const auto nodeTint=Lighting::tint(sourceTint?*sourceTint:color(tint));
    const bool transparent=std::isfinite(nodeTint.a)&&std::isfinite(Lighting::ambient.a);
    EditorRaster::clear(scratch,256,w,h);
    const auto*coordinates=shaderAxesFor(view,left,top,w,h,step);
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
      const size_t i=y*w+x;const uint32_t at=decodeScratch[i];
      if(at==UINT_MAX)continue;
      const uint32_t raw=immutable.chunkPixel(sx,sy,sw,sh,at);
      if(transparent&&!(raw>>24))continue;
      float worldX=coordinates?coordinates->x[left+x]:float(view.cameraX)+(left+x+.5f-ViewX-float(view.centerX)*Scale)*step,
            worldY=coordinates?coordinates->y[top+y]:float(view.cameraY)+(top+y+.5f-float(view.centerY)*Scale)*step;
      scratch[y*256+x]=Lighting::shade(raw,worldX,worldY,nodeTint);
    }
    nativeImage(scratch,unsigned(left),unsigned(top),w,h,White);return true;
  }
  // The caller plan lives in decodeScratch. Only already-owned hot pages are
  // safe here: a cold lookup/decoder could overwrite it, or evict this source.
  if(!file)for(auto&cached:pages)
    if(cached.pixels&&cached.file.empty()&&cached.id==page){
      const Texture hot=texture(cached);cached.age=++clock;
      const auto nodeTint=Lighting::tint(sourceTint?*sourceTint:color(tint));
      const bool transparent=std::isfinite(nodeTint.a)&&std::isfinite(Lighting::ambient.a);
      EditorRaster::clear(scratch,256,w,h);
      const auto*coordinates=shaderAxesFor(view,left,top,w,h,step);
      for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
        const size_t i=y*w+x;const uint32_t at=decodeScratch[i];
        if(at==UINT_MAX)continue;
        const uint32_t raw=texturePixel(hot,at&255,at>>8);
        if(transparent&&!(raw>>24))continue;
        float worldX=coordinates?coordinates->x[left+x]:float(view.cameraX)+(left+x+.5f-ViewX-float(view.centerX)*Scale)*step,
              worldY=coordinates?coordinates->y[top+y]:float(view.cameraY)+(top+y+.5f-float(view.centerY)*Scale)*step;
        scratch[y*256+x]=Lighting::shade(raw,worldX,worldY,nodeTint);
      }
      nativeImage(scratch,unsigned(left),unsigned(top),w,h,White);return true;
    }
  SampleSource source{page,file,sx,sy,sw,sh};
  const auto key=file?EditorSampleCache::Key::custom(file->data(),file->size(),sx,sy,sw,sh):EditorSampleCache::Key::bank(page);
  auto nodeTint=Lighting::tint(sourceTint?*sourceTint:color(tint));
#ifdef UCG_EDITOR_ENABLE_LIT_MEMO
  if(litCacheEnabled){
    EditorShadeKey::Input input;input.source=key;
    input.shader={float(view.cameraX),float(view.cameraY),float(view.centerX),float(view.centerY),float(view.zoom),step,0,0};
    input.left=left;input.top=top;input.width=w;input.height=h;
    input.nodeTint={nodeTint.r,nodeTint.g,nodeTint.b,nodeTint.a};
    input.selected=Lighting::active.data();input.selectedCount=Lighting::count;
    char encoded[EditorShadeKey::MaxKeyBytes];size_t encodedBytes=0;
    if(EditorShadeKey::encode(input,encoded,sizeof encoded,encodedBytes)==EditorShadeKey::Status::Ok){
      LitSampleSource context{source,&view,nodeTint,left,top,w,h,step};
      const auto lit=litCache.get({EditorSampleCache::Key::custom(encoded,encodedBytes,0,0,0,0),w,h,decodeScratch,size_t(w)*h},readLitSample,&context);
      if(lit){
        EditorRaster::clear(scratch,256,w,h);
        for(unsigned y=0;y<h;++y)std::memcpy(scratch+y*256,lit.rgba+y*w,w*4);
        nativeImage(scratch,unsigned(left),unsigned(top),w,h,White);return true;
      }
      // A failed decoder may have borrowed decodeScratch. Rebuild this caller's
      // original plan before the ordinary exact shader fallback.
      if(!EditorSamplePlan::build(q,left,top,w,h,256,256,decodeScratch))return false;
    }
  }
#endif
  const auto result=sampleCache.get({key,w,h,decodeScratch,size_t(w)*h},readSampleSource,&source);
  if(!result)return false;
  const bool transparent=std::isfinite(nodeTint.a)&&std::isfinite(Lighting::ambient.a);
  EditorRaster::clear(scratch,256,w,h);
  const auto*coordinates=shaderAxesFor(view,left,top,w,h,step);
  for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
    const size_t i=y*w+x;if(result.plan[i]==UINT_MAX||(transparent&&!(result.rgba[i]>>24)))continue;
    float worldX=coordinates?coordinates->x[left+x]:float(view.cameraX)+(left+x+.5f-ViewX-float(view.centerX)*Scale)*step,
          worldY=coordinates?coordinates->y[top+y]:float(view.cameraY)+(top+y+.5f-float(view.centerY)*Scale)*step;
    scratch[y*256+x]=Lighting::shade(result.rgba[i],worldX,worldY,nodeTint);
  }
  nativeImage(scratch,unsigned(left),unsigned(top),w,h,White);
  return true;
}
