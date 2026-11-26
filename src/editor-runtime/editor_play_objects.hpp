#pragma once
namespace EditorPlay {
inline void Scene::touchObject(size_t index) {
  auto &n = entities[index];
  bool touch = (n.objectId == 19 || n.objectId == 20 || n.objectId == 23)
                   ? inPolygon(n.polygon, game.position, course.playerRadius)
               : n.objectId == 12 || n.objectId == 3
                   ? rectOverlap(n, game.position, course.playerRadius)
                   : overlap(n, game.position, course.playerRadius);
  if (n.objectId == 19) {
    if (touch && !sandDamped) {
      if (!n.inside)
        thought(3, 19);
      game.velocity = game.velocity * .85f;
      sandDamped = true;
    }
    n.inside = touch;
    return;
  }
  if (n.objectId == 23) {
    if (touch) {
      ScopeAudioOwner playerOwner(audioOwner, 0);
      sound("sfxWaterSplash", 23);
      die(5);
    }
    return;
  }
  if (n.objectId == 20 || n.objectId == 21 || n.objectId == 24)
    return;
  if (!touch) {
    n.inside = false;
    return;
  }
  if (n.objectId == 12) {
    bool entered = !n.inside;
    n.inside = true;
    if (entered && n.linked >= 0 && !n.inactive && portalCooldown == 0) {
      game.position = entities[size_t(n.linked)].position;
      portalCooldown = 3;
      sound("sfxPortal", 12);
      thought(16, 12);
    }
    return;
  }
  if (n.objectId == 22) {
    if (!n.inside) {
      n.inside = true;
      float speed = length(game.velocity);
      if (speed <= 20)
        speed = 20;
      if (n.speed > 20)
        speed = n.speed;
      game.velocity = rotate({speed + 10, 0}, n.rotation);
      game.position = n.position;
      enterJump();
      interruptSpawn();
      sound("jump", 22);
      thought(6, 22);
    }
    return;
  }
  if (n.objectId == 2) {
    if (game.won)
      return;
    wonScore = final_score();
    winTarget = n.position;
    game.won = true;
    game.velocity = {};
    for (auto &ghost : entities)
      if (ghost.objectId == 1)
        ghost.active = false;
    Event goal{Event::Goal, 2, double(wonScore), ""};
    goal.score = wonScore;
    goal.position = n.position;
    goal.hasPosition = true;
    events.push_back(goal);
    {
      ScopeAudioOwner hudOwner(audioOwner, 0);
      sound("goal", 2);
    }
    thought(11, 2);
    return;
  }
  if (n.objectId == 8) {
    n.taken = true;
    n.running = true;
    // Timer rejects a nonpositive wait_time; the original Timer keeps its
    // default one second.
    n.timer =
        std::max(1, int(std::ceil((n.timeGiven > 0 ? n.timeGiven : 1) * 60)));
    n.visualAlpha = .5f;
    for (auto &coin : entities)
      if (coin.objectId == 9 && sameColor(coin.color, n.color)) {
        coin.active = true;
        coin.taken = false;
      }
    ringAudioPitch = .5f;
    n.ringAudioPitch = .5f;
    if (n.ringBell)
      sound("ring_start", 8);
    return;
  }
  if (n.objectId == 9) {
    if (n.linked >= 0) {
      float pitch = 1 + entities[size_t(n.linked)].coinsGrabbed * .05f;
      ++entities[size_t(n.linked)].coinsGrabbed;
      n.active = false;
      n.taken = true;
      sound("smiley_coin", 9, pitch);
      Event popup{Event::Popup, 9,
                  double(entities[size_t(n.linked)].coinsGrabbed), ""};
      popup.score = entities[size_t(n.linked)].coinsGrabbed;
      popup.position = n.position;
      popup.scale = n.scale;
      popup.tint = n.color;
      popup.hasPosition = true;
      events.push_back(popup);
    } else if (!n.inside) {
      Event popup{Event::Popup, 9, 0, ""};
      popup.position = n.position;
      popup.scale = n.scale;
      popup.tint = n.color;
      popup.hasPosition = true;
      popup.text = "I don't have a ring.";
      events.push_back(popup);
      n.inside = true;
    }
    return;
  }
  if ((n.objectId == 3 && (n.collectionMask & 2)) || n.objectId == 4 ||
      n.objectId == 5 || n.objectId == 6 || n.objectId == 7 ||
      n.objectId == 10 || n.objectId == 25) {
    const int id = n.objectId;
    n.taken = true;
    if (id == 10)
      n.held = true;
    else
      n.active = false;
    if (n.doesSpeed)
      game.velocity = unit(game.velocity) * n.setSpeed;
    int64_t score = n.scoreValue;
    if (n.coinMultiply) {
      score =
          wrap_add(score, wrap_multiply(n.coinMultiplyValue, coinMultiplier));
      if (coinMultiplier <= 9)
        ++coinMultiplier;
    }
    addScore(score, id, n);
    if (id != 10 && (!n.coinMultiply || coinMultiplier >= 6))
      thought(n.thoughtCategory, id);
    sound(id == 3    ? "spawner_collect"
          : id == 4  ? "coin_collect"
          : id == 5  ? "speed_collect"
          : id == 6  ? "stop_collect"
          : id == 7  ? "nuke_collect"
          : id == 10 ? "key_collect"
                     : "modifier_collect",
          id);
    if (id == 7) {
      Event effect{Event::Effect, 7, 0, "nuke"};
      effect.position = n.position;
      effect.hasPosition = true;
      events.push_back(effect);
      for (auto &enemy : entities)
        if (enemy.objectId == 17 || enemy.objectId == 18 ||
            enemy.objectId == 26) {
          enemy.active = false;
          enemy.dead = true;
          // Nuke's class-wide audio stop also covers already-queued Baddies.
          enemy.boundsRetirementQueued = false;
        }
    }
    if (id == 25)
      enableMod(n.mod);
  }
}
inline void Scene::enableMod(int value) {
  mods |= 1u << unsigned(value);
  events.push_back({Event::Modifier, 25, float(value), ""});
  if (value == 3)
    addBuddy();
  if (value == 7)
    addMissile();
  if (value == 10)
    addBorts();
  if (value == 11)
    addBees();
}
inline bool Scene::appendAux(Entity n) {
  if (!canAppend(1))
    return false;
  n.audioUid = nextAudioUid++;
  entities.push_back(std::move(n));
  return true;
}
inline void Scene::addBuddy() {
  int buddies = 0;
  for (const auto &n : entities)
    if (n.objectId == 27)
      ++buddies;
  Entity n;
  n.objectId = 27;
  n.scenePath = "res://assetsNEW/scenes/objects/modifier/little_buddy.tscn";
  n.position = game.position + rotate({32, 0}, random(0, 2 * Pi));
  n.scale = {0, 0};
  n.speedMinus = std::min(1000.0f, 200.0f * buddies);
  appendAux(n);
}
inline void Scene::addMissile() {
  if (!canAppend(1))
    return;
  Entity n;
  n.objectId = 28;
  n.scenePath = "res://assetsNEW/scenes/objects/modifier/missile_attacker.tscn";
  n.position = game.position;
  n.timer = 240;
  n.variant = int(nextMissileGroup++);
  appendAux(n);
}
inline void Scene::addBorts() {
  if (!canAppend(25))
    return;
  for (int i = 0; i < 25; ++i) {
    Entity n;
    n.objectId = 29;
    n.scenePath =
        "res://assetsNEW/scenes/game/minigames/bartbash/objects/bort.tscn";
    n.screenSpace = true;
    n.position = {random(320, 704), random(64, 448)};
    n.velocity = rotate({random(100, 700), 0},
                        random(.261f, 1.309f) +
                            float(int(random(0, 3.999f))) * Pi * .5f);
    n.countdown = int(random(3, 10.999f));
    appendAux(n);
  }
}
inline void Scene::addBees() {
  if (!canAppend(20))
    return;
  const int group = int(nextBeeGroup++);
  for (int i = 0; i < 20; ++i) {
    Entity n;
    n.objectId = 30;
    n.variant = group;
    n.scenePath = "res://assetsNEW/scenes/objects/modifier/bee.tscn";
    n.screenSpace = true;
    n.position = {random(0, 768), random(0, 512)};
    n.acceleration = random(20, 50);
    n.speedLimit = random(100, 1000);
    appendAux(n);
  }
  ScopeAudioOwner owner(audioOwner, entities[entities.size() - 20].audioUid);
  sound("bee_buzz", 30);
}
inline void Scene::stepModifierBody(size_t index) {
  auto &n = entities[index];
  if (n.objectId == 27) {
    ++n.timer;
    float grow = std::min(1.0f, float(n.timer) / 60);
    n.scale = {grow, grow};
    ::Vec2 direction = unit(game.position - n.position);
    float weight = (length(game.position - n.position) - 32) / (384 - 32);
    n.velocity = direction * ((1024 - n.speedMinus) * weight) +
                 ::Vec2{-direction.y, direction.x} * (96 * (1 - weight));
    n.position = n.position + n.velocity * (1.0f / 60);
    for (const auto &ghost : entities)
      if (ghost.objectId == 1 && ghost.active &&
          length(ghost.position - n.position) <= ghost.radius + 8 * grow) {
        n.active = false;
        n.dead = true;
        sound("buddy_die", 27, 3);
        break;
      }
  } else if (n.objectId == 28) {
    if (n.timer > 0) {
      if (n.timer <= 180 && n.timer > 60)
        n.position = n.position * .8f + game.position * .2f;
      if (n.timer == 60)
        sound("missile_ding", 28);
      --n.timer;
    } else {
      n.timer = 240;
      n.countdown = 5;
      sound("missile_boom", 28);
    }
    if (n.countdown > 0) {
      --n.countdown;
      if (!dead && !game.won && jumpFrames == 0 &&
          length(n.position - game.position) <= 40 + course.playerRadius)
        die(3);
    }
  } else if (n.objectId == 29) {
    // HUD BortCage has four source rectangles; expand by the body's scaled half
    // extents.
    constexpr float hx = 26.667f * .6f * .5f, hy = 60 * .533f * .5f;
    WallContact contact;
    ::Vec2 motion = n.velocity * (1.0f / 60);
    for (auto wall : {SolidBlock{{240, 256}, {16 + hx, 288 + hy}},
                      SolidBlock{{784, 256}, {16 + hx, 288 + hy}},
                      SolidBlock{{512, -16}, {256 + hx, 16 + hy}},
                      SolidBlock{{512, 528}, {256 + hx, 16 + hy}}})
      TileField::sweepBlock(wall, n.position, motion, 0, contact);
    n.position = n.position + motion * std::min(1.0f, contact.time);
    if (contact.time <= 1)
      n.velocity =
          n.velocity - contact.normal * (2 * dot(n.velocity, contact.normal));
    if (n.timer > n.countdown) {
      n.timer = 0;
      n.frame = !n.frame;
    } else
      ++n.timer;
  } else if (n.objectId == 30) {
    n.velocity = n.velocity + unit(pointerScreen - n.position) * n.acceleration;
    float speed = length(n.velocity);
    if (speed > n.speedLimit)
      n.velocity = n.velocity * (n.speedLimit / speed);
    n.position = n.position + n.velocity * (1.0f / 60);
  }
}
} // namespace EditorPlay
