#pragma once
#include "release_paths.hpp"
// Include inside Main's anonymous namespace, after its GU helpers and audio.
// All pixels remain in aligned RAM. No shared VRAM address is owned or changed.
#include "editor_builder_audio.hpp"
#include "editor_builder_gift_raster.hpp"
#include "editor_builder_present.hpp"
#include <cmath>
#include <malloc.h>
#include <string>
namespace EditorBuilderMain {
enum class Menu { Other, Main, GameMenu, Extras, MinigameMenu, LevelSelect };
struct Update {
  bool consumeInput = false, ok = true;
};
inline void *allocate(size_t n, void *) { return memalign(64, n); }
inline void release(void *p, void *) { std::free(p); }
struct Memory {
  void *(*allocate)(size_t, void *) = EditorBuilderMain::allocate;
  void (*release)(void *, void *) = EditorBuilderMain::release;
  void *context = nullptr;
};
class Bridge {
public:
  explicit Bridge(Memory m = {}) : memory(m), presentation(store) {
    static_assert(ucg::editor_audio::Count > 361 &&
                      ucg::editor_audio::BuilderExplode.id == 361,
                  "Builder requires the verified 362-cue bank and exclusive "
                  "Explosion361");
    store.save = [this](uint32_t value, std::string &e) {
      if (value == 1 && !visibleReady) {
        waitingVisible = true;
        e = "Builder reveal has not been presented";
        return false;
      }
      if (!EditorBuilder::File(marker).write(value)) {
        e = "Cannot save Builder reward";
        return false;
      }
      return true;
    };
    presentation.clock = [this]() { return timelineUs / 1000; };
    presentation.onSound = [this](EditorBuilder::Cue cue) {
      using namespace ucg::editor_audio;
      const auto c = cue == EditorBuilder::Cue::Get   ? BuilderGet
                     : cue == EditorBuilder::Cue::Fly ? BuilderFly
                                                      : BuilderExplode;
      audio.playTuned(static_cast<Sound>(c.id), c.gainDb, c.pitch);
      if (cue == EditorBuilder::Cue::Explode)
        explosionIssued = true;
    };
  }
  Bridge(const Bridge &) = delete;
  Bridge &operator=(const Bridge &) = delete;
  // Call after selecting the game root; allocations and bank I/O are lazy.
  bool init(const char *directory) {
    shutdown();
    if (graphicsFatal || output || workspace)
      return false;
    message.clear();
    if (!directory || !*directory || !memory.allocate || !memory.release) {
      message = "Invalid Builder bridge initialization";
      return false;
    }
    root = directory;
    if (root.back() != '/' && root.back() != '\\')
      root += '/';
    marker = ReleasePaths::stateFromDirectory(root,EditorBuilder::FileName);
    if (!store.restore(EditorBuilder::File(marker).read(), message))
      return false;
    presentation.restorePending();
    initialized = true;
    visibleReady = revealDrawn = waitingVisible = skipClick = false;
    explosionIssued = false;
    suppressed = 0;
    timelineUs = lastUs = 0;
    clockValid = false;
    return true;
  }
  // Call immediately after init, before the first clock update. Normalize a
  // uint64 transport age as age<500 ? age : 500+(age-500)%2000: this preserves
  // the source 16-frame/8fps phase and settled pulse without long loops.
  // Absolute/idempotent; an already observed newer age cannot be rewound.
  bool restoreReturnAge(const char *text) {
    if (!text || !*text)
      return false;
    uint32_t milliseconds = 0;
    for (const char *p = text; *p; ++p) {
      if (*p < '0' || *p > '9')
        return false;
      const uint32_t digit = uint32_t(*p - '0');
      if (milliseconds > (2499u - digit) / 10)
        return false;
      milliseconds = milliseconds * 10 + digit;
    }
    return restoreIndicatorAge(milliseconds);
  }
  bool restoreIndicatorAge(uint32_t milliseconds) {
    if (milliseconds >= 2500)
      return false;
    const auto current = presentation.view();
    if (!initialized || !store.pending() ||
        current.phase != EditorBuilder::Phase::Idle ||
        current.animationMs > milliseconds)
      return false;
    presentation.advance(milliseconds - uint32_t(current.animationMs), false);
    return true;
  }
  Update update(Screen screen, bool crossPressed, uint64_t clockUs) {
    return updateMenu(screen == Screen::Main          ? Menu::Main
                      : screen == Screen::GameMenu    ? Menu::GameMenu
                      : screen == Screen::LevelSelect ? Menu::LevelSelect
                                                      : Menu::Other,
                      crossPressed, clockUs);
  }
  Update updateMenu(Menu menu, bool crossPressed, uint64_t clockUs) {
    if (clockValid && clockUs >= lastUs && message.empty()) {
      const uint64_t delta = clockUs - lastUs;
      timelineUs =
          delta > UINT64_MAX - timelineUs ? UINT64_MAX : timelineUs + delta;
    }
    lastUs = clockUs;
    clockValid = true;
    if (!initialized)
      return {false, message.empty()};
    const bool before = presentation.blocksInput();
    if (!message.empty())
      return {before || store.pending(), false};
    if (waitingVisible && visibleReady) {
      waitingVisible = false;
      if (!presentation.retryCommit(message))
        return {true, false};
      presentation.update(false);
      return {true, true}; // never acknowledge with the commit frame's edge
    }
    if (store.pending() && menu != Menu::Other && !presentation.active()) {
      if (!resources())
        return {true, false};
      presentation.enterMenu();
    }
    const bool ignore = skipClick;
    skipClick = false;
    const auto beforePhase = presentation.view().phase;
    presentation.update(crossPressed && !ignore);
    // Source ACK resets/hides the present child. Only its exclusive Explosion
    // stream stops; Master's Get/Fly streams retain their natural lifetimes.
    if (beforePhase == EditorBuilder::Phase::AwaitClick &&
        presentation.view().phase == EditorBuilder::Phase::Fading &&
        explosionIssued) {
      audio.stop(static_cast<Sound>(ucg::editor_audio::BuilderExplode.id));
      explosionIssued = false;
    }
    if (!presentation.error.empty() && !waitingVisible)
      message = presentation.error;
    // The previous draw fenced all readers. Release after the fade even if
    // the screen changed and no further Builder draw will run.
    if (output && !store.pending() && !presentation.active())
      freeResources();
    return {before || presentation.blocksInput() || ignore, message.empty()};
  }
  // Call only after Main swaps a successfully completed frame.
  void presented() {
    if (revealDrawn && !graphicsFatal) {
      visibleReady = true;
      revealDrawn = false;
    }
  }
  bool hasOverlay() const {
    return initialized && clockValid && (store.pending() || presentation.active());
  }
  bool blocksInput() const {
    return presentation.blocksInput() || (store.pending() && !message.empty());
  }
  // Called inside an active GU list, before Main's outer bars. Every output
  // mutation is preceded by a FINISH/SYNC; every draw completes before return.
  bool draw() {
    if (graphicsFatal)
      return false;
    if (!hasOverlay())
      return true;
    if (!message.empty())
      return false;
    if (!resources())
      return false;
    if (!fence())
      return false;
    const unsigned savedStatus = sceGuGetAllStatus();
    unsigned saved[8];
    const int ids[8] = {0xd4, 0xd5, 0xc6, 0xc7, 0xc9, 0xdf, 0xe0, 0xe1};
    for (unsigned i = 0; i < 8; ++i)
      saved[i] = sceGeGetCmd(ids[i]) & 0xffffffu;
    clip = {float(saved[0] & 1023), float((saved[0] >> 10) & 1023),
            float((saved[1] & 1023) + 1), float(((saved[1] >> 10) & 1023) + 1)};
    if (!(savedStatus & (1u << GU_SCISSOR_TEST)))
      clip = {36, 0, 444, 272};
    clip.left = std::max(36.f, clip.left);
    clip.top = std::max(0.f, clip.top);
    clip.right = std::min(444.f, clip.right);
    clip.bottom = std::min(272.f, clip.bottom);
    sceGuEnable(GU_BLEND);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    const auto v = presentation.view();
    EditorBuilderGift::Params params;
    params.x = v.x;
    params.y = v.y;
    params.scale = v.scale;
    params.darkAlpha = v.darkAlpha;
    params.spinFrame = v.spinFrame;
    params.ru = russian();
    params.shown = v.indicator || (presentation.active() &&
                                   v.phase != EditorBuilder::Phase::Fading);
    params.explodeMs = (v.phase == EditorBuilder::Phase::Idle ||
                        v.phase == EditorBuilder::Phase::Flying)
                           ? -1
                           : int(v.explodeMs);
    unsigned labels = 0;
    bool icon = false;
    EditorBuilderGift::visit(params, [&](const EditorBuilderGift::Command &c) {
      if (!message.empty())
        return;
      const bool painted = command(c);
      if (painted && c.kind == EditorBuilderGift::Kind::Bank && c.record >= 16)
        ++labels;
      if (painted && c.kind == EditorBuilderGift::Kind::External &&
          std::strcmp(c.path, EditorBuilderGift::BuilderPath) == 0)
        icon = true;
    });
    if (graphicsFatal || !fence())
      return false;
    for (unsigned i = 0; i < 8; ++i)
      sceGuSendCommandi(ids[i], int(saved[i]));
    sceGuSetAllStatus(savedStatus);
    if (!message.empty()) {
      // A resource retry must reopen the bank after the completed GE fence.
      freeResources();
      return false;
    }
    if (waitingVisible && labels == 6 && icon &&
        clip.left <= 36 && clip.top <= 0 && clip.right >= 444 &&
        clip.bottom >= 272)
      revealDrawn = true;
    return true;
  }
  bool retry() {
    if (!initialized || graphicsFatal)
      return false;
    if (message.empty())
      return true;
    message.clear();
    skipClick = true;
    if (presentation.view().phase == EditorBuilder::Phase::CommitError &&
        visibleReady) {
      waitingVisible = false;
      if (!presentation.retryCommit(message))
        return false;
    } else if (!resources())
      return false;
    // Reset elapsed origin after an error dialog; no hidden wall-clock jump.
    presentation.update(false);
    return true;
  }
  // A failed GE call leaves list/buffer ownership uncertain. Process teardown
  // reclaims the allocation without attempting another GU operation.
  bool abortGraphics(const char *reason) {
    if (!graphicsFatal) {
      graphicsFatal = true;
      revealDrawn = false;
      message = reason;
    }
    return false;
  }
  bool graphicsFailed() const { return graphicsFatal; }
  void shutdown() {
    if (graphicsFatal)
      return;
    freeResources();
    initialized = false;
  }
  const std::string &error() const { return message; }
  bool pending() const { return store.pending(); }
  bool owned() const { return store.owned(); }
  size_t resourceBytes() const {
    return (output ? 262144u : 0u) + (workspace ? 65536u : 0u);
  }
  EditorBuilder::View view() const { return presentation.view(); }
  bool setSlot(unsigned slot) { return presentation.setSlot(slot, message); }
  template <class Pad>
  void filterInput(const Update &u, uint32_t &pressed, uint32_t &released,
                   Pad &pad) {
    const uint32_t held = pad.Buttons;
    if (u.consumeInput) {
      suppressed |= held | pressed;
      pressed = released = pad.Buttons = 0;
      pad.Lx = pad.Ly = 128;
      return;
    }
    pressed &= ~suppressed;
    released &= ~suppressed;
    pad.Buttons &= ~suppressed;
    suppressed &= held;
  }

private:
  Memory memory;
  EditorBuilder::Store store;
  EditorBuilder::Presentation presentation;
  EditorBuilderGift::Reader reader;
  std::string root, marker, message;
  void *output = nullptr, *workspace = nullptr;
  uint64_t timelineUs = 0, lastUs = 0;
  uint32_t suppressed = 0;
  bool initialized = false, visibleReady = false, revealDrawn = false,
       waitingVisible = false, skipClick = false;
  bool clockValid = false, graphicsFatal = false, explosionIssued = false;
  EditorBuilderGiftRaster::PhysicalClip clip;
  void freeResources() {
    reader.close();
    if (workspace)
      memory.release(workspace, memory.context);
    if (output)
      memory.release(output, memory.context);
    workspace = output = nullptr;
  }
  bool resources() {
    if (output && workspace)
      return true;
    output = memory.allocate(262144, memory.context);
    if (output)
      workspace = memory.allocate(65536, memory.context);
    if (!output || !workspace) {
      freeResources();
      message = "Not enough memory for Builder present";
      return false;
    }
    if ((reinterpret_cast<uintptr_t>(output) & 63) ||
        (reinterpret_cast<uintptr_t>(workspace) & 63)) {
      freeResources();
      message = "Builder buffers must be 64-byte aligned";
      return false;
    }
    if (!reader.open((root + EditorBuilderGift::BankPath).c_str())) {
      freeResources();
      message = "Cannot open Builder gift bank";
      return false;
    }
    return true;
  }
  bool fence() {
    if (sceGuFinish() < 0)
      return abortGraphics("Cannot finish Builder GE list");
    if (sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE) < 0)
      return abortGraphics("Cannot fence Builder GE readers");
    if (sceGuStart(GU_DIRECT, displayList) < 0)
      return abortGraphics("Cannot resume Builder GE list");
    return true;
  }
  bool command(const EditorBuilderGift::Command &c) {
    constexpr float k = 17.f / 32.f;
    auto bounds = clip;
    if (c.clipped) {
      bounds.left = std::max(bounds.left, 36 + c.clip.x * k);
      bounds.top = std::max(bounds.top, c.clip.y * k);
      bounds.right = std::min(bounds.right, 36 + (c.clip.x + c.clip.w) * k);
      bounds.bottom = std::min(bounds.bottom, (c.clip.y + c.clip.h) * k);
    }
    const int left = int(std::ceil(bounds.left - .5f)),
              top = int(std::ceil(bounds.top - .5f));
    const int right = int(std::ceil(bounds.right - .5f)),
              bottom = int(std::ceil(bounds.bottom - .5f));
    if (left >= right || top >= bottom)
      return false;
    sceGuScissor(left, top, right, bottom);
    const float x = 36 + c.dest.x * k, y = c.dest.y * k, w = c.dest.w * k,
                h = c.dest.h * k;
    if (x + w <= bounds.left || y + h <= bounds.top || x >= bounds.right ||
        y >= bounds.bottom)
      return false;
    if (c.kind == EditorBuilderGift::Kind::Solid) {
      rectangle(x, y, w, h, c.tint);
      return true;
    }
    if (!fence())
      return false; // output may still be sampled by the previous command
    float dx = x, dy = y, dw = w, dh = h, u = c.source.x, v = c.source.y,
          uw = c.source.w, vh = c.source.h;
    unsigned bytes = 0;
    if (c.kind == EditorBuilderGift::Kind::Bank) {
      if (!reader.decode(c.record, output, 262144, workspace, 65536)) {
        message = "Cannot decode Builder gift bank record";
        return false;
      }
      const auto &r = EditorBuilderGift::Data::Records[c.record];
      bytes = r.raw;
      sceKernelDcacheWritebackRange(output, bytes);
      sceGuClutMode(GU_PSM_8888, 0, 255, 0);
      sceGuClutLoad(32, static_cast<uint8_t *>(output) + unsigned(r.pw) * r.ph);
      setTexture(GU_PSM_T8, r.pw, r.ph, output);
    } else {
      EditorBuilderGiftRaster::Texture t;
      const auto result = EditorBuilderGiftRaster::rasterize(
          (root + EditorBuilderGiftSource::BankPath).c_str(), c, bounds, output, 262144, workspace, 65536,
          t);
      if (result == EditorBuilderGiftRaster::Result::Empty)
        return false;
      if (result == EditorBuilderGiftRaster::Result::Error) {
        message = t.error ? t.error : "Cannot rasterize Builder gift image";
        return false;
      }
      bytes = t.bytes;
      dx = float(t.x);
      dy = float(t.y);
      dw = float(t.width);
      dh = float(t.height);
      u = v = 0;
      uw = float(t.width);
      vh = float(t.height);
      sceKernelDcacheWritebackRange(output, bytes);
      setTexture(GU_PSM_8888, t.potWidth, t.potHeight, output);
    }
    sceGuTexFlush();
    auto *vertices =
        static_cast<TextureVertex *>(sceGuGetMemory(2 * sizeof(TextureVertex)));
    vertices[0] = {u, v, c.tint, dx, dy, 0};
    vertices[1] = {u + uw, v + vh, c.tint, dx + dw, dy + dh, 0};
    sceGuDrawArray(GU_SPRITES,
                   GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF |
                       GU_TRANSFORM_2D,
                   2, nullptr, vertices);
    return true;
  }
};
} // namespace EditorBuilderMain
