#pragma once
#include "editor_input.hpp"
#include "ucg_editor_hit_shapes.hpp"
#include "ucg_editor_model.hpp"
namespace EditorController {
using ucg::editor::Document;
using ucg::editor::Value;
using ucg::editor::Vec2;
inline Vec2 snap(Vec2 p, double step) {
  return {std::round(p.x / step) * step, std::round(p.y / step) * step};
}
struct Controller {
  ucg::editor::Editor model;
  unsigned tab = 0, layer = 0, object = 0, deco = 0, atlasX = 0, atlasY = 0,
           switchPreview = 0;
  bool selectMode = false, tools = false, grid = false, music = true,
       dragging = false, painting = false, erasing = false;
  double userZoom = 0, gridX = 8, gridY = 8, decoRotation = 0, decoScaleX = 1,
         decoScaleY = 1;
  int decoZ = 0, decoFrame = 0, decoH = 1, decoV = 1;
  double decoSpeed = 0;
  bool decoCollision = false;
  Vec2 camera{}, dragOffset{}, lastCell{};
  Document stroke, dragOriginal;
  std::string strokeError;
  int point = -1;
  bool beginDrag(std::string &e) {
    e.clear();
    if (dragging)
      return true;
    if (!model.reserve_copy(model.document, e))
      return false;
    dragOriginal = model.document;
    dragging = true;
    return true;
  }
  bool beginDrag() { return beginDrag(strokeError); }
  bool moveDrag(Vec2 p, std::string &e) {
    if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
      e = "Invalid position";
      return false;
    }
    auto *n = model.selected();
    if (!dragging || !n) {
      e = "No drag selection";
      return false;
    }
    n->array[0] = ucg::editor::vector_value(p);
    return true;
  }
  bool moveDragPoint(unsigned index, Vec2 worldPos, bool free, std::string &e) {
    auto *n = model.selected();
    if (!dragging || !n || model.selection_deco) {
      e = "No polygon drag selection";
      return false;
    }
    Vec2 pos, s;
    ucg::editor::read_vector(n->array[0], pos);
    ucg::editor::read_vector(n->array[1], s);
    std::vector<Vec2> points;
    ucg::editor::read_polygon(n->array[3], points);
    if (index >= points.size() || !s.x || !s.y) {
      e = "Invalid polygon handle";
      return false;
    }
    Vec2 p = free ? worldPos : snap(worldPos, 8);
    points[index] = {(p.x - pos.x) / s.x, (p.y - pos.y) / s.y};
    n->array[3] = ucg::editor::polygon_value(points);
    return true;
  }
  bool finishDrag(std::string &e) {
    if (!dragging)
      return true;
    dragging = false;
    Document candidate = std::move(model.document);
    model.document = std::move(dragOriginal);
    return model.mutate([&](Document &d) { d = std::move(candidate); }, e);
  }
  std::string path;
  std::vector<std::string> customDeco;
  const Value &get(const char *key) const {
    static const Value empty;
    const auto *v = model.document.root.find(key);
    return v ? *v : empty;
  }
  double number(const char *k, double fallback = 0) const {
    return get(k).numeric(fallback);
  }
  bool set(const char *k, const Value &v, std::string &e) {
    return model.mutate([&](Document &d) { d.root[k] = v; }, e);
  }
  double rawZoom() const { return 256 / number("levelBounds", 256) + userZoom; }
  double zoom() const { return std::max(.01, rawZoom()); }
  Vec2 world(Vec2 s) const {
    return {(s.x - 384) / zoom() + camera.x + 256 * rawZoom() - 256,
            (s.y - 256) / zoom() + camera.y};
  }
  Vec2 screen(Vec2 p) const {
    return {384 + zoom() * (p.x - camera.x - (256 * rawZoom() - 256)),
            256 + zoom() * (p.y - camera.y)};
  }
  struct Hit {
    bool deco = false;
    size_t index = 0;
  };
  std::function<bool(const Value &, double &, double &)> decorationSize;
  bool hitAt(Vec2 p, Hit &hit, std::string &e) const {
    std::vector<Hit> hits;
    collectHits(p, hits, e);
    if (hits.empty())
      return false;
    hit = hits.back();
    return true;
  }
  bool eraseAt(Vec2 p, std::string &e) {
    Hit hit;
    if (!hitAt(p, hit, e))
      return false;
    bool valid = model.selection_valid, dec = model.selection_deco;
    size_t index = model.selection_index;
    if (!model.mutate(
            [&](Document &d) {
              auto &entries = hit.deco ? d.decorations() : d.nodes();
              entries.erase(entries.begin() + hit.index);
            },
            e))
      return false;
    if (valid && !(dec == hit.deco && index == hit.index)) {
      if (dec == hit.deco && index > hit.index)
        --index;
      model.select(dec, index);
    } else {
      model.selection_valid = false;
      point = -1;
    }
    return true;
  }
  bool variable(const std::string &name, Value &out, std::string &e) const {
    const auto *n = model.selected();
    if (!n || model.selection_deco) {
      e = "No object property selection";
      return false;
    }
    if (selectedObject() == 20 && name == "deco") {
      if (!model.reserve_copy(n->array[6], e))
        return false;
      out = n->array[6];
      e.clear();
      return true;
    }
    if (n->array[3].kind == Value::Kind::Object) {
      if (const auto *value = n->array[3].find(name)) {
        if (!model.reserve_copy(*value, e))
          return false;
        out = *value;
        e.clear();
        return true;
      }
    }
    int id = selectedObject();
    if (id >= 0)
      for (size_t i = 0; i < ucg::editor::object_definitions[id].variable_count;
           ++i) {
        const auto &v = ucg::editor::object_definitions[id].variables[i];
        if (name == v.varname)
          return ucg::editor::parse_json(v.default_json, out, e);
      }
    e = "Unknown object property: " + name;
    return false;
  }
  bool setVariable(const std::string &name, const Value &value,
                   std::string &e) {
    if (selectedObject() == 20 && name == "deco") {
      double n = value.numeric(-1);
      if (value.kind != Value::Kind::Number || n < 0 || n > 10 ||
          std::floor(n) != n) {
        e = "Deco floor variant must be 0..10";
        return false;
      }
      return model.edit_selected([&](Value &node) { node.array[6] = value; },
                                 e);
    }
    return model.set_variable(name, value, e);
  }
  int selectedObject() const {
    const auto *n = model.selected();
    if (!n || model.selection_deco)
      return -1;
    for (size_t i = 0; i < ucg::editor::object_definition_count; ++i)
      if (n->array[4].text == ucg::editor::object_definitions[i].scene_path ||
          n->array[4].text == ucg::editor::object_definitions[i].uid)
        return int(i);
    return -1;
  }
  bool load(const std::string &file, std::string &e) {
    Document d;
    d.limits = model.document.limits;
    if (!ucg::editor::load_file(file, d, e))
      return false;
    if (!model.load_document(std::move(d), e))
      return false;
    path = file;
    model.document.filename = file;
    camera = {};
    userZoom = 0;
    return true;
  }
  bool save(std::string &e) {
    e.clear();
    ucg::editor::AllocationBudget budget;
    if (!saveBudget(budget, e) || !model.reserve_work(budget, e))
      return false;
    return ucg::editor::save_atomic(path, model.document, e);
  }
  bool clearLikePc(std::string &e) {
    return model.mutate(
        [&](Document &d) {
          for (int i = 0; i < 6; ++i)
            d.tiles(i).clear();
          d.nodes().clear();
          d.decorations().clear();
          for (auto k : {"rank1", "rank2", "rank3", "rank4", "levelBounds",
                         "par", "time", "song", "light"}) {
            const Document fresh = Document::fresh();
            d.root[k] = *fresh.root.find(k);
          }
          for (auto k :
               {"customSongName", "customtiles", "switch1customtiles",
                "switch2customtiles", "switch3customtiles",
                "switch4customtiles", "dogcustomtiles", "mods", "customfloor"})
            d.root[k] = Value("");
        },
        e);
  }
  bool beginStroke(Vec2 p, bool erase, std::string &e) {
    e.clear();
    if (!model.reserve_copy(model.document, e))
      return false;
    painting = true;
    strokeError.clear();
    erasing = erase;
    stroke = model.document;
    lastCell = {std::floor(p.x / 16), std::floor(p.y / 16)};
    continueStroke(p);
    return true;
  }
  bool beginStroke(Vec2 p, bool erase) {
    return beginStroke(p, erase, strokeError);
  }
  void continueStroke(Vec2 p) {
    if (!painting || !strokeError.empty())
      return;
    if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
      strokeError = "Invalid tile stroke position";
      return;
    }
    Vec2 cell{std::floor(p.x / 16), std::floor(p.y / 16)};
    std::string e;
    if (std::abs(cell.x) > 1000000 || std::abs(cell.y) > 1000000) {
      strokeError = "Tile stroke coordinate limit";
      return;
    }
    int x = int(lastCell.x), y = int(lastCell.y), ex = int(cell.x),
        ey = int(cell.y), dx = std::abs(ex - x), sx = x < ex ? 1 : -1,
        dy = -std::abs(ey - y), sy = y < ey ? 1 : -1, err = dx + dy;
    if (dx > 4096 || -dy > 4096) {
      strokeError = "Tile stroke segment limit 4096";
      return;
    }
    for (;;) {
      if (erasing)
        stroke.erase_tile(int(layer), x, y, e);
      else
        stroke.paint(int(layer), x, y, int(atlasX), int(atlasY), e);
      if (!e.empty()) {
        strokeError = e;
        break;
      }
      if (x == ex && y == ey)
        break;
      int e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        x += sx;
      }
      if (e2 <= dx) {
        err += dx;
        y += sy;
      }
    }
    lastCell = cell;
  }
  bool endStroke(std::string &e) {
    if (!painting)
      return true;
    painting = false;
    if (!strokeError.empty()) {
      e = strokeError;
      strokeError.clear();
      stroke = Document();
      return false;
    }
    bool ok = model.mutate(
        [&](Document &d) {
          if (model.reserve_copy(stroke, e))
            d = stroke;
        },
        e);
    stroke = Document();
    return ok;
  }
  bool place(Vec2 p, bool free, bool replacePlayer, std::string &e) {
    Value item;
    if (!ucg::editor::make_object(int(object), free ? p : snap(p, 16), item, e))
      return false;
    size_t player = size_t(-1);
    if (object == 0)
      for (size_t i = 0; i < model.document.nodes().size(); ++i)
        if (model.document.nodes()[i].array[4].text ==
                ucg::editor::object_definitions[0].scene_path ||
            model.document.nodes()[i].array[4].text ==
                ucg::editor::object_definitions[0].uid) {
          player = i;
          break;
        }
    if (player != size_t(-1) && !replacePlayer) {
      e = "Player already exists; hold L+R+Triangle to replace";
      return false;
    }
    size_t index = model.document.nodes().size();
    if (!model.mutate(
            [&](Document &d) {
              if (player != size_t(-1)) {
                d.nodes().erase(d.nodes().begin() + player);
                --index;
              }
              d.nodes().push_back(item);
            },
            e))
      return false;
    model.select(false, index);
    return true;
  }
  bool placeDeco(Vec2 p, const char *texture, const char *filename, bool free,
                 std::string &e) {
    Value n = Value::list();
    n.array = {ucg::editor::vector_value(free ? p : snap(p, 8)),
               ucg::editor::vector_value({decoScaleX, decoScaleY}),
               Value::number(decoRotation),
               Value::number(decoZ),
               Value::number(decoFrame),
               Value::number(decoSpeed),
               Value::number(decoH),
               Value::number(decoV),
               Value(decoCollision),
               texture ? Value(texture) : Value(),
               Value(filename ? filename : "")};
    size_t index = model.document.decorations().size();
    if (!model.mutate([&](Document &d) { d.decorations().push_back(n); }, e))
      return false;
    model.select(true, index);
    return true;
  }
  bool selectAt(Vec2 p, std::string &e) {
    std::vector<Hit> hits;
    collectHits(p, hits, e);
    if (hits.empty()) {
      model.selection_valid = false;
      point = -1;
      return false;
    }
    size_t hit = hits.size() - 1;
    for (size_t i = 0; i < hits.size(); ++i)
      if (model.selection_valid && model.selection_deco == hits[i].deco &&
          model.selection_index == hits[i].index) {
        hit = i == 0 ? hits.size() - 1 : i - 1;
        break;
      }
    model.select(hits[hit].deco, hits[hit].index);
    point = -1;
    e.clear();
    return true;
  }

