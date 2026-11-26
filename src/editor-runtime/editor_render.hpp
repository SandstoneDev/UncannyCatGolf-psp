#pragma once
#include "editor_art.hpp"
#include "editor_raster.hpp"
#include "editor_identity_math.hpp"
#include "editor_projection_math.hpp"
#include "editor_raster_arena.hpp"
#include "editor_sample_plan.hpp"
#include "editor_shader_axes.hpp"
#include "editor_ground_cold_reclaim.hpp"
#include "editor_indexed_file.hpp"
#include "editor_transform_memo.hpp"
#include "editor_animation_memo.hpp"
#include "editor_sprite_prefix_memo.hpp"
#include "editor_zero_alpha_pages.hpp"
#include "editor_plan_memo.hpp"
#define UCG_EDITOR_SAMPLE_CACHE_BYTES (304 * 1024)
#define UCG_EDITOR_SAMPLE_CACHE_CAPACITY 512
#include "editor_sample_cache.hpp"
#include "editor_whole_cpu_index.hpp"
#ifdef UCG_EDITOR_ENABLE_LIT_MEMO
#include "editor_shade_key.hpp"
#endif
#include "editor_sample_hash.hpp"
#include "editor_source_rows.hpp"
#include "editor_nine_patch_axes.hpp"
#include "editor_tile_values.hpp"
#include "editor_texture_padding.hpp"
#include "editor_ground_cache.hpp"
#include "editor_ground_retire.hpp"
#include "editor_allocation_probe.hpp"
#include "editor_aux_visuals.hpp"
#include "editor_object_visuals.hpp"
#include "editor_picker_cells.hpp"
#include "editor_picker_icons.hpp"
#include "editor_visual_extras.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <new>
#include <string>
#include <vector>

