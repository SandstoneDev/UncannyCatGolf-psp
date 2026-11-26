#pragma once
// PC level JSON and editor model. C++17, no PSP dependency, exceptions or RTTI.
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cctype>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>
namespace ucg {
namespace editor {
namespace detail {
inline double ascii_number(const std::string &s) {
    const char *decimal = std::localeconv()->decimal_point;
    const char *input = s.c_str();
    std::string localized;
    if (decimal && std::strcmp(decimal, ".") != 0) {
        localized = s;
        size_t at = localized.find('.');
        if (at != std::string::npos)
            localized.replace(at, 1, decimal);
        input = localized.c_str();
    }
    char *end = nullptr;
    double result = std::strtod(input, &end);
    return end && *end == 0 ? result : std::numeric_limits<double>::quiet_NaN();
}
} // namespace detail
struct Limits {
    size_t max_file_bytes = 1024 * 1024, max_values = 65536, max_depth = 32,
           max_string_bytes = 64 * 1024;
};
struct Value {
    enum class Kind { Null, Bool, Number, String, Array, Object };
    Kind kind = Kind::Null;
    std::string text;
    bool boolean = false;
    std::vector<Value> array;
    std::map<std::string, Value> object;
    Value() = default;
    Value(const char *s) : kind(Kind::String), text(s ? s : "") {}
    Value(const std::string &s) : kind(Kind::String), text(s) {}
    Value(bool b) : kind(Kind::Bool), boolean(b) {}
    static Value number_text(const std::string &s) {
        Value v;
        v.kind = Kind::Number;
        v.text = s;
        return v;
    }
    static Value number(double d) {
        char b[64];
        std::snprintf(b, sizeof(b), "%.17g", d);
        for (char *c = b; *c; ++c)
            if (*c == ',')
                *c = '.';
        return number_text(b);
    }
    static Value list() {
        Value v;
        v.kind = Kind::Array;
        return v;
    }
    static Value dict() {
        Value v;
        v.kind = Kind::Object;
        return v;
    }
    const Value *find(const std::string &k) const {
        auto i = object.find(k);
        return i == object.end() ? nullptr : &i->second;
    }
    Value *find(const std::string &k) {
        auto i = object.find(k);
        return i == object.end() ? nullptr : &i->second;
    }
    Value &operator[](const std::string &k) {
        if (kind == Kind::Null)
            kind = Kind::Object;
        return object[k];
    }
    double numeric(double fallback = 0) const {
        return kind == Kind::Number ? detail::ascii_number(text) : fallback;
    }
};
namespace detail {
inline void append_utf8(std::string &s, unsigned c) {
    if (c < 128)
        s += char(c);
    else if (c < 2048) {
        s += char(192 | (c >> 6));
        s += char(128 | (c & 63));
    } else if (c < 65536) {
        s += char(224 | (c >> 12));
        s += char(128 | ((c >> 6) & 63));
        s += char(128 | (c & 63));
    } else {
        s += char(240 | (c >> 18));
        s += char(128 | ((c >> 12) & 63));
        s += char(128 | ((c >> 6) & 63));
        s += char(128 | (c & 63));
    }
}
inline bool valid_utf8(const std::string &s) {
    for (size_t i = 0; i < s.size();) {
        unsigned a = (unsigned char)s[i++], c = 0, n = 0, min = 0;
        if (a < 128)
            continue;
        if (a >= 194 && a <= 223) {
            c = a & 31;
            n = 1;
            min = 128;
        } else if (a >= 224 && a <= 239) {
            c = a & 15;
            n = 2;
            min = 2048;
        } else if (a >= 240 && a <= 244) {
            c = a & 7;
            n = 3;
            min = 65536;
        } else
            return false;
        if (i + n > s.size())
            return false;
        while (n--) {
            unsigned b = (unsigned char)s[i++];
            if ((b & 192) != 128)
                return false;
            c = (c << 6) | (b & 63);
        }
        if (c < min || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
            return false;
    }
    return true;
}
struct Parser {
    const std::string &s;
    Limits limits;
    size_t p = 0, count = 0;
    std::string error;
    bool fail(const char *why) {
        if (error.empty())
            error = std::string(why) + " at byte " + std::to_string(p);
        return false;
    }
    void ws() {
        while (p < s.size() && (s[p] == ' ' || s[p] == '\t' || s[p] == '\r' || s[p] == '\n'))
            ++p;
    }
    bool hex(unsigned &c) {
        c = 0;
        for (int i = 0; i < 4; ++i) {
            if (p == s.size())
                return fail("truncated unicode escape");
            char a = s[p++];
            unsigned d = a >= '0' && a <= '9'   ? a - '0'
                         : a >= 'a' && a <= 'f' ? a - 'a' + 10
                         : a >= 'A' && a <= 'F' ? a - 'A' + 10
                                                : 16;
            if (d > 15)
                return fail("invalid unicode escape");
            c = (c << 4) | d;
        }
        return true;
    }
    bool string(std::string &out) {
        if (p == s.size() || s[p++] != '"')
            return fail("expected string");
        out.clear();
        while (p < s.size()) {
            unsigned char c = (unsigned char)s[p++];
            if (c == '"') {
                if (!valid_utf8(out))
                    return fail("invalid UTF-8");
                return true;
            }
            if (c < 32)
                return fail("control character in string");
            if (c == '\\') {
                if (p == s.size())
                    return fail("truncated escape");
                char e = s[p++];
                switch (e) {
                case '"':
                    out += '"';
                    break;
                case '\\':
                    out += '\\';
                    break;
                case '/':
                    out += '/';
                    break;
                case 'b':
                    out += '\b';
                    break;
                case 'f':
                    out += '\f';
                    break;
                case 'n':
                    out += '\n';
                    break;
                case 'r':
                    out += '\r';
                    break;
                case 't':
                    out += '\t';
                    break;
                case 'u': {
                    unsigned u;
                    if (!hex(u))
                        return false;
                    if (u >= 0xd800 && u <= 0xdbff) {
                        if (p + 2 > s.size() || s[p++] != '\\' || s[p++] != 'u')
                            return fail("missing low surrogate");
                        unsigned lo;
                        if (!hex(lo))
                            return false;
                        if (lo < 0xdc00 || lo > 0xdfff)
                            return fail("invalid low surrogate");
                        u = 0x10000 + ((u - 0xd800) << 10) + (lo - 0xdc00);
                    } else if (u >= 0xdc00 && u <= 0xdfff)
                        return fail("unpaired low surrogate");
                    append_utf8(out, u);
                    break;
                }
                default:
                    return fail("invalid escape");
                }
            } else
                out += char(c);
            if (out.size() > limits.max_string_bytes)
                return fail("string size limit");
        }
        return fail("unterminated string");
    }
    bool value(Value &v, size_t depth) {
        ws();
        if (++count > limits.max_values)
            return fail("value count limit");
        if (depth > limits.max_depth)
            return fail("nesting limit");
        if (p == s.size())
            return fail("missing value");
        char c = s[p];
        if (c == '"') {
            v.kind = Value::Kind::String;
            return string(v.text);
        }
        if (c == '{' || c == '[') {
            bool obj = c == '{';
            ++p;
            v = obj ? Value::dict() : Value::list();
            ws();
            if (p < s.size() && s[p] == (obj ? '}' : ']')) {
                ++p;
                return true;
            }
            for (;;) {
                std::string key;
                if (obj) {
                    ws();
                    if (!string(key))
                        return false;
                    ws();
                    if (p == s.size() || s[p++] != ':')
                        return fail("expected colon");
                    if (v.object.find(key) != v.object.end())
                        return fail("duplicate object key");
                }
                Value child;
                if (!value(child, depth + 1))
                    return false;
                if (obj)
                    v.object.emplace(key, std::move(child));
                else
                    v.array.push_back(std::move(child));
                ws();
                if (p == s.size())
                    return fail("unterminated container");
                char end = s[p++];
                if (end == (obj ? '}' : ']'))
                    return true;
                if (end != ',')
                    return fail("expected comma");
            }
        }
        for (const auto &literal :
             std::vector<std::pair<std::string, int>>{{"true", 1}, {"false", 2}, {"null", 0}}) {
            if (s.compare(p, literal.first.size(), literal.first) == 0) {
                p += literal.first.size();
                v = literal.second ? Value(literal.second == 1) : Value();
                return true;
            }
        }
        size_t start = p;
        if (c == '-')
            ++p;
        if (p == s.size())
            return fail("invalid number");
        if (s[p] == '0')
            ++p;
        else {
            if (s[p] < '1' || s[p] > '9')
                return fail("invalid value");
            while (p < s.size() && s[p] >= '0' && s[p] <= '9')
                ++p;
        }
        if (p < s.size() && s[p] == '.') {
            ++p;
            size_t z = p;
            while (p < s.size() && s[p] >= '0' && s[p] <= '9')
                ++p;
            if (p == z)
                return fail("fraction missing digits");
        }
        if (p < s.size() && (s[p] == 'e' || s[p] == 'E')) {
            ++p;
            if (p < s.size() && (s[p] == '+' || s[p] == '-'))
                ++p;
            size_t z = p;
            while (p < s.size() && s[p] >= '0' && s[p] <= '9')
                ++p;
            if (p == z)
                return fail("exponent missing digits");
        }
        v = Value::number_text(s.substr(start, p - start));
        if (!std::isfinite(v.numeric()))
            return fail("nonfinite number");
        return true;
    }
};
inline void quoted(std::string &out, const std::string &s) {
    out += '"';
    const char *h = "0123456789abcdef";
    for (unsigned char c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c < 32) {
                out += "\\u00";
                out += h[c >> 4];
                out += h[c & 15];
            } else
                out += char(c);
        }
    }
    out += '"';
}
inline void emit(std::string &out, const Value &v) {
    switch (v.kind) {
    case Value::Kind::Null:
        out += "null";
        break;
    case Value::Kind::Bool:
        out += v.boolean ? "true" : "false";
        break;
    case Value::Kind::Number:
        out += v.text;
        break;
    case Value::Kind::String:
        quoted(out, v.text);
        break;
    case Value::Kind::Array: {
        out += '[';
        bool first = true;
        for (const auto &i : v.array) {
            if (!first)
                out += ',';
            first = false;
            emit(out, i);
        }
        out += ']';
        break;
    }
    case Value::Kind::Object: {
        out += '{';
        bool first = true;
        for (const auto &i : v.object) {
            if (!first)
                out += ',';
            first = false;
            quoted(out, i.first);
            out += ':';
            emit(out, i.second);
        }
        out += '}';
        break;
    }
    }
}
} // namespace detail
inline bool parse_json(const std::string &s, Value &out, std::string &error, Limits limits = {}) {
    if (s.size() > limits.max_file_bytes) {
        error = "file size limit";
        return false;
    }
    detail::Parser p{s, limits, 0, 0, {}};
    if (s.size() >= 3 && (unsigned char)s[0] == 239 && (unsigned char)s[1] == 187 &&
        (unsigned char)s[2] == 191)
        p.p = 3;
    Value candidate;
    if (!p.value(candidate, 0)) {
        error = p.error;
        return false;
    }
    p.ws();
    if (p.p != s.size()) {
        error = "trailing JSON data at byte " + std::to_string(p.p);
        return false;
    }
    out = std::move(candidate);
    error.clear();
    return true;
}
inline std::string write_json(const Value &v) {
    std::string out;
    detail::emit(out, v);
    return out;
}
} // namespace editor
} // namespace ucg