private:
  static bool saveNumberLength(const Value &v, size_t &largest, size_t depth,
                               size_t maxDepth) {
    if (depth > maxDepth)
      return false;
    if (v.kind == Value::Kind::Number)
      largest = std::max(largest, v.text.size());
    if (v.kind == Value::Kind::Array)
      for (const auto &child : v.array)
        if (!saveNumberLength(child, largest, depth + 1, maxDepth))
          return false;
    if (v.kind == Value::Kind::Object)
      for (const auto &item : v.object)
        if (!saveNumberLength(item.second, largest, depth + 1, maxDepth))
          return false;
    return true;
  }
  bool saveBudget(ucg::editor::AllocationBudget &budget, std::string &e) const {
    using ucg::editor::detail::size_add;
    using ucg::editor::detail::size_product;
    using Budget = ucg::editor::AllocationBudget;
    if (path.empty() || path.find('\0') != std::string::npos) {
      e = "invalid save filename";
      return false;
    }
    const auto usage = model.document.memory_usage();
    auto overflow = [&]() {
      e = "save working-memory size overflow";
      return false;
    };
    size_t raw = usage.encoded_bytes, pathBlock = path.size(), pathWork = 0;
    if (usage.overflow ||
        usage.encoded_bytes > model.document.limits.max_file_bytes ||
        !size_add(raw, 1) || !size_add(pathBlock, 64) ||
        !size_product(pathBlock, 2, pathBlock) ||
        !size_product(pathBlock, 4, pathWork))
      return overflow();
    budget = {raw, std::max<size_t>(8192, std::max(raw, pathBlock)),
              raw > Budget::chunk_bytes ? size_t(1) : size_t(0)};
    if (!size_add(budget.total_bytes, pathWork) ||
        !size_add(budget.total_bytes, 8192))
      return overflow();
    if (pathBlock > Budget::chunk_bytes && !size_add(budget.large_blocks, 4))
      return overflow();
    size_t number = 0, tuple = 0, geometryPeak = 0, geometryLargest = 0,
           geometryBig = 0;
    if (!saveNumberLength(model.document.root, number, 0,
                          model.document.limits.max_depth))
      return overflow();
    auto noteTuple = [&](const Value &v) {
      if (v.kind == Value::Kind::String)
        tuple = std::max(tuple, v.text.size());
    };
    for (const char *key : {"light", "tBright", "tNormal", "tDark", "tExtra"})
      if (const auto *v = model.document.root.find(key))
        noteTuple(*v);
    for (int layer = 0; layer < 6; ++layer)
      for (const auto &cell : model.document.tiles(layer))
        if (cell.kind == Value::Kind::Array && cell.array.size() >= 2) {
          noteTuple(cell.array[0]);
          noteTuple(cell.array[1]);
        }
    for (const char *key : {"nodes", "deco"})
      for (const auto &record : model.document.array_field(key)) {
        if (record.kind != Value::Kind::Array || record.array.size() < 3)
          continue;
        noteTuple(record.array[0]);
        noteTuple(record.array[1]);
        if (key[0] == 'n')
          noteTuple(record.array[2]);
        if (key[0] != 'n' || record.array.size() < 7 ||
            record.array[5].text != "sand" ||
            record.array[3].kind != Value::Kind::String)
          continue;
        const auto &text = record.array[3].text;
        size_t points = 0, start = 0;
        for (size_t i = 0; i < text.size(); ++i) {
          if (text[i] == '(')
            start = i;
          if (text[i] == ')') {
            ++points;
            tuple = std::max(tuple, i - start + 1);
          }
        }
        size_t capacity = points ? 1 : 0, bytes = 0;
        while (capacity < points)
          if (!size_product(capacity, 2, capacity))
            return overflow();
        if (!size_product(capacity, sizeof(Vec2), bytes))
          return overflow();
        const size_t oldBytes = bytes / 2;
        geometryLargest = std::max(geometryLargest, bytes);
        geometryBig =
            std::max(geometryBig, size_t(bytes > Budget::chunk_bytes) +
                                      size_t(oldBytes > Budget::chunk_bytes));
        if (!size_add(bytes, oldBytes))
          return overflow();
        geometryPeak = std::max(geometryPeak, bytes);
      }
    // Validation finishes before the one raw JSON buffer is allocated. The sum
    // conservatively covers temporary numeric/tuple spellings and vector
    // growth.
    size_t numericWork = number, tupleWork = tuple;
    if (!size_add(numericWork, 1) ||
        !size_product(numericWork, 3, numericWork) || !size_add(tupleWork, 1) ||
        !size_product(tupleWork, 6, tupleWork) ||
        !size_add(budget.total_bytes, numericWork) ||
        !size_add(budget.total_bytes, tupleWork) ||
        !size_add(budget.total_bytes, geometryPeak))
      return overflow();
    budget.largest_block = std::max(budget.largest_block, geometryLargest);
    budget.large_blocks = std::max(budget.large_blocks, geometryBig);
    if (number >= Budget::chunk_bytes)
      budget.large_blocks = std::max<size_t>(budget.large_blocks, 3);
    return true;
  }
  static int definition(const Value &n) {
    for (size_t i = 0; i < ucg::editor::object_definition_count; ++i) {
      const auto &d = ucg::editor::object_definitions[i];
      if (n.array[4].text == d.scene_path || n.array[4].text == d.uid)
        return int(i);
    }
    return -1;
  }
  static bool ellipseHit(Vec2 p, Vec2 pos, Vec2 scale, double radius) {
    double sx = std::abs(scale.x), sy = std::abs(scale.y);
    if (!sx || !sy)
      return false;
    double dx = std::max(std::abs(p.x - pos.x) - .5, 0.) / sx,
           dy = std::max(std::abs(p.y - pos.y) - .5, 0.) / sy;
    return dx * dx + dy * dy <= radius * radius;
  }
  static bool segmentBox(Vec2 a, Vec2 b, Vec2 center) {
    double first = 0, last = 1;
    for (int axis = 0; axis < 2; ++axis) {
      double p = axis ? a.y : a.x, d = axis ? b.y - a.y : b.x - a.x,
             c = axis ? center.y : center.x;
      if (std::abs(d) < 1e-15) {
        if (p < c - .5 || p > c + .5)
          return false;
      } else {
        double lo = (c - .5 - p) / d, hi = (c + .5 - p) / d;
        if (lo > hi)
          std::swap(lo, hi);
        first = std::max(first, lo);
        last = std::min(last, hi);
        if (first > last)
          return false;
      }
    }
    return true;
  }
  static bool polygonHit(const std::vector<Vec2> &points, Vec2 p) {
    if (points.size() < 3)
      return false;
    bool inside = false;
    for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
      const auto a = points[j], b = points[i];
      if (segmentBox(a, b, p))
        return true;
      if ((a.y > p.y) != (b.y > p.y) &&
          p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)
        inside = !inside;
    }
    if (inside)
      return true;
    for (Vec2 corner : std::vector<Vec2>{{p.x - .5, p.y - .5},
                                         {p.x + .5, p.y - .5},
                                         {p.x - .5, p.y + .5},
                                         {p.x + .5, p.y + .5}}) {
      inside = false;
      for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
        const auto a = points[j], b = points[i];
        if ((a.y > corner.y) != (b.y > corner.y) &&
            corner.x < (b.x - a.x) * (corner.y - a.y) / (b.y - a.y) + a.x)
          inside = !inside;
      }
      if (inside)
        return true;
    }
    return false;
  }
  bool recordHit(const Value &n, bool dec, Vec2 p, std::string &e) const {
    Vec2 pos, scale;
    ucg::editor::read_vector(n.array[0], pos);
    ucg::editor::read_vector(n.array[1], scale);
    if (dec) {
      double width = 0, height = 0;
      bool custom = n.array.size() > 10 && !n.array[10].text.empty(),
           known = false;
      if (!custom)
        for (const auto &image : ucg::editor::deco_hit_images)
          if (n.array[9].text == image.path) {
            width = image.width;
            height = image.height;
            known = true;
            break;
          }
      if (!known && decorationSize)
        known = decorationSize(n, width, height);
      if (!known || width <= 0 || height <= 0) {
        e = "Decoration dimensions unavailable: " +
            (custom ? n.array[10].text : n.array[9].text);
        return false;
      }
      double hframes = n.array[6].numeric(), vframes = n.array[7].numeric();
      if (hframes < 1 || vframes < 1)
        return false;
      double hx = std::abs(width / hframes * vframes * scale.x) * .5,
             hy = std::abs(height / hframes * vframes * scale.y) * .5;
      double r = n.array[2].numeric(), c = std::cos(r), sn = std::sin(r),
             dx = p.x - pos.x, dy = p.y - pos.y, ac = std::abs(c),
             as = std::abs(sn);
      return std::abs(dx) <= hx * ac + hy * as + .5 &&
             std::abs(dy) <= hx * as + hy * ac + .5 &&
             std::abs(dx * c + dy * sn) <= hx + .5 * (ac + as) &&
             std::abs(-dx * sn + dy * c) <= hy + .5 * (ac + as);
    }
    int id = definition(n);
    if (id < 0) {
      if (std::abs(pos.x - p.x) < 20 && std::abs(pos.y - p.y) < 20)
        e = "Unsupported object selection shape: " + n.array[4].text;
      return false;
    }
    const auto &shape = ucg::editor::object_hit_shapes[id];
    if (shape.kind == ucg::editor::HitShape::Circle)
      return ellipseHit(p, pos, scale, shape.x);
    if (shape.kind == ucg::editor::HitShape::Rectangle)
      return std::abs(pos.x - p.x) <= std::abs(shape.x * scale.x) + .5 &&
             std::abs(pos.y - p.y) <= std::abs(shape.y * scale.y) + .5;
    std::vector<Vec2> local;
    if (!ucg::editor::read_polygon(n.array[3], local)) {
      e = "Invalid polygon selection shape";
      return false;
    }
    std::vector<Vec2> world;
    for (auto q : local) {
      Vec2 center{pos.x + q.x * scale.x, pos.y + q.y * scale.y};
      if (ellipseHit(p, center, scale, 12))
        return true;
      world.push_back(center);
    }
    return polygonHit(world, p);
  }
  void collectHits(Vec2 p, std::vector<Hit> &hits, std::string &e) const {
    e.clear();
    if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
      e = "Invalid selection cursor";
      return;
    }
    auto append = [&](bool dec, bool bodies) {
      const auto &entries =
          dec ? model.document.decorations() : model.document.nodes();
      for (size_t i = 0; i < entries.size(); ++i) {
        int id = dec ? -1 : definition(entries[i]);
        bool body = id == 0 || id == 1 || id == 11 || id == 13 || id == 14 ||
                    id == 15 || id == 17 || id == 18;
        if (!dec && body != bodies)
          continue;
        std::string local;
        if (recordHit(entries[i], dec, p, local))
          hits.push_back({dec, i});
        else if (e.empty() && !local.empty())
          e = local;
      }
    };
    append(false, true);
    append(true, false);
    append(false, false);
    if (!hits.empty())
      e.clear();
  }

