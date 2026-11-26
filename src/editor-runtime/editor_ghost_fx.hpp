#pragma once
// Source ghost.gd / ghost.tscn visual parameters. Random particle placement is
// deterministic here; source GPU particle seeds are implementation-specific.
namespace GhostFx {
struct Trace {
  float x = 0, y = 0, alpha = 0, life = 0;
  unsigned born = 0;
};
struct Particle {
  float x = 0, y = 0, vx = 0, vy = 0, rotation = 0, sx = 1, sy = 1;
  unsigned born = 0;
  bool live = false;
};
struct State {
  std::array<Trace, 5> traces{};
  std::array<Particle, 16> particles{};
  unsigned tick = 0, head = 0, particleHead = 0, seed = 1;
  float x = 0, y = 0, emission = 0;
  bool grazing = false;
};
inline std::array<State *, EditorPlay::Scene::MaxEntities> states{};
inline const EditorPlay::Scene *previousScene = nullptr;
inline unsigned previousTick = 0;
inline void clear() {
  for (auto *&s : states) {
    delete s;
    s = nullptr;
  }
  previousScene = nullptr;
  previousTick = 0;
}
inline float random(State &s) {
  s.seed = s.seed * 1664525u + 1013904223u;
  return float(s.seed >> 8) * (1.f / 16777216);
}
inline float curve(float t) {
  t = std::clamp(t, 0.f, 1.f);
  float t2 = t * t, t3 = t2 * t;
  return std::max(0.f, (2 * t3 - 3 * t2 + 1) * .5f + (t3 - t2) * -2.01542f);
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
  }
  previousTick = scene.tick;
  for (size_t i = 0; i < scene.entities.size() && i < states.size(); ++i) {
    const auto &n = scene.entities[i];
    if (n.objectId != 1 || !n.active) {
      delete states[i];
      states[i] = nullptr;
      continue;
    }
    if (!states[i]) {
      states[i] = new (std::nothrow) State{};
      if (!states[i]) {
        fail("Cannot allocate source ghost visual state");
        continue;
      }
      states[i]->tick = scene.tick;
      states[i]->x = n.position.x;
      states[i]->y = n.position.y;
      states[i]->seed = 0x9e3779b9u ^ unsigned(i + 1);
    }
    auto &s = *states[i];
    unsigned delta = scene.tick - s.tick;
    if (!delta)
      continue;
    unsigned first = delta > 45 ? scene.tick - 45 : s.tick;
    const float vx = n.velocity.x, vy = n.velocity.y,
                speed = std::sqrt(vx * vx + vy * vy),
                weight = std::clamp((speed - 20) * (1.f / 130), 0.f, 1.f);
    for (unsigned tick = first + 1; tick <= scene.tick; ++tick) {
      float ratio = float(tick - s.tick) / float(delta),
            x = s.x + (n.position.x - s.x) * ratio,
            y = s.y + (n.position.y - s.y) * ratio;
      for (auto &p : s.particles)
        if (p.live) {
          if (tick - p.born >= 45)
            p.live = false;
          else {
            p.x += p.vx * (1.f / 60);
            p.y += p.vy * (1.f / 60);
          }
        }
      if ((scene.modifier_mask() & 1u) && weight > 0 && tick % 3 == 0) {
        auto &t = s.traces[s.head++ % s.traces.size()];
        t = {x, y, .75f * weight, 12 * weight, tick};
      }
      if (n.grazing) {
        if (!s.grazing)
          s.emission = 1;
        s.emission += 16.f / 45;
        while (s.emission >= 1) {
          s.emission -= 1;
          float angle = std::atan2(scene.game.position.y - y,
                                   scene.game.position.x - x),
                direction =
                    angle + (random(s) * 2 - 1) * 3.14159265358979323846f,
                velocity = 9.5f + random(s) * 5;
          auto &p = s.particles[s.particleHead++ % s.particles.size()];
          p = {x + std::cos(angle) * 16 * float(n.scale.x),
               y + std::sin(angle) * 16 * float(n.scale.y),
               std::cos(direction) * velocity,
               std::sin(direction) * velocity,
               random(s) * 6.2831853071795864769f,
               float(n.scale.x),
               float(n.scale.y),
               tick,
               true};
        }
      }
      s.grazing = n.grazing;
    }
    s.tick = scene.tick;
    s.x = n.position.x;
    s.y = n.position.y;
  }
}
inline void plain(const View &view, Vec2 position, Vec2 scale, double angle,
                  Color tint) {
  if (ghostPlainHook && ghostPlainHook(view, position, scale, angle, tint))
    return;
  const auto &o = EditorArt::Visuals::Objects[1];
  for (unsigned j = 0; j < o.count; ++j) {
    const auto &s = EditorArt::Visuals::Sprites[o.first + j];
    if (!s.visible || s.image < 0 || s.polygonCount)
      continue;
    const auto &i = EditorArt::Packed::Images[unsigned(s.image)];
    builtinImage(
        unsigned(s.image), {0, 0, float(i.width), float(i.height)},
        [&](Vec2 p) {
          return view.vertex(
              add(position, rotate({(p.x - i.width * .5) * s.sx * scale.x,
                                    (p.y - i.height * .5) * s.sy * scale.y},
                                   angle)));
        },
        rgba(tint), &view);
  }
}
inline void trails(const View &view) {
  if (!view.scene)
    return;
  for (auto *s : states)
    if (s)
      for (const auto &t : s->traces) {
        float age = float(view.scene->tick - t.born);
        if (t.life > 0 && age < t.life)
          plain(view, {t.x, t.y}, {1, 1}, 0,
                {1, 1, 1, t.alpha * (1 - age / t.life)});
      }
}
inline void particles(const View &view, const EditorPlay::Entity &n,
                      size_t index) {
  if (!view.scene || index >= states.size() || !states[index])
    return;
  for (const auto &p : states[index]->particles)
    if (p.live) {
      float age = float(view.scene->tick - p.born),
            scale = curve(age * (1.f / 45)) * (.01f / .064f);
      Vec2 dimensions{p.sx * scale, p.sy * scale};
      if (!ghostHook ||
          !ghostHook(view, {p.x, p.y}, dimensions, p.rotation, n.color))
        plain(view, {p.x, p.y}, dimensions, p.rotation, n.color);
    }
}
inline void indicators(const View &view) {
  if (!view.scene)
    return;
  const auto &scene = *view.scene;
  const float bounds = scene.bounds, inv = 1 / float(view.zoom);
  for (const auto &n : scene.entities)
    if (n.active && n.objectId == 1) {
      float x = n.position.x - scene.camera.x - 128 * inv,
            y = n.position.y - scene.camera.y, ax = std::abs(x),
            ay = std::abs(y);
      bool outsideX = ax > bounds + 20 &&
                      ax<bounds + 120, outsideY = ay> bounds + 20 &&
                      ay < bounds + 120;
      if (!outsideX && !outsideY)
        continue;
      float ix = 0, iy = 0;
      if (outsideX && outsideY) {
        ix = std::copysign(bounds - 16, x);
        iy = std::copysign(bounds - 16, y);
      } else if (outsideX) {
        ix = std::copysign(bounds - 16, x);
        iy = ix * y / x;
      } else {
        iy = std::copysign(bounds - 16, y);
        ix = iy * x / y;
      }
      Vec2 local{ix - x, iy - y},
          origin = add({n.position.x, n.position.y},
                       rotate(mul(local, {n.scale.x, n.scale.y}), n.rotation));
      float ratio = (ix * ix + iy * iy) / std::max(.000001f, x * x + y * y),
            scale = std::max(.25f, -.4f + 1.15f * ratio);
      Vec2 dimensions{n.scale.x * scale, n.scale.y * scale};
      if (!ghostHook ||
          !ghostHook(view, origin, dimensions, n.rotation, n.color))
        plain(view, origin, dimensions, n.rotation, n.color);
      const Vec2 points[] = {{9, -3}, {10, 0}, {9, 3}, {14, 0}};
      Vertex arrow[4];
      float angle =
          std::atan2(float(local.y), float(local.x)) + 3.14159265358979323846f;
      for (unsigned j = 0; j < 4; ++j)
        arrow[j] = view.vertex(add(origin, rotate(mul(rotate(points[j], angle),
                                                      {n.scale.x, n.scale.y}),
                                                  n.rotation)));
      const Vertex upper[]={arrow[0],arrow[1],arrow[3]},lower[]={arrow[1],arrow[2],arrow[3]};
      const auto tint=rgba(multiply(multiply(Color{1,0,0,1},n.color),view.light));
      solidPolygon(upper,3,tint);solidPolygon(lower,3,tint);
    }
}
} // namespace GhostFx