// Included after the model/controller/play/image modules and the module's GU
// helpers.
namespace EditorRender {
using ucg::editor::Color;
using ucg::editor::Document;
using ucg::editor::Value;
using ucg::editor::Vec2;
inline constexpr float Scale = 17.f / 32;
inline constexpr unsigned PageBytes = 256 * 256 * 4, PageCacheCount = 96;
inline unsigned pageLimit = PageCacheCount, cacheAllocationFailures = 0;
inline bool cacheAllocationBlocked = false;
inline bool mutableTextureDirty = true;
inline void markTextureDirty() { mutableTextureDirty = true; }
struct Clip {
  float x = 172, y = 0, right = 444, bottom = 272;
};
inline Clip clip{};
inline unsigned clock = 0, textureReads = 0, customReads = 0, failedImages = 0;
inline EditorIndexedSource::File<EditorArt::Packed::Records> indexedSource;
inline unsigned indexedSourceReads=0,indexedSourceFallbacks=0,indexedSourceFaults=0;
#include "editor_decode_costs.hpp"
inline std::string lastError;
inline const std::string &getError() { return lastError; }
inline void clearError() { lastError.clear(); }
inline void fail(const std::string &message) {
  ++failedImages;
  if (lastError.empty())
    lastError = message;
}
inline void invalidateTiles();
inline void releaseRasterArena();
inline void borrowSharedSwizzle();
inline void releaseRows();
inline void releaseLighting();
inline void releasePicker();
inline void releaseThumbnails();
inline void releaseGhostFx();
inline void releasePlayerFx();
inline void releaseFontMasks();
namespace GroundRuntime {inline constexpr size_t accountedBytes();inline void invalidate();}
inline unsigned reservedSampleBytes();
inline size_t wholeCpuAccountedBytes();
inline bool wholeCpuSourceBacked(const std::string&);
#define UCG_EDITOR_SAMPLE_RESERVED_BYTES reservedSampleBytes()
#include "editor_texture_cache.hpp"
#undef UCG_EDITOR_SAMPLE_RESERVED_BYTES
inline void*allocateSample(size_t,void*);
inline void releaseSample(void*p,void*){std::free(p);}
inline EditorSampleCache::Cache sampleCache({allocateSample,releaseSample,nullptr},EditorSampleHash::words);
inline EditorSourceRows::Cache sourceRows;
inline EditorPlanMemo::Cache planCache({allocateSample,releaseSample,nullptr});
#ifdef UCG_EDITOR_ENABLE_LIT_MEMO
inline void*allocateLit(size_t,void*);
inline EditorSampleCache::BasicCache<2*1024*1024,2048> litCache({allocateLit,releaseSample,nullptr},EditorSampleHash::words);
inline EditorShadeKey::Epoch lightingEpoch;
inline bool litCacheEnabled=false;
#endif
inline EditorAnimationMemo::Cache<> animationMemo;
inline EditorSpritePrefixMemo::Cache<Vec2> spritePrefixMemo;
inline EditorZeroAlphaPages::Set<EditorArt::Packed::Records> zeroAlphaPages;
inline unsigned reservedSampleBytes(){
  unsigned bytes=unsigned(sizeof indexedSource+EditorAnimationMemo::Cache<>::fixedBytes()+EditorSpritePrefixMemo::Cache<Vec2>::fixedBytes()+EditorZeroAlphaPages::Set<EditorArt::Packed::Records>::fixedBytes()+sampleCache.accountedBytes()+planCache.accountedBytes()+EditorSourceRows::Cache::fixedBytes()+wholeCpuAccountedBytes()+GroundRuntime::accountedBytes()
#ifdef UCG_EDITOR_CAPTURE
    +::EditorGroundNativeProof::FixedBytes+sizeof decodeCosts
#endif
  );
#ifdef UCG_EDITOR_ENABLE_LIT_MEMO
  bytes+=unsigned(litCache.accountedBytes()+EditorShadeKey::EpochBytes);
#endif
  bytes+=unsigned(EditorShaderAxes::Cache::fixedBytes());
  return bytes;
}
alignas(64) inline uint32_t scratch[256 * 256]{};
alignas(64) inline uint32_t decodeScratch[256 * 256]{};
alignas(64) inline uint8_t swizzled[PageBytes]{};
inline FILE *tileFile = nullptr;
inline FILE *floorFile = nullptr;
inline std::array<std::array<uint32_t, 4>, 11> floorIndex{};
inline unsigned cacheGeneration = 0;
inline uint32_t rgba(Color c) {
  auto b = [](double v) {
    return uint32_t(std::lround(std::clamp(v, 0., 1.) * 255));
  };
  return b(c.r) | (b(c.g) << 8) | (b(c.b) << 16) | (b(c.a) << 24);
}
inline Color color(uint32_t c) {
  return {(c & 255) / 255., ((c >> 8) & 255) / 255., ((c >> 16) & 255) / 255.,
          (c >> 24) / 255.};
}
inline Color multiply(Color a, Color b) {
  return {a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a};
}
inline Color readColor(const Value &root, const char *key,
                       Color fallback = {}) {
  const auto *p = root.find(key);
  Color c;
  return p && ucg::editor::read_color(*p, c) ? c : fallback;
}
inline double number(const Value &root, const char *key, double fallback = 0) {
  const auto *p = root.find(key);
  return p ? p->numeric(fallback) : fallback;
}
inline std::string string(const Value &root, const char *key) {
  const auto *p = root.find(key);
  return p && p->kind == Value::Kind::String ? p->text : std::string();
}
inline Vec2 vector(const Value &v, Vec2 fallback = {}) {
  Vec2 p;
  return ucg::editor::read_vector(v, p) ? p : fallback;
}
inline Vec2 add(Vec2 a, Vec2 b) {
  return {EditorIdentityMath::add(a.x, b.x), EditorIdentityMath::add(a.y, b.y)};
}
inline Vec2 sub(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 mul(Vec2 a, Vec2 b) {
  return {EditorIdentityMath::multiply(a.x, b.x),
          EditorIdentityMath::multiply(a.y, b.y)};
}
inline Vec2 rotate(Vec2 a, double angle) {
  if (EditorIdentityMath::zeroRotation(a.x, a.y, angle))
    return a;
  double c = std::cos(angle), s = std::sin(angle);
  return {a.x * c - a.y * s, a.x * s + a.y * c};
}
inline std::string folder(const std::string &file) {
  const auto p = file.find_last_of("/\\");
  return p == std::string::npos ? std::string() : file.substr(0, p + 1);
}
inline std::string path(const std::string &dir, const std::string &file) {
  if (file.empty())
    return {};
  if (file[0] == '/' || file.find(":/") != std::string::npos)
    return file;
  return dir + file;
}
struct ImageInfoEntry {
  std::string file;
  EditorImages::Info info{};
  unsigned age = 0;
  bool valid = false;
};
inline std::array<ImageInfoEntry, 64> imageInfo{};
inline bool inspectImage(const char *file, EditorImages::Info &info) {
  for (auto &i : imageInfo)
    if (i.file == file) {
      i.age = ++clock;
      info = i.info;
      return i.valid;
    }
  auto *entry = &imageInfo[0];
  for (auto &i : imageInfo)
    if (i.file.empty()) {
      entry = &i;
      break;
    } else if (i.age < entry->age)
      entry = &i;
  EditorImages::Info loaded{};
  bool ok = EditorImages::inspect(file, loaded);
  *entry = {file, loaded, ++clock, ok};
  info = loaded;
  return ok;
}
inline int image(const char *name) {
  if (!name)
    return -1;
  const char *p = !std::strncmp(name, "res://", 6) ? name + 6 : name;
  for (unsigned i = 0; i < std::size(EditorArt::Packed::Images); ++i)
    if (!std::strcmp(EditorArt::Packed::Images[i].path, p))
      return int(i);
  return -1;
}
inline void ensureBatch() {
  if (!drawOpen)
    beginBatch();
}
inline void applyClip() {
  sceGuScissor(int(clip.x), int(clip.y), int(clip.right), int(clip.bottom));
}
inline unsigned swizzleOffset(unsigned x, unsigned y) {
  return ((y / 8) * 64 + x / 4) * 128 + (y % 8) * 16 + (x % 4) * 4;
}
inline void swizzleRGBA(const uint32_t *in, uint8_t *out) {
  for (unsigned y = 0; y < 256; ++y)
    for (unsigned x = 0; x < 256; ++x)
      std::memcpy(out + swizzleOffset(x, y), in + y * 256 + x, 4);
}
struct ColdStage {
  const Cold::Entry*entry=nullptr;
  unsigned serial=0,token=0;
  bool promotionTried=false;
};
inline std::array<ColdStage,EditorArt::CacheSlots> coldStages{};
inline unsigned coldStageDecodes=0,coldStageHits=0,coldPromotions=0;
inline unsigned hotReserveDenials=0,coldReserveDenials=0;
#if defined(UCG_EDITOR_CAPTURE) && defined(UCG_EDITOR_CACHE_SEGMENTS)
inline constexpr unsigned ReservedColdSegments=UCG_EDITOR_CACHE_SEGMENTS;
static_assert(ReservedColdSegments>=1&&ReservedColdSegments<=Cold::SegmentPool::MaxSegments);
#else
inline constexpr unsigned ReservedColdSegments=20;
#endif
inline bool coldPoolAttempted=false;
inline unsigned coldPoolInitializationAttempts=0,coldPoolInitializationFailures=0;
inline void*protectedTextureAllocation(size_t bytes,bool aligned){
  void*memory=aligned?memalign(64,bytes):std::malloc(bytes);
  if(!memory)return nullptr;
  // Keep room for the live audio decoder, PNG rows and small UI transactions.
  // Probe real allocations rather than mallinfo, whose arena can still grow.
  if(!EditorMemory::probe(512*1024,128*1024,2)){
    std::free(memory);
    if(aligned)++hotReserveDenials;else ++coldReserveDenials;
    return nullptr;
  }
  return memory;
}
inline void*protectedHotAlloc(size_t bytes){return protectedTextureAllocation(bytes,true);}
inline void*protectedColdAlloc(size_t bytes){return protectedTextureAllocation(bytes,false);}
inline void*allocateCpuCache(size_t bytes){
  for(unsigned attempt=0;attempt<=PageCacheCount;++attempt){
    if(uint64_t(cacheTotalBytes())+reservedSampleBytes()+bytes<=CacheBudget)
      if(void*p=protectedColdAlloc(bytes))return p;
    Page*oldest=nullptr;
    for(auto&p:pages)if(p.pixels&&(!oldest||p.age<oldest->age))oldest=&p;
    if(!oldest)return nullptr;
    flushBatch();discardPage(*oldest);
  }
  return nullptr;
}
#include "editor_whole_cpu_runtime.hpp"
inline void*allocateSample(size_t bytes,void*){return allocateCpuCache(bytes);}
#ifdef UCG_EDITOR_ENABLE_LIT_MEMO
inline void*allocateLit(size_t bytes,void*){return allocateCpuCache(bytes);}
#endif
inline bool evictColdPoolEntry(){
  auto*entry=coldCache.oldest();if(!entry)return false;
  coldCache.erase(*entry);return true;
}
inline void releaseReservedCold(){
  Cold::releaseColdPool(coldCache);
  coldPoolAttempted=false;
}
inline void clearColdStages(){for(auto&stage:coldStages)stage=ColdStage{};}
inline void prepareCold(){
  if(EditorArt::ready&&EditorArt::workspace){
    configureCold(EditorArt::workspace,Cold::WorkspaceBytes);
    pageAllocate=protectedHotAlloc;
    if(!coldPoolAttempted){
      coldPoolAttempted=true;++coldPoolInitializationAttempts;
      const unsigned resident=coldCache.residentBytes();
      const unsigned fixed=cacheTotalBytes()-resident;
      if(!Cold::prepareColdPool(coldCache,ReservedColdSegments,protectedColdAlloc,std::free,
                               evictColdPoolEntry,fixed<cacheAvailableBudget()?cacheAvailableBudget()-fixed:0)){
        ++coldPoolInitializationFailures;
        coldCache.allocate=protectedColdAlloc;
      }
    }
  }
}
inline bool stageValid(unsigned slot){
  const auto&stage=coldStages[slot];
  return stage.entry&&stage.entry->blocks&&stage.entry->serial==stage.serial&&
    EditorArt::cached[slot]==-1&&EditorArt::ages[slot]==stage.token;
}
inline Texture stagedCold(Cold::Entry&entry,bool cpuResident){
  int slot=-1;
  for(unsigned i=0;i<coldStages.size();++i)
    if(stageValid(i)&&coldStages[i].entry==&entry&&coldStages[i].serial==entry.serial){slot=int(i);++coldStageHits;break;}
  if(slot<0){
    // decodeScratch may still be queued by the ambient-only shader fallback.
    // Complete all earlier readers before borrowing it, even if promotion fails.
    flushBatch();
    auto*decoded=reinterpret_cast<uint8_t*>(decodeScratch);
    const bool decodedOkay=[&](){UCG_DECODE_COST(entry.id,0);return coldCache.decode(entry,decoded,PageBytes,EditorArt::workspace,Cold::WorkspaceBytes);}();
    if(!decodedOkay){
      fail(std::string("Invalid lossless editor cache page: ")+coldCache.error);
      coldCache.erase(entry);return {};
    }
    ++coldStageDecodes;
    const bool attemptedPromotion=cpuResident||entry.format==GU_PSM_T8;
    if(attemptedPromotion){
      if(auto*hot=promoteCold(entry,decoded)){++coldPromotions;return texture(*hot);}
    }
    // Only GPU-needed/denied-promotion output enters VRAM. Keep immutable RAM
    // private: allocator callbacks must not reenter renderer lookups while
    // promoteCold copies from this scratch (true of current production hooks).
    unsigned chosen=0;
    for(unsigned i=0;i<coldStages.size();++i){
      if(!stageValid(i)){chosen=i;break;}
      if(coldStages[i].token<coldStages[chosen].token)chosen=i;
    }
    slot=int(chosen);coldStages[chosen]=ColdStage{};EditorArt::cached[chosen]=-1;
    auto*target=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(EditorArt::video(EditorArt::CacheStart+chosen*EditorArt::SlotBytes))|0x40000000u);
    std::memcpy(target,decoded,entry.rawBytes);
    coldStages[chosen]={&entry,entry.serial,++EditorArt::age,attemptedPromotion};
    EditorArt::ages[chosen]=coldStages[chosen].token;
    EditorArt::textureDirty=true;markTextureDirty();
  }else{
    coldStages[unsigned(slot)].token=++EditorArt::age;
    EditorArt::ages[unsigned(slot)]=coldStages[unsigned(slot)].token;
  }
  auto*target=reinterpret_cast<const uint8_t*>(reinterpret_cast<uintptr_t>(EditorArt::video(EditorArt::CacheStart+unsigned(slot)*EditorArt::SlotBytes))|0x40000000u);
  if((cpuResident||entry.format==GU_PSM_T8)&&!coldStages[unsigned(slot)].promotionTried){
    coldStages[unsigned(slot)].promotionTried=true;
    if(auto*hot=promoteCold(entry,target)){++coldPromotions;return texture(*hot);}
  }
  return {target,entry.format,entry.format==GU_PSM_T8?reinterpret_cast<const uint32_t*>(target+65536):nullptr,entry.colors,256,256};
}
inline Texture builtinPage(unsigned id,bool cpuResident=false){
 prepareCold();
 for(auto&p:pages)if(p.pixels&&p.file.empty()&&p.id==id){p.age=++clock;return texture(p);}
 if(auto*cold=findCold(id))if(auto pixels=stagedCold(*cold,cpuResident))return pixels;
 if(id>=EditorArt::Packed::StaticPages&&id<EditorArt::records.size()){
  const auto opened=indexedSource.open(companion(executable.c_str(),"data/world/editor-source-indexed.bin").c_str());
  auto result=opened;
  if(opened==EditorIndexedSource::Result::Ready){
   // Scratch and eDRAM may still have queued GE readers. This is the same
   // publication fence used by cold page staging, before either is borrowed.
   flushBatch();
   UCG_DECODE_COST(id,2);
   result=indexedSource.reader.decode(id,EditorArt::records[id].crc,decodeScratch,sizeof decodeScratch,EditorArt::workspace,EditorArt::InflateWorkspaceRequired);
  }
  if(result==EditorIndexedSource::Result::Fault){++indexedSourceFaults;fail("Invalid exact indexed editor source: "+std::to_string(id));return {};}
  if(result==EditorIndexedSource::Result::Ready){
   ++indexedSourceReads;
   const auto*decoded=reinterpret_cast<const uint8_t*>(decodeScratch);
   const unsigned colors=indexedSource.reader.colors(id);
   if(auto*p=storePage(decoded,id,nullptr,0,0,0,0,GU_PSM_T8,colors,nullptr,true))return texture(*p);
   ++indexedSourceFallbacks;
   EditorArt::PageWriteScope writeScope;
   coldStages[0]=ColdStage{};EditorArt::cached[0]=-1;EditorArt::ages[0]=++EditorArt::age;
   auto*target=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(EditorArt::video(EditorArt::CacheStart))|0x40000000u);
   std::memcpy(target,decoded,EditorIndexedSource::PayloadBytes);
   EditorArt::textureDirty=true;markTextureDirty();
   return {target,GU_PSM_T8,reinterpret_cast<const uint32_t*>(target+65536),colors,256,256};
  }
 }
 flushBatch();int resident=EditorArt::resident(id);if(resident<0){UCG_DECODE_COST(id,1);if(!EditorArt::inflatePage(id,EditorArt::CacheStart)){fail("Original editor texture record failed CRC/inflate: "+std::to_string(id));return {};}
 EditorArt::cached[0]=int(id);EditorArt::ages[0]=++EditorArt::age;EditorArt::textureDirty=true;markTextureDirty();resident=int(EditorArt::CacheStart);++textureReads;}
 const auto*source=reinterpret_cast<const uint8_t*>(reinterpret_cast<uintptr_t>(EditorArt::video(unsigned(resident)))|0x40000000u);
 if(auto*p=storePage(source,id)){return texture(*p);}
 return {source,GU_PSM_8888,nullptr,0,256,256};
}
inline Texture customPage(const std::string&file,unsigned x,unsigned y,unsigned w,unsigned h){
 prepareCold();
 for(auto&p:pages)if(p.pixels&&p.file==file&&p.x==x&&p.y==y&&p.w==w&&p.h==h){p.age=++clock;return texture(p);}
 if(auto*cold=findCold(UINT_MAX,&file,x,y,w,h))if(auto pixels=stagedCold(*cold,false))return pixels;
 flushBatch();if(!EditorImages::read(file.c_str(),{x,y,w,h},decodeScratch,256)){fail("Cannot decode original/custom image: "+file);return {};}
 if(!EditorTexturePadding::clear(decodeScratch,w,h)){fail("Invalid editor custom image cache region");return {};}
 ++customReads;borrowSharedSwizzle();swizzleRGBA(decodeScratch,swizzled);if(auto*p=storePage(swizzled,UINT_MAX,&file,x,y,w,h))return texture(*p);
 sceKernelDcacheWritebackRange(swizzled,PageBytes);markTextureDirty();borrowSharedSwizzle();return {swizzled,GU_PSM_8888,nullptr,0,256,256};
}
inline Texture immutableCustomPage(const std::string&file,unsigned x,unsigned y,unsigned w,unsigned h,EditorWholeCpu::Asset asset){
 if(!EditorWholeCpu::known(asset))return customPage(file,x,y,w,h);
 prepareCold();
 for(auto&p:pages)if(p.pixels&&p.file==file&&p.x==x&&p.y==y&&p.w==w&&p.h==h){p.age=++clock;return texture(p);}
 // Acquiring can evict hot pages and borrow the shared inflate workspace.
 // No page/plan pointer survives it; the lease pins its immutable CPU source.
 auto source=acquireWholeCpu(asset);
 if(!source)return customPage(file,x,y,w,h);
 flushBatch();
 if(bindWholeCpuSource(asset,file)){
  for(auto&entry:coldCache.entries)
   if(entry.blocks && &entry!=coldCache.pinned && entry.file && file==entry.file)coldCache.erase(entry);
  if(coldCache.residentCharge==Cold::poolResidentBytes&&Cold::pool.owner==&coldCache)Cold::pool.releaseUnusedSegments();
 }
 if(!source.swizzleRegion(x,y,w,h,decodeScratch,sizeof decodeScratch)){fail("Invalid immutable editor source region");return {};}
 const auto*decoded=reinterpret_cast<const uint8_t*>(decodeScratch);
 if(auto*p=storePage(decoded,UINT_MAX,&file,x,y,w,h,0,0,nullptr,true))return texture(*p);
 EditorArt::PageWriteScope writeScope;
 coldStages[0]=ColdStage{};EditorArt::cached[0]=-1;EditorArt::ages[0]=++EditorArt::age;
 auto*target=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(EditorArt::video(EditorArt::CacheStart))|0x40000000u);
 std::memcpy(target,decoded,PageBytes);EditorArt::textureDirty=true;markTextureDirty();
 return {target,GU_PSM_8888,nullptr,0,256,256};
}
inline void invalidate() {
  flushBatch();
  releaseRasterArena();
  sampleCache.clear();planCache.clear();sourceRows.clear();releaseWholeCpu();GroundRuntime::invalidate();animationMemo.clear();spritePrefixMemo.clear();zeroAlphaPages.clear();
#ifdef UCG_EDITOR_ENABLE_LIT_MEMO
  litCache.clear();lightingEpoch.invalidate();litCacheEnabled=false;
#endif
  clearCold();clearColdStages();releaseReservedCold();
  for (auto &p : pages) {
    p.id = UINT_MAX;
    p.file.clear();
    p.age = 0;
  }
  ++cacheGeneration;
  cacheAllocationBlocked = false;
  for (auto &i : imageInfo)
    i = ImageInfoEntry{};
  invalidateTiles();
  clearError();
}
inline void shutdown() {
  flushBatch();
  indexedSource.close();
  releaseRasterArena();
  sampleCache.clear();planCache.clear();sourceRows.clear();releaseWholeCpu();GroundRuntime::invalidate();animationMemo.clear();spritePrefixMemo.clear();zeroAlphaPages.clear();
#ifdef UCG_EDITOR_ENABLE_LIT_MEMO
  litCache.clear();lightingEpoch.invalidate();litCacheEnabled=false;
#endif
  clearCold();clearColdStages();releaseReservedCold();configureCold(nullptr,0);
  cacheAllocationBlocked = false;
  for (auto &p : pages) {
    discardPage(p);
  }
  if (tileFile)
    std::fclose(tileFile);
  if (floorFile)
    std::fclose(floorFile);
  tileFile = floorFile = nullptr;
  releaseRows();
  releaseLighting();
  releasePicker();
  releaseThumbnails();
  releaseGhostFx();
  releasePlayerFx();
  releaseFontMasks();
}
// Reclaim only immutable decoded pages before an editor document transaction.
// Retain source FX clocks, prepared geometry and the independent floor raster.
inline void reclaimPageMemory(){
  flushBatch();
  releaseRasterArena();
  sampleCache.clear();planCache.clear();sourceRows.clear();releaseWholeCpu();GroundRuntime::invalidate();animationMemo.clear();spritePrefixMemo.clear();zeroAlphaPages.clear();
#ifdef UCG_EDITOR_ENABLE_LIT_MEMO
  litCache.clear();lightingEpoch.invalidate();litCacheEnabled=false;
#endif
  clearCold();clearColdStages();releaseReservedCold();
  for(auto&p:pages)discardPage(p);
  cacheAllocationBlocked=false;
}
inline void releaseCaches() {
  shutdown();
  invalidateTiles();
}
struct Vertex {
  float x, y, u, v;
};
inline Vertex mix(Vertex a, Vertex b, float t) {
  return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.u + (b.u - a.u) * t,
          a.v + (b.v - a.v) * t};
}
inline unsigned clipAxis(const Vertex *in, unsigned count, Vertex *out,
                         unsigned axis, float limit, bool greater) {
  if (!count)
    return 0;
  unsigned n = 0;
  auto value = [&](Vertex a) {
    return axis == 0 ? a.x : axis == 1 ? a.y : axis == 2 ? a.u : a.v;
  };
  Vertex previous = in[count - 1];
  bool before = greater ? value(previous) >= limit : value(previous) <= limit;
  for (unsigned i = 0; i < count; ++i) {
    Vertex current = in[i];
    bool inside = greater ? value(current) >= limit : value(current) <= limit;
    if (inside != before) {
      float a = value(previous), b = value(current);
      out[n++] = mix(previous, current, b != a ? (limit - a) / (b - a) : 0);
    }
    if (inside)
      out[n++] = current;
    previous = current;
    before = inside;
  }
  return n;
}
inline unsigned clipped(const Vertex *in, unsigned n, Vertex *out) {
  Vertex a[24], b[24];
  std::copy_n(in, n, a);
  n = clipAxis(a, n, b, 0, clip.x, true);
  n = clipAxis(b, n, a, 0, clip.right, false);
  n = clipAxis(a, n, b, 1, clip.y, true);
  n = clipAxis(b, n, a, 1, clip.bottom, false);
  std::copy_n(a, n, out);
  return n;
}
inline void polygonTexture(const uint8_t *pixels, unsigned format,
                           unsigned width, unsigned height,
                           const Vertex *vertices, unsigned count,
                           uint32_t tint = White,
                           const uint32_t *palette = nullptr,
                           unsigned colors = 0) {
  Vertex out[24];
  count = clipped(vertices, count, out);
  if (!pixels || count < 3)
    return;
  ensureBatch();
  applyClip();
  if (mutableTextureDirty) {
    sceGuTexFlush();
    mutableTextureDirty = false;
  }
  setTexture(format, width, height, pixels);
  sceGuTexFilter(GU_NEAREST, GU_NEAREST);
  if (format == GU_PSM_T8 && palette) {
    sceGuClutMode(GU_PSM_8888, 0, 255, 0);
    sceGuClutLoad((colors + 7) / 8, palette);
  }
  auto *v = static_cast<TextureVertex *>(
      sceGuGetMemory(count * sizeof(TextureVertex)));
  for (unsigned i = 0; i < count; ++i)
    v[i] = {out[i].u, out[i].v, tint, out[i].x, out[i].y, 0};
  sceGuDrawArray(GU_TRIANGLE_FAN,
                 GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF |
                     GU_TRANSFORM_2D,
                 count, nullptr, v);
}
inline void solidPolygon(const Vertex *vertices, unsigned count,
                         uint32_t tint) {
  Vertex out[24];
  count = clipped(vertices, count, out);
  if (count < 3)
    return;
  ensureBatch();
  applyClip();
  sceGuDisable(GU_TEXTURE_2D);
  auto *v =
      static_cast<ColorVertex *>(sceGuGetMemory(count * sizeof(ColorVertex)));
  for (unsigned i = 0; i < count; ++i)
    v[i] = {tint, out[i].x, out[i].y, 0};
  sceGuDrawArray(GU_TRIANGLE_FAN,
                 GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, count,
                 nullptr, v);
}
struct View {
  const EditorController::Controller &controller;
  const EditorPlay::Scene *scene = nullptr;
  double zoom = 1;
  Vec2 offset{};
  Color light{};
  std::string dir;
  double seconds = 0;
  double centerX = 384, centerY = 256, cameraX = 0, cameraY = 0,
         inverseZoom = 1;
  mutable EditorShaderAxes::Cache shaderAxes;
  View(const EditorController::Controller &c,
       const EditorPlay::Scene *s = nullptr)
      : controller(c), scene(s), zoom(s ? 256. / s->bounds : c.zoom()),
        offset(s ? Vec2{s->worldOffset.x, s->worldOffset.y} : Vec2{}),
        light(readColor((c.painting ? c.stroke : c.model.document).root,
                        "light")),
        dir(folder(c.path)), seconds(s ? s->tick / 60. : 0) {
    inverseZoom = 1 / zoom;
    centerX = 384;
    cameraX = s ? s->camera.x : c.camera.x + 256 * c.rawZoom() - 256;
    cameraY = s ? s->camera.y : c.camera.y;
  }
  bool mathNearestEven=EditorProjectionMath::nativeNearestEven();
  bool mathRange(Vec2 p,double factor) const {
    return EditorProjectionMath::withinProjectionRange(p.x) && EditorProjectionMath::withinProjectionRange(p.y) &&
      EditorProjectionMath::withinProjectionRange(centerX) && EditorProjectionMath::withinProjectionRange(centerY) &&
      EditorProjectionMath::withinProjectionRange(cameraX) && EditorProjectionMath::withinProjectionRange(cameraY) &&
      EditorProjectionMath::withinProjectionRange(factor);
  }
  Vec2 screen(Vec2 p) const {
    if(!mathRange(p,zoom))
      return {centerX + (p.x - cameraX) * zoom, centerY + (p.y - cameraY) * zoom};
    return {EditorProjectionMath::add(centerX,EditorProjectionMath::multiply(EditorProjectionMath::subtract(p.x,cameraX),zoom)),
            EditorProjectionMath::add(centerY,EditorProjectionMath::multiply(EditorProjectionMath::subtract(p.y,cameraY),zoom))};
  }
  Vec2 world(Vec2 s) const {
    if(!mathRange(s,inverseZoom))
      return {(s.x - centerX) * inverseZoom + cameraX,
              (s.y - centerY) * inverseZoom + cameraY};
    return {EditorProjectionMath::add(EditorProjectionMath::multiply(EditorProjectionMath::subtract(s.x,centerX),inverseZoom),cameraX),
            EditorProjectionMath::add(EditorProjectionMath::multiply(EditorProjectionMath::subtract(s.y,centerY),inverseZoom),cameraY)};
  }
  Vertex vertex(Vec2 p, float u = 0, float v = 0) const {
    Vec2 s = screen(p);
    if(!mathRange(p,zoom)) return {float(ViewX + s.x * Scale), float(s.y * Scale), u, v};
    return {float(EditorProjectionMath::add(ViewX,EditorProjectionMath::scale17by32Checked(s.x,mathNearestEven))),
            float(EditorProjectionMath::scale17by32Checked(s.y,mathNearestEven)),u,v};
  }
};
using PlayerHook = bool (*)(const View &, Vec2, Vec2, double, Color);
inline PlayerHook playerHook = nullptr;
inline PlayerHook ghostHook = nullptr;
inline PlayerHook ghostPlainHook = nullptr;
#include "editor_lighting.hpp"
inline void releaseLighting() { Lighting::close(); }
inline void nativeImage(const uint32_t *, unsigned, unsigned, unsigned,
                        unsigned, uint32_t);
