#pragma once
// Included inside EditorRender. Page payloads retain all 65536 exact RGBA
// texels, including RGB behind zero alpha. No asset or UV changes occur.
#include "editor_texture_cold.hpp"
struct Texture {
  const uint8_t *pixels = nullptr;
  unsigned format = GU_PSM_8888;
  const uint32_t *palette = nullptr;
  unsigned colors = 0, width = 256, height = 256;
  explicit operator bool() const { return pixels != nullptr; }
};
struct Page {
  unsigned id = UINT_MAX, age = 0;
  uint8_t *pixels = nullptr;
  std::string file;
  unsigned x = 0, y = 0, w = 0, h = 0;
  unsigned format = GU_PSM_8888, bytes = 0, colors = 0;
};
#if defined(__mips__)
static_assert(sizeof(Page) == 64, "PSP cache Page ABI");
#endif
inline std::array<Page, PageCacheCount> pages{};
struct PaletteSlot {
  uint32_t color = 0;
  unsigned id = UINT_MAX;
};
struct PaletteEncoder {
  std::array<PaletteSlot, 512> slots{};
  std::array<uint32_t, 256> colors{};
  unsigned count = 0;
  unsigned lookup(uint32_t color) const {
    unsigned at = (color * 2654435761u) >> 23;
    while (slots[at].id != UINT_MAX) {
      if (slots[at].color == color)
        return slots[at].id;
      at = (at + 1) & 511;
    }
    return UINT_MAX;
  }
  bool classify(const uint8_t *source) {
    for (auto &s : slots)
      s.id = UINT_MAX;
    count = 0;
    colors.fill(0);
    for (unsigned i = 0; i < 65536; ++i) {
      uint32_t color;
      std::memcpy(&color, source + i * 4, 4);
      unsigned at = (color * 2654435761u) >> 23;
      while (slots[at].id != UINT_MAX && slots[at].color != color)
        at = (at + 1) & 511;
      if (slots[at].id == UINT_MAX) {
        if (count == 256)
          return false;
        slots[at] = {color, count};
        colors[count++] = color;
      }
    }
    return true;
  }
  void encode(const uint8_t *source, uint8_t *target) const {
    for (unsigned y = 0; y < 256; ++y)
      for (unsigned x = 0; x < 256; ++x) {
        unsigned src = ((y >> 3) * 64 + (x >> 2)) * 128 + (y & 7) * 16 +
                       (x & 3) * 4,
                 dst =
                     ((y >> 3) * 16 + (x >> 4)) * 128 + (y & 7) * 16 + (x & 15);
        uint32_t color;
        std::memcpy(&color, source + src, 4);
        target[dst] = uint8_t(lookup(color));
      }
    std::memcpy(target + 65536, colors.data(), 1024);
  }
};
inline PaletteEncoder paletteEncoder;
inline Cold::Cache<PageCacheCount> coldCache;
inline void*coldWorkspace=nullptr;
inline unsigned coldWorkspaceBytes=0,coldStoreFailures=0,coldStoreSkips=0;
inline Cold::Result coldLastStore=Cold::Result::Invalid;
inline void configureCold(void*workspace,unsigned bytes){
  coldWorkspace=bytes>=Cold::WorkspaceBytes?workspace:nullptr;
  coldWorkspaceBytes=coldWorkspace?bytes:0;
}
inline void clearCold(){coldCache.clear();}
#ifndef UCG_EDITOR_SAMPLE_RESERVED_BYTES
#define UCG_EDITOR_SAMPLE_RESERVED_BYTES 0
#endif
inline constexpr unsigned CacheBudget = 8 * 1024 * 1024,
                          IndexedPageBytes = 65536 + 1024;