public:
  int pointAt(Vec2 p) const {
    const auto *n = model.selected();
    if (!n || model.selection_deco || n->array[5].text != "sand")
      return -1;
    Vec2 pos, s;
    ucg::editor::read_vector(n->array[0], pos);
    ucg::editor::read_vector(n->array[1], s);
    std::vector<Vec2> points;
    ucg::editor::read_polygon(n->array[3], points);
    for (size_t i = points.size(); i > 0; --i)
      if (ellipseHit(
              p, {pos.x + points[i - 1].x * s.x, pos.y + points[i - 1].y * s.y},
              s, 12))
        return int(i - 1);
    return -1;
  }
  bool setPoint(unsigned index, Vec2 worldPos, bool free, std::string &e) {
    return model.edit_selected(
        [&](Value &n) {
          Vec2 pos, s;
          ucg::editor::read_vector(n.array[0], pos);
          ucg::editor::read_vector(n.array[1], s);
          std::vector<Vec2> points;
          ucg::editor::read_polygon(n.array[3], points);
          if (index >= points.size() || s.x == 0 || s.y == 0) {
            e = "Invalid polygon handle";
            return;
          }
          Vec2 p = free ? worldPos : snap(worldPos, 8);
          points[index] = {(p.x - pos.x) / s.x, (p.y - pos.y) / s.y};
          n.array[3] = ucg::editor::polygon_value(points);
        },
        e);
  }
  bool scale(int wheel, const EditorInput::Actions &a, std::string &e) {
    if (a.control) {
      userZoom = std::clamp(userZoom + wheel * .05, -1.2, .5);
      return true;
    }
    double d = wheel * (a.shift ? .01 : .1);
    if (auto *n = model.selected()) {
      Vec2 s;
      ucg::editor::read_vector(n->array[1], s);
      if (!a.lockX)
        s.x += d;
      if (!a.lockY)
        s.y += d;
      return model.set_scale(s, e);
    }
    if (tab == 3) {
      if (!a.lockX)
        decoScaleX = std::clamp(decoScaleX + d, .001, 100.);
      if (!a.lockY)
        decoScaleY = std::clamp(decoScaleY + d, .001, 100.);
    }
    return true;
  }
};
} // namespace EditorController