inline void shadeTexture(const View &, const uint8_t *,
                         const std::array<Vertex, 4> &, uint32_t);
inline void shadeTexture(const View &, const uint8_t *, unsigned, unsigned,
                         unsigned, const std::array<Vertex, 4> &, uint32_t,
                         const uint32_t * = nullptr, unsigned = 0,
                         const Color * = nullptr);
inline bool sampledShade(const View&,const std::array<Vertex,4>&,uint32_t,const Color*,unsigned,const std::string* = nullptr,unsigned=0,unsigned=0,unsigned=0,unsigned=0,EditorWholeCpu::Asset=EditorWholeCpu::Asset::Unknown);
inline bool highColor(const Color *tint) {
  return tint && (tint->r > 1 || tint->g > 1 || tint->b > 1 || tint->a > 1 ||
                  tint->r < 0 || tint->g < 0 || tint->b < 0 || tint->a < 0);
}
inline void unlitTexture(Texture, const std::array<Vertex, 4> &, Color);
inline uint32_t ambientTint(uint32_t c, const View *view) {
  return view ? rgba(multiply(color(c), view->light)) : c;
}
inline Vertex uiVertex(Vec2 p, float u, float v) {
  return {float(ViewX + p.x * Scale), float(p.y * Scale), u, v};
}
inline bool visible(const std::array<Vertex, 4> &v) {
  float l = v[0].x, r = l, t = v[0].y, b = t;
  for (const auto &p : v) {
    l = std::min(l, p.x);
    r = std::max(r, p.x);
    t = std::min(t, p.y);
    b = std::max(b, p.y);
  }
  return r >= clip.x && l <= clip.right && b >= clip.y && t <= clip.bottom;
}
template <class Transform>
inline void builtinImage(unsigned id, EditorArt::Rect region,
                         const Transform &transform, uint32_t tint = White,
                         const View *view = nullptr,
                         const Color *sourceTint = nullptr) {
  if (id >= std::size(EditorArt::Packed::Images) || region.w <= 0 ||
      region.h <= 0)
    return;
  const auto &f =
      EditorArt::Packed::Frames[EditorArt::Packed::Images[id].frame];
  EditorTransformMemo::Cache<Vec2,Vertex> transformed;
  for (unsigned i = 0; i < f.count; ++i) {
    const auto &p = EditorArt::Packed::Parts[f.first + i];
    float l = std::max(region.x, p.x), t = std::max(region.y, p.y),
          r = std::min(region.x + region.w, p.x + p.w),
          b = std::min(region.y + region.h, p.y + p.h);
    if (l >= r || t >= b)
      continue;
    std::array<Vertex, 4> q{};
    const float xs[] = {l, r, r, l}, ys[] = {t, t, b, b};
    for (unsigned k = 0; k < 4; ++k) {
      q[k] = transformed.get(Vec2{xs[k], ys[k]},transform);
      q[k].u = p.u + (xs[k] - p.x) / p.w * p.width;
      q[k].v = p.v + (ys[k] - p.y) / p.h * p.height;
    }
    if (!visible(q))
      continue;
    if(view&&(!Lighting::points.empty()||highColor(sourceTint)||highColor(&view->light))&&
       sampledShade(*view,q,tint,sourceTint,p.page))continue;
    const auto pixels = builtinPage(p.page);
    if (view && (!Lighting::points.empty() || highColor(sourceTint) ||
                 highColor(&view->light)))
      shadeTexture(*view, pixels.pixels, pixels.format, pixels.width, pixels.height, q, tint, pixels.palette, pixels.colors,
                   sourceTint);
    else if (!view && highColor(sourceTint))
      unlitTexture(pixels, q, *sourceTint);
    else
      polygonTexture(pixels.pixels, pixels.format, pixels.width, pixels.height, q.data(), 4, ambientTint(tint, view),pixels.palette,pixels.colors);
  }
}
template <class Transform>
inline void customImage(const std::string &file, const EditorImages::Info &info,
                        EditorArt::Rect region, const Transform &transform,
                        uint32_t tint = White, const View *view = nullptr,
                        const Color *sourceTint = nullptr,
                        EditorWholeCpu::Asset immutableAsset=EditorWholeCpu::Asset::Unknown) {
  EditorTransformMemo::Cache<Vec2,Vertex> transformed;
  const unsigned firstX =
                     unsigned(std::max(0.f, std::floor(region.x / 240) * 240)),
                 firstY =
                     unsigned(std::max(0.f, std::floor(region.y / 240) * 240));
  for (unsigned y = firstY; y < info.height && float(y) < region.y + region.h;
       y += 240)
    for (unsigned x = firstX; x < info.width && float(x) < region.x + region.w;
         x += 240) {
      float l = std::max(region.x, float(x)), t = std::max(region.y, float(y)),
            r = std::min(region.x + region.w,
                         float(std::min(x + 240, info.width))),
            b = std::min(region.y + region.h,
                         float(std::min(y + 240, info.height)));
      if (l >= r || t >= b)
        continue;
      std::array<Vertex, 4> q{};
      const float xs[] = {l, r, r, l}, ys[] = {t, t, b, b};
      for (unsigned k = 0; k < 4; ++k) {
        q[k] = transformed.get(Vec2{xs[k], ys[k]},transform);
        q[k].u = xs[k] - x;
        q[k].v = ys[k] - y;
      }
      if (!visible(q))
        continue;
      if(view&&(!Lighting::points.empty()||highColor(sourceTint)||highColor(&view->light))&&
         sampledShade(*view,q,tint,sourceTint,UINT_MAX,&file,x,y,std::min(240u,info.width-x),std::min(240u,info.height-y),immutableAsset))continue;
      const auto pixels =
          immutableCustomPage(file, x, y, std::min(240u, info.width - x),
                     std::min(240u, info.height - y),immutableAsset);
      if (view && (!Lighting::points.empty() || highColor(sourceTint) ||
                   highColor(&view->light)))
        shadeTexture(*view, pixels.pixels, pixels.format, pixels.width, pixels.height, q, tint, pixels.palette, pixels.colors,
                     sourceTint);
      else if (!view && highColor(sourceTint))
        unlitTexture(pixels, q, *sourceTint);
      else
        polygonTexture(pixels.pixels, pixels.format, pixels.width, pixels.height, q.data(), 4,
                       ambientTint(tint, view),pixels.palette,pixels.colors);
    }
}
#include "editor_picker_runtime.hpp"
inline void releasePicker() { PickerRuntime::release(); }
inline void drawThumbnail(unsigned id, float x, float y, float w, float h) {
  if (PickerRuntime::draw(id, x, y, w, h))
    return;
  if (id >= std::size(EditorArt::Packed::Images))
    return;
  const auto &i = EditorArt::Packed::Images[id];
  double scale = std::min(double(w) / i.width, double(h) / i.height);
  double ox = x + (w - i.width * scale) * .5,
         oy = y + (h - i.height * scale) * .5;
  const auto previousClip = clip;
  const auto previousScissor = EditorArt::uiScissor;
  clip = {float(std::max(36, previousScissor[0])),
          float(std::max(0, previousScissor[1])),
          float(std::min(444, previousScissor[2])),
          float(std::min(272, previousScissor[3]))};
  builtinImage(id, {0, 0, float(i.width), float(i.height)}, [&](Vec2 p) {
    return uiVertex({ox + p.x * scale, oy + p.y * scale}, 0, 0);
  });
  ensureBatch();
  clip = previousClip;
  EditorArt::scissor(previousScissor[0], previousScissor[1], previousScissor[2],
                     previousScissor[3]);
}
#include "editor_thumbnail_runtime.hpp"
inline void releaseThumbnails() { ThumbnailRuntime::release(); }
inline bool drawCustomThumbnail(const std::string &file, float x, float y,
                                float w, float h) {
  return ThumbnailRuntime::draw(file, x, y, w, h);
}