inline unsigned cacheAvailableBudget(){const size_t reserved=UCG_EDITOR_SAMPLE_RESERVED_BYTES;return reserved<CacheBudget?CacheBudget-unsigned(reserved):0;}
using PageAllocator=void*(*)(size_t);
inline void*defaultPageAllocate(size_t bytes){return memalign(64,bytes);}
inline PageAllocator pageAllocate=&defaultPageAllocate;
inline unsigned coldPromotionSkips=0;
inline unsigned cacheUsed = 0;
inline unsigned cacheFixedBytes() {
  return sizeof(pages) + sizeof(paletteEncoder) + coldCache.fixedBytes()+sizeof(pageAllocate)+sizeof(coldPromotionSkips)+Cold::poolFixedBytes();
}
inline unsigned cacheBytes() { return cacheUsed; }
inline unsigned cacheTotalBytes() {
  unsigned result = cacheUsed + cacheFixedBytes() + coldCache.residentBytes();
  for (const auto &p : pages)
    if (p.file.capacity() > 15)
      result += unsigned(p.file.capacity() + 1);
  return result;
}
inline Cold::Key coldKey(const Page&p){return {p.id,p.file.empty()?nullptr:p.file.c_str(),p.x,p.y,p.w,p.h};}
inline Cold::Entry*findCold(unsigned id,const std::string*file=nullptr,unsigned x=0,unsigned y=0,unsigned w=0,unsigned h=0){
  return coldCache.find({id,file?file->c_str():nullptr,x,y,w,h});
}
inline Cold::Result saveColdPage(const Page&p,unsigned reservedBytes=0){
  if(!coldWorkspace||!p.pixels||(p.id==UINT_MAX&&p.file.empty()))return Cold::Result::Invalid;
  if(!p.file.empty()&&wholeCpuSourceBacked(p.file)){++coldStoreSkips;return Cold::Result::NotUseful;}
  const uint64_t fixedHot=uint64_t(cacheTotalBytes())-coldCache.residentBytes()+reservedBytes;
  coldLastStore=coldCache.save(coldKey(p),p.pixels,p.format,p.colors,coldWorkspace,coldWorkspaceBytes,
                              fixedHot<cacheAvailableBudget()?cacheAvailableBudget()-fixedHot:0);
  if(coldLastStore==Cold::Result::AllocationFailed||coldLastStore==Cold::Result::Invalid)++coldStoreFailures;
  else if(coldLastStore!=Cold::Result::Stored&&coldLastStore!=Cold::Result::AlreadyPresent)++coldStoreSkips;
  return coldLastStore;
}
inline Texture texture(const Page &p) {
  return {p.pixels,
          p.format,
          p.format == GU_PSM_T8
              ? reinterpret_cast<const uint32_t *>(p.pixels + 65536)
              : nullptr,
          p.colors,
          256,
          256};
}
inline uint32_t texturePixel(const Texture &t, unsigned x, unsigned y) {
  if (!t.pixels || x >= t.width || y >= t.height)
    return 0;
  unsigned b = t.format == GU_PSM_T8 ? 1 : 4, row = t.width * b, bx = x * b,
           at = ((y / 8) * (row / 16) + bx / 16) * 128 + (y % 8) * 16 + bx % 16;
  uint32_t value = 0;
  if (t.format == GU_PSM_T8) {
    unsigned i = t.pixels[at];
    if (t.palette && i < t.colors)
      value = t.palette[i];
  } else
    std::memcpy(&value, t.pixels + at, 4);
  return value;
}
inline void discardPage(Page &p) {
  cacheUsed -= p.bytes;
  std::free(p.pixels);
  p = Page{};
}
inline void retirePage(Page&p,unsigned reservedBytes=0){saveColdPage(p,reservedBytes);discardPage(p);}
// Retire metadata while retaining an already-aligned, same-size allocation.
// Avoid free+memalign: PSP dlmalloc can fail to realign the block just freed.
inline uint8_t *detachPageStorage(Page &p) {
  saveColdPage(p);
  uint8_t *memory = p.pixels;
  cacheUsed -= p.bytes;
  p.pixels = nullptr;
  p.bytes = 0;
  discardPage(p);
  return memory;
}
inline Page *oldestPage(const Page *except = nullptr,unsigned requiredBytes=0) {
  Page *result = nullptr;
  for (unsigned i = 0; i < pageLimit; ++i) {
    auto &p = pages[i];
    if (&p != except && p.pixels && (!requiredBytes||p.bytes==requiredBytes) && (!result || p.age < result->age))
      result = &p;
  }
  return result;
}
inline Page *storePage(const uint8_t *source, unsigned id,
                       const std::string *file = nullptr, unsigned x = 0,
                       unsigned y = 0, unsigned w = 0, unsigned h = 0,
                       unsigned encodedFormat=0,unsigned encodedColors=0,
                       const char*encodedFile=nullptr,bool optionalPromotion=false) {
  if (!source) {
    fail("Empty editor texture source");
    return nullptr;
  }
  const bool encoded=encodedFormat!=0;
  if(encoded&&encodedFormat!=GU_PSM_T8&&encodedFormat!=GU_PSM_8888){fail("Invalid encoded cold texture format");return nullptr;}
  if(encodedFormat==GU_PSM_T8&&(!encodedColors||encodedColors>256)){fail("Invalid cold texture palette");return nullptr;}
  const bool indexed = encoded?encodedFormat==GU_PSM_T8:paletteEncoder.classify(source);
  const unsigned needed = indexed ? IndexedPageBytes : PageBytes;
  const bool firstClass=oldestPage(nullptr,needed)==nullptr;
  flushBatch();
  Page *target = nullptr;
  for (unsigned i = 0; i < pageLimit; ++i)
    if (!pages[i].pixels ||
        (pages[i].id == UINT_MAX && pages[i].file.empty())) {
      target = &pages[i];
      break;
    }
  if (!target)
    target = oldestPage(nullptr,needed);
  if(!target&&firstClass)target=oldestPage();
  if (!target) {
    cacheAllocationBlocked=true;
    if(optionalPromotion)++coldPromotionSkips;else fail("No matching editor texture cache slot");
    return nullptr;
  }
  uint8_t *reuse = nullptr;
  // Admit the first page of a new format without making the old format a
  // casualty of a denied fresh allocation. Optional compressed data can go
  // first; an occupied metadata slot stays live until allocation succeeds.
  if(firstClass){
    while(uint64_t(cacheTotalBytes())+needed>cacheAvailableBudget()){
      if(!coldCache.reclaimResidentEntry()){cacheAllocationBlocked=true;if(optionalPromotion)++coldPromotionSkips;else fail("Editor texture cache byte budget exhausted");return nullptr;}
    }
    reuse=pageAllocate?static_cast<uint8_t*>(pageAllocate(needed)):nullptr;
    if(!reuse){++cacheAllocationFailures;cacheAllocationBlocked=true;if(optionalPromotion)++coldPromotionSkips;else fail("Cannot allocate first editor texture cache format");return nullptr;}
  }
  if (target->pixels && target->bytes == needed)
    reuse = detachPageStorage(*target);
  else
    retirePage(*target,reuse?needed:0);
  if (file) {
    target->file = *file;
  }else if(encodedFile)target->file=encodedFile;
  target->id = id;
  target->x = x;
  target->y = y;
  target->w = w;
  target->h = h;
  while (uint64_t(cacheTotalBytes()) + needed > cacheAvailableBudget()) {
    if(firstClass&&coldCache.reclaimResidentEntry())continue;
    auto *p = oldestPage(target,needed);
    if (!p) {
      std::free(reuse);
      *target = Page{};
      cacheAllocationBlocked=true;
      if(optionalPromotion)++coldPromotionSkips;else fail("Editor texture cache byte budget exhausted");
      return nullptr;
    }
    if (!reuse && p->bytes == needed)
      reuse = detachPageStorage(*p);
    else
      retirePage(*p,reuse?needed:0);
  }
  uint8_t *memory = reuse;
  while (!memory) {
    memory = pageAllocate?static_cast<uint8_t *>(pageAllocate(needed)):nullptr;
    if (memory)
      break;
    ++cacheAllocationFailures;
    // A refused fresh allocation must not dismantle the other format's
    // resident working set. Reuse an aligned block of this exact size.
    auto *p = oldestPage(target,needed);
    if (!p) {
      cacheAllocationBlocked = true;
      *target = Page{};
      if(optionalPromotion)++coldPromotionSkips;else fail("Cannot allocate lossless editor texture cache page");
      return nullptr;
    }
    if (p->bytes == needed)
      memory = detachPageStorage(*p);
    else
      retirePage(*p);
  }
  target->pixels = memory;
  target->bytes = needed;
  target->format = indexed ? GU_PSM_T8 : GU_PSM_8888;
  target->colors = indexed ? (encoded?encodedColors:paletteEncoder.count) : 0;
  target->age = ++clock;
  cacheUsed += needed;
  cacheAllocationBlocked = false;
  if (indexed&&!encoded)
    paletteEncoder.encode(source, memory);
  else
    std::memcpy(memory, source, needed);
  sceKernelDcacheWritebackRange(memory, needed);
  markTextureDirty();
  return target;
}
inline Page*promoteCold(Cold::Entry&entry,const uint8_t*decoded){
  auto*previous=coldCache.pinned;coldCache.pinned=&entry;
  auto*p=storePage(decoded,entry.id,nullptr,entry.x,entry.y,entry.w,entry.h,entry.format,entry.colors,entry.file,true);
  coldCache.pinned=previous;return p;
}
inline void setCachePagesLimit(unsigned count) {
  flushBatch();
  cacheAllocationBlocked = false;
  pageLimit = std::clamp(count, 1u, PageCacheCount);
  for (unsigned i = pageLimit; i < pages.size(); ++i)
    discardPage(pages[i]);
}