#include "ucg_editor_catalog.hpp"
namespace ucg {
namespace editor {
struct Vec2 {
    double x = 0, y = 0;
};
struct Color {
    double r = 1, g = 1, b = 1, a = 1;
};
inline std::string decimal(double n) { return Value::number(n).text; }
inline Value vector_value(Vec2 p) { return Value("(" + decimal(p.x) + ", " + decimal(p.y) + ")"); }
inline Value color_value(Color c) {
    auto pc = [](double d) { return decimal(std::round(d * 10000.0) / 10000.0); };
    return Value("(" + pc(c.r) + ", " + pc(c.g) + ", " + pc(c.b) + ", " + pc(c.a) + ")");
}
inline bool tuple(const Value &v, std::vector<double> &out, size_t n) {
    if (v.kind != Value::Kind::String)
        return false;
    const auto &s = v.text;
    size_t p = 0;
    auto ws = [&]() {
        while (p < s.size() && std::isspace((unsigned char)s[p]))
            ++p;
    };
    ws();
    if (p == s.size() || s[p++] != '(')
        return false;
    out.clear();
    for (size_t i = 0; i < n; ++i) {
        ws();
        size_t start = p;
        while (p < s.size() && ((s[p] >= '0' && s[p] <= '9') || s[p] == '-' || s[p] == '+' ||
                                s[p] == '.' || s[p] == 'e' || s[p] == 'E'))
            ++p;
        if (start == p)
            return false;
        std::string token = s.substr(start, p - start);
        detail::Parser parser{token, Limits{}, 0, 0, {}};
        Value number;
        if (!parser.value(number, 0) || parser.p != token.size() ||
            number.kind != Value::Kind::Number)
            return false;
        out.push_back(number.numeric());
        ws();
        if (p == s.size() || s[p++] != (i + 1 == n ? ')' : ','))
            return false;
    }
    ws();
    return p == s.size();
}
inline bool read_vector(const Value &v, Vec2 &p) {
    std::vector<double> a;
    if (!tuple(v, a, 2))
        return false;
    p = {a[0], a[1]};
    return true;
}
inline bool read_color(const Value &v, Color &c) {
    std::vector<double> a;
    if (!tuple(v, a, 4))
        return false;
    c = {a[0], a[1], a[2], a[3]};
    return true;
}
inline bool read_polygon(const Value &v, std::vector<Vec2> &points) {
    if (v.kind != Value::Kind::String)
        return false;
    points.clear();
    const auto &s = v.text;
    size_t p = 0;
    auto ws = [&]() {
        while (p < s.size() && std::isspace((unsigned char)s[p]))
            ++p;
    };
    ws();
    if (p == s.size() || s[p++] != '[')
        return false;
    ws();
    if (p < s.size() && s[p] == ']') {
        ++p;
        ws();
        return p == s.size();
    }
    while (p < s.size()) {
        ws();
        size_t end = s.find(')', p);
        if (end == std::string::npos)
            return false;
        Vec2 q;
        if (!read_vector(Value(s.substr(p, end - p + 1)), q))
            return false;
        points.push_back(q);
        p = end + 1;
        ws();
        if (p == s.size())
            return false;
        if (s[p] == ']') {
            ++p;
            ws();
            return p == s.size();
        }
        if (s[p++] != ',')
            return false;
    }
    return false;
}
inline Value polygon_value(const std::vector<Vec2> &points) {
    std::string s = "[";
    for (size_t i = 0; i < points.size(); ++i) {
        if (i)
            s += ", ";
        s += vector_value(points[i]).text;
    }
    return Value(s + "]");
}
inline const char *tile_layer_name(int layer) {
    static const char *keys[] = {"tiles",        "switchTiles1", "switchTiles2",
                                 "switchTiles3", "switchTiles4", "dogtiles"};
    return layer >= 0 && layer < 6 ? keys[layer] : nullptr;
}
namespace detail {
inline bool checked_tree(const Value &v, const Limits &l, size_t &count, size_t depth,
                         std::string &e) {
    if (++count > l.max_values) {
        e = "value count limit";
        return false;
    }
    if (depth > l.max_depth) {
        e = "nesting limit";
        return false;
    }
    if (v.text.size() > l.max_string_bytes) {
        e = "string size limit";
        return false;
    }
    if (v.kind == Value::Kind::String && !valid_utf8(v.text)) {
        e = "invalid UTF-8";
        return false;
    }
    if (v.kind == Value::Kind::Number) {
        Parser p{v.text, l, 0, 0, {}};
        Value n;
        if (!p.value(n, 0) || p.p != v.text.size() || n.kind != Value::Kind::Number) {
            e = "invalid numeric value";
            return false;
        }
    }
    if (v.kind == Value::Kind::Array)
        for (const auto &x : v.array)
            if (!checked_tree(x, l, count, depth + 1, e))
                return false;
    if (v.kind == Value::Kind::Object)
        for (const auto &x : v.object) {
            if (x.first.size() > l.max_string_bytes || !valid_utf8(x.first)) {
                e = "invalid object key";
                return false;
            }
            if (!checked_tree(x.second, l, count, depth + 1, e))
                return false;
        }
    return true;
}
inline bool integer(const Value &v, double min, double max) {
    return v.kind == Value::Kind::Number && std::isfinite(v.numeric()) && v.numeric() >= min &&
           v.numeric() <= max && std::floor(v.numeric()) == v.numeric();
}
inline bool cell_vector(const Value &v, Vec2 &p) {
    return read_vector(v, p) && std::floor(p.x) == p.x && std::floor(p.y) == p.y &&
           std::abs(p.x) <= 1000000 && std::abs(p.y) <= 1000000;
}
} // namespace detail
struct AllocationBudget {
    size_t total_bytes = 0, largest_block = 0;
    size_t large_blocks = 0;
    static constexpr size_t chunk_bytes = 64 * 1024;
};
struct MemoryUsage {
    // Requested C++ payload, including the inline Document; malloc headers are separate.
    size_t resident_bytes = 0, encoded_bytes = 0, allocations = 0;
    size_t largest_block = 0, copy_largest_block = 0, parse_largest_block = 0;
    size_t copy_large_blocks = 0, parse_large_blocks = 0;
    bool overflow = false;
};
namespace detail {
inline bool size_add(size_t &total, size_t n) {
    if (n > std::numeric_limits<size_t>::max() - total) {
        total = std::numeric_limits<size_t>::max();
        return false;
    }
    total += n;
    return true;
}
inline bool size_product(size_t a, size_t b, size_t &out) {
    if (b && a > std::numeric_limits<size_t>::max() / b) {
        out = std::numeric_limits<size_t>::max();
        return false;
    }
    out = a * b;
    return true;
}
inline bool quoted_size(const std::string &text, size_t &out) {
    if (!size_add(out, 2))
        return false;
    for (unsigned char c : text) {
        const size_t n = c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t' ? 2
                         : c < 32                                                     ? 6
                                                                                      : 1;
        if (!size_add(out, n))
            return false;
    }
    return true;
}
inline bool encoded_size(const Value &v, size_t &out, size_t depth, size_t max_depth) {
    if (depth > max_depth)
        return false;
    switch (v.kind) {
    case Value::Kind::Null:
        return size_add(out, 4);
    case Value::Kind::Bool:
        return size_add(out, v.boolean ? 4 : 5);
    case Value::Kind::Number:
        return size_add(out, v.text.size());
    case Value::Kind::String:
        return quoted_size(v.text, out);
    case Value::Kind::Array:
        if (!size_add(out, 2) || (!v.array.empty() && !size_add(out, v.array.size() - 1)))
            return false;
        for (const auto &child : v.array)
            if (!encoded_size(child, out, depth + 1, max_depth))
                return false;
        return true;
    case Value::Kind::Object:
        if (!size_add(out, 2) || (!v.object.empty() && !size_add(out, v.object.size() - 1)))
            return false;
        for (const auto &item : v.object)
            if (!quoted_size(item.first, out) || !size_add(out, 1) ||
                !encoded_size(item.second, out, depth + 1, max_depth))
                return false;
        return true;
    }
    return false;
}
inline bool string_memory(const std::string &text, MemoryUsage &out) {
    if (text.size() > std::string().capacity()) {
        size_t copy = text.size();
        if (!size_add(copy, 1))
            return false;
        out.copy_largest_block = std::max(out.copy_largest_block, copy);
        if (copy > AllocationBudget::chunk_bytes && !size_add(out.copy_large_blocks, 1))
            return false;
    }
    // libstdc++'s SSO buffer belongs to the string object, not a heap allocation.
    const uintptr_t data = reinterpret_cast<uintptr_t>(text.data());
    const uintptr_t object = reinterpret_cast<uintptr_t>(&text);
    if (data >= object && data - object < sizeof(text))
        return true;
    size_t block = text.capacity();
    if (!size_add(block, 1))
        return false;
    out.largest_block = std::max(out.largest_block, block);
    return size_add(out.resident_bytes, text.capacity()) && size_add(out.resident_bytes, 1) &&
           size_add(out.allocations, 1);
}
inline bool value_memory(const Value &v, MemoryUsage &out, size_t depth, size_t max_depth) {
    if (depth > max_depth || !string_memory(v.text, out))
        return false;
    size_t bytes = 0;
    if (!size_product(v.array.capacity(), sizeof(Value), bytes) ||
        !size_add(out.resident_bytes, bytes) ||
        (v.array.capacity() && !size_add(out.allocations, 1)))
        return false;
    out.largest_block = std::max(out.largest_block, bytes);
    if (!size_product(v.array.size(), sizeof(Value), bytes))
        return false;
    out.copy_largest_block = std::max(out.copy_largest_block, bytes);
    if (bytes > AllocationBudget::chunk_bytes && !size_add(out.copy_large_blocks, 1))
        return false;
    // GCC libstdc++ RB-tree node: color + three pointers, then aligned pair storage.
    // This is verified against the host and PSP compiled ABI in the focused test.
    using Pair = std::pair<const std::string, Value>;
    constexpr size_t base = 4 * sizeof(void *);
    constexpr size_t node =
        (base + alignof(Pair) - 1) / alignof(Pair) * alignof(Pair) + sizeof(Pair);
    if (!size_product(v.object.size(), node, bytes) || !size_add(out.resident_bytes, bytes) ||
        !size_add(out.allocations, v.object.size()))
        return false;
    if (!v.object.empty()) {
        out.largest_block = std::max(out.largest_block, node);
        out.copy_largest_block = std::max(out.copy_largest_block, node);
        if (node > AllocationBudget::chunk_bytes &&
            !size_add(out.copy_large_blocks, v.object.size()))
            return false;
    }
    // Include inactive containers too: they still own allocations even if JSON omits them.
    for (const auto &child : v.array)
        if (!value_memory(child, out, depth + 1, max_depth))
            return false;
    for (const auto &item : v.object)
        if (!string_memory(item.first, out) ||
            !value_memory(item.second, out, depth + 1, max_depth))
            return false;
    return true;
}
inline bool parse_string_block(size_t length, size_t &largest, size_t &large_blocks) {
    size_t capacity = std::string().capacity();
    if (length <= capacity)
        return true;
    if (!capacity)
        capacity = 1;
    while (capacity < length)
        if (!size_product(capacity, 2, capacity))
            return false;
    const size_t old_capacity = capacity / 2;
    if (!size_add(capacity, 1))
        return false;
    largest = std::max(largest, capacity);
    if (capacity > AllocationBudget::chunk_bytes && !size_add(large_blocks, 1))
        return false;
    if (old_capacity >= AllocationBudget::chunk_bytes && !size_add(large_blocks, 1))
        return false;
    return true;
}
inline bool parse_largest(const Value &v, size_t &largest, size_t &large_blocks, size_t depth,
                          size_t max_depth) {
    if (depth > max_depth)
        return false;
    if (v.kind == Value::Kind::String)
        return parse_string_block(v.text.size(), largest, large_blocks);
    if (v.kind == Value::Kind::Number) {
        if (v.text.size() > std::string().capacity()) {
            size_t bytes = v.text.size();
            if (!size_add(bytes, 1))
                return false;
            largest = std::max(largest, bytes);
            // A numeric token temporarily owns both substring and copied spelling.
            if (bytes > AllocationBudget::chunk_bytes && !size_add(large_blocks, 2))
                return false;
        }
    } else if (v.kind == Value::Kind::Array) {
        if (!v.array.empty()) {
            size_t capacity = 1, bytes = 0;
            while (capacity < v.array.size())
                if (!size_product(capacity, 2, capacity))
                    return false;
            if (!size_product(capacity, sizeof(Value), bytes))
                return false;
            largest = std::max(largest, bytes);
            if (bytes > AllocationBudget::chunk_bytes && !size_add(large_blocks, 1))
                return false;
            if (bytes / 2 > AllocationBudget::chunk_bytes && !size_add(large_blocks, 1))
                return false;
        }
        for (const auto &child : v.array)
            if (!parse_largest(child, largest, large_blocks, depth + 1, max_depth))
                return false;
    } else if (v.kind == Value::Kind::Object) {
        using Pair = std::pair<const std::string, Value>;
        constexpr size_t base = 4 * sizeof(void *);
        constexpr size_t node =
            (base + alignof(Pair) - 1) / alignof(Pair) * alignof(Pair) + sizeof(Pair);
        if (!v.object.empty())
            largest = std::max(largest, node);
        for (const auto &item : v.object) {
            if (!parse_string_block(item.first.size(), largest, large_blocks) ||
                !parse_largest(item.second, largest, large_blocks, depth + 1, max_depth))
                return false;
            if (item.first.size() >= AllocationBudget::chunk_bytes && !size_add(large_blocks, 1))
                return false;
        }
    }
    return true;
}
} // namespace detail
struct Document {
    Value root = Value::dict();
    std::string filename;
    Limits limits;
    static Document fresh() {
        Document d;
        std::string e;
        for (const auto &m : metadata_definitions) {
            Value v;
            parse_json(m.default_json, v, e);
            d.root[m.key] = std::move(v);
        }
        return d;
    }
    MemoryUsage memory_usage() const {
        MemoryUsage out;
        out.resident_bytes = sizeof(Document);
        out.overflow = !detail::string_memory(filename, out) ||
                       !detail::value_memory(root, out, 0, limits.max_depth) ||
                       !detail::encoded_size(root, out.encoded_bytes, 0, limits.max_depth);
        if (!out.overflow) {
            if (filename.size() > std::string().capacity())
                out.parse_largest_block = filename.size() + 1;
            if (out.parse_largest_block > AllocationBudget::chunk_bytes)
                out.parse_large_blocks = 1;
            out.overflow = !detail::parse_largest(root, out.parse_largest_block,
                                                  out.parse_large_blocks, 0, limits.max_depth);
        }
        return out;
    }
    std::string serialize() const {
        std::string out;
        size_t bytes = 0;
        if (detail::encoded_size(root, bytes, 0, limits.max_depth))
            out.reserve(bytes);
        detail::emit(out, root);
        return out;
    }
    bool validate(std::string &e) const {
        if (root.kind != Value::Kind::Object) {
            e = "level root must be object";
            return false;
        }
        size_t count = 0;
        if (!detail::checked_tree(root, limits, count, 0, e))
            return false;
        for (const auto &m : metadata_definitions) {
            const Value *v = root.find(m.key);
            if (!v)
                continue;
            Value prototype;
            std::string ignored;
            parse_json(m.default_json, prototype, ignored);
            if (v->kind != prototype.kind) {
                e = std::string(m.key) + ": wrong JSON type";
                return false;
            }
        }
        for (const char *key : {"light", "tBright", "tNormal", "tDark", "tExtra"}) {
            const auto *v = root.find(key);
            Color c;
            if (v && !read_color(*v, c)) {
                e = std::string(key) + ": invalid Color string";
                return false;
            }
        }
        for (int layer = 0; layer < 6; ++layer) {
            const auto *v = root.find(tile_layer_name(layer));
            if (!v)
                continue;
            if (v->kind != Value::Kind::Array) {
                e = "tile layer must be array";
                return false;
            }
            for (const auto &cell : v->array) {
                Vec2 p, a;
                if (cell.kind != Value::Kind::Array || cell.array.size() < 2 ||
                    !detail::cell_vector(cell.array[0], p) ||
                    !detail::cell_vector(cell.array[1], a) || a.x < 0 || a.y < 0) {
                    e = "invalid tile record";
                    return false;
                }
            }
        }
        const auto *objects = root.find("nodes");
        if (objects) {
            if (objects->kind != Value::Kind::Array) {
                e = "nodes must be array";
                return false;
            }
            for (const auto &n : objects->array) {
                Vec2 p, s;
                Color c;
                if (n.kind != Value::Kind::Array || n.array.size() < 7 ||
                    !read_vector(n.array[0], p) || !read_vector(n.array[1], s) ||
                    !read_color(n.array[2], c) || n.array[4].kind != Value::Kind::String ||
                    n.array[4].text.empty() || n.array[5].kind != Value::Kind::String ||
                    !detail::integer(n.array[6], -2147483648.0, 2147483647.0)) {
                    e = "invalid node record";
                    return false;
                }
                if (n.array[5].text == "sand") {
                    std::vector<Vec2> polygon;
                    if (!read_polygon(n.array[3], polygon)) {
                        e = "invalid polygon string";
                        return false;
                    }
                } else if (n.array[3].kind != Value::Kind::Object) {
                    e = "node variables must be object";
                    return false;
                }
            }
        }
        const auto *deco = root.find("deco");
        if (deco) {
            if (deco->kind != Value::Kind::Array) {
                e = "deco must be array";
                return false;
            }
            for (const auto &n : deco->array) {
                Vec2 p, s;
                if (n.kind != Value::Kind::Array || n.array.size() < 10 ||
                    !read_vector(n.array[0], p) || !read_vector(n.array[1], s) ||
                    n.array[2].kind != Value::Kind::Number ||
                    !detail::integer(n.array[3], -4096, 4096) ||
                    !detail::integer(n.array[4], 0, 2147483647) ||
                    n.array[5].kind != Value::Kind::Number ||
                    !detail::integer(n.array[6], 1, 4096) ||
                    !detail::integer(n.array[7], 1, 4096) || n.array[8].kind != Value::Kind::Bool ||
                    (n.array[9].kind != Value::Kind::String &&
                     n.array[9].kind != Value::Kind::Null) ||
                    (n.array.size() > 10 && n.array[10].kind != Value::Kind::String)) {
                    e = "invalid decoration record";
                    return false;
                }
            }
        }
        size_t encoded_bytes = 0;
        if (!detail::encoded_size(root, encoded_bytes, 0, limits.max_depth) ||
            encoded_bytes > limits.max_file_bytes) {
            e = "file size limit";
            return false;
        }
        e.clear();
        return true;
    }
    bool load(const std::string &s, std::string &e) {
        Document candidate;
        candidate.limits = limits;
        candidate.filename = filename;
        if (!parse_json(s, candidate.root, e, limits) || !candidate.validate(e))
            return false;
        *this = std::move(candidate);
        return true;
    }
    std::vector<Value> &array_field(const char *key) {
        Value &v = root[key];
        if (v.kind == Value::Kind::Null)
            v = Value::list();
        return v.array;
    }
    std::vector<Value> &nodes() { return array_field("nodes"); }
    std::vector<Value> &decorations() { return array_field("deco"); }
    std::vector<Value> &tiles(int layer) {
        return array_field(tile_layer_name(layer) ? tile_layer_name(layer) : "__invalid_layer");
    }
    const std::vector<Value> &array_field(const char *key) const {
        static const std::vector<Value> empty;
        const auto *v = root.find(key);
        return v ? v->array : empty;
    }
    const std::vector<Value> &nodes() const { return array_field("nodes"); }
    const std::vector<Value> &decorations() const { return array_field("deco"); }
    const std::vector<Value> &tiles(int layer) const {
        return array_field(tile_layer_name(layer) ? tile_layer_name(layer) : "__invalid_layer");
    }
    bool paint(int layer, int x, int y, int ax, int ay, std::string &e) {
        if (!tile_layer_name(layer) || std::abs(double(x)) > 1000000 ||
            std::abs(double(y)) > 1000000 || ax < 0 || ay < 0 || ax > 1000000 || ay > 1000000) {
            e = "tile coordinate/layer out of range";
            return false;
        }
        auto &t = tiles(layer);
        for (auto &cell : t) {
            Vec2 p;
            if (read_vector(cell.array[0], p) && p.x == x && p.y == y) {
                cell.array[1] = vector_value({double(ax), double(ay)});
                e.clear();
                return true;
            }
        }
        Value cell = Value::list();
        cell.array = {vector_value({double(x), double(y)}), vector_value({double(ax), double(ay)})};
        t.push_back(std::move(cell));
        if (!validate(e)) {
            t.pop_back();
            return false;
        }
        return true;
    }
    bool erase_tile(int layer, int x, int y, std::string &e) {
        if (!tile_layer_name(layer)) {
            e = "invalid tile layer";
            return false;
        }
        auto &t = tiles(layer);
        t.erase(std::remove_if(t.begin(), t.end(),
                               [&](const Value &n) {
                                   Vec2 p;
                                   return read_vector(n.array[0], p) && p.x == x && p.y == y;
                               }),
                t.end());
        e.clear();
        return true;
    }
    bool shift_tiles(int dx, int dy, std::string &e) {
        for (int l = 0; l < 6; ++l)
            for (const auto &n : tiles(l)) {
                Vec2 p;
                read_vector(n.array[0], p);
                if (std::abs(p.x + dx) > 1000000 || std::abs(p.y + dy) > 1000000) {
                    e = "shift exceeds tile coordinate limit";
                    return false;
                }
            }
        for (int l = 0; l < 6; ++l)
            for (auto &n : tiles(l)) {
                Vec2 p;
                read_vector(n.array[0], p);
                n.array[0] = vector_value({p.x + dx, p.y + dy});
            }
        for (const char *key : {"nodes", "deco"})
            for (auto &n : array_field(key)) {
                Vec2 p;
                read_vector(n.array[0], p);
                n.array[0] = vector_value({p.x + double(dx) * 16, p.y + double(dy) * 16});
            }
        e.clear();
        return true;
    }
    bool paint_stroke(int layer, int x0, int y0, int x1, int y1, int ax, int ay, std::string &e) {
        if (std::abs(double(x1) - x0) > 4096 || std::abs(double(y1) - y0) > 4096) {
            e = "stroke length limit";
            return false;
        }
        Document candidate = *this;
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -std::abs(y1 - y0),
            sy = y0 < y1 ? 1 : -1, err = dx + dy;
        for (;;) {
            if (!candidate.paint(layer, x0, y0, ax, ay, e))
                return false;
            if (x0 == x1 && y0 == y1)
                break;
            int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
        *this = std::move(candidate);
        return true;
    }
};
} // namespace editor
} // namespace ucg

#include <zlib.h>
namespace ucg {
namespace editor {
inline bool make_object(int id, Vec2 position, Value &out, std::string &e) {
    if (id < 0 || size_t(id) >= object_definition_count) {
        e = "unknown object definition";
        return false;
    }
    const auto &def = object_definitions[id];
    Value vars;
    if (!parse_json(def.defaults_json, vars, e))
        return false;
    Value n = Value::list();
    n.array = {vector_value(position),
               vector_value({1, 1}),
               color_value({1, 1, 1, 1}),
               vars,
               Value(def.scene_path),
               Value(def.polygon ? "sand" : "obj"),
               Value::number(def.polygon ? 0 : -1)};
    if (def.polygon)
        n.array[3] = polygon_value({{-24, -24},
                                    {0, -24},
                                    {24, -24},
                                    {24, -0.0403981},
                                    {24, 24},
                                    {0, 24},
                                    {-24, 24},
                                    {-24, -0.0403981}});
    if (id == 8 || id == 9)
        n.array[2] = color_value({1, 238.0 / 255, 140.0 / 255, 1});
    out = std::move(n);
    e.clear();
    return true;
}
struct Snapshot {
    std::vector<unsigned char> bytes;
    size_t raw_size = 0;
    // Transient parse budget for this state; never written into the user's JSON.
    size_t restore_memory_bytes = 0;
    size_t restore_largest_block = 0;
    size_t restore_large_blocks = 0;
};
inline bool snapshot(const Document &d, Snapshot &out, std::string &e) {
    const auto usage = d.memory_usage();
    if (usage.overflow) {
        e = "document memory size overflow";
        return false;
    }
    std::string raw = d.serialize();
    if (raw.size() > d.limits.max_file_bytes) {
        e = "snapshot file size limit";
        return false;
    }
    Snapshot s;
    s.bytes.resize(size_t(compressBound(uLong(raw.size()))));
    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef *>(&raw[0]);
    stream.avail_in = uInt(raw.size());
    stream.next_out = s.bytes.data();
    stream.avail_out = uInt(s.bytes.size());
    if (deflateInit2(&stream, Z_BEST_SPEED, Z_DEFLATED, 12, 3, Z_DEFAULT_STRATEGY) != Z_OK) {
        e = "history compressor allocation failed";
        return false;
    }
    int result = deflate(&stream, Z_FINISH);
    size_t len = size_t(stream.total_out);
    deflateEnd(&stream);
    if (result != Z_STREAM_END) {
        e = "history compression failed";
        return false;
    }
    std::vector<unsigned char>(s.bytes.begin(), s.bytes.begin() + len).swap(s.bytes);
    s.raw_size = raw.size();
    // Parser growth can briefly keep old + new vector/string buffers alive.
    // Three times the known tree payload bounds that growth; this uses the
    // actual state, rather than max_file_bytes * max_values or max_depth.
    size_t tree = usage.resident_bytes - sizeof(Document), bookkeeping = 0;
    if (!detail::size_product(tree, 3, s.restore_memory_bytes) ||
        !detail::size_product(usage.allocations, 3 * 32, bookkeeping) ||
        !detail::size_add(s.restore_memory_bytes, bookkeeping) ||
        !detail::size_add(s.restore_memory_bytes, raw.size()) ||
        !detail::size_add(s.restore_memory_bytes, 32 * 1024)) {
        e = "history memory size overflow";
        return false;
    }
    s.restore_largest_block = std::max<size_t>(usage.parse_largest_block, 32 * 1024);
    size_t raw_block = raw.size();
    if (!detail::size_add(raw_block, 1)) {
        e = "history contiguous size overflow";
        return false;
    }
    s.restore_largest_block = std::max(s.restore_largest_block, raw_block);
    s.restore_large_blocks = usage.parse_large_blocks;
    if (raw_block > AllocationBudget::chunk_bytes && !detail::size_add(s.restore_large_blocks, 1)) {
        e = "history allocation count overflow";
        return false;
    }
    out = std::move(s);
    return true;
}
struct Editor {
    Document document = Document::fresh();
    size_t max_history_bytes = 2 * 1024 * 1024;
    static constexpr size_t max_history = 100;
    std::vector<Snapshot> history;
    size_t history_point = 0;
    bool selection_deco = false, selection_valid = false;
    size_t selection_index = 0;
    Value clipboard;
    bool clipboard_deco = false, clipboard_valid = false;
    // Optional UI-only reclamation hook. False rejects the operation before
    // changing the live document/history/selection; no renderer dependency.
    std::function<bool(size_t, std::string &)> before_allocation;
    std::function<bool(const AllocationBudget &, std::string &)> before_reserve;
    bool reserve_work(AllocationBudget budget, std::string &e) const {
        if (budget.total_bytes == std::numeric_limits<size_t>::max() ||
            budget.largest_block > budget.total_bytes) {
            e = "editor memory size overflow";
            return false;
        }
        if (budget.largest_block > AllocationBudget::chunk_bytes) {
            budget.large_blocks = std::max<size_t>(budget.large_blocks, 1);
            size_t contiguous = 0;
            if (!detail::size_product(budget.largest_block, budget.large_blocks, contiguous) ||
                contiguous == std::numeric_limits<size_t>::max()) {
                e = "editor contiguous budget overflow";
                return false;
            }
            budget.total_bytes = std::max(budget.total_bytes, contiguous);
        } else {
            if (budget.large_blocks) {
                e = "invalid large allocation count";
                return false;
            }
        }
        const bool ok = before_reserve
                            ? before_reserve(budget, e)
                            : !before_allocation || before_allocation(budget.total_bytes, e);
        if (!ok) {
            if (e.empty())
                e = "insufficient editor working memory; document unchanged";
            return false;
        }
        return true;
    }
    bool reserve_work(size_t bytes, std::string &e) const {
        return reserve_work(AllocationBudget{bytes, bytes}, e);
    }
    bool reserve_copy(const Document &d, std::string &e) const {
        const auto usage = d.memory_usage();
        size_t bytes = usage.resident_bytes - sizeof(Document), bookkeeping = 0;
        if (usage.overflow || !detail::size_product(usage.allocations, 32, bookkeeping) ||
            !detail::size_add(bytes, bookkeeping)) {
            e = "document copy size overflow";
            return false;
        }
        return !bytes ||
               reserve_work(
                   AllocationBudget{bytes, usage.copy_largest_block, usage.copy_large_blocks}, e);
    }
    bool reserve_copy(const Value &v, std::string &e) const {
        MemoryUsage usage;
        size_t bookkeeping = 0;
        if (!detail::value_memory(v, usage, 0, document.limits.max_depth) ||
            !detail::size_product(usage.allocations, 32, bookkeeping) ||
            !detail::size_add(usage.resident_bytes, bookkeeping)) {
            e = "value copy size overflow";
            return false;
        }
        return !usage.resident_bytes ||
               reserve_work(AllocationBudget{usage.resident_bytes, usage.copy_largest_block,
                                             usage.copy_large_blocks},
                            e);
    }
    bool snapshot_budget(const Document &d, AllocationBudget &budget, std::string &e) const {
        const auto usage = d.memory_usage();
        if (usage.overflow || usage.encoded_bytes > d.limits.max_file_bytes ||
            usage.encoded_bytes > std::numeric_limits<uInt>::max()) {
            e = "snapshot size limit";
            return false;
        }
        const size_t bound = size_t(compressBound(uLong(usage.encoded_bytes)));
        if (bound < usage.encoded_bytes) {
            e = "snapshot compressor size overflow";
            return false;
        }
        budget.total_bytes = usage.encoded_bytes;
        // Raw string + compressor output + max(zlib workspace, final shrink copy).
        // 32KiB covers deflate windowBits12/memLevel3 requested workspace.
        const size_t history_block = (history.size() + 1) * sizeof(Snapshot);
        budget.largest_block = std::max<size_t>(
            32 * 1024, std::max(bound, std::max(usage.encoded_bytes + 1, history_block)));
        budget.large_blocks = budget.largest_block > AllocationBudget::chunk_bytes ? 3 : 0;
        return detail::size_add(budget.total_bytes, 1) &&
               detail::size_add(budget.total_bytes, bound) &&
               detail::size_add(budget.total_bytes, std::max<size_t>(32 * 1024, bound)) &&
               detail::size_add(budget.total_bytes, history_block);
    }
    bool snapshot_work(const Document &d, size_t &bytes, std::string &e) const {
        AllocationBudget budget;
        const bool ok = snapshot_budget(d, budget, e);
        bytes = budget.total_bytes;
        return ok;
    }
    bool reserve_snapshot(const Document &d, std::string &e) const {
        AllocationBudget budget;
        if (!snapshot_budget(d, budget, e)) {
            if (e.empty())
                e = "snapshot memory size overflow";
            return false;
        }
        return reserve_work(budget, e);
    }
    Editor() {
        std::string e;
        reset_history(e);
    }
    bool reset_history(std::string &e) {
        Snapshot s;
        if (!document.validate(e) || !reserve_snapshot(document, e) || !snapshot(document, s, e))
            return false;
        if (s.bytes.size() > max_history_bytes) {
            e = "history memory budget exceeded";
            return false;
        }
        std::vector<Snapshot> fresh;
        fresh.push_back(std::move(s));
        history = std::move(fresh);
        history_point = 0;
        selection_valid = false;
        return true;
    }
    size_t history_size() const { return history.size(); }
    size_t history_bytes() const {
        size_t n = 0;
        for (const auto &s : history)
            n += s.bytes.capacity();
        return n;
    }
    bool load(const std::string &text, std::string &e) {
        Document d;
        d.limits = document.limits;
        d.filename = document.filename;
        if (!parse_json(text, d.root, e, d.limits))
            return false;
        return load_document(std::move(d), e);
    }
    // Adopt an already parsed file without serializing and parsing its tree again.
    // Validation and the initial undo snapshot must succeed before any live state changes.
    bool load_document(Document &&d, std::string &e) {
        d.limits = document.limits;
        if (!d.validate(e))
            return false;
        Snapshot s;
        if (!reserve_snapshot(d, e) || !snapshot(d, s, e))
            return false;
        if (s.bytes.size() > max_history_bytes) {
            e = "history memory budget exceeded";
            return false;
        }
        std::vector<Snapshot> fresh;
        fresh.push_back(std::move(s));
        document = std::move(d);
        history = std::move(fresh);
        history_point = 0;
        selection_valid = false;
        clipboard_valid = false;
        clipboard = Value();
        e.clear();
        return true;
    }
    bool mutate(const std::function<void(Document &)> &edit, std::string &e) {
        e.clear();
        const auto usage = document.memory_usage();
        AllocationBudget work;
        size_t copy = 0, overhead = 0;
        if (usage.overflow || !snapshot_budget(document, work, e) ||
            !detail::size_product(usage.allocations, 32, overhead)) {
            e = "editor memory size overflow";
            return false;
        }
        copy = usage.resident_bytes - sizeof(Document);
        if (!detail::size_add(work.total_bytes, copy) ||
            !detail::size_add(work.total_bytes, overhead)) {
            e = "editor memory size overflow";
            return false;
        }
        work.largest_block = std::max(work.largest_block, usage.copy_largest_block);
        if (!detail::size_add(work.large_blocks, usage.copy_large_blocks)) {
            e = "editor allocation count overflow";
            return false;
        }
        if (!reserve_work(work, e))
            return false;
        Document candidate = document;
        edit(candidate);
        if (!e.empty())
            return false;
        if (!candidate.validate(e))
            return false;
        {
            // These comparison strings die before allocating the undo snapshot.
            const auto candidate_usage = candidate.memory_usage();
            size_t compare = candidate_usage.encoded_bytes;
            if (candidate_usage.overflow || !detail::size_add(compare, usage.encoded_bytes) ||
                !detail::size_add(compare, 2 + 2 * 32)) {
                e = "editor comparison size overflow";
                return false;
            }
            const size_t largest = std::max(candidate_usage.encoded_bytes, usage.encoded_bytes) + 1;
            if (!reserve_work(AllocationBudget{compare, largest,
                                               largest > AllocationBudget::chunk_bytes ? size_t(2)
                                                                                       : size_t(0)},
                              e))
                return false;
            if (candidate.serialize() == document.serialize()) {
                e.clear();
                return true;
            }
        }
        Snapshot s;
        if (!reserve_snapshot(candidate, e) || !snapshot(candidate, s, e))
            return false;
        size_t first = (history_point + 2 > max_history) ? 1 : 0, bytes = s.bytes.size();
        for (size_t i = first; i < history.size() && i <= history_point; ++i)
            bytes += history[i].bytes.size();
        if (bytes > max_history_bytes) {
            e = "history memory budget exceeded; document unchanged";
            return false;
        }
        if (history.empty())
            history_point = 0;
        else
            history.resize(history_point + 1);
        if (first && !history.empty())
            history.erase(history.begin());
        history.push_back(std::move(s));
        history_point = history.size() - 1;
        document = std::move(candidate);
        if (selection_valid) {
            const auto &a = selection_deco ? document.decorations() : document.nodes();
            if (selection_index >= a.size())
                selection_valid = false;
        }
        e.clear();
        return true;
    }
    bool restore(size_t point, std::string &e) {
        if (point >= history.size()) {
            e = "history boundary";
            return false;
        }
        const auto &s = history[point];
        if (s.raw_size > document.limits.max_file_bytes) {
            e = "history raw size limit";
            return false;
        }
        size_t restore_bytes = s.restore_memory_bytes;
        if (!restore_bytes || !s.restore_largest_block) {
            // Public history can contain legacy/manually supplied snapshots;
            // without a known tree footprint, reject rather than guess low.
            e = "history working-memory metadata missing";
            return false;
        }
        if (!reserve_work(
                AllocationBudget{restore_bytes, s.restore_largest_block, s.restore_large_blocks},
                e))
            return false;
        std::string raw(s.raw_size, '\0');
        uLongf len = uLongf(raw.size());
        int code = uncompress(reinterpret_cast<Bytef *>(&raw[0]), &len, s.bytes.data(),
                              uLong(s.bytes.size()));
        if (code != Z_OK || size_t(len) != s.raw_size) {
            e = "history decompression failed";
            return false;
        }
        if (!document.load(raw, e))
            return false;
        history_point = point;
        selection_valid = false;
        e.clear();
        return true;
    }
    bool undo(std::string &e) {
        if (history.empty() || history_point == 0) {
            e = "no undo state";
            return false;
        }
        return restore(history_point - 1, e);
    }
    bool redo(std::string &e) {
        if (history.empty() || history_point + 1 >= history.size()) {
            e = "no redo state";
            return false;
        }
        return restore(history_point + 1, e);
    }
    bool select(bool deco, size_t index) {
        const auto &a = deco ? document.decorations() : document.nodes();
        selection_valid = index < a.size();
        selection_deco = deco;
        selection_index = index;
        return selection_valid;
    }
    Value *selected() {
        if (!selection_valid)
            return nullptr;
        auto &a = selection_deco ? document.decorations() : document.nodes();
        if (selection_index >= a.size()) {
            selection_valid = false;
            return nullptr;
        }
        return &a[selection_index];
    }
    const Value *selected() const {
        if (!selection_valid)
            return nullptr;
        const auto &a = selection_deco ? document.decorations() : document.nodes();
        return selection_index < a.size() ? &a[selection_index] : nullptr;
    }
    bool copy_selected(std::string &e) {
        e.clear();
        const auto *n = static_cast<const Editor &>(*this).selected();
        if (!n)
            return false;
        if (!reserve_copy(*n, e))
            return false;
        clipboard = *n;
        clipboard_deco = selection_deco;
        clipboard_valid = true;
        return true;
    }
    bool copy_selected() {
        std::string e;
        return copy_selected(e);
    }
    bool paste(Vec2 pos, std::string &e) {
        if (!clipboard_valid) {
            e = "clipboard empty";
            return false;
        }
        if (!reserve_copy(clipboard, e))
            return false;
        Value item = clipboard;
        item.array[0] = vector_value(pos);
        const bool deco = clipboard_deco;
        size_t index = deco ? document.decorations().size() : document.nodes().size();
        if (!mutate([&](Document &d) { (deco ? d.decorations() : d.nodes()).push_back(item); }, e))
            return false;
        select(deco, index);
        return true;
    }
    bool edit_selected(const std::function<void(Value &)> &edit, std::string &e) {
        if (!selected()) {
            e = "nothing selected";
            return false;
        }
        bool deco = selection_deco;
        size_t i = selection_index;
        return mutate([&](Document &d) { edit((deco ? d.decorations() : d.nodes())[i]); }, e);
    }
    bool erase_selected(std::string &e) {
        if (!selected()) {
            e = "nothing selected";
            return false;
        }
        bool deco = selection_deco;
        size_t i = selection_index;
        if (!mutate(
                [&](Document &d) {
                    auto &a = deco ? d.decorations() : d.nodes();
                    a.erase(a.begin() + i);
                },
                e))
            return false;
        selection_valid = false;
        return true;
    }
    bool set_position(Vec2 p, std::string &e) {
        return edit_selected([&](Value &n) { n.array[0] = vector_value(p); }, e);
    }
    bool set_scale(Vec2 p, std::string &e) {
        return edit_selected([&](Value &n) { n.array[1] = vector_value(p); }, e);
    }
    bool set_color(Color c, std::string &e) {
        if (selection_deco) {
            e = "PC decoration record has no tint field";
            return false;
        }
        return edit_selected([&](Value &n) { n.array[2] = color_value(c); }, e);
    }
    bool set_variable(const std::string &name, const Value &v, std::string &e) {
        const auto *n = selected();
        if (!n || selection_deco || n->array[3].kind != Value::Kind::Object) {
            e = "selection has no variable dictionary";
            return false;
        }
        return edit_selected([&](Value &item) { item.array[3][name] = v; }, e);
    }
    void clear() {
        document = Document::fresh();
        history.clear();
        history_point = 0;
        selection_valid = false;
        clipboard_valid = false;
        clipboard = Value();
        std::string e;
        reset_history(e);
    }
    void clear_memory() {
        document = Document();
        std::vector<Snapshot>().swap(history);
        history_point = 0;
        selection_valid = false;
        clipboard_valid = false;
        clipboard = Value();
    }
};
struct FileOps {
    std::function<int(const char *, const char *)> rename_file =
        [](const char *from, const char *to) { return std::rename(from, to); };
};
namespace detail {
inline bool exists(const std::string &path) {
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f)
        return false;
    std::fclose(f);
    return true;
}
inline std::string unused_path(const std::string &path, const char *suffix) {
    for (unsigned i = 0; i < 1000; ++i) {
        std::string p = path + suffix + std::to_string(i);
        if (!exists(p))
            return p;
    }
    return {};
}
} // namespace detail
inline bool save_atomic(const std::string &path, const Document &d, std::string &e,
                        const FileOps &ops = {}) {
    if (path.empty() || path.find('\0') != std::string::npos) {
        e = "invalid save filename";
        return false;
    }
    if (!d.validate(e))
        return false;
    const std::string raw = d.serialize(), temp = detail::unused_path(path, ".tmp"),
                      backup = detail::unused_path(path, ".bak");
    if (temp.empty() || backup.empty()) {
        e = "no unused transaction filename";
        return false;
    }
    FILE *f = std::fopen(temp.c_str(), "wb");
    if (!f) {
        e = "cannot open temporary save: " + temp;
        return false;
    }
    bool ok = std::fwrite(raw.data(), 1, raw.size(), f) == raw.size();
    if (std::fflush(f) != 0)
        ok = false;
    if (std::fclose(f) != 0)
        ok = false;
    if (!ok) {
        std::remove(temp.c_str());
        e = "temporary save write failed; original preserved";
        return false;
    }
    if (ops.rename_file(temp.c_str(), path.c_str()) == 0) {
        e.clear();
        return true;
    }
    if (!detail::exists(path)) {
        std::remove(temp.c_str());
        e = "save rename failed";
        return false;
    }
    if (ops.rename_file(path.c_str(), backup.c_str()) != 0) {
        std::remove(temp.c_str());
        e = "cannot stage original save; original preserved";
        return false;
    }
    if (ops.rename_file(temp.c_str(), path.c_str()) != 0) {
        bool restored = ops.rename_file(backup.c_str(), path.c_str()) == 0;
        std::remove(temp.c_str());
        e = restored ? "save commit failed; original restored"
                     : "save commit and rollback failed; original retained at " + backup;
        return false;
    }
    std::remove(backup.c_str());
    e.clear();
    return true;
}
inline bool load_file(const std::string &path, Document &d, std::string &e) {
    if (path.empty() || path.find('\0') != std::string::npos) {
        e = "invalid load filename";
        return false;
    }
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) {
        e = "cannot open level file: " + path;
        return false;
    }
    std::string raw;
    char buffer[4096];
    bool ok = true;
    for (;;) {
        size_t n = std::fread(buffer, 1, sizeof(buffer), f);
        if (n) {
            if (raw.size() + n > d.limits.max_file_bytes) {
                e = "file size limit";
                ok = false;
                break;
            }
            raw.append(buffer, n);
        }
        if (n < sizeof(buffer)) {
            if (std::ferror(f)) {
                e = "level read failed";
                ok = false;
            }
            break;
        }
    }
    std::fclose(f);
    if (!ok || !d.load(raw, e))
        return false;
    d.filename = path;
    e.clear();
    return true;
}
} // namespace editor
} // namespace ucg