inline bool readSourceRow(void*,unsigned page,unsigned row,uint32_t*out){
  const auto texture=builtinPage(page,true);if(!texture)return false;
  for(unsigned x=0;x<256;++x)out[x]=texturePixel(texture,x,row);
  return true;
}
inline uint32_t sourcePixel(unsigned id, int x, int y) {
  const auto &i = EditorArt::Packed::Images[id];
  x = (x % int(i.width) + int(i.width)) % int(i.width);
  y = (y % int(i.height) + int(i.height)) % int(i.height);
  const auto &f = EditorArt::Packed::Frames[i.frame];
  unsigned columns = (i.width + 239) / 240,
           part = unsigned(y) / 240 * columns + unsigned(x) / 240;
  if (part >= f.count)
    return 0;
  const auto &p = EditorArt::Packed::Parts[f.first + part];
  const unsigned sourceX=p.u+unsigned(x)-unsigned(p.x),sourceY=p.v+unsigned(y)-unsigned(p.y);
  if(const auto*row=sourceRows.get(p.page,sourceY,readSourceRow,nullptr))return row[sourceX];
  const auto image=builtinPage(p.page,true);
  return texturePixel(image,sourceX,sourceY);
}
struct alignas(64) Tile {
  std::array<uint8_t, 256> pixels{};
  std::array<uint32_t, 256> colors{};
  unsigned count = 0, age = 0;
  int ax = -1, ay = -1;
  uint64_t key = 0;
  std::string file;
};
inline std::array<Tile, 192> tiles{};
inline void invalidateTiles() {
  for (auto &tile : tiles)
    tile.ax = -1;
}
inline uint64_t paletteKey(const std::array<uint32_t, 4> &colors) {
  uint64_t h = 1469598103934665603ull;
  for (uint32_t c : colors) {
    h ^= c;
    h *= 1099511628211ull;
  }
  return h;
}
inline std::array<uint32_t, 4> tileColors(const Document &doc, unsigned layer) {
  if (layer == 0 || layer == 5)
    return {rgba(readColor(doc.root, "tBright")),
            rgba(readColor(doc.root, "tNormal")),
            rgba(readColor(doc.root, "tDark")),
            rgba(readColor(doc.root, "tExtra"))};
  static constexpr Color values[4][4] = {
      {{1, .501961, .67451, 1},
       {.19, 0, .0665, 1},
       {.49, 0, .1715, 1},
       {0, 0, 0, 1}},
      {{.5, .833333, 1, 1},
       {0, .126667, .19, 1},
       {0, .326667, .49, 1},
       {0, 0, 0, 1}},
      {{0, 1, 0, 1}, {0, .192157, 0, 1}, {0, .486275, 0, 1}, {0, 0, 0, 1}},
      {{1, .916667, .5, 1},
       {.19, .158333, 0, 1},
       {.49, .408333, 0, 1},
       {0, 0, 0, 1}}};
  std::array<uint32_t, 4> result{};
  for (unsigned i = 0; i < 4; ++i)
    result[i] = rgba(values[(layer - 1) % 4][i]);
  return result;
}
inline uint32_t swapColor(uint32_t input,
                          const std::array<uint32_t, 4> &colors) {
  unsigned k = input == 0xffffffffu   ? 0
               : input == 0xff0000ffu ? 1
               : input == 0xff000000u ? 2
               : input == 0xffff0000u ? 3
                                      : 4;
  if (k == 4)
    return input;
  return (colors[k] >> 24) ? ((colors[k] & 0xffffffu) | (input & 0xff000000u))
                           : 0;
}
inline Tile *prepareTile(int ax, int ay, const std::array<uint32_t, 4> &colors,
                         const std::string &file = {}) {
  const uint64_t key = paletteKey(colors);
  for (auto &t : tiles)
    if (t.ax == ax && t.ay == ay && t.key == key && t.file == file) {
      t.age = ++clock;
      return &t;
    }
  flushBatch();
  Tile *t = &tiles[0];
  for (auto &candidate : tiles)
    if (candidate.ax < 0) {
      t = &candidate;
      break;
    } else if (candidate.age < t->age)
      t = &candidate;
  uint32_t raw[256]{};
  bool ok = true;
  if (file.empty()) {
    if (!tileFile) {
      auto name = companion(executable.c_str(), "data/world/editor-tiles.rgba");
      tileFile = std::fopen(name.c_str(), "rb");
    }
    if (!tileFile || ax < 0 || ay < 0 || ax >= 40 || ay >= 4)
      ok = false;
    else
      for (unsigned row = 0; row < 16; ++row)
        if (std::fseek(tileFile,
                       long(((ay * 16 + int(row)) * 640 + ax * 16) * 4),
                       SEEK_SET) ||
            std::fread(raw + row * 16, 4, 16, tileFile) != 16) {
          ok = false;
          break;
        }
  } else
    ok = EditorImages::read(file.c_str(),
                            {unsigned(std::max(0, ax) * 16),
                             unsigned(std::max(0, ay) * 16), 16, 16},
                            raw, 16);
  if (!ok) {
    fail("Cannot read tile atlas cell " + std::to_string(ax) + "," +
         std::to_string(ay) + ": " +
         (file.empty() ? "editor-tiles.rgba" : file));
    return nullptr;
  }
  t->colors.fill(0);
  t->count = 0;
  for (unsigned y = 0; y < 16; ++y)
    for (unsigned x = 0; x < 16; ++x) {
      uint32_t c = swapColor(raw[y * 16 + x], colors);
      unsigned id = 0;
      while (id < t->count && t->colors[id] != c)
        ++id;
      if (id == t->count)
        t->colors[t->count++] = c;
      const unsigned at = (y / 8) * 128 + (y % 8) * 16 + x;
      t->pixels[at] = uint8_t(id);
    }
  t->ax = ax;
  t->ay = ay;
  t->key = key;
  t->file = file;
  t->age = ++clock;
  sceKernelDcacheWritebackRange(t->pixels.data(), t->pixels.size());
  markTextureDirty();
  sceKernelDcacheWritebackRange(t->colors.data(), t->colors.size() * 4);
  markTextureDirty();
  return t;
}
inline void drawTile(const View &view, const Document &doc, unsigned layer,
                     int x, int y, int ax, int ay, double alpha = 1,
                     const std::array<uint32_t,4>* preparedColors=nullptr,
                     const std::string* preparedFile=nullptr) {
  Vec2 p{double(x * 16) + view.offset.x, double(y * 16) + view.offset.y};
  std::array<Vertex, 4> q = {view.vertex(p, 0, 0),
                             view.vertex({p.x + 16, p.y}, 16, 0),
                             view.vertex({p.x + 16, p.y + 16}, 16, 16),
                             view.vertex({p.x, p.y + 16}, 0, 16)};
  if (!visible(q))
    return;
  static const char *keys[] = {"customtiles",        "switch1customtiles",
                               "switch2customtiles", "switch3customtiles",
                               "switch4customtiles", "dogcustomtiles"};
  const auto colors=preparedColors?*preparedColors:tileColors(doc,layer);
  const std::string file=preparedFile?*preparedFile:path(view.dir,string(doc.root,keys[layer]));
  auto *t = prepareTile(ax, ay, colors, file);
  if (!t && !file.empty())
    t = prepareTile(ax, ay, colors);
  if (t && !Lighting::points.empty()) {
    Lighting::select(float(p.x), float(p.y), float(p.x + 16), float(p.y + 16));
    if (Lighting::count) {
      int left = std::max(int(clip.x), int(std::floor(q[0].x))),
          right = std::min(int(clip.right), int(std::ceil(q[2].x))),
          top = std::max(int(clip.y), int(std::floor(q[0].y))),
          bottom = std::min(int(clip.bottom), int(std::ceil(q[2].y)));
      const float step = 1 / (float(view.zoom) * Scale);
      auto nodeTint = Lighting::FloatColor{1, 1, 1, float(alpha)};
      for (int by = top; by < bottom; by += 256)
        for (int bx = left; bx < right; bx += 256) {
          unsigned w = unsigned(std::min(256, right - bx)),
                   h = unsigned(std::min(256, bottom - by));
          EditorRaster::clear(scratch,256,w,h);
          for (unsigned iy = 0; iy < h; ++iy)
            for (unsigned ix = 0; ix < w; ++ix) {
              float wx = float(view.cameraX) +
                         (bx + ix + .5f - ViewX - float(view.centerX) * Scale) *
                             step,
                    wy = float(view.cameraY) +
                         (by + iy + .5f - float(view.centerY) * Scale) * step;
              int u = int(std::floor(wx - float(p.x))),
                  v = int(std::floor(wy - float(p.y)));
              if (u < 0 || v < 0 || u >= 16 || v >= 16)
                continue;
              unsigned at =
                  unsigned(v / 8) * 128 + unsigned(v % 8) * 16 + unsigned(u);
              uint32_t pixel = t->colors[t->pixels[at]];
              Lighting::Normal n{};
              uint32_t spec = 0;
              if (file.empty()) {
                n = Lighting::normal(
                    Lighting::pixel(1, ax * 16 + u, ay * 16 + v));
                spec = Lighting::pixel(2, ax * 16 + u, ay * 16 + v);
              }
              scratch[iy * 256 + ix] = Lighting::shade(
                  pixel, wx, wy, nodeTint, n, spec, file.empty(), 35.f / 255);
            }
          nativeImage(scratch, unsigned(bx), unsigned(by), w, h, White);
        }
      return;
    }
  }
  Color tint = view.light;
  tint.a *= alpha;
  if (t)
    polygonTexture(t->pixels.data(), GU_PSM_T8, 16, 16, q.data(), 4, rgba(tint),
                   t->colors.data(), t->count);
}
inline void drawTilePicker(const EditorController::Controller &c) {
  const auto &doc = c.painting ? c.stroke : c.model.document;
  View view(c);
  clip = {36, 0, 444, 272};
  const auto colors = tileColors(doc, c.layer);
  static const char *keys[] = {"customtiles",        "switch1customtiles",
                               "switch2customtiles", "switch3customtiles",
                               "switch4customtiles", "dogcustomtiles"};
  auto custom = path(view.dir, string(doc.root, keys[c.layer % 6]));
  for (const auto &cell : EditorArt::Packed::PickerCells) {
    double x = 19 + cell.x * 16, y = 40 + cell.y * 16;
    std::array<Vertex, 4> q = {
        uiVertex({x, y}, 0, 0), uiVertex({x + 16, y}, 16, 0),
        uiVertex({x + 16, y + 16}, 16, 16), uiVertex({x, y + 16}, 0, 16)};
    auto *t = prepareTile(int(cell.ax), int(cell.ay), colors, custom);
    if (!t)
      t = prepareTile(int(cell.ax), int(cell.ay), colors);
    if (t)
      polygonTexture(t->pixels.data(), GU_PSM_T8, 16, 16, q.data(), 4, White,
                     t->colors.data(), t->count);
  }
  ensureBatch();
  EditorArt::fullView();
}

