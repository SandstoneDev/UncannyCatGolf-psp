#pragma once
// Included inside Module's anonymous namespace after editor_render.hpp.
inline bool groundFence(void *opaque) {
  auto &g = *static_cast<EditorGroundCache::Graphics *>(opaque);
  if (!g.contextReady)
    return false;
  flushBatch();
  if(!g.fenceOK)return false;
  EditorRender::ensureBatch();
  EditorRender::applyClip();
  return true;
}
inline bool groundCopy(void *opaque, const EditorGroundCache::Transfer &t) {
  auto &g = *static_cast<EditorGroundCache::Graphics *>(opaque);
  if (!g.contextReady || !drawOpen || !EditorGroundCache::validTransfer(t))
    return false;
  sceGuCopyImage(GU_PSM_5650, int(t.sx), int(t.sy), int(t.w), int(t.h),
                 int(t.sourceStride), EditorArt::video(t.source), int(t.dx),
                 int(t.dy), int(t.destinationStride),
                 EditorArt::video(t.destination));
  return true;
}
inline void bindGroundGraphics() {
  groundGraphics.contextReady = true;
  EditorRender::GroundRuntime::bind(&groundGraphics,
                                    {groundFence, groundCopy, &groundGraphics});
}
inline void swappedGroundGraphics(void *pointer) {
  const uintptr_t value = reinterpret_cast<uintptr_t>(pointer),
                  base = reinterpret_cast<uintptr_t>(sceGeEdramGetAddr());
  unsigned offset = ~0u;
  if (value == 0 || value == EditorGroundCache::FrameBytes)
    offset = unsigned(value);
  else if (value == base || value == base + EditorGroundCache::FrameBytes)
    offset = unsigned(value - base);
  else if (value == (base | 0x40000000u) ||
           value == (base | 0x40000000u) + EditorGroundCache::FrameBytes)
    offset = unsigned(value - (base | 0x40000000u));
  groundGraphics.drawOffset = offset;
  if (offset == ~0u) {
    groundGraphics.contextReady = false;
    EditorRender::GroundRuntime::invalidate();
  }
}
