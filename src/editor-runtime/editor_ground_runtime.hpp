#pragma once
// Included inside EditorRender after GhostFx/PlayerFx and before renderWorld.
namespace GroundRuntime {
namespace GC = ::EditorGroundCache;
static_assert(EditorArt::End == GC::Tail,
              "ground uses only the audited unused tail");
struct Storage {
  GC::Cache cache;
  EditorGroundRetire::Batch retirement;
  EditorGroundRetire::Result lastRetired{};
  GC::Backend backend{};
  GC::Graphics *graphics = nullptr;
  GC::Stamp frameStamp{};
  uint64_t sceneEpoch = 1;
  const EditorPlay::Scene *seen = nullptr;
  unsigned lastTick = 0;
  std::array<unsigned,16> postActive{};
  unsigned postCount = 0;
  unsigned coldRetiredEntries=0,coldRetiredBytes=0,coldReleasedSegmentBytes=0;
};
inline Storage storage;
inline const EditorGroundRetire::Stats&retiredStats(){return storage.retirement.stats();}
inline EditorGroundRetire::Epoch retireEpoch(){return {storage.frameStamp.scene,storage.frameStamp.document,storage.frameStamp.assets,cacheGeneration};}
inline bool snapshotRetirePage(void*,unsigned slot,EditorGroundRetire::Info&out){
 if(slot>=pages.size())return false;
 const auto&p=pages[slot];
 if(!p.pixels)return false;
 out={reinterpret_cast<uintptr_t>(p.pixels),p.id,p.age,p.bytes,p.format,p.file.empty()&&p.id<EditorArt::Packed::Records};return true;
}
inline void discardRetirePage(void*,unsigned slot){
  const auto retired=EditorGroundCold::retire(coldCache,pages[slot].id);
  storage.coldRetiredEntries+=retired.entries;
  storage.coldRetiredBytes+=retired.bytes;
  discardPage(pages[slot]);
}

inline constexpr size_t accountedBytes() {
  return sizeof(Storage) + sizeof(GC::Graphics) + sizeof(GC::State) +
         2 * sizeof(GC::Key);
}
inline void invalidate() {
#ifdef UCG_EDITOR_CAPTURE
  ::EditorGroundNativeProof::outcome(0);
#endif
  storage.retirement.cancel();storage.lastRetired={};
  storage.cache.invalidate();
  storage.frameStamp = {};
  storage.seen = nullptr;
  storage.lastTick = 0;
  if (!++storage.sceneEpoch)
    ++storage.sceneEpoch;
}
inline void sceneChanged() { invalidate(); }
inline void bind(GC::Graphics *graphics, GC::Backend backend) {
  invalidate();
  storage.graphics = graphics;
  storage.backend = backend;
}
inline void finishFrame(bool fenced) {
  const bool published=storage.cache.finishFrame(fenced, storage.frameStamp);
  storage.lastRetired=storage.retirement.finish(fenced,published,retireEpoch(),clock,snapshotRetirePage,discardRetirePage,nullptr);
  if(storage.lastRetired.pages&&coldCache.residentCharge==Cold::poolResidentBytes&&Cold::pool.owner==&coldCache)
    storage.coldReleasedSegmentBytes+=Cold::pool.releaseUnusedSegments();
}
inline GC::Color colorState(Color c) { return {c.r, c.g, c.b, c.a}; }
inline bool textPresent(const Value &root, const char *key) {
  const auto *v = root.find(key);
  return v && v->kind == Value::Kind::String && !v->text.empty();
}
inline bool trails(const View &view) {
  if (!view.scene)
    return true;
  for (const auto *s : GhostFx::states)
    if (s)
      for (const auto &t : s->traces)
        if (t.life > 0 && float(view.scene->tick - t.born) < t.life)
          return true;
  for (auto *t = PlayerFx::traces; t; t = t->next) {
    const float age = float(view.scene->tick - t->born);
    if (!(age >= t->life || t->life <= 0))
      return true;
  }
  return false;
}
inline std::array<Color, 4> sourcePalette(const Document &doc, unsigned layer) {
  if (layer == 0 || layer == 5)
    return {{readColor(doc.root, "tBright"), readColor(doc.root, "tNormal"),
             readColor(doc.root, "tDark"), readColor(doc.root, "tExtra")}};
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
  return {{values[layer - 1][0], values[layer - 1][1], values[layer - 1][2],
           values[layer - 1][3]}};
}
inline bool snapshot(const View &view, const Document &doc, GC::State &out) {
  const auto *scene = view.scene;
#ifdef UCG_EDITOR_CAPTURE
  ::EditorGroundNativeProof::outcome(0);
#endif
  if (!scene || !storage.graphics || !storage.graphics->contextReady ||
      !storage.backend.fence || !storage.backend.copy)
    return false;
  // Explicit Session start/stop/restart hooks supply the primary epoch. Tick
  // regression/pointer replacement is an additional conservative safety net.
  if (storage.seen != scene || (storage.seen && scene->tick < storage.lastTick))
    invalidate();
  storage.seen = scene;
  storage.lastTick = scene->tick;
  const double bg = number(doc.root, "bg");
  if (!std::isfinite(bg) || bg < 3 || bg >= 4 ||
      textPresent(doc.root, "customfloor"))
    return false;
  static constexpr const char *customKeys[] = {
      "customtiles",        "switch1customtiles", "switch2customtiles",
      "switch3customtiles", "switch4customtiles", "dogcustomtiles"};
  for (const auto *key : customKeys)
    if (textPresent(doc.root, key))
      return false;
  if (Lighting::points.size() > GC::MaxLights || trails(view))
    return false;
  for (const auto &d : scene->decorations)
    if (d.z < 0)
      return false;
  // A final visible builtin tile guarantees deterministic selection on every
  // original pass, even when the independent CPU floor raster is already warm.
  if(Lighting::points.empty())return false;
  bool selectedTile=false;
  for(const auto&t:scene->tiles){
    if(t.layer<0||t.layer>=6||t.ax<0||t.ax>=40||t.ay<0||t.ay>=4)continue;
    Vec2 p{double(t.x*16)+view.offset.x,double(t.y*16)+view.offset.y};
    const std::array<Vertex,4> q{{view.vertex(p,0,0),view.vertex({p.x+16,p.y},16,0),
      view.vertex({p.x+16,p.y+16},16,16),view.vertex({p.x,p.y+16},0,16)}};
    if(visible(q)){selectedTile=true;break;}
  }
  if(!selectedTile)return false;
  out = GC::State{};
  out.incomingActive=Lighting::active;
  out.incomingCount=Lighting::count;
  out.stamp = {storage.sceneEpoch, storage.sceneEpoch,
               uint64_t(cacheGeneration) + 1};
  out.play = out.immutableInputs = out.asphaltOpaque = true;
  out.knownClear = out.defaultBlend = out.depthOff = out.ditherOff = true;
  const auto &floorImage =
      EditorArt::Packed::Images[EditorArt::Packed::Floors[3].image];
  out.floorWidth = floorImage.width;
  out.floorHeight = floorImage.height;
  out.view = {view.cameraX,
              view.cameraY,
              view.centerX,
              view.centerY,
              view.zoom,
              view.inverseZoom,
              view.offset.x,
              view.offset.y,
              clip.x,
              clip.y,
              clip.right,
              clip.bottom,
              0,
              0,
              0};
  const Vec2 zero = view.world({(172.5 - ViewX) / Scale, .5 / Scale});
  out.view.floorStartX = float(zero.x);
  out.view.floorStartY = float(zero.y);
  out.view.floorStep = 1.f / (float(view.zoom) * Scale);
  out.ambientSource = colorState(view.light);
  out.ambientShader = {{Lighting::ambient.r, Lighting::ambient.g,
                        Lighting::ambient.b, Lighting::ambient.a}};
  out.ambientRgba = rgba(view.light);
  out.lightCount = unsigned(Lighting::points.size());
  for (unsigned i = 0; i < out.lightCount; ++i) {
    const auto &p = Lighting::points[i];
    out.lights[i] = {p.x, p.y, p.xx, p.xy, p.yx,      p.yy,
                     p.r, p.g, p.b,  p.a,  p.extentX, p.extentY};
  }
  for (unsigned layer = 0; layer < 6; ++layer) {
    out.layerAlpha[layer] = scene->layer_active(int(layer)) ? 1 : .2;
    const auto source = sourcePalette(doc, layer);
    out.palettes[layer].rgba = tileColors(doc, layer);
    for (unsigned i = 0; i < 4; ++i)
      out.palettes[layer].source[i] = colorState(source[i]);
  }
  for (unsigned i = 0; i < scene->entities.size(); ++i) {
    const auto &n = scene->entities[i];
    if (!n.active)
      continue;
    if (n.objectId == 23)
      return false;
    if (n.objectId != 19 && n.objectId != 20)
      continue;
    if (out.nodeCount >= GC::MaxNodes ||
        n.polygon.size() > GC::MaxVertices - out.vertexCount)
      return false;
    auto &v = out.nodes[out.nodeCount++];
    v.order = i;
    v.id = unsigned(n.objectId);
    v.variant = n.variant;
    v.x = n.position.x;
    v.y = n.position.y;
    v.sx = n.scale.x;
    v.sy = n.scale.y;
    v.rotation = n.rotation;
    v.tint = colorState(n.color);
    v.first = out.vertexCount;
    v.count = unsigned(n.polygon.size());
    for (auto p : n.polygon)
      out.vertices[out.vertexCount++] = {p.x, p.y};
  }
  storage.frameStamp = out.stamp;
#ifdef UCG_EDITOR_CAPTURE
  ::EditorGroundNativeProof::outcome(1);
#endif
  return true;
}
inline GC::Restore restore(const GC::Key &key) {
#if defined(UCG_EDITOR_DISABLE_GROUND_CACHE) && UCG_EDITOR_DISABLE_GROUND_CACHE
  (void)key;return GC::Restore::Miss;
#endif
  const auto result=storage.cache.restore(
      key, storage.graphics ? storage.graphics->drawOffset : ~0u,
      storage.backend);
#ifdef UCG_EDITOR_CAPTURE
  if(result==GC::Restore::Hit)::EditorGroundNativeProof::outcome(2);
#endif
  if(result==GC::Restore::Hit){Lighting::active=storage.postActive;Lighting::count=storage.postCount;}
  return result;
}
inline bool capture(const GC::Key &key, unsigned groundBeginClock=clock) {
  const auto before=storage.cache.stats().fenceFailures;
  const bool result=storage.cache.capture(
      key, storage.graphics ? storage.graphics->drawOffset : ~0u,
      storage.backend);
  if(storage.cache.stats().fenceFailures!=before)fail("Ground framebuffer capture fence failed");
#ifdef UCG_EDITOR_CAPTURE
  ::EditorGroundNativeProof::outcome(result?3:4);
#endif
  if(result){storage.postActive=Lighting::active;storage.postCount=Lighting::count;
#if defined(UCG_EDITOR_DISABLE_GROUND_CACHE) && UCG_EDITOR_DISABLE_GROUND_CACHE
    (void)groundBeginClock;storage.retirement.cancel();
#else
    storage.retirement.stage(groundBeginClock,clock,unsigned(pages.size()),retireEpoch(),EditorArt::Packed::Records,true,snapshotRetirePage,nullptr);
#endif
  }else storage.retirement.cancel();
  return result;

}
} // namespace GroundRuntime