struct Row {
  unsigned floor = UINT_MAX, y = UINT_MAX, age = 0, width = 0;
  uint32_t *pixels = nullptr;
};
inline std::array<Row, 8> rows{};
inline void releaseRows() {
  for (auto &row : rows) {
    std::free(row.pixels);
    row = Row{};
  }
}
inline const uint32_t *floorRow(unsigned index, unsigned y) {
  for (auto &r : rows)
    if (r.pixels && r.floor == index && r.y == y) {
      r.age = ++clock;
      return r.pixels;
    }
  if (!floorFile) {
    auto name = companion(executable.c_str(), "data/world/editor-floor.bin");
    floorFile = std::fopen(name.c_str(), "rb");
    uint32_t header[4]{};
    if (!floorFile || std::fread(header, sizeof header, 1, floorFile) != 1 ||
        header[0] != 0x46454355 || header[1] != 1 || header[2] != 11 ||
        std::fread(floorIndex.data(), sizeof floorIndex, 1, floorFile) != 1) {
      if (floorFile)
        std::fclose(floorFile);
      floorFile = nullptr;
      fail("Cannot read original floor bank: " + name);
      return nullptr;
    }
  }
  if (index >= 11 || y >= floorIndex[index][2]) {
    fail("Cannot allocate/read original floor scanline");
    return nullptr;
  }
  Row *r = &rows[0];
  for (auto &candidate : rows)
    if (!candidate.pixels) {
      r = &candidate;
      break;
    } else if (candidate.age < r->age)
      r = &candidate;
  unsigned width = floorIndex[index][1];
  if (r->width != width) {
    std::free(r->pixels);
    r->pixels = static_cast<uint32_t *>(std::malloc(width * 4));
    r->width = width;
  }
  if (!r->pixels ||
      std::fseek(floorFile, long(floorIndex[index][0] + y * width * 4),
                 SEEK_SET) ||
      std::fread(r->pixels, 4, width, floorFile) != width) {
    fail("Cannot allocate/read original floor scanline");
    return nullptr;
  }
  r->floor = index;
  r->y = y;
  r->age = ++clock;
  return r->pixels;
}
inline EditorRaster::Arena rasterArena;
inline void releaseRasterArena(){rasterArena.resetGeneration();}
inline void borrowSharedSwizzle(){ensureBatch();rasterArena.markExclusive(batchEpoch);}
inline void nativeImage(const uint32_t *linear, unsigned x, unsigned y,
                        unsigned w, unsigned h, uint32_t tint) {
  const auto texture=EditorRaster::dimensions(w,h);
  if(!texture.bytes)return;
  ensureBatch();
  unsigned offset=0;
  if(!rasterArena.reserve(texture.bytes,batchEpoch,offset)){
    flushBatch();ensureBatch();
    if(!rasterArena.reserve(texture.bytes,batchEpoch,offset)){fail("Invalid raster batch capacity");return;}
  }
  auto*packed=swizzled+offset;
  EditorRaster::pack(linear,256,packed,w,h);
  sceKernelDcacheWritebackRange(packed,texture.bytes);
  markTextureDirty();
  std::array<Vertex, 4> q = {
      Vertex{float(x), float(y), 0, 0},
      Vertex{float(x + w), float(y), float(w), 0},
      Vertex{float(x + w), float(y + h), float(w), float(h)},
      Vertex{float(x), float(y + h), 0, float(h)}};
  polygonTexture(packed, 3, texture.width, texture.height, q.data(), 4, tint);
}
inline void shadeTexture(const View &view, const uint8_t *pixels,
                         const std::array<Vertex, 4> &q, uint32_t tint) {
  shadeTexture(view, pixels, GU_PSM_8888, 256, 256, q, tint, nullptr, 0);
}
inline void unlitTexture(Texture image, const std::array<Vertex, 4> &q,
                         Color tint) {
  if (!image)
    return;
  flushBatch();
  if (image.pixels == swizzled) {
    std::memcpy(decodeScratch, image.pixels, PageBytes);
    image.pixels = reinterpret_cast<const uint8_t *>(decodeScratch);
  }
  const float red = float(tint.r), green = float(tint.g), blue = float(tint.b),
              alpha = float(tint.a);
  auto byte = [](float value) {
    return uint32_t(std::clamp(value + .5f, 0.f, 255.f));
  };
  for (unsigned y = 0; y < 256; ++y)
    for (unsigned x = 0; x < 256; ++x) {
      uint32_t source=texturePixel(image,x,y);
      scratch[y * 256 + x] = byte(float(source & 255) * red) |
                             (byte(float((source >> 8) & 255) * green) << 8) |
                             (byte(float((source >> 16) & 255) * blue) << 16) |
                             (byte(float(source >> 24) * alpha) << 24);
    }
  swizzleRGBA(scratch, swizzled);
  sceKernelDcacheWritebackRange(swizzled, PageBytes);
  markTextureDirty();
  polygonTexture(swizzled, GU_PSM_8888, 256, 256, q.data(), 4, White);
  flushBatch();
}
inline void shadeTexture(const View &view, const uint8_t *pixels,
                         unsigned format, unsigned width, unsigned height,
                         const std::array<Vertex, 4> &q, uint32_t tint,
                         const uint32_t *palette, unsigned colors,
                         const Color *sourceTint) {
  if (!pixels)
    return;
  if (pixels == swizzled) {
    std::memcpy(decodeScratch, pixels, PageBytes);
    pixels = reinterpret_cast<const uint8_t *>(decodeScratch);
  }
  float l = q[0].x, r = l, t = q[0].y, b = t;
  for (const auto &v : q) {
    l = std::min(l, v.x);
    r = std::max(r, v.x);
    t = std::min(t, v.y);
    b = std::max(b, v.y);
  }
  int left = std::max(int(clip.x), int(std::floor(l))),
      right = std::min(int(clip.right), int(std::ceil(r))),
      top = std::max(int(clip.y), int(std::floor(t))),
      bottom = std::min(int(clip.bottom), int(std::ceil(b)));
  if (left >= right || top >= bottom)
    return;
  float step = 1 / (float(view.zoom) * Scale),
        wx = float(view.cameraX) +
             (l - ViewX - float(view.centerX) * Scale) * step,
        wy = float(view.cameraY) + (t - float(view.centerY) * Scale) * step;
  Lighting::select(wx, wy, wx + (r - l) * step, wy + (b - t) * step);
  if (!Lighting::count && !highColor(sourceTint) && !highColor(&view.light)) {
    polygonTexture(pixels, format, width, height, q.data(), 4,
                   ambientTint(tint, &view), palette, colors);
    return;
  }
  const float ax = q[1].x - q[0].x, ay = q[1].y - q[0].y, bx = q[3].x - q[0].x,
              by = q[3].y - q[0].y, det = ax * by - ay * bx;
  if (std::abs(det) < .000001f)
    return;
  const float inv = 1 / det, du = q[1].u - q[0].u, dv = q[3].v - q[0].v;
  uintptr_t address = reinterpret_cast<uintptr_t>(pixels),
            first = reinterpret_cast<uintptr_t>(EditorArt::video(0));
  if (address >= first && address < first + 0x200000)
    pixels = reinterpret_cast<const uint8_t *>(address | 0x40000000u);
  auto nodeTint = Lighting::tint(sourceTint ? *sourceTint : color(tint));
  const bool transparent=std::isfinite(nodeTint.a)&&std::isfinite(Lighting::ambient.a);
  // A custom-image cache refusal can supply the shared swizzle buffer itself.
  // Preserve that input before batched output reuses the same storage.
  if(pixels==swizzled){
    flushBatch();std::memcpy(decodeScratch,pixels,PageBytes);
    pixels=reinterpret_cast<const uint8_t*>(decodeScratch);
  }
  for (int oy = top; oy < bottom; oy += 256)
    for (int ox = left; ox < right; ox += 256) {
      unsigned w = unsigned(std::min(256, right - ox)),
               h = unsigned(std::min(256, bottom - oy));
      EditorRaster::clear(scratch,256,w,h);
      for (unsigned y = 0; y < h; ++y)
        for (unsigned x = 0; x < w; ++x) {
          float dx = ox + x + .5f - q[0].x, dy = oy + y + .5f - q[0].y,
                a = (dx * by - dy * bx) * inv, b2 = (dy * ax - dx * ay) * inv;
          if (a < 0 || b2 < 0 || a >= 1 || b2 >= 1)
            continue;
          unsigned u = unsigned(std::clamp(int(std::floor(q[0].u + a * du)), 0,
                                           int(width) - 1)),
                   v = unsigned(std::clamp(int(std::floor(q[0].v + b2 * dv)), 0,
                                           int(height) - 1));
          uint32_t pixel = 0;
          unsigned bytes = format == GU_PSM_T8 ? 1 : 4,
                   rowBytes = width * bytes, byteX = u * bytes,
                   address = ((v / 8) * (rowBytes / 16) + byteX / 16) * 128 +
                             (v % 8) * 16 + byteX % 16;
          if (format == GU_PSM_T8) {
            unsigned k = pixels[address];
            if (palette && k < colors)
              pixel = palette[k];
          } else
            std::memcpy(&pixel, pixels + address, 4);
          if(transparent&&!(pixel>>24))continue;
          float worldX =
                    float(view.cameraX) +
                    (ox + x + .5f - ViewX - float(view.centerX) * Scale) * step,
                worldY = float(view.cameraY) +
                         (oy + y + .5f - float(view.centerY) * Scale) * step;
          scratch[y * 256 + x] =
              Lighting::shade(pixel, worldX, worldY, nodeTint);
        }
      nativeImage(scratch, unsigned(ox), unsigned(oy), w, h, White);
    }
}
#include "editor_sample_render.hpp"
struct Ground {
  alignas(64) std::array<uint8_t, 272 * 272 * 4> pixels{};
  uint64_t key = 0;
  bool valid = false;
};
inline Ground ground{};
inline uint64_t groundHash(unsigned index, const std::string &custom, float x,
                           float y, float step) {
  uint64_t h = 1469598103934665603ull;
  auto mix = [&](uint32_t v) {
    h ^= v;
    h *= 1099511628211ull;
  };
  mix(index);
  mix(cacheGeneration);
  mix(uint32_t(Lighting::signature));
  mix(uint32_t(Lighting::signature >> 32));
  for (float v : {x, y, step}) {
    uint32_t bits;
    std::memcpy(&bits, &v, 4);
    mix(bits);
  }
  for (unsigned char c : custom)
    mix(c);
  return h;
}
inline void floor(const View &view, const Document &doc) {
  const unsigned index =
      unsigned(std::clamp(int(number(doc.root, "bg")), 0, 10));
  const auto &image =
      EditorArt::Packed::Images[EditorArt::Packed::Floors[index].image];
  const std::string custom = path(view.dir, string(doc.root, "customfloor"));
  EditorImages::Info customInfo{};
  const bool external =
      !custom.empty() && inspectImage(custom.c_str(), customInfo);
  if (!custom.empty() && !external)
    fail("Cannot read custom floor image: " + custom);
  const unsigned width = external ? customInfo.width : image.width,
                 height = external ? customInfo.height : image.height;
  const Vec2 zero = view.world({(172.5 - ViewX) / Scale, .5 / Scale});
  const float startX = float(zero.x), startY = float(zero.y);
  const float step = 1.f / (float(view.zoom) * Scale);
  const uint64_t key = groundHash(index, custom, startX, startY, step);
  const unsigned widths[] = {256, 16, 256, 16}, heights[] = {256, 256, 16, 16};
  const unsigned xs[] = {172, 428, 172, 428}, ys[] = {0, 0, 256, 256};
  if (!ground.valid || ground.key != key) {
    Lighting::select(-43256.f, -43256.f, 44903.8f, 44903.8f);
    // Godot uses Vector2 float32. The 74k pixel inner loop contains only
    // integer source lookup, never JSON parsing, double division or floor().
    int sourceX[272], sourceY[272];
    for (unsigned x = 0; x < 272; ++x) {
      float world = startX + x * step;
      int p = int(std::floor((world + 43256.f) * (1.f / .6f)));
      sourceX[x] = world < -43256.f || world >= 44903.8f
                       ? -1
                       : (p % int(width) + int(width)) % int(width);
    }
    for (unsigned y = 0; y < 272; ++y) {
      float world = startY + y * step;
      int p = int(std::floor((world + 43256.f) * (1.f / .6f)));
      sourceY[y] = world < -43256.f || world >= 44903.8f
                       ? -1
                       : (p % int(height) + int(height)) % int(height);
    }
    unsigned output = 0;
    for (unsigned part = 0; part < 4; ++part) {
      const unsigned w = widths[part], h = heights[part];
      EditorRaster::clear(scratch,256,w,h);
      for (unsigned y = 0; y < h; ++y) {
        int sy = sourceY[ys[part] + y];
        if (sy < 0)
          continue;
        const uint32_t *row =
            external ? nullptr : floorRow(index, unsigned(sy));
        for (unsigned x = 0; x < w; ++x) {
          int sx = sourceX[xs[part] - 172 + x];
          if (sx < 0)
            continue;
          if (row)
            scratch[y * 256 + x] = row[sx];
          else if (external) {
            unsigned bx = unsigned(sx) / 240 * 240,
                     by = unsigned(sy) / 240 * 240;
            const auto pixels =
                customPage(custom, bx, by, std::min(240u, width - bx),
                           std::min(240u, height - by));
            if (pixels)scratch[y*256+x]=texturePixel(pixels,unsigned(sx)-bx,unsigned(sy)-by);
          }
          if (!Lighting::points.empty())
            scratch[y * 256 + x] = Lighting::shade(
                scratch[y * 256 + x], startX + (xs[part] - 172 + x) * step,
                startY + (ys[part] + y) * step);
        }
      }
      for (unsigned y = 0; y < h; y += 8)
        for (unsigned x = 0; x < w * 4; x += 16)
          for (unsigned row = 0; row < 8; ++row) {
            std::memcpy(ground.pixels.data() + output,
                        reinterpret_cast<const uint8_t *>(scratch) +
                            (y + row) * 1024 + x,
                        16);
            output += 16;
          }
    }
    sceKernelDcacheWritebackRange(ground.pixels.data(), ground.pixels.size());
    markTextureDirty();
    ground.key = key;
    ground.valid = true;
  }
  unsigned offset = 0;
  for (unsigned part = 0; part < 4; ++part) {
    const unsigned w = widths[part], h = heights[part], x = xs[part],
                   y = ys[part];
    const Vertex q[] = {{float(x), float(y), 0, 0},
                        {float(x + w), float(y), float(w), 0},
                        {float(x + w), float(y + h), float(w), float(h)},
                        {float(x), float(y + h), 0, float(h)}};
    polygonTexture(ground.pixels.data() + offset, 3, w, h, q, 4,
                   Lighting::points.empty() ? rgba(view.light) : White);
    offset += w * h * 4;
  }
}

