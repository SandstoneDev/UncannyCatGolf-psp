#pragma once
// Source player.gd/PIdle/PParry/PMoving and water_death.gd visual children.
namespace PlayerFx {
struct Trace {
  Trace *next = nullptr;
  Vec2 position{}, scale{1, 1};
  float alpha = 0, life = 0;
  unsigned born = 0;
};
inline Trace *traces = nullptr;
inline const EditorPlay::Scene *previousScene = nullptr;
inline unsigned previousTick = 0;
inline double cooldown = 0;
inline bool jumpySeen = false;
inline Vec2 previousPosition{};
inline void clear() {
  while (traces) {
    auto *next = traces->next;
    delete traces;
    traces = next;
  }
  previousScene = nullptr;
  previousTick = 0;
  cooldown = 0;
  jumpySeen = false;
}
inline const EditorPlay::Entity *player(const EditorPlay::Scene &scene) {
  for (const auto &n : scene.entities)
    if (n.objectId == 0)
      return &n;
  return nullptr;
}
inline float cubic(float t) {
  return t < .5f ? 4 * t * t * t : 1 - 4 * (1 - t) * (1 - t) * (1 - t);
}
inline Color bodyTint(const EditorPlay::Scene &scene, Color tint) {
  auto state = scene.visual_state();
  float intensity = 1;
  if (state.idlePulseTicks < 12) {
    float age = float(state.idlePulseTicks);
    intensity = age < 6 ? 1 + .25f * cubic(age / 6)
                        : 1.25f - .25f * cubic((age - 6) / 6);
    tint.r *= intensity;
    tint.g *= intensity;
    tint.b *= intensity;
  } else if (state.phase == EditorPlay::VisualState::Phase::Parry)
    tint.b *= 5;
  return tint;
}
inline void update(const View &view) {
  if (!view.scene) {
    if (previousScene)
      clear();
    return;
  }
  const auto &scene = *view.scene;
  if (previousScene != &scene || scene.tick < previousTick) {
    clear();
    previousScene = &scene;
    previousTick = scene.tick;
    auto v = scene.visual_state();
    previousPosition = {v.motionPosition.x, v.motionPosition.y};
  }
  Trace **at = &traces;
  while (*at) {
    auto *t = *at;
    if (float(scene.tick - t->born) >= t->life) {
      *at = t->next;
      delete t;
    } else
      at = &t->next;
  }
  const auto *entity = player(scene);
  const auto state = scene.visual_state();
  if ((scene.modifier_mask() & (1u << 5)) &&
      (state.phase == EditorPlay::VisualState::Phase::Moving ||
       state.phase == EditorPlay::VisualState::Phase::Telekinetic ||
       state.phase == EditorPlay::VisualState::Phase::Jump ||
       state.jumpyFraction != 1))
    jumpySeen = true;
  if (!entity) {
    previousTick = scene.tick;
    return;
  }
  unsigned delta = scene.tick - previousTick;
  float speed = std::sqrt(state.motionVelocity.x * state.motionVelocity.x +
                          state.motionVelocity.y * state.motionVelocity.y),
        weight = (speed - 500) * (1.f / 400);
  for (unsigned step = 1; step <= delta; ++step) {
    cooldown -= 1. / 60;
    if (speed < .00001f)
      cooldown = 0;
    if (cooldown <= 0) {
      cooldown += .05;
      if (weight > 0 && !scene.dead && !scene.game.won) {
        auto *t = new (std::nothrow) Trace;
        if (!t) {
          fail("Cannot allocate original player afterimage");
          break;
        }
        float f = float(step) / float(delta);
        t->position = {previousPosition.x +
                           (state.motionPosition.x - previousPosition.x) * f,
                       previousPosition.y +
                           (state.motionPosition.y - previousPosition.y) * f};
        t->scale = {entity->scale.x * scene.playerVisualScale,
                    entity->scale.y * scene.playerVisualScale};
        t->alpha = .75f * weight;
        t->life = 9 * weight;
        t->born = previousTick + step;
        t->next = traces;
        traces = t;
      }
    }
  }
  previousTick = scene.tick;
  previousPosition = {state.motionPosition.x, state.motionPosition.y};
}
inline void
drawSprite(const View &view, unsigned objectId, unsigned child, Vec2 position,
           Vec2 rootScale, double angle, Color tint, double seconds,
           double overrideX = std::numeric_limits<double>::quiet_NaN(),
           double localParentScale = 1) {
  const auto &object = EditorArt::Visuals::Objects[objectId];
  if (child >= object.count)
    return;
  const auto &s = EditorArt::Visuals::Sprites[object.first + child];
  int imageId = s.image;
  if (s.animationCount) {
    const auto &a = EditorArt::Visuals::Animations
        [s.animationFirst +
         EditorArt::Visuals::SpriteAnimationOffsets[object.first + child]];
    double duration = 0;
    for (unsigned i = 0; i < a.count; ++i)
      duration += EditorArt::Visuals::Frames[a.first + i].duration;
    if (a.count && duration > 0) {
      double time = std::max(0., seconds) * a.speed;
      time = a.loop ? std::fmod(time, duration)
                    : std::min(time, duration - .00001);
      unsigned i = 0;
      while (i + 1 < a.count &&
             time >= EditorArt::Visuals::Frames[a.first + i].duration) {
        time -= EditorArt::Visuals::Frames[a.first + i].duration;
        ++i;
      }
      imageId = EditorArt::Visuals::Frames[a.first + i].image;
    }
  }
  if (imageId < 0)
    return;
  const auto &im = EditorArt::Packed::Images[unsigned(imageId)];
  const double sx = std::isnan(overrideX) ? s.sx : overrideX;
  Color mod = multiply(
      tint, color(EditorArt::Visuals::SpriteTints[object.first + child]));
  builtinImage(
      unsigned(imageId), {0, 0, float(im.width), float(im.height)},
      [&](Vec2 p) {
        p = {(p.x - (s.centered ? im.width * .5 : 0) + s.ox) * sx *
                 localParentScale,
             (p.y - (s.centered ? im.height * .5 : 0) + s.oy) * s.sy *
                 localParentScale};
        p = add(rotate(p, s.rotation), {s.x, s.y});
        return view.vertex(add(position, rotate(mul(p, rootScale), angle)));
      },
      rgba(mod), &view, &mod);
}
inline void trails(const View &view) {
  if (!view.scene)
    return;
  for (auto *t = traces; t; t = t->next) {
    float age = float(view.scene->tick - t->born);
    if (age >= t->life || t->life <= 0)
      continue;
    Color tint{1, 1, 1, t->alpha * (1 - age / t->life)};
    if (playerHook && playerHook(view, t->position, t->scale, 0, tint))
      continue;
    drawSprite(view, 0, 0, t->position, t->scale, 0, tint, 0);
  }
}
inline bool touches(const EditorPlay::Entity &entity, Vec2 p, float radius) {
  bool inside = false;
  for (size_t i = 0; i < entity.polygon.size(); ++i) {
    const auto &a = entity.polygon[i];
    const auto &b = entity.polygon[(i + 1) % entity.polygon.size()];
    float x = b.x - a.x, y = b.y - a.y, ox = float(p.x) - a.x,
          oy = float(p.y) - a.y, projection = ox * x + oy * y,
          square = x * x + y * y, cross = x * oy - y * ox;
    if (ox * ox + oy * oy <= radius * radius ||
        (projection > 0 && projection < square &&
         cross * cross <= radius * radius * square))
      return true;
    if ((a.y > p.y) != (b.y > p.y) && ((cross > 0) == (y > 0)))
      inside = !inside;
  }
  return inside;
}
inline void children(const View &view) {
  if (!view.scene)
    return;
  const auto &scene = *view.scene;
  const auto *entity = player(scene);
  if (!entity)
    return;
  const auto state = scene.visual_state();
  Vec2 position{scene.game.position.x, scene.game.position.y},
      scale{entity->scale.x, entity->scale.y};
  // This child is independent of Sprite2D's Spawn/Jump/Win scale and Dizzy
  // rotation.
  if ((scene.modifier_mask() & (1u << 5)) && jumpySeen)
    drawSprite(view, 0, 5, position, scale, entity->rotation, entity->color,
               view.seconds, state.jumpyFraction, scene.bounds / 256.);
  if (state.phase == EditorPlay::VisualState::Phase::Moving &&
      (state.additionalVelocity.x != 0 || state.additionalVelocity.y != 0)) {
    Vec2 visualScale{scale.x * scene.playerVisualScale,
                     scale.y * scene.playerVisualScale};
    drawSprite(view, 0, 1, position, visualScale,
               entity->rotation + scene.playerRotation, entity->color,
               view.seconds);
  }
  if (scene.dead && scene.deathReason == 3)
    drawSprite(view, 0, 3, {state.deathPosition.x, state.deathPosition.y},
               scale, entity->rotation, entity->color, state.deathTicks / 60.);
  if (scene.dead && scene.deathReason == 5) {
    Vec2 death{state.deathPosition.x, state.deathPosition.y};
    for (const auto &water : scene.entities)
      if (water.objectId == 23 &&
          touches(water, death, scene.course.playerRadius)) {
        drawSprite(view, 23, 0, death, {water.scale.x, water.scale.y},
                   water.rotation, water.color, state.deathTicks / 60.);
        break;
      }
  }
}
} // namespace PlayerFx
