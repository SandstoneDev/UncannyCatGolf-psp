#pragma once
namespace EditorPlay {
inline void Scene::restart() {
  entities = initialEntities;
  // Mixer voices must be cleared before restart/load allows session UID reuse.
  audioOwner = 0;
  nextAudioUid = 1;
  for (auto &entity : entities)
    entity.audioUid = entity.objectId == 0 ? 0 : nextAudioUid++;
  entities.reserve(MaxEntities);
  decorations = initialDecorations;
  game.restart();idleSpeech059.clearSpeech();
  game.goalEnabled = false;
  dead = false;
  deathReason = -1;
  deathTint = {1, 1, 1, 1};
  fault.clear();
  tick = 0;
  elapsedTicks = 0;
  spawnFrames = IntroTicks;
  spawnSpriteScale = .003f;
  playerVisualScale = .003f / .064f;
  playerRotation = 0;
  lastPhase = VisualState::Phase::Spawn;
  phaseEnteredTick = 0;
  idlePulseTick = UINT32_MAX;
  jumpyBarFraction = 1;
  frameMotionPosition = deathPosition = game.position;
  frameMotionVelocity = {};
  ringAudioPitch = .5f;
  tkAudioPitch = .75f;
  bonus = 0;
  wonScore = 0;
  coinMultiplier = 0;
  dblockMultiplier = 0;
  switchState = 0;
  dogTiles = true;
  camera = worldOffset;
  events.clear();
  queuedEvents.clear();
  stepping = false;
  mods = initialMods;
  telekineticState = hasMod(Telekinesis);
  portalCooldown = jumpFrames = jumpBuffer = parryActive = parryFreeze = 0;
  wallStuck = wallWindow = 0;
  tkStrokes = distanceTraveled = 0;
  strokeSaturation = .5;
  tkTextActive = false;
  movingDistance = tkDistance = 0;
  lastMovingPosition = lastTkPosition = {};
  wallPitch = 1;
  wasMoving = shotBuffered = false;
  tkBurstRequested = false;
  parryRequested = shotThisTick = false;
  introInterrupted = false;
  nextBeeGroup = nextMissileGroup = 0;
  pointerDelta = {};
  randomState = 0x553053u;
  field.beginDynamic();
  for (size_t i = 0; i < entities.size(); ++i) {
    auto &n = entities[i];
    n.linked = -1;
    if (n.objectId == 12 && !n.inactive)
      for (size_t j = 0; j < entities.size(); ++j)
        if (i != j && entities[j].objectId == 12 &&
            sameColor(n.color, entities[j].color))
          n.linked = int(j);
    if (n.objectId == 9)
      for (size_t j = 0; j < entities.size(); ++j)
        if (entities[j].objectId == 8 && sameColor(n.color, entities[j].color))
          n.linked = int(j);
  }
  if (hasMod(Buddy))
    addBuddy();
  if (hasMod(Missiles))
    addMissile();
  if (hasMod(Borts))
    addBorts();
  if (hasMod(Bees))
    addBees();
  events.clear();
  sound("sfxJoin");
  updateCamera();
}
inline bool Scene::begin_drag() {
  if (dead || game.won || jumpFrames || parryFreeze || telekineticState)
    return false;
  if (spawnFrames || (game.moving() && !hasMod(Burst)))
    return bufferShots;
  sound("sfxGrab");
  return true;
}
inline bool Scene::shoot(::Vec2 drag) {
  if (dead || game.won || jumpFrames || parryFreeze)
    return false;
  if (spawnFrames) {
    if (bufferShots) {
      bufferedShot = drag;
      bufferedAt = tick;
      shotBuffered = true;
    }
    return false;
  }
  float power = length(drag);
  bool burst = hasMod(Burst) && game.moving();
  if (telekineticState) {
    if (hasMod(Burst)) {
      game.velocity = game.velocity * -1;
      sound("sfxBurst");
      return true;
    }
    return false;
  }
  if (game.moving() && !burst) {
    if (bufferShots) {
      bufferedShot = drag;
      bufferedAt = tick;
      shotBuffered = true;
    }
    return false;
  }
  if (burst && length(game.velocity) < 100) {
    game.velocity = {};
    sound("sfxGrabCancel");
    return false;
  }
  if (!burst && power < 20) {
    sound("sfxGrabCancel");
    return false;
  }
  if (!hasMod(God) && power > 175)
    drag = drag * (175 / power);
  game.velocity = drag * 5;
  addStroke();
  coinMultiplier = 0;
  wasMoving = true;
  shotThisTick = true;
  float pitch = .9f + .2f * ((length(game.velocity) - 100) / (875 - 100));
  sound(burst           ? "sfxBurst"
        : hasMod(Dizzy) ? "sfxLaunchedDizzy"
                        : "sfxLaunched",
        0, burst ? 1 : pitch);
  thought(9);
  return true;
}
inline void Scene::action() {
  if (hasMod(Parry))
    parryRequested = true;
}
inline bool Scene::teleport(::Vec2 position) {
  if (!hasMod(God) || dead || game.won)
    return false;
  game.position = position;
  return true;
}
inline void Scene::click_pointer(::Vec2 screen) {
  pointerScreen = screen;
  if (hasMod(Burst))
    tkBurstRequested = true;
  for (auto &n : entities)
    if (n.objectId == 29 && n.active &&
        std::abs(n.position.x - screen.x) <= 20 &&
        std::abs(n.position.y - screen.y) <= 32) {
      n.active = false;
      n.taken = true;
      ScopeAudioOwner owner(audioOwner, n.audioUid);
      addScore(100, 29, n, false);
      sound("bort_ow", 29);
    }
}
inline void Scene::movePlayer() {
  passiveCircles.clear();
  passiveBoxes.clear();
  for (const auto &n : entities) {
    if (!n.active || n.taken)
      continue;
    if (n.objectId == 15 || n.objectId == 17 || n.objectId == 18)
      passiveCircles.push_back({n.position, n.radius});
    else if (n.objectId == 26)
      passiveBoxes.push_back({n.position, n.halfSize});
  }
  const ::Vec2 beforeRecovery = game.position;
  const ::Vec2 sourceVelocity = game.velocity;
  game.velocity = mul(game.velocity, playerScale);
  lastObstacle = -1;
  lastNormal = {};
  Scene *previous = running;
  running = this;
  unsigned bounces = game.bounces;
  game.step(nullptr, 0, {}, nullptr, 0, nullptr, 0, 0, passiveBoxes.data(),
            unsigned(passiveBoxes.size()), nullptr, 0, nullptr, nullptr,
            passiveCircles.data(), unsigned(passiveCircles.size()), {}, true,
            &Scene::playerSweep, false);
  running = previous;
  game.velocity = sourceVelocity;
  if (game.pushed && dot(sourceVelocity, sourceVelocity) == 0) {
    ::Vec2 correction = game.position - beforeRecovery;
    const auto wall =
        sweep(beforeRecovery, correction, course.playerRadius, -1, false, true);
    game.position = beforeRecovery + correction * std::min(1.0f, wall.time);
    sound("sfxPush");
  }
  if (game.bounces != bounces) {
    game.velocity =
        sourceVelocity - lastNormal * (2 * dot(sourceVelocity, lastNormal));
    bool moving = !telekineticState ? wasMoving : length(sourceVelocity) > 32;
    if (!moving) {
      sound("sfxPush");
    } else if (lastObstacle >= 0) {
      sound("sfxWallBump", 0, wallPitch);
      int id = entities[size_t(lastObstacle)].objectId;
      strike(size_t(lastObstacle), false);
      if (id == 17 || id == 18 || id == 26)
        game.position = game.position + game.velocity * (1.0f / 60);
    } else {
      bool inside = insideTerrain(game.position);
      if (inside && ++wallStuck >= 9)
        die(3);
      if (hasMod(Sticky)) {
        sound("sfxStick", 0,
              std::max(.8f, std::min(1.0f, .8f + .2f * length(sourceVelocity) /
                                                     500)));
        game.velocity = {};
        if (inside)
          die(3);
      } else {
        sound("sfxWallBump", 0, wallPitch);
        wallPitch += .05f;
      }
    }
  }
}
inline void Scene::step60() {
  idleSpeech059.tick(16667);
  events.clear();
  events.swap(queuedEvents);
  if (!fault.empty())
    return;
  if (dead || game.won) {
    stepping = true;
    ++tick;
    frameMotionPosition = game.position;
    frameMotionVelocity = game.velocity;
    if (game.won) {
      game.position = game.position * .95f + winTarget * .05f;
      playerVisualScale =
          std::max(.001f / .064f, playerVisualScale - .001f / .064f);
    }
    stepWorld(false, false);
    updateVisualPhase();
    stepping = false;
    return;
  }
  stepping = true;
  sandDamped = false;
  ++tick;
  frameMotionPosition = game.position;
  bool spawning = spawnFrames > 0;
  bool dispatchMovement = false;
  if (spawning) {
    --spawnFrames;
    spawnSpriteScale += .001f;
    playerVisualScale = std::min(1.0f, spawnSpriteScale / .064f);
    if (!spawnFrames) {
      dispatchMovement = true;
      telekineticState = hasMod(Telekinesis);
      if (!telekineticState) {
        sound("sfxPickUp");
        idlePulseTick = tick;
        if (shotBuffered && tick - bufferedAt < 12)
          shoot(bufferedShot);
      }
      shotBuffered = false;
    }
  } else if (!introInterrupted)
    ++elapsedTicks;
  if (portalCooldown > 0)
    --portalCooldown;
  if (jumpBuffer > 0)
    --jumpBuffer;
  if (parryActive > 0)
    --parryActive;
  if (parryRequested && parryActive <= 0)
    parryActive = 33;
  parryRequested = false;
  if (++wallWindow > 10) {
    wallWindow = 0;
    wallStuck = 0;
    wallPitch = 1;
  }
  bool jumping = jumpFrames > 0;
  bool jumpStarted = false;
  if (spawning) {
    // PSpawn still participates in collision/area signals; its state applies no
    // motion input.
  } else if (parryFreeze > 0) {
    --parryFreeze;
    if (parryFreeze == 0) {
      game.velocity = parryVelocity;
      telekineticState = hasMod(Telekinesis);
      wasMoving = !telekineticState;
      dispatchMovement = true;
    }
  } else if (jumping) {
    if (--jumpFrames == 0) {
      jumpBuffer = 5;
      telekineticState = hasMod(Telekinesis);
      wasMoving = !telekineticState;
      playerVisualScale = 1;
      dispatchMovement = true;
    } else {
      float scale = playerVisualScale * .064f;
      float weight = (27 - jumpFrames) > 13 ? -.13f : .13f;
      playerVisualScale = (scale + (.15f - scale) * weight) / .064f;
      game.position =
          game.position + mul(game.velocity, playerScale) * (1.0f / 60);
    }
  } else {
    dispatchMovement = true;
  }
  // Player.transition_to runs enter() and update() synchronously, including on
  // the final Spawn/Jump/Parry frame. Dormant PTK input is consumed only by
  // PTK.update.
  if (dispatchMovement && !shotThisTick) {
    if (telekineticState) {
      game.velocity = game.velocity * .99f + pointerDelta * .25f;
      if (tkBurstRequested) {
        game.velocity = game.velocity * -1;
        sound("sfxBurst");
      }
      float nextPitch = .75f + 2.25f * (length(pointerDelta) / 128);
      tkAudioPitch = nextPitch > tkAudioPitch
                         ? nextPitch
                         : tkAudioPitch + (.75f - tkAudioPitch) * .05f;
      tkStrokes += double(length(pointerDelta)) / 3400.0;
      while (tkStrokes >= 1) {
        addStroke();
        tkStrokes -= 1;
      }
      tkTextActive = true;
      pointerDelta = {};
    } else {
      float speed = length(game.velocity);
      if (speed <= 20) {
        game.velocity = {};
        if (wasMoving)
          sound("sfxPickUp");
        if (wasMoving)
          idlePulseTick = tick;
        wasMoving = false;
        if (shotBuffered && tick - bufferedAt < 12)
          shoot(bufferedShot);
        shotBuffered = false;
      } else {
        wasMoving = true;
        game.velocity =
            game.velocity * (1 - 175 * std::pow(.04f / speed, 1.06f));
      }
    }
    if (!shotThisTick && (telekineticState || wasMoving)) {
      auto &distance = telekineticState ? tkDistance : movingDistance;
      auto &last = telekineticState ? lastTkPosition : lastMovingPosition;
      distance += length(game.position - last);
      last = game.position;
      if (hasMod(Jumpy))
        jumpyBarFraction = 1 - distance / 1024;
      if (hasMod(Jumpy) && distance >= 1024) {
        distance -= 1024;
        enterJump();
        jumpStarted = true;
        sound("jump");
        thought(6);
      }
      distanceTraveled = distance;
    }
    if (!shotThisTick && (telekineticState || wasMoving) && hasMod(Dizzy)) {
      game.velocity = rotate(game.velocity, 1.309f / 60);
      playerRotation += 1.309f / 60;
    }
  }
  AvatarIdle059::Context idle059;idle059.active=playerPhase()==VisualState::Phase::Idle;
  AvatarIdle059::step(AvatarIdle059::cosmetic,idle059,idleSpeech059);
  frameMotionVelocity = game.velocity;
  if (jumpStarted)
    game.position =
        game.position + mul(game.velocity, playerScale) * (1.0f / 60);
  if (jumpFrames == 0)
    movePlayer();
  tkBurstRequested = false;
  shotThisTick = false;
  stepWorld(jumping, true);
  updateVisualPhase();
  stepping = false;
}
inline void Scene::stepWorld(bool jumping, bool canTouch) {
  // Player is pre-existing in EditorLevel; imported objects run after its
  // physics.
  for (size_t i = 0; i < entities.size(); ++i) {
    if (!fault.empty())
      break;
    auto &n = entities[i];
    ScopeAudioOwner owner(audioOwner, n.audioUid);
    if (n.objectId == 0) {
      if (n.active)
        n.position = game.position;
      continue;
    }
    if (n.objectId == 1 && n.active)
      stepGhost(i);
    else if ((n.objectId == 15 || n.objectId == 17 || n.objectId == 18 ||
              n.objectId == 26) &&
             n.active)
      stepBody(i);
    else if (n.objectId == 3 && n.active)
      stepSpawner(i);
    else if (n.objectId == 8 && n.active)
      stepRing(i);
    else if (n.objectId == 10 && n.active && n.held)
      stepKey(i);
    else if (n.objectId >= 27 && n.active)
      stepModifierBody(i);
    if (canTouch && dead)
      break;
    if (n.objectId == 12 && n.active && n.linked >= 0)
      n.rotation += .1f;
    if (n.objectId == 14 && !n.keepFace)
      n.frame = unsigned(switchState);
    if (n.objectId == 13) {
      bool matching = false;
      for (const auto &key : entities)
        matching |=
            key.objectId == 10 && key.held && sameColor(key.color, n.lockColor);
      n.lockAlpha += ((matching ? 0.0f : 1.0f) - n.lockAlpha) * .1f;
    }
    if (jumping && (n.objectId == 22 || n.objectId == 19 || n.objectId == 12))
      n.inside = false;
    if (canTouch && n.active && !n.taken && jumpFrames == 0)
      touchObject(i);
  }
  for (auto &d : decorations) {
    d.phase += d.animSpeed;
    if (d.phase > 100) {
      d.phase = 0;
      if (++d.animatedFrame >= d.hframes * d.vframes)
        d.animatedFrame = 0;
    }
    d.frame = d.animatedFrame;
  }
  finishBaddieRetirements();
  updateCamera();
  if (canTouch && !dead && !game.won && !introInterrupted && !hasCamera() &&
      std::max(std::abs(game.position.x), std::abs(game.position.y)) >
          bounds + 24)
    die(1); // EditorLevel.tscn always_O_B=true; followers disable this check
            // entirely.
}
inline void Scene::die(int reason) {
  ScopeAudioOwner owner(audioOwner, 0);
  if (dead || game.won)
    return;
  dead = true;
  deathReason = reason;
  deathPosition = game.position;
  thought(12);
  game.velocity = {};
  for (auto &n : entities)
    if (n.objectId == 0)
      n.active = false;
  events.push_back({Event::Death, 0, float(reason), ""});
  if (reason != 2)
    sound(reason == 5   ? "sfxWaterDeath"
          : reason == 3 ? "sfxWallDeath"
          : reason == 1 ? "out_of_bounds"
          : reason == 4 ? "horse_caught"
                        : "ghost_caught");
}
inline WallContact Scene::sweep(::Vec2 position, ::Vec2 motion, float radius,
                                int excluded, bool player, bool recovery) {
  WallContact result;
  int chosen = -1;
  const unsigned mask =
      player                                                       ? 9u
      : excluded >= 0 && entities[size_t(excluded)].objectId == 26 ? 0u
      : excluded >= 0 && entities[size_t(excluded)].objectId == 18 ? 1u
                                                                   : 9u;
  if (!mask) {
    lastBodyObstacle = -1;
    return result;
  }
  // A zero sweep cannot update a contact; keep the common output publication.
  if (motion.x != 0.0f || motion.y != 0.0f) {
  const auto query = detail::Bounds::swept(position, motion, radius);
  const auto sweepTile = [&](const Tile &t) {
    if (!query.intersects(t.bounds) || !layer_active(t.layer))
      return;
    const auto &shape = EditorArt::TileShapes::Shapes[size_t(t.shape)];
    ::Vec2 vertices[4];
    for (unsigned j = 0; j < shape.count; ++j) {
      const auto q = EditorArt::TileShapes::Points[shape.first + j];
      vertices[j] = {q.x + 16 * t.x + 8 + worldOffset.x, q.y + 16 * t.y + 8};
    }
    TileField::sweepConvex(vertices, shape.count, position, motion, radius,
                           result);
  };
  if (tileIndex.collect(query, staticCandidates)) {
    for (unsigned id : staticCandidates)
      sweepTile(tiles[id]);
  } else
    for (const auto &t : tiles)
      sweepTile(t);
  const auto sweepCollider = [&](const Convex &c) {
    if (query.intersects(c.bounds))
      TileField::sweepConvex(c.points, c.count, position, motion, radius,
                             result);
  };
  if (colliderIndex.collect(query, staticCandidates)) {
    for (unsigned id : staticCandidates)
      sweepCollider(colliders[id]);
  } else
    for (const auto &c : colliders)
      sweepCollider(c);
  for (size_t i = 0; i < entities.size(); ++i) {
    const auto &n = entities[i];
    if (int(i) == excluded || !n.active || n.held)
      continue;
    if (recovery && (n.objectId == 15 || n.objectId == 17 || n.objectId == 18 ||
                     n.objectId == 26))
      continue;
    unsigned layer = n.objectId == 15                         ? 65u
                     : (n.objectId == 17 || n.objectId == 26) ? 8u
                     : (n.objectId == 11 || n.objectId == 13 ||
                        n.objectId == 14 || n.objectId == 18)
                         ? 1u
                         : 0u;
    if (!(mask & layer))
      continue;
    const bool rectangular = n.objectId == 11 || n.objectId == 13 ||
                             n.objectId == 14 || n.objectId == 26;
    if (!query.intersects(detail::Bounds::centered(
            n.position, rectangular ? n.halfSize : ::Vec2{n.radius, n.radius})))
      continue;
    float old = result.time;
    if (n.objectId == 11 || n.objectId == 13 || n.objectId == 14 ||
        n.objectId == 26)
      TileField::sweepBlock({n.position, n.halfSize}, position, motion, radius,
                            result);
    else if (n.objectId == 17 || n.objectId == 18 || n.objectId == 15)
      sweepCircle({n.position, n.radius}, position, motion, radius, result);
    if (result.time < old)
      chosen = int(i);
  }
  }
  if (player) {
    lastObstacle = chosen;
    lastNormal = result.normal;
  } else
    lastBodyObstacle = chosen;
  return result;
}
inline bool Scene::inPolygon(const std::vector<::Vec2> &points, ::Vec2 p,
                             float radius) {
  bool inside = false;
  for (size_t i = 0; i < points.size(); ++i) {
    ::Vec2 a = points[i], b = points[(i + 1) % points.size()], edge = b - a,
           offset = p - a;
    float projection = dot(offset, edge), square = dot(edge, edge),
          cross = edge.x * offset.y - edge.y * offset.x;
    if (dot(offset, offset) <= radius * radius ||
        (projection > 0 && projection < square &&
         cross * cross <= radius * radius * square))
      return true;
    if ((a.y > p.y) != (b.y > p.y) && ((cross > 0) == (edge.y > 0)))
      inside = !inside;
  }
  return inside;
}
inline bool Scene::insideTerrain(::Vec2 position) const {
  for (const auto &t : tiles) {
    if (!layer_active(t.layer))
      continue;
    const auto &shape = EditorArt::TileShapes::Shapes[size_t(t.shape)];
    std::vector<::Vec2> polygon;
    for (unsigned j = 0; j < shape.count; ++j) {
      const auto q = EditorArt::TileShapes::Points[shape.first + j];
      polygon.push_back(
          {q.x + 16 * t.x + 8 + worldOffset.x, q.y + 16 * t.y + 8});
    }
    if (inPolygon(polygon, position, 0))
      return true;
  }
  for (const auto &c : colliders) {
    std::vector<::Vec2> polygon(c.points, c.points + c.count);
    if (inPolygon(polygon, position, 0))
      return true;
  }
  return false;
}
inline void Scene::strike(size_t index, bool dog) {
  auto &n = entities[index];
  ScopeAudioOwner owner(audioOwner, n.audioUid);
  if (!n.active)
    return;
  if (n.objectId == 11) {
    n.active = false;
    n.taken = true;
    addScore(n.dblockScore ? wrap_multiply(n.scoreValue, ++dblockMultiplier)
                           : n.scoreValue,
             11, n);
    sound("dblock_hit", 11);
  } else if (n.objectId == 13 && !dog) {
    for (auto &key : entities)
      if (key.objectId == 10 && key.held && sameColor(key.color, n.lockColor)) {
        key.held = false;
        key.active = false;
        key.taken = true;
        n.active = false;
        n.taken = true;
        addScore(n.dblockScore ? wrap_multiply(n.scoreValue, ++dblockMultiplier)
                               : n.scoreValue,
                 13, n);
        sound("keydoor_open", 13);
        break;
      }
  } else if (n.objectId == 14) {
    if (!switchLayers.empty())
      switchState = (switchState + 1) % int(switchLayers.size());
    sound("switch_hit", 14,
          switchLayers.empty() ? random(.7f, .8f) : random(.8f, 1.2f));
  } else if (n.objectId == 15 && !dog && n.bounceOffPlayer) {
    float angle = std::round(std::atan2(n.position.y - game.position.y,
                                        n.position.x - game.position.x) /
                             (Pi / 4)) *
                  (Pi / 4);
    n.velocity = rotate({length(n.velocity), 0}, angle);
  } else if (n.objectId == 17 || n.objectId == 18 || n.objectId == 26)
    defeat(index, true);
}
inline bool Scene::baddieOutside(const Entity &n) const {
  return (n.objectId == 17 || n.objectId == 18 || n.objectId == 26) &&
         (n.ignoreY ? std::abs(n.position.x)
                    : std::max(std::abs(n.position.x),
                               std::abs(n.position.y))) > bounds + 128;
}
inline void Scene::queueBaddieRetirement(Entity &n) {
  if (n.boundsRetirementQueued)
    return;
  n.boundsRetirementQueued = true;
  // Source emits on_death before queue_free/motion, releasing the spawner slot
  // now.
  if (n.spawnOwner >= 0 && size_t(n.spawnOwner) < entities.size())
    entities[size_t(n.spawnOwner)].spawnedCount =
        std::max(entities[size_t(n.spawnOwner)].spawnedCount - 1, 0);
}
inline void Scene::finishBaddieRetirements() {
  for (auto &n : entities)
    if (n.boundsRetirementQueued) {
      Event stop{Event::StopAudioOwner, n.objectId, 0, ""};
      stop.voiceOwner = n.audioUid;
      events.push_back(stop);
      n.boundsRetirementQueued = false;
      n.active = false;
      n.dead = true;
      n.velocity = {};
    }
}
inline void Scene::defeat(size_t index, bool reward) {
  auto &n = entities[index];
  if (!n.active)
    return;
  if (baddieOutside(n))
    queueBaddieRetirement(n);
  n.active = false;
  n.dead = true;
  n.velocity = {};
  if (reward) {
    thought(n.thoughtCategory, n.objectId);
    addScore(n.scoreValue, n.objectId, n);
    sound(n.objectId == 18   ? "chuckle_death"
          : n.objectId == 26 ? "car_death"
                             : "skull_death",
          n.objectId, random(n.deathPitch.x, n.deathPitch.y));
    if (n.objectId == 26)
      sound(random(0, 1) < .5f ? "car_horn_a" : "car_horn_b", 26);
  }
  if (!n.boundsRetirementQueued && n.spawnOwner >= 0 &&
      size_t(n.spawnOwner) < entities.size())
    entities[size_t(n.spawnOwner)].spawnedCount =
        std::max(entities[size_t(n.spawnOwner)].spawnedCount - 1, 0);
}
inline void Scene::stepGhost(size_t index) {
  auto &n = entities[index];
  if (n.parryStun > 0) {
    if (--n.parryStun == 0)
      n.velocity = n.storedVelocity;
  } else {
    n.velocity = n.velocity + unit(game.position - n.position) * n.acceleration;
    float speed = length(n.velocity);
    if (speed > n.speedLimit && speed > 0)
      n.velocity = n.velocity * (n.speedLimit / speed);
  }
  const float bodyRadius =
      16.2788f * std::max(std::abs(n.scale.x), std::abs(n.scale.y));
  for (size_t j = 0; j < entities.size(); ++j) {
    const auto &other = entities[j];
    if (j == index || other.objectId != 1 || !other.active)
      continue;
    float otherRadius =
        16.2788f * std::max(std::abs(other.scale.x), std::abs(other.scale.y));
    ::Vec2 radial = n.position - other.position;
    float distance = length(radial), sum = bodyRadius + otherRadius;
    if (distance < sum) {
      ::Vec2 normal = distance > .0001f ? radial * (1 / distance)
                                        : ::Vec2{index < j ? -1.0f : 1.0f, 0};
      n.position = n.position + normal * (sum - distance + .001f);
    }
  }
  ::Vec2 motion = n.velocity * (1.0f / 60);
  WallContact contact;
  for (size_t j = 0; j < entities.size(); ++j)
    if (j != index && entities[j].objectId == 1 && entities[j].active) {
      const auto &other = entities[j];
      sweepCircle(
          {other.position, 16.2788f * std::max(std::abs(other.scale.x),
                                               std::abs(other.scale.y))},
          n.position, motion, bodyRadius, contact);
    }
  n.position = n.position + motion * std::min(1.0f, contact.time);
  if (contact.time <= 1) {
    float toward = dot(n.velocity, contact.normal);
    if (toward < 0)
      n.velocity = n.velocity - contact.normal * toward;
    n.position = n.position + n.velocity * ((1 - contact.time) / 60);
  }
  bool lethal = !dead && !game.won && jumpFrames == 0 &&
                overlap(n, game.position, course.playerRadius);
  if (lethal && !n.inside) {
    if (parryActive > 23) {
      interruptSpawn();
      n.storedVelocity = n.velocity;
      n.velocity = {};
      n.parryStun = 16;
      parryVelocity = game.velocity;
      game.velocity = {};
      parryActive = 0;
      parryFreeze =
          15; // PParry's immediate entry update consumes the first of 16 ticks.
      {
        ScopeAudioOwner playerOwner(audioOwner, 0);
        sound("sfxParry");
      }
    } else {
      deathTint = n.color;
      die(0);
    }
  }
  n.inside = lethal;
  bool graze = !dead && !game.won && jumpFrames == 0 &&
               length(n.position - game.position) <=
                   24 * std::max(std::abs(n.scale.x), std::abs(n.scale.y)) +
                       course.playerRadius;
  if (n.grazing && !graze)
    sound("ghost_graze", 1, 2, -10);
  n.grazing = graze;
}
inline void Scene::stepBody(size_t index) {
  auto &n = entities[index];
  if (baddieOutside(n))
    queueBaddieRetirement(n);
  // queue_free does not return in Baddie._physics_process. Finish its motion
  // and collision, and let later imported nodes observe it until the frame
  // ends.
  if (n.objectId == 15 && n.taken) {
    n.velocity = {};
    if (n.linked >= 0)
      n.position =
          n.position * .95f + entities[size_t(n.linked)].position * .05f;
    return;
  }
  if (n.chaser) {
    n.velocity = n.velocity + unit(game.position - n.position) * n.acceleration;
    float speed = length(n.velocity);
    if (speed > n.speedLimit && speed > 0)
      n.velocity = n.velocity * (n.speedLimit / speed);
  }
  ::Vec2 motion = mul(n.velocity, n.scale) * (1.0f / 60);
  auto contact = sweep(n.position, motion, n.radius, int(index), false);
  n.position = n.position + motion * std::min(1.0f, contact.time);
  if (contact.time <= 1) {
    n.velocity =
        n.velocity - contact.normal * (2 * dot(n.velocity, contact.normal));
    if (n.objectId == 15) {
      sound("dog_bounce", 15);
      if (lastBodyObstacle >= 0)
        strike(size_t(lastBodyObstacle), true);
      for (size_t j = 0; j < entities.size(); ++j)
        if (j != index && entities[j].active &&
            rectOverlap(entities[j], n.position, n.radius) &&
            (entities[j].objectId == 11 || entities[j].objectId == 14))
          strike(j, true);
    }
  }
  if (n.objectId == 15) {
    for (size_t j = 0; j < entities.size(); ++j) {
      auto &goal = entities[j];
      if (goal.objectId == 16 && !goal.taken &&
          overlap(goal, n.position, n.radius)) {
        goal.taken = true;
        n.taken = true;
        n.linked = int(j);
        n.velocity = {};
        dogTiles = false;
        {
          ScopeAudioOwner goalOwner(audioOwner, goal.audioUid);
          sound("dog_goal", 16);
          sound("dog_bark", 16);
        }
        thought(14, 16);
        break;
      }
    }
  }
}
inline void Scene::stepSpawner(size_t index) {
  if (--entities[index].countdown > 0)
    return;
  if (entities[index].spawnedCount >= entities[index].maxBaddies)
    return;
  if (entities.size() >= MaxEntities) {
    bool reusable = false;
    for (size_t j = initialEntities.size(); j < entities.size(); ++j)
      reusable |= !entities[j].active && entities[j].spawnOwner >= 0;
    if (!reusable) {
      fault = "playtest entity memory limit 2048 reached";
      diagnostics.push_back(fault);
      return;
    }
  }
  const auto source = entities[index];
  Entity n;
  n.objectId = source.spawnEnemy == 0 ? 26 : source.spawnEnemy == 1 ? 17 : 18;
  n.scenePath =
      source.spawnEnemy == 0
          ? "res://assetsNEW/scenes/objects/baddies/car.tscn"
          : ucg::editor::object_definitions[size_t(n.objectId)].scene_path;
  n.spawnOwner = int(index);
  n.position = source.position;
  n.velocity = unit(source.velocity) * source.speed;
  n.speed = source.speed;
  n.scoreValue = source.spawnScoreValue;
  n.thoughtCategory = n.objectId == 26 ? 10 : n.objectId == 17 ? 4 : 15;
  n.rotation = source.spawnRotation;
  n.ignoreY = source.ignoreY;
  n.chaser = n.objectId == 18;
  if (n.chaser)
    n.velocity = {};
  n.acceleration = 4;
  n.speedLimit = 200;
  n.deathPitch = n.objectId == 17   ? ::Vec2{.8f, 1.55f}
                 : n.objectId == 18 ? ::Vec2{.7f, 1.3f}
                                    : ::Vec2{1, 1};
  if (source.colorful) {
    static const ModelColor colors[] = {
        {.4, .2, .6, 1}, {1, 0, 0, 1}, {0, .501960784, 0, 1}, {1, 1, 0, 1},
        {0, 1, 1, 1},    {0, 0, 1, 1}, {0, 1, 0, 1}};
    const auto c = colors[unsigned(random(0, 6.999f))];
    n.color = {1 - c.r * .35, 1 - c.g * .35, 1 - c.b * .35, 1};
  }
  n.radius = n.objectId == 26 ? 8 : 16;
  if (n.objectId == 26)
    n.halfSize = {8, 8};
  size_t slot = entities.size();
  for (size_t j = initialEntities.size(); j < entities.size(); ++j)
    if (!entities[j].active && entities[j].spawnOwner >= 0) {
      slot = j;
      break;
    }
  n.audioUid = nextAudioUid++;
  if (slot < entities.size())
    entities[slot] = n;
  else
    entities.push_back(n);
  ++entities[index].spawnedCount;
  entities[index].countdown = source.spawnRate;
  events.push_back({Event::Spawn, n.objectId, 0, ""});
}
inline void Scene::stepRing(size_t index) {
  auto &ring = entities[index];
  if (!ring.running) {
    if (ring.taken && ring.countdown > 0 && --ring.countdown == 0) {
      if (!ring.ringWon && ring.redoable) {
        ring.taken = false;
        ring.visualAlpha = 1;
      } else
        ring.active = false;
    }
    return;
  }
  float seconds = ring.timeGiven > 0 ? ring.timeGiven : 1;
  ring.ringAudioPitch = 3 - 2.5f * (float(ring.timer) / (seconds * 60));
  ringAudioPitch = ring.ringAudioPitch;
  int total = 0;
  for (const auto &coin : entities)
    if (coin.objectId == 9 && sameColor(coin.color, ring.color))
      ++total;
  if (ring.coinsGrabbed == total) {
    ring.running = false;
    ring.taken = true;
    ring.ringWon = true;
    // AudioStreamPlayer.finished controls source removal, not the physics
    // completion tick.
    ring.countdown = 110; // ceil(80530 source samples / 44100 * 60)
    ring.coinsGrabbed = 0;
    addScore(ring.scoreValue, 8, ring, false);
    sound("ring_win", 8);
    for (auto &coin : entities)
      if (coin.objectId == 9 && sameColor(coin.color, ring.color))
        coin.active = false;
    return;
  }
  if (--ring.timer <= 0) {
    ring.running = false;
    ring.coinsGrabbed = 0;
    ring.taken = true;
    ring.ringWon = false;
    ring.countdown = 83; // ceil(60480 source samples / 44100 * 60)
    for (auto &coin : entities)
      if (coin.objectId == 9 && sameColor(coin.color, ring.color)) {
        coin.active = false;
        coin.taken = false;
      }
    sound("ring_lose", 8);
  }
}
inline void Scene::stepKey(size_t index) {
  auto &key = entities[index];
  int count = 0, order = 0;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].objectId == 10 && entities[i].held) {
      if (i < index)
        ++order;
      ++count;
    }
  float angle = 2.5f * float(tick) / 60 +
                2 * Pi * float(order) / float(std::max(count, 1));
  ::Vec2 target = game.position + rotate({48, 0}, angle);
  key.velocity = (key.velocity + (target - key.position) * key.accel) * .35f;
  key.accel = std::min(5.0f, key.accel + .01f);
  key.position = key.position + key.velocity * .75f;
  key.visualAlpha = std::max(.5f, key.visualAlpha - .05f);
}
inline bool Scene::hasCamera() const {
  for (const auto &n : entities)
    if (n.objectId == 24 && n.active)
      return true;
  return false;
}
inline void Scene::updateCamera() {
  for (const auto &n : entities)
    if (n.objectId == 24 && n.active) {
      ::Vec2 desired{
          std::floor((game.position.x + 256) / n.segmentX) * n.segmentX +
              worldOffset.x,
          std::floor((game.position.y + 256) / n.segmentY) * n.segmentY};
      camera = n.smoothing ? camera + (desired - camera) *
                                          (1 - std::exp(-n.smoothingSpeed / 60))
                           : desired;
    }
}
} // namespace EditorPlay
#include "editor_play_objects.hpp"