inline void line(const View &view, Vec2 a, Vec2 b, uint32_t tint,
                 double width = 2) {
  auto av = view.vertex(a), bv = view.vertex(b);
  double dx = bv.x - av.x, dy = bv.y - av.y, n = std::hypot(dx, dy);
  if (n == 0)
    return;
  double sx = -dy / n * width * view.zoom * Scale * .5,
         sy = dx / n * width * view.zoom * Scale * .5;
  Vertex q[] = {{float(av.x + sx), float(av.y + sy), 0, 0},
                {float(bv.x + sx), float(bv.y + sy), 0, 0},
                {float(bv.x - sx), float(bv.y - sy), 0, 0},
                {float(av.x - sx), float(av.y - sy), 0, 0}};
  solidPolygon(q, 4, tint);
}
inline void grid(const View &view) {
  const auto &c = view.controller;
  if (!c.grid || view.scene)
    return;
  double gx = std::max(.001, c.gridX), gy = std::max(.001, c.gridY),
         z = view.zoom;
  Vec2 center{c.camera.x + 256 * c.rawZoom() - 256, c.camera.y};
  double left = std::ceil((-768 * z + center.x) / gx) * gx,
         top = -512 * z + center.y, bottom = 512 * z + center.y;
  double first = std::max(0., std::ceil((view.world({256, 0}).x - left) / gx)),
         last = std::min(std::floor(768 / z),
                         std::ceil((view.world({768, 0}).x - left) / gx));
  for (double i = first; i <= last; ++i)
    line(view, {left + i * gx, top}, {left + i * gx, bottom}, 0x80ffffffu);
  double y = std::ceil(top / gy) * gy;
  first = std::max(0., std::ceil((view.world({256, 0}).y - y) / gy));
  last = std::min(std::floor(512 / z),
                  std::ceil((view.world({256, 512}).y - y) / gy));
  for (double i = first; i <= last; ++i)
    line(view, {-768 * z + center.x, y + i * gy},
         {768 * z + center.x, y + i * gy}, 0x80ffffffu);
}

