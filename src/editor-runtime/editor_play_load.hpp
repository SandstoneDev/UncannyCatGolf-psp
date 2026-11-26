#pragma once
namespace EditorPlay {
inline bool Scene::load(const ucg::editor::Document &document, const Course &base,
                        std::string &error) {
    if (!document.validate(error))
        return false;
    for (const auto &meta : ucg::editor::metadata_definitions) {
        const auto *v = document.root.find(meta.key);
        if (v && v->kind == ModelValue::Kind::Number && std::abs(v->numeric()) > 100000000) {
            error = std::string("playtest numeric metadata limit: ") + meta.key;
            return false;
        }
    }
    const float nextBounds = scalar(document.root, "levelBounds", 256);
    if (nextBounds < 16 || nextBounds > 8192) {
        error = "playtest levelBounds must be 16..8192";
        return false;
    }
    const ::Vec2 offset{-nextBounds * .5f, 0};
    std::vector<Entity> next;
    std::vector<Tile> nextTiles;
    std::vector<Decoration> nextDecorations;
    std::vector<Convex> nextColliders;
    std::vector<std::string> nextDiagnostics;
    if (document.nodes().size() > MaxInitialEntities) {
        error = "playtest initial object limit 1024";
        return false;
    }
    int lastPlayer = -1;
    for (const auto &record : document.nodes()) {
        const auto &a = record.array;
        int id = -1;
        for (size_t j = 0; j < ucg::editor::object_definition_count; ++j) {
            const auto &def = ucg::editor::object_definitions[j];
            if (a[4].text == def.scene_path || a[4].text == def.uid) {
                id = int(j);
                break;
            }
        }
        if (id < 0) {
            error = "unsupported playtest object script: " + a[4].text;
            return false;
        }
        const auto &def = ucg::editor::object_definitions[id];
        Entity n;
        n.objectId = id;
        n.scenePath = def.scene_path;
        ucg::editor::Vec2 p, s;
        ucg::editor::read_vector(a[0], p);
        ucg::editor::read_vector(a[1], s);
        ucg::editor::read_color(a[2], n.color);
        n.position = {float(p.x) + offset.x, float(p.y)};
        n.scale = {float(s.x), float(s.y)};
        if (std::abs(n.position.x) > 1000000 || std::abs(n.position.y) > 1000000 ||
            std::abs(n.scale.x) > 1000 || std::abs(n.scale.y) > 1000) {
            error = "playtest transform limit exceeded";
            return false;
        }
        ModelValue vars;
        if (!ucg::editor::parse_json(def.defaults_json, vars, error))
            return false;
        if (a[5].text != "sand")
            for (const auto &v : a[3].object)
                if (v.second.kind != ModelValue::Kind::Null)
                    vars[v.first] = v.second;
        for (const char *key :
             {"speed",           "rotate",         "acceleration",  "speed_limit",
              "b_speed",         "b_rotate",       "eEnemy",        "spawn_rate",
              "spawn_countdown", "max_baddies",    "dirx",          "diry",
              "time_given",      "sizex",          "sizey",         "chase_accel",
              "chase_limit",     "direction",      "speed_granted", "smoothing_speed",
              "segment_size_x",  "segment_size_y", "mod",           "set_speed"}) {
            const auto *value = vars.find(key);
            if (value && value->kind == ModelValue::Kind::Number &&
                std::abs(value->numeric()) > 100000000) {
                error = std::string("playtest numeric variable limit: ") + key;
                return false;
            }
        }
        if (def.polygon) {
            std::vector<ucg::editor::Vec2> poly;
            if (!ucg::editor::read_polygon(a[3], poly) || poly.size() < 3 || poly.size() > 256) {
                error = "playtest polygon requires 3..256 vertices";
                return false;
            }
            for (auto q : poly)
                n.polygon.push_back(n.position +
                                    ::Vec2{float(q.x) * n.scale.x, float(q.y) * n.scale.y});
            n.variant = int(a[6].numeric());
        }
        n.speed = float(integer(vars, "speed"));
        n.scoreValue = integer(vars, "score_value");
        if (const auto *value = vars.find("do_thought"))
            n.thoughtCategory = thought_category(value->text);
        n.rotation = scalar(vars, "rotate") * Pi / 180;
        n.chaser = boolean(vars, "chaser");
        n.acceleration = scalar(vars, "acceleration", .35f);
        n.speedLimit = scalar(vars, "speed_limit", 40);
        n.velocity = vector(vars, "velocity");
        ::Vec2 direction = vector(vars, "direction");
        float dirx = scalar(vars, "dirx"), diry = scalar(vars, "diry");
        if (dirx != 0)
            direction.x = dirx;
        if (diry != 0)
            direction.y = diry;
        n.halfSize = vector(vars, "size", {32, 32}) * .5f;
        n.lockColor = {1, 1, 1, 1};
        if (const auto *c = vars.find("color")) {
            ModelColor color;
            if (ucg::editor::read_color(*c, color))
                n.lockColor = color;
        }
        n.coinMultiply = boolean(vars, "coin_multiply");
        n.coinMultiplyValue = integer(vars, "coin_multiply_value");
        n.dblockScore = boolean(vars, "dblock_score");
        n.deathPitch = vector(vars, "death_sound_pitch", {1, 1});
        n.collectionMask = unsigned(integer(vars, "collision_mask", id == 3 ? 0 : 2));
        n.doesSpeed = boolean(vars, "does_speed");
        n.setSpeed = scalar(vars, "set_speed");
        n.keepFace = boolean(vars, "keep_face");
        n.bounceOffPlayer = boolean(vars, "bounce_off_player", true);
        n.ignoreY = boolean(vars, "ignorey");
        static const float radii[] = {15.8f, 12.1655f, 16, 16, 11.1803f, 16, 10.05f, 16,      24,
                                      20,    16,       16, 8,  16,       16, 15.8f,  16,      16,
                                      16,    0,        0,  20, 20,       0,  0,      11.1803f};
        n.radius = radii[id] * std::max(std::abs(n.scale.x), std::abs(n.scale.y));
        if (id == 0) {
            if (lastPlayer >= 0)
                next[size_t(lastPlayer)].active = false;
            lastPlayer = int(next.size());
        }
        if (id == 3) {
            n.spawnEnemy = int(scalar(vars, "eEnemy"));
            n.spawnRate = int(scalar(vars, "spawn_rate"));
            n.countdown = int(scalar(vars, "spawn_countdown"));
            n.maxBaddies = int(scalar(vars, "max_baddies"));
            n.speed = float(integer(vars, "b_speed"));
            n.spawnScoreValue = integer(vars, "b_score_value");
            n.colorful = boolean(vars, "colorful");
            n.spawnRotation = scalar(vars, "b_rotate") * Pi / 180;
            n.rotation = 0;
            n.velocity = vector(vars, "b_direction");
            if (dirx)
                n.velocity.x = dirx;
            if (diry)
                n.velocity.y = diry;
            n.ignoreY = boolean(vars, "b_ignorey_caronly");
            if (n.spawnEnemy < 0 || n.spawnEnemy > 2 || n.maxBaddies < 0 ||
                n.maxBaddies > 1000000 || std::abs(double(n.spawnRate)) > 1000000 ||
                std::abs(double(n.countdown)) > 1000000) {
                error = "invalid/oversized spawner parameters";
                return false;
            }
        }
        if (id == 8) {
            n.redoable = boolean(vars, "redoable");
            n.ringBell = boolean(vars, "ring_bell", true);
            n.timeGiven = scalar(vars, "time_given");
            if (n.timeGiven < 0 || n.timeGiven > 86400) {
                error = "ring time_given must be 0..86400 seconds";
                return false;
            }
        }
        if (id == 9)
            n.active = false;
        if (id == 12)
            n.inactive = boolean(vars, "inactive");
        if (id == 13) {
            n.halfSize = {scalar(vars, "sizex", 32) * .5f, scalar(vars, "sizey", 32) * .5f};
            if (n.halfSize.x <= 0 || n.halfSize.y <= 0) {
                error = "door size must be positive";
                return false;
            }
        }
        if (id == 15 || id == 17 || id == 18)
            n.velocity = n.chaser ? ::Vec2{} : unit(direction) * n.speed;
        if (id == 17 || id == 18) {
            n.acceleration = scalar(vars, "chase_accel", 4);
            n.speedLimit = scalar(vars, "chase_limit", 200);
        }
        if (id == 22) {
            n.rotation = scalar(vars, "direction") * Pi / 180;
            n.speed = scalar(vars, "speed_granted");
        }
        if (id == 24) {
            n.smoothing = boolean(vars, "smoothing");
            n.smoothingSpeed = scalar(vars, "smoothing_speed", 10);
            n.segmentX = scalar(vars, "segment_size_x", 512);
            n.segmentY = scalar(vars, "segment_size_y", 512);
            if (n.segmentX < 1 || n.segmentY < 1 || n.segmentX > 1000000 || n.segmentY > 1000000 ||
                n.smoothingSpeed < 0 || n.smoothingSpeed > 1000) {
                error = "invalid camera follower segment/smoothing";
                return false;
            }
        }
        if (id == 25) {
            n.mod = int(scalar(vars, "mod"));
            if (n.mod < 0 || n.mod > 11) {
                error = "unknown modifier collectible value";
                return false;
            }
            n.frame = unsigned(n.mod);
        }
        n.halfSize = {std::abs(n.halfSize.x * n.scale.x), std::abs(n.halfSize.y * n.scale.y)};
        if (!std::isfinite(n.speed) || std::abs(n.speed) > 100000 ||
            !std::isfinite(n.acceleration) || std::abs(n.acceleration) > 10000 ||
            n.speedLimit < 0 || n.speedLimit > 100000) {
            error = "playtest movement parameter limit";
            return false;
        }
        next.push_back(std::move(n));
    }
    if (lastPlayer < 0) {
        error = "place a Player before playtest";
        return false;
    }
    for (int layer = 0; layer < 6; ++layer)
        for (const auto &record : document.tiles(layer)) {
            ucg::editor::Vec2 p, a;
            ucg::editor::read_vector(record.array[0], p);
            ucg::editor::read_vector(record.array[1], a);
            Tile tile{layer, int(p.x), int(p.y), int(a.x), int(a.y), -1};
            for (size_t j = 0; j < sizeof(EditorArt::TileShapes::Shapes) /
                                       sizeof(EditorArt::TileShapes::Shapes[0]);
                 ++j) {
                const auto &shape = EditorArt::TileShapes::Shapes[j];
                if (shape.x == unsigned(tile.ax) && shape.y == unsigned(tile.ay)) {
                    tile.shape = int(j);
                    break;
                }
            }
            if (tile.shape < 0) {
                error = "unsupported tile atlas coordinates " + std::to_string(tile.ax) + "," +
                        std::to_string(tile.ay);
                return false;
            }
            const auto &tileShape = EditorArt::TileShapes::Shapes[size_t(tile.shape)];
            for (unsigned j = 0; j < tileShape.count; ++j) {
                const auto q = EditorArt::TileShapes::Points[tileShape.first + j];
                const ::Vec2 point{q.x + 16 * tile.x + 8 + offset.x, q.y + 16 * tile.y + 8};
                if (!j)
                    tile.bounds = detail::Bounds::point(point);
                else
                    tile.bounds.include(point);
            }
            bool replaced = false;
            for (auto &old : nextTiles)
                if (old.layer == layer && old.x == tile.x && old.y == tile.y) {
                    old = tile;
                    replaced = true;
                    break;
                }
            if (!replaced)
                nextTiles.push_back(tile);
            if (nextTiles.size() > MaxTiles) {
                error = "playtest tile limit 20000";
                return false;
            }
        }
    for (const auto &record : document.decorations()) {
        const auto &a = record.array;
        Decoration d;
        ucg::editor::Vec2 p, s;
        ucg::editor::read_vector(a[0], p);
        ucg::editor::read_vector(a[1], s);
        if (std::abs(p.x) > 1000000 || std::abs(p.y) > 1000000 || std::abs(s.x) > 1000 ||
            std::abs(s.y) > 1000 || std::abs(a[2].numeric()) > 1000000 ||
            std::abs(a[5].numeric()) > 1000000) {
            error = "playtest decoration transform/speed limit";
            return false;
        }
        d.position = {float(p.x) + offset.x, float(p.y)};
        d.scale = {float(s.x), float(s.y)};
        d.rotation = float(a[2].numeric());
        d.z = int(a[3].numeric());
        d.frame = int(a[4].numeric());
        d.animSpeed = float(int(a[5].numeric()));
        d.hframes = int(a[6].numeric());
        d.vframes = int(a[7].numeric());
        d.collision = a[8].boolean;
        d.texture = a[9].text;
        if (a.size() > 10)
            d.imageFilename = a[10].text;
        if (d.collision) {
            if (!decorationCollider) {
                error = "decoration collision needs image alpha polygon loader";
                return false;
            }
            std::vector<std::vector<::Vec2>> pieces;
            if (!decorationCollider(d, pieces, error))
                return false;
            for (const auto &piece : pieces) {
                if (piece.size() < 3 || piece.size() > 4096) {
                    error = "invalid decoration convex piece";
                    return false;
                }
                for (size_t i = 1; i + 1 < piece.size(); ++i) {
                    Convex c;
                    c.count = 3;
                    c.points[0] = d.position + rotate(mul(piece[0], d.scale), d.rotation);
                    c.points[1] = d.position + rotate(mul(piece[i], d.scale), d.rotation);
                    c.points[2] = d.position + rotate(mul(piece[i + 1], d.scale), d.rotation);
                    float cross =
                        (c.points[1].x - c.points[0].x) * (c.points[2].y - c.points[0].y) -
                        (c.points[1].y - c.points[0].y) * (c.points[2].x - c.points[0].x);
                    if (cross < 0)
                        std::swap(c.points[1], c.points[2]);
                    c.bounds = detail::Bounds::point(c.points[0]);
                    for (unsigned j = 1; j < c.count; ++j)
                        c.bounds.include(c.points[j]);
                    nextColliders.push_back(c);
                    if (nextColliders.size() > MaxColliders) {
                        error = "decoration collision polygon limit 32768";
                        return false;
                    }
                }
            }
        }
        nextDecorations.push_back(std::move(d));
    }
    unsigned nextMods = 0;
    if (const auto *configured = document.root.find("mods"))
        parse_mods(configured->text, nextMods, nextDiagnostics);
    size_t auxiliary = (nextMods & unsigned(Buddy) ? 1 : 0) +
                       (nextMods & unsigned(Missiles) ? 1 : 0) +
                       (nextMods & unsigned(Borts) ? 25 : 0) + (nextMods & unsigned(Bees) ? 20 : 0);
    if (next.size() + auxiliary > MaxEntities) {
        error = "initial modifier entities exceed playtest memory limit";
        return false;
    }
    detail::SpatialIndex nextTileIndex, nextColliderIndex;
    nextTileIndex.build(nextTiles);
    nextColliderIndex.build(nextColliders);
    course = base;
    bounds = nextBounds;
    worldOffset = offset;
    par = int(scalar(document.root, "par"));
    onTime = scalar(document.root, "time");
    course.par = unsigned(std::max(0, par));
    for (int i = 0; i < 6; ++i)
        rankReqs[i] = i == 1 ? 1 : 0;
    for (int i = 0; i < 4; ++i)
        rankReqs[5 - i] = int(scalar(document.root, ("rank" + std::to_string(i + 1)).c_str(),
                                     float(rankReqs[5 - i])));
    course.spawn = next[size_t(lastPlayer)].position;
    course.playerRadius = next[size_t(lastPlayer)].radius;
    course.goal = {1000000, 1000000};
    course.goalRadius = 16;
    course.maxDrag = 175;
    course.minDrag = 20;
    course.dragMultiplier = 5;
    course.frictionCoeff = .04f;
    course.frictionPow = 1.06f;
    course.stopSpeed = 20;
    playerScale = next[size_t(lastPlayer)].scale;
    speechTint059 = next[size_t(lastPlayer)].color;
    entities = std::move(next);
    entities.reserve(MaxEntities);
    tiles = std::move(nextTiles);
    decorations = std::move(nextDecorations);
    colliders = std::move(nextColliders);
    tileIndex = std::move(nextTileIndex);
    colliderIndex = std::move(nextColliderIndex);
    diagnostics = std::move(nextDiagnostics);
    initialEntities = entities;
    initialDecorations = decorations;
    initialMods = nextMods;
    switchLayers.clear();
    for (int l = 1; l <= 4; ++l)
        for (const auto &tile : tiles)
            if (tile.layer == l) {
                switchLayers.push_back(l);
                break;
            }
    restart();
    error.clear();
    return true;
}
inline void Scene::parse_mods(const std::string &text, unsigned &mask,
                              std::vector<std::string> &messages) {
    size_t start = 0;
    for (;;) {
        size_t end = text.find(',', start);
        std::string token = text.substr(start, end == std::string::npos ? end : end - start);
        token.erase(std::remove_if(token.begin(), token.end(),
                                   [](unsigned char c) { return std::isspace(c) != 0; }),
                    token.end());
        for (char &c : token)
            c = char(std::tolower((unsigned char)c));
        static const char *names[] = {"hard_mode", "burst", "telekinesis", "buddy",  "parry",
                                      "jumpy",     "dizzy", "missiles",    "sticky", "god",
                                      "bort",      "bees",  "easy_mode"};
        bool found = false;
        for (unsigned i = 0; i < 13; ++i)
            if (token == names[i]) {
                mask |= 1u << i;
                found = true;
                break;
            }
        if (!found && !token.empty())
            messages.push_back("PC ignores unknown modifier: " + token);
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
}
inline void Scene::clear_memory() {
    audioOwner = 0;
    nextAudioUid = 1;
    std::vector<Entity>().swap(entities);
    std::vector<Entity>().swap(initialEntities);
    std::vector<Tile>().swap(tiles);
    std::vector<Decoration>().swap(decorations);
    std::vector<Decoration>().swap(initialDecorations);
    std::vector<Event>().swap(events);
    std::vector<Event>().swap(queuedEvents);
    stepping = false;
    std::vector<Convex>().swap(colliders);
    tileIndex.clear_memory();
    colliderIndex.clear_memory();
    std::vector<unsigned>().swap(staticCandidates);
    std::vector<::CircleCollider>().swap(passiveCircles);
    std::vector<::SolidBlock>().swap(passiveBoxes);
    std::vector<int>().swap(switchLayers);
    std::vector<std::string>().swap(diagnostics);
    dead = false;
    game.restart();
}
} // namespace EditorPlay
