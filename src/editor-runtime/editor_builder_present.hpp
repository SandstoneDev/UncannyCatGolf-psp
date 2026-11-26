#pragma once
#include "editor_builder_reward.hpp"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
namespace EditorBuilder {
enum class Phase { Idle, Flying, Exploding, AwaitClick, CommitError, Fading };
enum class Cue { Get, Fly, Explode };
struct View {
  Phase phase = Phase::Idle;
  uint32_t elapsedMs = 0, explodeMs = 0;
  uint64_t animationMs = 0;
  unsigned slot = 0, spinFrame = 0;
  float x = 227, y = 128, scale = .2f;
  // Master.Darkener MODULATE alpha only. Renderer multiplies source color.a
  // (0.901961, displayed byte230); do not apply that factor twice.
  float darkAlpha = 0;
  bool indicator = false, presentVisible = false, detailsVisible = false,
       explosionVisible = false, inputBlocked = false;
};
class Presentation {
public:
  explicit Presentation(Store &s) : store(s) { restorePending(); }
  Presentation(const Presentation &) = delete;
  Presentation &operator=(const Presentation &) = delete;
  std::function<void(Cue)> onSound;
  std::function<uint64_t()> clock; // optional monotonic milliseconds
  std::function<void(const View &)> render;
  std::string error;
  bool queue(std::string &e) {
    bool added = false;
    if (!store.queue(added, e)) {
      error = e;
      return false;
    }
    if (added) {
      restorePending();
      emit(Cue::Get);
    }
    return true;
  }
  // Restore a persisted pending indicator without replaying the award sound.
  // Call after Store.restore; this does not open it or change its saved state.
  void restorePending() {
    phase = Phase::Idle;
    elapsed = 0;
    pulse = store.pending() ? 0 : 500;
    error.clear();
    clockValid = false;
  }
  bool setSlot(unsigned next, std::string &e) {
    e.clear();
    if (next > 60) {
      e = "Builder present slot exceeds source limit61";
      return false;
    }
    if (phase != Phase::Idle) {
      e = "Cannot move an active Builder present";
      return false;
    }
    slot = next;
    return true;
  }
  // Root calls this only for source menus with open_unlock_presents=true.
  // Indicators are intentionally not clickable and no opening hotkey exists.
  bool enterMenu() {
    if (!store.pending() || phase != Phase::Idle)
      return false;
    flyScale = indicatorScale();
    phase = Phase::Flying;
    elapsed = 0;
    error.clear();
    if (clock) {
      lastClock = clock();
      clockValid = true;
    } else
      clockValid = false;
    emit(Cue::Fly);
    return true;
  }
  bool retryCommit(std::string &e) {
    e.clear();
    if (phase != Phase::CommitError) {
      e = "Builder present has no failed ownership commit";
      return false;
    }
    if (!store.markOpened(e)) {
      error = e;
      return false;
    }
    phase = Phase::AwaitClick;
    error.clear();
    return true;
  }
  // click is a new Click edge, never a held button or Circle. The callback
  // which invokes retryCommit must consume its retry action before update.
  void advance(uint32_t milliseconds, bool click) {
    animation = add(animation, milliseconds);
    pulse = std::min(uint32_t(500), add32(pulse, milliseconds));
    if (phase == Phase::Idle)
      return;
    if (phase == Phase::Fading) {
      elapsed = add32(elapsed, milliseconds);
      if (elapsed >= 500) {
        phase = Phase::Idle;
        elapsed = 0;
      }
      return;
    }
    const Phase beforePhase = phase;
    const uint32_t before = elapsed;
    elapsed = add32(elapsed, milliseconds);
    if (beforePhase == Phase::Flying || beforePhase == Phase::Exploding) {
      if (before < 1500 && elapsed >= 1500)
        emit(Cue::Explode);
      phase = elapsed < 1000 ? Phase::Flying : Phase::Exploding;
      if (elapsed >= 2000) {
        std::string e;
        if (store.markOpened(e)) {
          phase = Phase::AwaitClick;
          error.clear();
        } else {
          phase = Phase::CommitError;
          error = e;
        }
      }
      // Even if this step crosses present_opened, its earlier click cannot
      // acknowledge an await that did not exist when that input was received.
      return;
    }
    if (beforePhase == Phase::AwaitClick && click) {
      phase = Phase::Fading;
      elapsed = 0;
    }
  }
  // Call this OR advance(), not both in one frame. First and backward clock
  // samples have zero elapsed time and cannot add a startup/retry tick.
  void update(bool click) {
    if (!clock) {
      advance(0, click);
      return;
    }
    const uint64_t now = clock();
    uint32_t delta = 0;
    if (clockValid && now >= lastClock)
      delta = uint32_t(std::min(uint64_t(UINT32_MAX), now - lastClock));
    lastClock = now;
    clockValid = true;
    advance(delta, click);
  }
  void draw() const {
    if (render)
      render(view());
  }
  bool active() const { return phase != Phase::Idle; }
  bool blocksInput() const {
    return phase != Phase::Idle && phase != Phase::Fading;
  }
  View view() const {
    View out;
    out.phase = phase;
    out.elapsedMs = elapsed;
    out.animationMs = animation;
    out.explodeMs =
        elapsed >= 1000 ? std::min(uint32_t(1500), elapsed - 1000) : 0;
    out.slot = slot;
    out.spinFrame = unsigned(animation / 125 % 16);
    out.x = homeX();
    out.y = 128;
    out.scale = indicatorScale();
    out.inputBlocked = blocksInput();
    out.indicator = phase == Phase::Idle && store.pending();
    out.presentVisible = out.indicator;
    if (phase == Phase::Flying) {
      const float t = cubic(elapsed / 1000.f);
      out.x += (512 - out.x) * t;
      out.y += (256 - out.y) * t;
      out.scale = flyScale + (1 - flyScale) * t;
      out.presentVisible = true;
    } else if (phase == Phase::Exploding || phase == Phase::AwaitClick ||
               phase == Phase::CommitError) {
      out.x = 512;
      out.y = 256;
      out.scale = 1;
      out.presentVisible = elapsed < 1750;
      out.detailsVisible = elapsed >= 1500;
      out.explosionVisible = elapsed >= 1500 && elapsed < 2000;
    }
    if (phase == Phase::Fading)
      out.darkAlpha = 1 - cubic(elapsed / 500.f);
    else if (phase != Phase::Idle)
      out.darkAlpha = cubic(elapsed / 500.f);
    return out;
  }

private:
  Store &store;
  Phase phase = Phase::Idle;
  uint32_t elapsed = 0, pulse = 500;
  uint64_t animation = 0, lastClock = 0;
  unsigned slot = 0;
  float flyScale = .2f;
  bool clockValid = false;
  static uint64_t add(uint64_t a, uint32_t b) {
    return a > UINT64_MAX - b ? UINT64_MAX : a + b;
  }
  static uint32_t add32(uint32_t a, uint32_t b) {
    return a > UINT32_MAX - b ? UINT32_MAX : a + b;
  }
  static float cubic(float t) {
    t = std::clamp(t, 0.f, 1.f);
    if (t < .5f)
      return 4 * t * t * t;
    const float back = 2 - 2 * t;
    return 1 - back * back * back * .5f;
  }
  float homeX() const { return slot < 9 ? 227.f - 25.6f * slot : -29.f; }
  float indicatorScale() const { return .25f - .05f * cubic(pulse / 500.f); }
  void emit(Cue cue) {
    if (onSound)
      onSound(cue);
  }
};
} // namespace EditorBuilder