inline int objectId(const Value &record) {
  if (record.array.size() < 5)
    return -1;
  for (unsigned i = 0; i < ucg::editor::object_definition_count; ++i) {
    const auto &d = ucg::editor::object_definitions[i];
    if (record.array[4].text == d.scene_path || record.array[4].text == d.uid)
      return int(i);
  }
  return -1;
}
inline void ninePatch(const View &view, int id, Vec2 position, Vec2 scale,
                      double angle, Color tint, Vec2 size, Color keyColor) {
  if (scale.x == 0 || scale.y == 0)
    return;
  for (const auto &n : EditorArt::Visuals::NinePatches)
    if (n.object == unsigned(id)) {
      double width = std::max(double(n.left + n.right), size.x),
             height = std::max(double(n.top + n.bottom), size.y);
      std::array<Vertex, 4> corners{};
      const Vec2 local[] = {{-width * .5, -height * .5},
                            {width * .5, -height * .5},
                            {width * .5, height * .5},
                            {-width * .5, height * .5}};
      for (unsigned i = 0; i < 4; ++i)
        corners[i] =
            view.vertex(add(position, rotate(mul(local[i], scale), angle)));
      if (!visible(corners))
        continue;
      float l = clip.right, r = clip.x, t = clip.bottom, b = clip.y;
      for (auto v : corners) {
        l = std::min(l, v.x);
        r = std::max(r, v.x);
        t = std::min(t, v.y);
        b = std::max(b, v.y);
      }
      int left = std::max(int(clip.x), int(std::floor(l))),
          right = std::min(int(clip.right), int(std::ceil(r))),
          top = std::max(int(clip.y), int(std::floor(t))),
          bottom = std::min(int(clip.bottom), int(std::ceil(b)));
      auto uv = [](double p, double size, unsigned near, unsigned far,
                   double extent, unsigned mode) {
        if (p < near)
          return p;
        if (p >= size - far)
          return extent - (size - p);
        double source = extent - near - far, target = size - near - far;
        return near + (mode == 1 ? std::fmod(p - near, source)
                                 : (p - near) / target * source);
      };
      const std::array<uint32_t, 4> shades = {0xffffffffu, rgba(keyColor),
                                              0xff000000u, rgba(keyColor)};
      int sourceImage = int(n.image);
      if (view.scene && id == 11)
        sourceImage = image("assetsNEW/graphics/gimmicks/dblock.png");
      if (sourceImage < 0)
        sourceImage = int(n.image);
      const float step = 1 / (float(view.zoom) * Scale),
                  ca = std::cos(float(angle)), sa = std::sin(float(angle)),
                  inverseX = 1 / float(scale.x), inverseY = 1 / float(scale.y);
      const bool lit = !Lighting::points.empty();
      if (lit) {
        Vec2 a = view.world({(left - ViewX) / Scale, top / Scale}),
             b2 = view.world({(right - ViewX) / Scale, bottom / Scale});
        Lighting::select(float(a.x), float(a.y), float(b2.x), float(b2.y));
      }
      EditorNinePatchAxes::Axes axes;
      for (int by = top; by < bottom; by += 256)
        for (int bx = left; bx < right; bx += 256) {
          unsigned w = unsigned(std::min(256, right - bx)),
                   h = unsigned(std::min(256, bottom - by));
          EditorNinePatchAxes::Input mapping;
          mapping.cameraX=float(view.cameraX);mapping.cameraY=float(view.cameraY);mapping.centerX=float(view.centerX);mapping.centerY=float(view.centerY);
          mapping.step=step;mapping.positionX=float(position.x);mapping.positionY=float(position.y);mapping.inverseX=inverseX;mapping.inverseY=inverseY;mapping.ca=ca;mapping.sa=sa;
          mapping.pixelScale=Scale;mapping.bx=bx;mapping.by=by;mapping.viewX=ViewX;mapping.w=w;mapping.h=h;mapping.width=width;mapping.height=height;
          mapping.x={n.regionX,n.regionW,n.left,n.right,n.modeX};mapping.y={n.regionY,n.regionH,n.top,n.bottom,n.modeY};
          const bool fastAxes=EditorNinePatchAxes::build(mapping,axes);
          EditorRaster::clear(scratch,256,w,h);
          for (unsigned y = 0; y < h; ++y)
            for (unsigned x = 0; x < w; ++x) {
              float wx,wy;int u,v;
              if(fastAxes){
                if(!axes.validX[x]||!axes.validY[y])continue;
                wx=axes.wx[x];wy=axes.wy[y];u=axes.u[x];v=axes.v[y];
              }else{
              wx = float(view.cameraX) +
                         (bx + x + .5f - ViewX - float(view.centerX) * Scale) *
                             step,
                    wy = float(view.cameraY) +
                         (by + y + .5f - float(view.centerY) * Scale) * step;
              const float dx=wx-float(position.x),dy=wy-float(position.y);
              Vec2 p{(dx * ca + dy * sa) * inverseX + float(width) * .5f,
                     (-dx * sa + dy * ca) * inverseY + float(height) * .5f};
              if (p.x < 0 || p.y < 0 || p.x >= width || p.y >= height)
                continue;
              u = int(std::floor(n.regionX + uv(p.x, width, n.left, n.right,
                                                    n.regionW, n.modeX))),
                  v = int(
                      std::floor(n.regionY + uv(p.y, height, n.top, n.bottom,
                                                n.regionH, n.modeY)));
              }
              uint32_t pixel = sourcePixel(unsigned(sourceImage), u, v);
              pixel = id == 13 ? swapColor(pixel, shades) : pixel;
              if(!(pixel>>24)&&std::isfinite(float(tint.a))&&std::isfinite(Lighting::ambient.a))continue;
              Lighting::Normal normal{};
              if (lit && view.scene && id == 11)
                normal = Lighting::normal(Lighting::pixel(3, u, v), 1, false,
                                          ca, sa, scale.x < 0 ? -1 : 1,
                                          scale.y < 0 ? -1 : 1);
              scratch[y * 256 + x] =
                  lit ? Lighting::shade(pixel, wx, wy, Lighting::tint(tint),
                                        normal)
                      : pixel;
            }
          nativeImage(scratch, unsigned(bx), unsigned(by), w, h,
                      lit ? White : rgba(multiply(tint, view.light)));
        }
    }
}
#include "editor_player_fx.hpp"
inline void releasePlayerFx() { PlayerFx::clear(); }
inline void sprite(const View &view, int id, Vec2 position, Vec2 scale,
                   double rotation, Color tint, double seconds,
                   unsigned frameOverride = UINT_MAX, Vec2 nineSize = {32, 32},
                   Color keyColor = {}) {
  if (id < 0 || unsigned(id) >= std::size(EditorArt::Visuals::Objects))
    return;
  if (id == 0 && view.scene) {
    tint = PlayerFx::bodyTint(*view.scene, tint);
    scale.x *= view.scene->playerVisualScale;
    scale.y *= view.scene->playerVisualScale;
    rotation += view.scene->playerRotation;
  }
  if (id == 0 && view.scene && playerHook &&
      playerHook(view, position, scale, rotation, tint))
    return;
  if (id == 1 && view.scene && ghostHook &&
      ghostHook(view, position, scale, rotation, tint))
    return;
  const auto &object = EditorArt::Visuals::Objects[id];
  ninePatch(view, id, position, scale, rotation, tint, nineSize, keyColor);
  for (unsigned n = 0; n < object.count; ++n) {
    if (view.scene && id == 0 && n != 0)
      continue;
    const auto &s = EditorArt::Visuals::Sprites[object.first + n];
    if (!s.visible || s.polygonCount || s.image < 0)
      continue;
    if (view.scene && (id == 3 || id == 21 || id == 24))
      continue;
    int im = s.image;
    unsigned frame = s.frame;
    if (s.animationCount) {
      const unsigned animationId = s.animationFirst +
          EditorArt::Visuals::SpriteAnimationOffsets[object.first + n];
      const auto selection = animationMemo.get(animationId, seconds, [&] {
        return EditorAnimationMemo::select(
            EditorArt::Visuals::Animations[animationId],
            EditorArt::Visuals::Frames, seconds);
      });
      if (selection.overrideImage) {
        im = selection.image;
        frame = 0;
      }
    }
    if (im < 0)
      continue;
    const auto &i = EditorArt::Packed::Images[unsigned(im)];
    unsigned hf = std::max(1u, s.hframes), vf = std::max(1u, s.vframes);
    if (frameOverride != UINT_MAX && hf * vf > 1)
      frame = frameOverride;
    frame %= hf * vf;
    double fw = double(i.width) / hf, fh = double(i.height) / vf;
    EditorArt::Rect region{float((frame % hf) * fw), float((frame / hf) * fh),
                           float(fw), float(fh)};
    const auto prefixContext = EditorSpritePrefixMemo::makeContext(
        object.first + n, unsigned(im), frame, region, fw, fh, s, scale,
        rotation, view.mathNearestEven);
    auto transform = [&](Vec2 pixel) {
      Vec2 p = spritePrefixMemo.get(prefixContext, pixel, [&] {
        Vec2 local{pixel.x - region.x + (s.centered ? -fw * .5 : 0) + s.ox,
                   pixel.y - region.y + (s.centered ? -fh * .5 : 0) + s.oy};
        local = rotate({local.x * s.sx, local.y * s.sy}, s.rotation);
        local = add(local, {s.x, s.y});
        local = rotate(mul(local, scale), rotation);
        return local;
      });
      return view.vertex(add(position, p));
    };
    Color mod = multiply(
        tint, color(EditorArt::Visuals::SpriteTints[object.first + n]));
    builtinImage(unsigned(im), region, transform, rgba(mod), &view, &mod);
  }
}
inline void decoration(const View &view, Vec2 position, Vec2 scale,
                       double angle, int frame, int hf, int vf,
                       const std::string &texture,
                       const std::string &filename) {
  int id = image(texture.c_str());
  EditorImages::Info info{};
  std::string file = path(view.dir, filename);
  bool external = !file.empty() && inspectImage(file.c_str(), info);
  if (!file.empty() && !external)
    fail("Cannot read custom decoration image: " + file);
  if (!external && id < 0)
    return;
  if (!external) {
    const auto &i = EditorArt::Packed::Images[unsigned(id)];
    info = {i.width, i.height, false};
  }
  hf = std::max(1, hf);
  vf = std::max(1, vf);
  frame = (frame % (hf * vf) + hf * vf) % (hf * vf);
  double fw = double(info.width) / hf, fh = double(info.height) / vf;
  EditorArt::Rect region{float((frame % hf) * fw), float((frame / hf) * fh),
                         float(fw), float(fh)};
  auto transform = [&](Vec2 p) {
    return view.vertex(
        add(position, rotate({(p.x - region.x - fw * .5) * scale.x,
                              (p.y - region.y - fh * .5) * scale.y},
                             angle)));
  };
  if (external)
    customImage(file, info, region, transform, White, &view);
  else
    builtinImage(unsigned(id), region, transform, White, &view);
}
inline void polygon(const View &view, int id, Vec2 position, Vec2 scale,
                    const std::vector<Vec2> &points, int variant, Color tint) {
  if (points.size() < 3)
    return;
  unsigned texture = 0;
  double textureScale = id == 20 ? 2 : 4;
  if (id == 20)
    texture =
        EditorArt::Packed::Floors[unsigned(std::clamp(variant, 0, 10))].image;
  else if (id == 23) {
    int im = image("assetsNEW/graphics/stage/deco/W1/water.png");
    if (im < 0)
      return;
    texture = unsigned(im);
  } else {
    const auto &o = EditorArt::Visuals::Objects[unsigned(id)];
    bool found = false;
    for (unsigned j = 0; j < o.count; ++j) {
      const auto &s = EditorArt::Visuals::Sprites[o.first + j];
      if (s.polygonCount && s.image >= 0) {
        texture = unsigned(s.image);
        found = true;
        break;
      }
    }
    if (!found)
      return;
  }
  // Rasterize only the visible native pixels. Source texels remain
  // full-resolution; this also bounds work when a polygon repeats a tiny
  // texture at extreme zoom.
  std::vector<Vec2> screen;
  float l = clip.right, r = clip.x, t = clip.bottom, b = clip.y;
  for (auto p : points) {
    auto q = view.vertex(add(position, mul(p, scale)));
    screen.push_back({q.x, q.y});
    l = std::min(l, q.x);
    r = std::max(r, q.x);
    t = std::min(t, q.y);
    b = std::max(b, q.y);
  }
  int left = std::max(int(clip.x), int(std::floor(l))),
      top = std::max(int(clip.y), int(std::floor(t))),
      right = std::min(int(clip.right), int(std::ceil(r))),
      bottom = std::min(int(clip.bottom), int(std::ceil(b)));
  if (left >= right || top >= bottom || scale.x == 0 || scale.y == 0)
    return;
  const float step = 1 / (float(view.zoom) * Scale),
              inverseX = 1 / float(scale.x), inverseY = 1 / float(scale.y);
  const bool lit = !Lighting::points.empty();
  if (lit) {
    Vec2 a = view.world({(left - ViewX) / Scale, top / Scale}),
         b2 = view.world({(right - ViewX) / Scale, bottom / Scale});
    Lighting::select(float(a.x), float(a.y), float(b2.x), float(b2.y));
  }
  const unsigned waterFrame = unsigned(std::floor(view.seconds * 25)) % 16;
  std::vector<float> intersections;
  intersections.reserve(screen.size());
  const auto nodeTint = Lighting::tint(tint);
  for (int by = top; by < bottom; by += 256)
    for (int bx = left; bx < right; bx += 256) {
      unsigned w = unsigned(std::min(256, right - bx)),
               h = unsigned(std::min(256, bottom - by));
      EditorRaster::clear(scratch,256,w,h);
      for (unsigned y = 0; y < h; ++y) {
        const float py = by + y + .5f;
        intersections.clear();
        for (size_t i = 0, j = screen.size() - 1; i < screen.size(); j = i++) {
          const auto &a = screen[i];
          const auto &b2 = screen[j];
          float ay = float(a.y), by2 = float(b2.y);
          if ((ay > py) != (by2 > py))
            intersections.push_back(float(a.x) + (float(b2.x) - float(a.x)) *
                                                     (py - ay) / (by2 - ay));
        }
        std::sort(intersections.begin(), intersections.end());
        float wy =
            float(view.cameraY) + (py - float(view.centerY) * Scale) * step;
        for (unsigned span = 0; span + 1 < intersections.size(); span += 2) {
          int first = std::max(bx, int(std::ceil(intersections[span] - .5f))),
              last = std::min(bx + int(w),
                              int(std::ceil(intersections[span + 1] - .5f)));
          for (int px = first; px < last; ++px) {
            float wx = float(view.cameraX) +
                       (px + .5f - ViewX - float(view.centerX) * Scale) * step;
            int u, v;
            if (id == 23) {
              float tx = wx * (1.f / 128), ty = wy * (1.f / 128);
              u = int(std::floor((tx - std::floor(tx)) * 512)) +
                  int(waterFrame) * 512;
              v = int(std::floor((ty - std::floor(ty)) * 512));
            } else {
              u = int(std::floor((wx - float(position.x)) * inverseX *
                                 float(textureScale)));
              v = int(std::floor((wy - float(position.y)) * inverseY *
                                 float(textureScale)));
            }
            uint32_t pixel;
            if (id == 20) {
              unsigned f = unsigned(std::clamp(variant, 0, 10));
              const auto &im = EditorArt::Packed::Images[texture];
              int sy = (v % int(im.height) + int(im.height)) % int(im.height),
                  sx = (u % int(im.width) + int(im.width)) % int(im.width);
              const auto *row = floorRow(f, unsigned(sy));
              pixel = row ? row[sx] : 0;
            } else
              pixel = sourcePixel(texture, u, v);
            Lighting::Normal normal{};
            if (lit && id == 23)
              normal =
                  Lighting::normal(Lighting::pixel(4, u, v), 4, true, 1, 0,
                                   scale.x < 0 ? -1 : 1, scale.y < 0 ? -1 : 1);
            scratch[y * 256 + unsigned(px - bx)] =
                lit ? Lighting::shade(pixel, wx, wy, nodeTint, normal) : pixel;
          }
        }
      }
      nativeImage(scratch, unsigned(bx), unsigned(by), w, h,
                  lit ? White : rgba(multiply(tint, view.light)));
    }
}
inline void recordObject(const View &view, const Value &record) {
  int id = objectId(record);
  if (id < 0)
    return;
  const auto &a = record.array;
  Vec2 p = add(vector(a[0]), view.offset), scale = vector(a[1], {1, 1});
  Color tint{};
  ucg::editor::read_color(a[2], tint);
  if (a[5].text == "sand") {
    std::vector<Vec2> points;
    ucg::editor::read_polygon(a[3], points);
    polygon(view, id, p, scale, points, int(a[6].numeric()), tint);
  } else {
    Vec2 size{32, 32};
    Color key{};
    if (id == 13 && a[3].kind == Value::Kind::Object) {
      size = {number(a[3], "sizex", 32), number(a[3], "sizey", 32)};
      key = readColor(a[3], "color");
      if (tint.r != 1 || tint.g != 1 || tint.b != 1 || tint.a != 1) {
        key = tint;
        tint = {};
      }
    }
    sprite(view, id, p, scale, 0, tint, view.seconds, UINT_MAX, size, key);
  }
}
struct AuxState {
  int timer = -1;
  bool active = false;
  unsigned blast = UINT_MAX, death = UINT_MAX;
};
inline std::array<AuxState, EditorPlay::Scene::MaxEntities> auxStates{};
inline const EditorPlay::Scene *auxiliaryScene = nullptr;
inline unsigned auxiliaryTick = 0;
inline void auxiliary(const View &view, const EditorPlay::Entity &entity,
                      size_t index) {
  if (index >= auxStates.size())
    return;
  auto &state = auxStates[index];
  if (entity.objectId == 28 &&
      (entity.countdown > 0 ||
       (state.timer >= 0 && entity.timer > state.timer + 100)))
    state.blast = view.scene->tick;
  if (state.active && !entity.active)
    state.death = view.scene->tick;
  state.timer = entity.timer;
  state.active = entity.active;
  for (const auto &s : Aux::Sprites)
    if (s.object == unsigned(entity.objectId)) {
      bool show = entity.active && s.visible;
      double time = view.seconds;
      if (entity.objectId == 27) {
        show = entity.dead ? !std::strcmp(s.name, "Gravestone")
                           : entity.active && !std::strcmp(s.name, "Cat");
      }
      if (entity.objectId == 28) {
        if (!std::strcmp(s.name, "explosion")) {
          time = state.blast == UINT_MAX
                     ? 1000
                     : (view.scene->tick - state.blast) / 60.;
          show = entity.active && state.blast != UINT_MAX && time < 1;
        } else
          time = entity.timer > 180  ? 0
                 : entity.timer > 60 ? (180 - entity.timer) / 60.
                                     : 2 + (60 - entity.timer) / 30.;
      }
      if (entity.objectId == 29 && !std::strcmp(s.name, "explode")) {
        time = state.death == UINT_MAX ? 1000
                                       : (view.scene->tick - state.death) / 60.;
        show = state.death != UINT_MAX && time < 1;
      }
      if (!show)
        continue;
      std::string file = s.file;
      EditorArt::Rect region{s.u, s.v, s.w, s.h};
      unsigned hf = std::max(1u, s.hf), vf = std::max(1u, s.vf),
               frame = s.frame;
      if (s.animations) {
        const auto &a = Aux::Animations[s.firstAnimation];
        float duration = 0;
        for (unsigned j = 0; j < a.count; ++j)
          duration += Aux::Frames[a.first + j].duration;
        double at = time * a.speed;
        if (a.loop)
          at = std::fmod(at, std::max(.00001, double(duration)));
        else
          at = std::min(at, double(duration) - .00001);
        unsigned selected = 0;
        while (selected + 1 < a.count &&
               at >= Aux::Frames[a.first + selected].duration) {
          at -= Aux::Frames[a.first + selected].duration;
          ++selected;
        }
        if (!a.count)
          continue;
        const auto &f = Aux::Frames[a.first + selected];
        file = f.file;
        region = {f.x, f.y, f.w, f.h};
        hf = vf = 1;
      }
      if (file.empty())
        continue;
      if (entity.objectId == 29 && !std::strcmp(s.name, "bart"))
        frame = entity.frame;
      if (hf * vf > 1) {
        frame %= hf * vf;
        region.w /= hf;
        region.h /= vf;
        region.x += (frame % hf) * region.w;
        region.y += (frame / hf) * region.h;
      }
      file = companion(executable.c_str(), file.c_str());
      EditorImages::Info info{};
      if (!inspectImage(file.c_str(), info)) {
        fail("Cannot load auxiliary source sprite: " + file);
        continue;
      }
      auto transform = [&](Vec2 p) {
        p = {p.x - region.x - region.w * .5 + s.ox,
             p.y - region.y - region.h * .5 + s.oy};
        p = rotate({p.x * s.sx, p.y * s.sy}, s.rotation);
        p = add(p, {s.x, s.y});
        p = rotate({p.x * entity.scale.x, p.y * entity.scale.y},
                   entity.rotation);
        p = add(p, {entity.position.x, entity.position.y});
        return entity.screenSpace ? uiVertex(p, 0, 0) : view.vertex(p);
      };
      Color tint = entity.color;
      tint.a *= entity.visualAlpha;
      customImage(file, info, region, transform, rgba(tint),
                  entity.screenSpace ? nullptr : &view);
    }
}
#include "editor_ghost_fx.hpp"
inline void releaseGhostFx() { GhostFx::clear(); }
#include "editor_ground_runtime.hpp"
#ifdef UCG_EDITOR_CAPTURE
struct RenderCosts{uint32_t frames=0,total=0,setup=0,floor=0,sand=0,behind=0,tiles=0,objects=0,aux=0,front=0;};
inline volatile RenderCosts renderCosts{};
inline volatile uint32_t renderCostsSequence=0;
#endif
inline void renderWorld(const EditorController::Controller &c,
                        const EditorPlay::Scene *scene = nullptr) {
#ifdef UCG_EDITOR_CAPTURE
  RenderCosts costs{};
  const unsigned renderStart=sceKernelGetSystemTimeLow();unsigned phaseStart=renderStart;
  auto markCost=[&](uint32_t&out){const unsigned now=sceKernelGetSystemTimeLow();out=now-phaseStart;phaseStart=now;};
#endif
  auto worldStamp067=::editorPerfStamp067();
  auto markWorld067=[&](unsigned zone){auto now=::editorPerfStamp067();::editorPerf067.measure(zone,worldStamp067,now);worldStamp067=now;};
  View view(c, scene);
  if (scene && (scene != auxiliaryScene || scene->tick < auxiliaryTick)) {
    auxiliaryScene = scene;
    auxStates.fill(AuxState{});
  }
  if (scene)
    auxiliaryTick = scene->tick;
  const auto &doc = c.painting ? c.stroke : c.model.document;
  Lighting::points.clear();
  Lighting::ambient = Lighting::tint(view.light);
  if (scene) {
    for (const auto &n : scene->entities)
      if (n.active && n.objectId == 21)
        Lighting::add(n.position.x, n.position.y, n.scale.x, n.scale.y,
                      n.rotation, n.color);
  } else
    for (const auto &n : doc.nodes())
      if (objectId(n) == 21) {
        const auto &a = n.array;
        Vec2 p = vector(a[0]), s = vector(a[1], {1, 1});
        Color col{};
        ucg::editor::read_color(a[2], col);
        Lighting::add(float(p.x), float(p.y), float(s.x), float(s.y), 0, col);
      }
  Lighting::signature = 1469598103934665603ull;
  auto hashLight = [](const void *data, unsigned bytes) {
    const auto *p = static_cast<const uint8_t *>(data);
    for (unsigned i = 0; i < bytes; ++i) {
      Lighting::signature ^= p[i];
      Lighting::signature *= 1099511628211ull;
    }
  };
  hashLight(&Lighting::ambient, sizeof Lighting::ambient);
  for (const auto &p : Lighting::points)
    hashLight(&p, sizeof p);
  if (!Lighting::points.empty())
    Lighting::opened();
#ifdef UCG_EDITOR_ENABLE_LIT_MEMO
  const auto litEpoch=lightingEpoch.update(Lighting::ambient,Lighting::points.data(),Lighting::points.size(),cacheGeneration);
  if(litEpoch!=EditorShadeKey::EpochStatus::Unchanged)litCache.clear();
  litCacheEnabled=litEpoch!=EditorShadeKey::EpochStatus::Disabled;
#endif
  GhostFx::update(view);
  PlayerFx::update(view);
  clip = {172, 0, 444, 272};
  EditorGroundCache::State groundState;
  EditorGroundCache::Key groundKey;
  const bool groundEligible=GroundRuntime::snapshot(view,doc,groundState)&&
    EditorGroundCache::buildKey(groundState,groundKey)==EditorGroundCache::Eligibility::Eligible;
  bool groundHit=false;
  if(groundEligible){
    const auto restored=GroundRuntime::restore(groundKey);
    if(restored==EditorGroundCache::Restore::BackendFault){fail("Ground framebuffer fence failed");return;}
    groundHit=restored==EditorGroundCache::Restore::Hit;
  }
  const unsigned groundFailedBefore=failedImages;
  const unsigned groundBeginClock=clock;
  markWorld067(EditorPerf067::Setup);
#ifdef UCG_EDITOR_CAPTURE
  markCost(costs.setup);
#endif
  if(!groundHit)floor(view, doc);
  grid(view);
  markWorld067(EditorPerf067::Floor);
#ifdef UCG_EDITOR_CAPTURE
  markCost(costs.floor);
#endif
  if(!groundHit){
  if (scene) {
    for (const auto &n : scene->entities)
      if (n.active &&
          (n.objectId == 19 || n.objectId == 20 || n.objectId == 23)) {
        std::vector<Vec2> points;
        for (auto p : n.polygon)
          points.push_back({n.scale.x ? (p.x - n.position.x) / n.scale.x : 0,
                            n.scale.y ? (p.y - n.position.y) / n.scale.y : 0});
        polygon(view, n.objectId, {n.position.x, n.position.y},
                {n.scale.x, n.scale.y}, points, n.variant, n.color);
      }
  } else
    for (const auto &n : doc.nodes())
      if (n.array.size() > 5 && n.array[5].text == "sand")
        recordObject(view, n);
  }
  markWorld067(EditorPerf067::Sand);
#ifdef UCG_EDITOR_CAPTURE
  markCost(costs.sand);
#endif
  if (scene) {
    for (const auto &d : scene->decorations)
      if (d.z < 0)
        decoration(view, {d.position.x, d.position.y}, {d.scale.x, d.scale.y},
                   d.rotation, d.frame, d.hframes, d.vframes, d.texture,
                   d.imageFilename);
  } else
    for (const auto &n : doc.decorations())
      if (n.array.size() >= 10 && n.array[3].numeric() < 0)
        decoration(view, add(vector(n.array[0]), view.offset),
                   vector(n.array[1], {1, 1}), n.array[2].numeric(),
                   int(n.array[4].numeric()), int(n.array[6].numeric()),
                   int(n.array[7].numeric()), n.array[9].text,
                   n.array.size() > 10 ? n.array[10].text : "");
  GhostFx::trails(view);
  PlayerFx::trails(view);
  markWorld067(EditorPerf067::Behind);
#ifdef UCG_EDITOR_CAPTURE
  markCost(costs.behind);
#endif
  if(!groundHit){
  static const char*tileKeys[]={"customtiles","switch1customtiles","switch2customtiles","switch3customtiles","switch4customtiles","dogcustomtiles"};
  for (unsigned layer = 0; layer < 6; ++layer) {
    // Palette/file are constant for the entire source TileMap layer.
    const auto preparedColors=tileColors(doc,layer);
    const auto preparedFile=path(view.dir,string(doc.root,tileKeys[layer]));
    double alpha = scene ? (scene->layer_active(int(layer)) ? 1 : .2)
                         : (layer >= 1 && layer <= 4 && c.switchPreview &&
                                    c.switchPreview != layer
                                ? .2
                                : 1);
    if (scene) {
      for (const auto &t : scene->tiles)
        if (t.layer == int(layer))
          drawTile(view, doc, layer, t.x, t.y, t.ax, t.ay, alpha,&preparedColors,&preparedFile);
    } else
      for (const auto &n : doc.tiles(int(layer))) {
        int x=0,y=0,ax=0,ay=0;
        if(!EditorTileValues::pair(n.array[0].text,x,y)){const auto p=vector(n.array[0]);x=int(p.x);y=int(p.y);}
        if(!EditorTileValues::pair(n.array[1].text,ax,ay)){const auto a=vector(n.array[1]);ax=int(a.x);ay=int(a.y);}
        drawTile(view, doc, layer,x,y,ax,ay,alpha,&preparedColors,&preparedFile);
      }
  }
  }
  if(groundEligible&&!groundHit&&failedImages==groundFailedBefore&&lastError.empty())
    {if(!GroundRuntime::capture(groundKey,groundBeginClock)&&!lastError.empty())return;}
  markWorld067(EditorPerf067::Tiles);
#ifdef UCG_EDITOR_CAPTURE
  markCost(costs.tiles);
#endif
  if (scene) {
    for (const auto &n : scene->entities)
      if (n.active && n.objectId >= 0 && n.objectId < 26 && n.objectId != 19 &&
          n.objectId != 20 && n.objectId != 23) {
        Vec2 p{n.position.x, n.position.y};
        if (n.objectId == 0)
          p = {scene->game.position.x, scene->game.position.y};
        Color tint = n.color;
        tint.a *= n.visualAlpha;
        Vec2 nineSize{32, 32};
        if (n.objectId == 13) {
          if (n.scale.x != 0)
            nineSize.x = n.halfSize.x * 2 / std::abs(n.scale.x);
          if (n.scale.y != 0)
            nineSize.y = n.halfSize.y * 2 / std::abs(n.scale.y);
        }
        sprite(view, n.objectId, p, {n.scale.x, n.scale.y}, n.rotation, tint,
               view.seconds, n.frame, nineSize, n.lockColor);
        if (n.objectId == 1)
          GhostFx::particles(view, n, size_t(&n - scene->entities.data()));
      }
  } else
    for (const auto &n : doc.nodes())
      if (n.array.size() > 5 && n.array[5].text != "sand")
        recordObject(view, n);
  markWorld067(EditorPerf067::Objects);
#ifdef UCG_EDITOR_CAPTURE
  markCost(costs.objects);
#endif
  if (scene)
    for (size_t i = 0; i < scene->entities.size(); ++i)
      if (scene->entities[i].objectId >= 26 &&
          scene->entities[i].objectId <= 30)
        auxiliary(view, scene->entities[i], i);
  markWorld067(EditorPerf067::Aux);
#ifdef UCG_EDITOR_CAPTURE
  markCost(costs.aux);
#endif
  if (scene) {
    for (const auto &d : scene->decorations)
      if (d.z >= 0)
        decoration(view, {d.position.x, d.position.y}, {d.scale.x, d.scale.y},
                   d.rotation, d.frame, d.hframes, d.vframes, d.texture,
                   d.imageFilename);
  } else
    for (const auto &n : doc.decorations())
      if (n.array.size() >= 10 && n.array[3].numeric() >= 0)
        decoration(view, add(vector(n.array[0]), view.offset),
                   vector(n.array[1], {1, 1}), n.array[2].numeric(),
                   int(n.array[4].numeric()), int(n.array[6].numeric()),
                   int(n.array[7].numeric()), n.array[9].text,
                   n.array.size() > 10 ? n.array[10].text : "");
  PlayerFx::children(view);
  GhostFx::indicators(view);
  ensureBatch();
  applyClip();
  markWorld067(EditorPerf067::Front);
#ifdef UCG_EDITOR_CAPTURE
  markCost(costs.front);costs.total=sceKernelGetSystemTimeLow()-renderStart;
  renderCostsSequence=renderCostsSequence+1;
  renderCosts.total=costs.total;renderCosts.setup=costs.setup;renderCosts.floor=costs.floor;
  renderCosts.sand=costs.sand;renderCosts.behind=costs.behind;renderCosts.tiles=costs.tiles;
  renderCosts.objects=costs.objects;renderCosts.aux=costs.aux;renderCosts.front=costs.front;
  renderCosts.frames=renderCosts.frames+1;
  renderCostsSequence=renderCostsSequence+1;
#endif
}
inline void renderPreview(const EditorController::Controller &c,
                          Vec2 cursorWorld) {
  if (c.selectMode)
    return;
  View view(c);
  clip = {172, 0, 444, 272};
  if (c.tab == 2) {
    Vec2 p = EditorController::snap(cursorWorld, 16);
    sprite(view, int(c.object), p, {1, 1}, 0, {1, 1, 1, .5}, 0);
  } else if (c.tab == 3 && c.deco < std::size(EditorArt::Packed::Deco)) {
    const auto &i = EditorArt::Packed::Deco[c.deco];
    decoration(view, EditorController::snap(cursorWorld, 8),
               {c.decoScaleX, c.decoScaleY}, c.decoRotation, c.decoFrame,
               c.decoH, c.decoV, EditorArt::Packed::Images[i.image].path, "");
  } else if (c.tab == 1) {
    const auto &doc = c.painting ? c.stroke : c.model.document;
    drawTile(view, doc, c.layer, int(std::floor(cursorWorld.x / 16)),
             int(std::floor(cursorWorld.y / 16)), int(c.atlasX), int(c.atlasY),
             .5);
  }
}
inline void drawPoof(const View &view, Vec2 position, unsigned ageTicks,
                     Vec2 entityScale = {1, 1}, double rotation = 0,
                     Color tint = {}) {
  if (ageTicks >= 51)
    return;
  const int id = image("assetsNEW/graphics/gimmicks/blockpoof1.png");
  if (id < 0) {
    fail("Original blockpoof1 image is missing");
    return;
  }
  unsigned frame = std::min(9u, ageTicks / 4);
  EditorArt::Rect region{float(frame * 243), 0, 243, 152};
  builtinImage(
      unsigned(id), region,
      [&](Vec2 pixel) {
        Vec2 local{(pixel.x - region.x - 121.5) * .25 * entityScale.x,
                   (-8 + (pixel.y - 76) * .25) * entityScale.y};
        return view.vertex(add(position, rotate(local, rotation)));
      },
      rgba(tint), &view);
}
} // namespace EditorRender
#include "editor_font_masks.hpp"
