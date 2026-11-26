#pragma once
#include "editor_tile_shapes.hpp"
#include "game.hpp"
#include "avatar_idle059.hpp"
#include "ucg_editor_model.hpp"
namespace EditorPlay {
using ModelValue = ucg::editor::Value;
using ModelColor = ucg::editor::Color;
inline int64_t source_integer(double n) {
  if (!std::isfinite(n) || n >= 9223372036854775808.0 ||
      n < -9223372036854775808.0)
    return INT64_MIN;
  return int64_t(n);
}
inline int64_t wrap_add(int64_t a, int64_t b) {
  uint64_t bits = uint64_t(a) + uint64_t(b);
  int64_t result;
  std::memcpy(&result, &bits, sizeof(result));
  return result;
}
inline int64_t wrap_multiply(int64_t a, int64_t b) {
  uint64_t bits = uint64_t(a) * uint64_t(b);
  int64_t result;
  std::memcpy(&result, &bits, sizeof(result));
  return result;
}
inline int64_t integer(const ModelValue &v, const char *k,
                       int64_t fallback = 0) {
  const auto *n = v.find(k);
  return n && n->kind == ModelValue::Kind::Number ? source_integer(n->numeric())
                                                  : fallback;
}
inline constexpr float Pi = 3.14159265358979323846f;
inline ::Vec2 unit(::Vec2 v) {
  float n = length(v);
  return n > 0 ? v * (1 / n) : ::Vec2{};
}
inline ::Vec2 mul(::Vec2 a, ::Vec2 b) { return {a.x * b.x, a.y * b.y}; }
inline ::Vec2 rotate(::Vec2 v, float r) {
  float c = std::cos(r), s = std::sin(r);
  return {c * v.x - s * v.y, s * v.x + c * v.y};
}
namespace detail {
struct Bounds {
  float left = 0, top = 0, right = 0, bottom = 0;
  static Bounds point(::Vec2 p) { return {p.x, p.y, p.x, p.y}; }
  static Bounds centered(::Vec2 p, ::Vec2 half) {
    return {p.x - half.x, p.y - half.y, p.x + half.x, p.y + half.y};
  }
  void include(::Vec2 p) {
    left = std::min(left, p.x);
    top = std::min(top, p.y);
    right = std::max(right, p.x);
    bottom = std::max(bottom, p.y);
  }
  bool intersects(const Bounds &b) const {
    return left <= b.right && right >= b.left && top <= b.bottom &&
           bottom >= b.top;
  }
  static Bounds swept(::Vec2 p, ::Vec2 motion, float radius) {
    const auto end = p + motion;
    // The unchanged narrowphase admits time >= -0.0001. Include that interval
    // and conservative float32 roundoff at large authored coordinates.
    const float eps = 8 * std::numeric_limits<float>::epsilon();
    const float px =
        radius + .001f + std::abs(motion.x) * .0001f +
        eps * (std::max(std::abs(p.x), std::abs(end.x)) + radius + 1);
    const float py =
        radius + .001f + std::abs(motion.y) * .0001f +
        eps * (std::max(std::abs(p.y), std::abs(end.y)) + radius + 1);
    return {std::min(p.x, end.x) - px, std::min(p.y, end.y) - py,
            std::max(p.x, end.x) + px, std::max(p.y, end.y) + py};
  }
};
class SpatialIndex {
  struct Entry {
    int x, y;
    unsigned id;
  };
  std::vector<Entry> entries;
  std::vector<unsigned> oversized;
  static constexpr size_t MaxReferences = 65536;
  static bool cells(const Bounds &b, int &left, int &top, int &right,
                    int &bottom, unsigned cap) {
    // Division by a power of two is exact for finite normal float32 bounds.
    // Outside float's exact integer cell range, keep the conservative flat
    // path.
    const float q[] = {std::floor(b.left / 64.f), std::floor(b.top / 64.f),
                       std::floor(b.right / 64.f), std::floor(b.bottom / 64.f)};
    for (float v : q)
      if (!std::isfinite(v) || v < -16777216.f || v > 16777216.f)
        return false;
    left = int(q[0]);
    top = int(q[1]);
    right = int(q[2]);
    bottom = int(q[3]);
    const int64_t w = int64_t(right) - left + 1, h = int64_t(bottom) - top + 1;
    return w > 0 && h > 0 && w <= cap && h <= cap && w * h <= cap;
  }
  static bool less(const Entry &a, const Entry &b) {
    return a.x != b.x ? a.x < b.x : a.y != b.y ? a.y < b.y : a.id < b.id;
  }

public:
  template <class Records> void build(const Records &records) {
    entries.clear();
    oversized.clear();
    entries.reserve(std::min(MaxReferences, records.size() * 2));
    for (unsigned id = 0; id < records.size(); ++id) {
      int left, top, right, bottom;
      if (!cells(records[id].bounds, left, top, right, bottom, 16) ||
          entries.size() + size_t(int64_t(right) - left + 1) *
                               size_t(int64_t(bottom) - top + 1) >
              MaxReferences) {
        oversized.push_back(id);
        continue;
      }
      for (int64_t x = left; x <= right; ++x)
        for (int64_t y = top; y <= bottom; ++y)
          entries.push_back({int(x), int(y), id});
    }
    std::sort(entries.begin(), entries.end(), less);
  }
  bool collect(const Bounds &query, std::vector<unsigned> &ids) const {
    if (entries.empty() && oversized.empty()) {
      ids.clear();
      return true;
    }
    int left, top, right, bottom;
    // Broad/invalid sweeps fall back to the original ordered flat traversal.
    if (!cells(query, left, top, right, bottom, 256))
      return false;
    ids.assign(oversized.begin(), oversized.end());
    for (int64_t x = left; x <= right; ++x)
      for (int64_t y = top; y <= bottom; ++y) {
        const Entry key{int(x), int(y), 0};
        auto it = std::lower_bound(entries.begin(), entries.end(), key, less);
        while (it != entries.end() && it->x == x && it->y == y) {
          ids.push_back(it->id);
          ++it;
        }
      }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return true;
  }
  void clear_memory() {
    std::vector<Entry>().swap(entries);
    std::vector<unsigned>().swap(oversized);
  }
};
} // namespace detail
inline bool sameColor(ModelColor a, ModelColor b) {
  return float(a.r) == float(b.r) && float(a.g) == float(b.g) &&
         float(a.b) == float(b.b) && float(a.a) == float(b.a);
}
inline float scalar(const ModelValue &v, const char *k, float fallback = 0) {
  const auto *n = v.find(k);
  return n && n->kind == ModelValue::Kind::Number ? float(n->numeric())
                                                  : fallback;
}
inline bool boolean(const ModelValue &v, const char *k, bool fallback = false) {
  const auto *n = v.find(k);
  return n && n->kind == ModelValue::Kind::Bool ? n->boolean : fallback;
}
inline ::Vec2 vector(const ModelValue &v, const char *k, ::Vec2 fallback = {}) {
  const auto *n = v.find(k);
  ucg::editor::Vec2 p;
  return n && ucg::editor::read_vector(*n, p) ? ::Vec2{float(p.x), float(p.y)}
                                              : fallback;
}
struct Entity {
  int objectId = 0, spawnOwner = -1, variant = 0;
  const char *scenePath = "";
  ::Vec2 position{}, scale{1, 1}, velocity{}, halfSize{16, 16};
  ModelColor color, lockColor;
  bool active = true, taken = false, dead = false;
  float rotation = 0, spawnRotation = 0;
  unsigned frame = 0;
  std::vector<::Vec2> polygon;
  float radius = 16, speed = 0, acceleration = .35f, speedLimit = 40,
        setSpeed = 0, timeGiven = 0;
  int64_t scoreValue = 0, spawnScoreValue = 0, coinMultiplyValue = 0;
  unsigned thoughtCategory = 0;
  ::Vec2 deathPitch{1, 1};
  unsigned collectionMask = 2;
  bool dblockScore = false, ringBell = true, colorful = false, grazing = false;
  bool ringWon = false;
  float lockAlpha = 1;
  float ringAudioPitch = .5f;
  bool chaser = false, inactive = false, redoable = false, coinMultiply = false,
       doesSpeed = false, ignoreY = false, keepFace = false,
       bounceOffPlayer = true;
  int spawnEnemy = 0, spawnRate = 0, countdown = 0, maxBaddies = 0,
      spawnedCount = 0, linked = -1, timer = 0, coinsGrabbed = 0, mod = 0,
      parryStun = 0;
  bool running = false, inside = false, held = false, screenSpace = false;
  ::Vec2 storedVelocity{};
  float smoothingSpeed = 10, segmentX = 512, segmentY = 512, phase = 0,
        visualAlpha = 1, accel = -.2f, speedMinus = 0;
  bool smoothing = false;
  uint32_t audioUid = 0;
  // Baddie.queue_free is deferred: retain motion/collision until the frame
  // ends.
  bool boundsRetirementQueued = false;
};
struct Tile {
  int layer = 0, x = 0, y = 0, ax = 0, ay = 0;
  int shape = -1;
  detail::Bounds bounds{};
};
struct Decoration {
  ::Vec2 position{}, scale{1, 1};
  float rotation = 0;
  int z = 0, frame = 0, hframes = 1, vframes = 1;
  float animSpeed = 0;
  bool collision = false;
  std::string texture, imageFilename;
  float phase = 0;
  int animatedFrame = 0;
};
struct Event {
  enum Type {
    Sound,
    Score,
    Death,
    Goal,
    Modifier,
    Spawn,
    Popup,
    Effect,
    Thought,
    StopAudioOwner
  };
  Type type = Sound;
  int objectId = 0;
  double value = 0;
  const char *sound = "";
  float pitch = 1;
  int64_t score = 0;
  ::Vec2 position{};
  bool hasPosition = false, popup = true;
  float gainDb = 0;
  const char *text = nullptr;
  ::Vec2 scale{1, 1};
  float rotation = 0;
  ModelColor tint{1, 1, 1, 1};
  uint32_t voiceOwner = 0;
};
struct Convex {
  ::Vec2 points[4];
  unsigned count = 0;
  detail::Bounds bounds{};
};
struct VisualState {
  enum class Phase { Spawn, Idle, Moving, Telekinetic, Jump, Parry, Win, Lose };
  Phase phase = Phase::Spawn;
  unsigned stateTicks = 0, idlePulseTicks = UINT32_MAX, deathTicks = 0;
  bool levelActive = false;
  float jumpyFraction = 1;
  ::Vec2 additionalVelocity{}, motionPosition{}, motionVelocity{},
      deathPosition{};
};
class Scene {
public:
  static constexpr size_t MaxEntities = 2048, MaxInitialEntities = 1024,
                          MaxTiles = 20000, MaxColliders = 32768;
  static constexpr unsigned IntroTicks =
      62; // Godot 4.4.1 Vector2 float accumulation oracle.
  Course course{};
  TileField field;
  Game game;
  std::vector<Entity> entities;
  std::vector<Tile> tiles;
  std::vector<Decoration> decorations;
  std::vector<Event> events;
  std::vector<std::string> diagnostics;
  ::Vec2 camera{}, worldOffset{}, pointerScreen{384, 256};
  unsigned tick = 0;
  AvatarIdle059::Speech idleSpeech059;
  ModelColor speechTint059{1,1,1,1};
  ::Vec2 speechScale059()const{return playerScale;}
  unsigned elapsedTicks = 0, spawnFrames = IntroTicks;
  float playerVisualScale = .003f / .064f, playerRotation = 0;
  float ringAudioPitch = .5f, tkAudioPitch = .75f;
  double strokeSaturation = .5;
  int64_t bonus = 0;
  int switchState = 0;
  bool dead = false, dogTiles = true, testMode = true;
  int deathReason = -1;
  ModelColor deathTint{1, 1, 1, 1};
  bool bufferShots = false;
  float bounds = 256, onTime = 0;
  int par = 0, rankReqs[6] = {0, 1, 0, 0, 0, 0};
  std::string fault;
  std::function<bool(const Decoration &, std::vector<std::vector<::Vec2>> &,
                     std::string &)>
      decorationCollider;
  Scene() : game(course, &field) {
    field.beginDynamic();
    game.goalEnabled = false;
  }
  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;
  bool load(const ucg::editor::Document &, const Course &, std::string &);
  void restart();
  void step60();
  bool shoot(::Vec2);
  bool begin_drag();
  void action();
  void pointer_delta(::Vec2 delta) { pointerDelta = pointerDelta + delta; }
  bool teleport(::Vec2 position);
  void click_pointer(::Vec2 screen);
  void clear_memory();
  double tk_fraction() const { return tkStrokes; }
  bool tk_display_active() const { return tkTextActive; }
  VisualState visual_state() const {
    VisualState state;
    state.phase = playerPhase();
    state.stateTicks = tick - phaseEnteredTick;
    state.idlePulseTicks =
        idlePulseTick == UINT32_MAX ? UINT32_MAX : tick - idlePulseTick;
    state.deathTicks = dead ? state.stateTicks : 0;
    state.levelActive = !spawnFrames && !introInterrupted && !dead && !game.won;
    state.jumpyFraction = jumpyBarFraction;
    state.motionPosition = frameMotionPosition;
    state.motionVelocity = frameMotionVelocity;
    state.deathPosition = deathPosition;
    // None of the 26 editor objects supplies wind; EditorLevel ignores Player
    // record vars.
    state.additionalVelocity = {};
    return state;
  }
  bool ready_for_input() const {
    return spawnFrames == 0 && !dead && !game.won;
  }
  float elapsed_seconds() const { return float(elapsedTicks) / 60; }
  bool telekinesis_audio_active() const {
    return ready_for_input() && telekineticState && !jumpFrames && !parryFreeze;
  }
  int64_t final_score() const {
    if (game.won)
      return wonScore;
    int64_t strokes = std::max<int64_t>(
        5000 + 500 * (int64_t(par) - int64_t(game.strokes)), 0);
    int64_t time = std::max<int64_t>(
        source_integer(2500 - 100 * (double(elapsedTicks) / 60 - onTime)), 0);
    return wrap_add(wrap_add(strokes, time), bonus);
  }
  bool layer_active(int layer) const {
    return layer == 0 || (layer == 5 && dogTiles) ||
           (layer >= 1 && layer <= 4 && !switchLayers.empty() &&
            switchLayers[size_t(switchState) % switchLayers.size()] == layer);
  }
  unsigned modifier_mask() const { return mods; }

private:
  enum Mod : unsigned {
    Hard = 1u << 0,
    Burst = 1u << 1,
    Telekinesis = 1u << 2,
    Buddy = 1u << 3,
    Parry = 1u << 4,
    Jumpy = 1u << 5,
    Dizzy = 1u << 6,
    Missiles = 1u << 7,
    Sticky = 1u << 8,
    God = 1u << 9,
    Borts = 1u << 10,
    Bees = 1u << 11,
    Easy = 1u << 12
  };
  unsigned mods = 0, initialMods = 0, randomState = 1;
  std::vector<Entity> initialEntities;
  std::vector<Decoration> initialDecorations;
  std::vector<Convex> colliders;
  detail::SpatialIndex tileIndex, colliderIndex;
  std::vector<unsigned> staticCandidates;
  std::vector<::CircleCollider> passiveCircles;
  std::vector<::SolidBlock> passiveBoxes;
  std::vector<int> switchLayers;
  ::Vec2 playerScale{1, 1}, pointerDelta{}, parryVelocity{}, lastNormal{};
  int lastObstacle = -1, lastBodyObstacle = -1, portalCooldown = 0,
      jumpFrames = 0, jumpBuffer = 0, parryActive = 0, parryFreeze = 0,
      coinMultiplier = 0, dblockMultiplier = 0, wallStuck = 0, wallWindow = 0;
  double tkStrokes = 0;
  float distanceTraveled = 0, wallPitch = 1, spawnSpriteScale = .003f;
  float movingDistance = 0, tkDistance = 0;
  ::Vec2 lastMovingPosition{}, lastTkPosition{};
  bool stepping = false, sandDamped = false, wasMoving = false,
       shotBuffered = false, tkBurstRequested = false, parryRequested = false,
       shotThisTick = false, telekineticState = false, introInterrupted = false,
       tkTextActive = false;
  unsigned nextBeeGroup = 0, nextMissileGroup = 0;
  ::Vec2 bufferedShot{};
  unsigned bufferedAt = 0;
  VisualState::Phase lastPhase = VisualState::Phase::Spawn;
  unsigned phaseEnteredTick = 0, idlePulseTick = UINT32_MAX;
  float jumpyBarFraction = 1;
  ::Vec2 frameMotionPosition{}, frameMotionVelocity{}, deathPosition{};
  VisualState::Phase playerPhase() const {
    using P = VisualState::Phase;
    return dead               ? P::Lose
           : game.won         ? P::Win
           : spawnFrames      ? P::Spawn
           : jumpFrames       ? P::Jump
           : parryFreeze      ? P::Parry
           : telekineticState ? P::Telekinetic
           : wasMoving        ? P::Moving
                              : P::Idle;
  }
  void updateVisualPhase() {
    auto phase = playerPhase();
    if (phase != lastPhase) {
      lastPhase = phase;
      phaseEnteredTick = tick;
    }
  }
  void stepWorld(bool jumping, bool canTouch);
  void enterJump() {
    jumpFrames =
        26; // transition_to executes the first of 27 updates immediately.
    const float scale = playerVisualScale * .064f;
    playerVisualScale = (scale + (.15f - scale) * .13f) / .064f;
  }
  ::Vec2 winTarget{};
  int64_t wonScore = 0;
  std::vector<Event> queuedEvents;
  uint32_t audioOwner = 0, nextAudioUid = 1;
  struct ScopeAudioOwner {
    uint32_t &slot;
    const uint32_t previous;
    ScopeAudioOwner(uint32_t &target, uint32_t owner)
        : slot(target), previous(target) {
      slot = owner;
    }
    ~ScopeAudioOwner() { slot = previous; }
  };
  inline static Scene *running = nullptr;
  bool hasMod(Mod mod) const { return (mods & unsigned(mod)) != 0; }
  float random(float lo, float hi) {
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return lo + (hi - lo) * float(randomState & 0xffffff) / 16777215.0f;
  }
  void sound(const char *cue, int id = 0, float pitch = 1, float gainDb = 0) {
    Event event{Event::Sound, id, 0, cue, pitch};
    event.gainDb = gainDb;
    event.voiceOwner = audioOwner;
    (stepping ? events : queuedEvents).push_back(event);
  }
  void thought(unsigned category, int id = 0) {
    if (!testMode && category)
      (stepping ? events : queuedEvents)
          .push_back({Event::Thought, id, double(category), ""});
  }
  void addStroke() {
    const int64_t old = game.strokes;
    if (old > par && strokeSaturation > 0)
      strokeSaturation = .5 - .05 * (old - par);
    ++game.strokes;
    tkTextActive = false;
  }
  static unsigned thought_category(const std::string &text) {
    static const char *names[] = {
        "",       "Terry",   "Money",  "Sand",  "Skull",    "Swim",  "Jump",
        "Police", "Idle",    "Moving", "Car",   "Win",      "Death", "Lights",
        "Dog",    "Chuckle", "Portal", "Horse", "Metronome"};
    for (unsigned i = 1; i < sizeof(names) / sizeof(names[0]); ++i)
      if (text == names[i])
        return i;
    return 0;
  }
  bool canAppend(size_t count) {
    if (count <= MaxEntities - std::min(MaxEntities, entities.size()))
      return true;
    fault = "playtest entity memory limit 2048 reached";
    diagnostics.push_back(fault);
    return false;
  }
  void interruptSpawn() {
    if (spawnFrames) {
      spawnFrames = 0;
      introInterrupted = true;
    }
  }
  void addScore(int64_t value, int id, const Entity &source,
                bool popup = true) {
    bonus = wrap_add(bonus, value);
    Event event{Event::Score, id, double(value), ""};
    event.score = value;
    event.position = source.position;
    event.scale = source.scale;
    event.tint = source.color;
    // Baddie.rotate changes only graphic, not the parent ScorePopup transform.
    event.rotation = 0;
    event.hasPosition = true;
    event.popup = popup;
    (stepping ? events : queuedEvents).push_back(event);
  }
  void die(int);
  static void parse_mods(const std::string &, unsigned &,
                         std::vector<std::string> &);
  static WallContact playerSweep(::Vec2 position, ::Vec2 motion, float radius) {
    return running ? running->sweep(position, motion, radius, -1, true)
                   : WallContact{};
  }
  WallContact sweep(::Vec2, ::Vec2, float, int, bool, bool recovery = false);
  void movePlayer();
  bool insideTerrain(::Vec2) const;
  bool overlap(const Entity &n, ::Vec2 p, float radius) const {
    return n.radius > 0 && length(n.position - p) <= n.radius + radius;
  }
  static bool inPolygon(const std::vector<::Vec2> &, ::Vec2, float);
  bool rectOverlap(const Entity &n, ::Vec2 p, float radius) const {
    float dx = std::max(std::abs(p.x - n.position.x) - n.halfSize.x, 0.0f),
          dy = std::max(std::abs(p.y - n.position.y) - n.halfSize.y, 0.0f);
    return dx * dx + dy * dy <= radius * radius;
  }
  void strike(size_t, bool);
  void defeat(size_t, bool);
  bool baddieOutside(const Entity &) const;
  void queueBaddieRetirement(Entity &);
  void finishBaddieRetirements();
  void stepGhost(size_t);
  void stepBody(size_t);
  void stepSpawner(size_t);
  void stepRing(size_t);
  void stepKey(size_t);
  void touchObject(size_t);
  bool hasCamera() const;
  void updateCamera();
  void enableMod(int);
  bool appendAux(Entity);
  void addBuddy();
  void addMissile();
  void addBorts();
  void addBees();
  void stepModifierBody(size_t);
};
} // namespace EditorPlay
#include "editor_play_load.hpp"
#include "editor_play_step.hpp"
