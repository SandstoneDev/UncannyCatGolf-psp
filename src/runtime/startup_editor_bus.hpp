#pragma once
// Bounded validating JSON scanner. Only requested top-level setting spans are patched.
// Public transaction/reset behavior matches the original StartupEditorBus bridge.
#include <cerrno>
#include <cmath>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
namespace StartupEditorBus {
enum class Load { Missing, Valid, Invalid };
struct Values {
    unsigned master = 20, music = 20, sounds = 20;
    double sensitivity = 1;
    bool invert = false, buffer = false;
};
inline bool valid(const Values &values) {
    return values.master <= 20 && values.music <= 20 && values.sounds <= 20 &&
        std::isfinite(values.sensitivity) && values.sensitivity >= 0 && values.sensitivity <= 2;
}
namespace detail {
constexpr size_t MaxBytes = 16 * 1024;
inline const char *busKey(unsigned i) {
    return i == 0 ? "master_volume" : i == 1 ? "music_volume" : "sound_volume";
}
inline const char *fieldKey(unsigned i) {
    return i < 3 ? busKey(i) : i == 3 ? "power_sensitivity" : i == 4 ? "invert_shot" : "buffer_shot";
}
struct Span {
    size_t begin = 0, end = 0;
    double number = 1;
    bool present = false;
};
struct Document {
    Span buses[6];
    Document() { buses[4].number = buses[5].number = 0; }
    size_t close = 0;
    bool members = false;
};
inline double asciiNumber(const std::string &s) {
    const char *decimal = std::localeconv()->decimal_point;
    const char *input = s.c_str();
    std::string localized;
    if (decimal && std::strcmp(decimal, ".") != 0) {
        localized = s;
        size_t at = localized.find('.');
        if (at != std::string::npos) localized.replace(at, 1, decimal);
        input = localized.c_str();
    }
    char *end = nullptr;
    const double result = std::strtod(input, &end);
    return end && *end == 0 ? result : std::numeric_limits<double>::quiet_NaN();
}
// Decode one Unicode scalar at a time. No decoded key/value strings are allocated.
struct StringReader {
    static constexpr unsigned End = 0x110000;
    const std::string &s;
    size_t p;
    bool hex(unsigned &c) {
        c = 0;
        for (unsigned i = 0; i < 4; ++i) {
            if (p == s.size()) return false;
            const char a = s[p++];
            const unsigned d = a >= '0' && a <= '9' ? a-'0' : a >= 'a' && a <= 'f' ? a-'a'+10 : a >= 'A' && a <= 'F' ? a-'A'+10 : 16;
            if (d > 15) return false;
            c = (c << 4) | d;
        }
        return true;
    }
    bool next(unsigned &c) {
        if (p == s.size()) return false;
        c = static_cast<unsigned char>(s[p++]);
        if (c == '"') { c = End; return true; }
        if (c < 32) return false;
        if (c == '\\') {
            if (p == s.size()) return false;
            switch (s[p++]) {
                case '"': c = '"'; return true;
                case '\\': c = '\\'; return true;
                case '/': c = '/'; return true;
                case 'b': c = '\b'; return true;
                case 'f': c = '\f'; return true;
                case 'n': c = '\n'; return true;
                case 'r': c = '\r'; return true;
                case 't': c = '\t'; return true;
                case 'u': {
                    unsigned u;
                    if (!hex(u)) return false;
                    if (u >= 0xd800 && u <= 0xdbff) {
                        if (p+2 > s.size() || s[p++] != '\\' || s[p++] != 'u') return false;
                        unsigned lo;
                        if (!hex(lo) || lo < 0xdc00 || lo > 0xdfff) return false;
                        u = 0x10000 + ((u-0xd800) << 10) + lo-0xdc00;
                    } else if (u >= 0xdc00 && u <= 0xdfff) return false;
                    c = u;
                    return true;
                }
                default: return false;
            }
        }
        if (c < 128) return true;
        unsigned n = 0, minimum = 0;
        if (c >= 194 && c <= 223) { c &= 31; n = 1; minimum = 128; }
        else if (c >= 224 && c <= 239) { c &= 15; n = 2; minimum = 2048; }
        else if (c >= 240 && c <= 244) { c &= 7; n = 3; minimum = 65536; }
        else return false;
        if (p+n > s.size()) return false;
        while (n--) {
            const unsigned b = static_cast<unsigned char>(s[p++]);
            if ((b & 192) != 128) return false;
            c = (c << 6) | (b & 63);
        }
        return c >= minimum && c <= 0x10ffff && !(c >= 0xd800 && c <= 0xdfff);
    }
};
struct Scanner {
    enum Kind { Number, Boolean, Other };
    const std::string &s;
    Document &root;
    size_t p = 0, count = 0, used = 0;
    // Only active ancestors' object keys are retained. Nested entries are recycled.
    // The file limit makes a 16-bit raw offset sufficient; value cap bounds entries.
    unsigned short keys[2048];
    Scanner(const std::string &text, Document &out) : s(text), root(out) {}
    void ws() {
        while (p < s.size() && (s[p] == ' ' || s[p] == '\t' || s[p] == '\r' || s[p] == '\n')) ++p;
    }
    bool string() {
        if (p == s.size() || s[p++] != '"') return false;
        StringReader reader{s,p};
        unsigned c;
        while (reader.next(c)) if (c == StringReader::End) { p = reader.p; return true; }
        return false;
    }
    bool equal(size_t a, size_t b) const {
        StringReader left{s,a+1}, right{s,b+1};
        unsigned x,y;
        while (left.next(x) && right.next(y)) {
            if (x != y) return false;
            if (x == StringReader::End) return true;
        }
        return false;
    }
    bool matches(size_t offset, const char *literal) const {
        StringReader reader{s,offset+1};
        unsigned c;
        while (reader.next(c)) {
            if (c == StringReader::End) return *literal == 0;
            if (!*literal || c != static_cast<unsigned char>(*literal++)) return false;
        }
        return false;
    }
    bool value(size_t depth, Kind &kind, double &number) {
        ws();
        kind = Other;
        if (++count > 2048 || depth > 16 || p == s.size()) return false;
        const char c = s[p];
        if (c == '"') return string();
        if (c == '{' || c == '[') {
            const bool object = c == '{';
            const size_t base = used;
            ++p; ws();
            if (p < s.size() && s[p] == (object ? '}' : ']')) {
                if (depth == 0 && object) root.close = p;
                ++p; return true;
            }
            for (;;) {
                int field = -1;
                if (object) {
                    ws();
                    const size_t offset = p;
                    if (!string()) return false;
                    for (size_t i = base; i < used; ++i) {
                        if (equal(keys[i],offset)) return false;
                    }
                    if (used == 2048) return false;
                    keys[used++] = static_cast<unsigned short>(offset);
                    ws();
                    if (p == s.size() || s[p++] != ':') return false;
                    if (depth == 0) {
                        root.members = true;
                        for (unsigned i = 0; i < 6; ++i) if (matches(offset,fieldKey(i))) field = int(i);
                    }
                }
                ws();
                const size_t begin = p;
                Kind childKind;
                double childNumber = 0;
                if (!value(depth+1,childKind,childNumber)) return false;
                if (field >= 0) {
                    if (field >= 4) { if (childKind != Boolean) return false; }
                    else if (childKind != Number || childNumber < 0 || childNumber > (field == 3 ? 2 : 1)) return false;
                    root.buses[field] = Span{begin,p,childNumber,true};
                }
                ws();
                if (p == s.size()) return false;
                const size_t end = p;
                const char close = s[p++];
                if (close == (object ? '}' : ']')) {
                    if (depth == 0 && object) root.close = end;
                    used = base;
                    return true;
                }
                if (close != ',') return false;
            }
        }
        if (s.compare(p,4,"true") == 0) { p += 4; kind = Boolean; number = 1; return true; }
        if (s.compare(p,5,"false") == 0) { p += 5; kind = Boolean; number = 0; return true; }
        if (s.compare(p,4,"null") == 0) { p += 4; return true; }
        const size_t begin = p;
        if (c == '-') ++p;
        if (p == s.size()) return false;
        if (s[p] == '0') ++p;
        else {
            if (s[p] < '1' || s[p] > '9') return false;
            while (p < s.size() && s[p] >= '0' && s[p] <= '9') ++p;
        }
        if (p < s.size() && s[p] == '.') {
            const size_t digits = ++p;
            while (p < s.size() && s[p] >= '0' && s[p] <= '9') ++p;
            if (p == digits) return false;
        }
        if (p < s.size() && (s[p] == 'e' || s[p] == 'E')) {
            ++p;
            if (p < s.size() && (s[p] == '+' || s[p] == '-')) ++p;
            const size_t digits = p;
            while (p < s.size() && s[p] >= '0' && s[p] <= '9') ++p;
            if (p == digits) return false;
        }
        number = asciiNumber(s.substr(begin,p-begin));
        kind = Number;
        return std::isfinite(number);
    }
};
inline bool parse(const std::string &raw, Document &out, std::string &error) {
    if (raw.size() > MaxBytes) { error = "Editor settings size limit"; return false; }
    Document candidate;
    Scanner scan(raw,candidate);
    if (raw.size() >= 3 && static_cast<unsigned char>(raw[0]) == 239 && static_cast<unsigned char>(raw[1]) == 187 && static_cast<unsigned char>(raw[2]) == 191) scan.p = 3;
    scan.ws();
    const bool object = scan.p < raw.size() && raw[scan.p] == '{';
    Scanner::Kind kind;
    double number = 0;
    if (!object || !scan.value(0,kind,number)) { error = "Invalid editor settings JSON or field"; return false; }
    scan.ws();
    if (scan.p != raw.size()) { error = "Trailing editor settings data"; return false; }
    out = candidate;
    error.clear();
    return true;
}
inline unsigned step(const Document &root, unsigned i) {
    return static_cast<unsigned>(std::floor(root.buses[i].number * 20.0 + 0.5));
}
inline std::string volume(unsigned step) {
    if (step == 20) return "1";
    if (step == 0) return "0";
    char text[8];
    std::snprintf(text,sizeof text,"0.%02u",step*5);
    return text;
}
// Only needed when preserving the original spelling would exceed the output cap.
// Matches the old emitter's string representation, without constructing an AST.
inline std::string compact(const std::string &raw) {
    std::string out;
    size_t p = raw.size() >= 3 && static_cast<unsigned char>(raw[0]) == 239 ? 3 : 0;
    while (p < raw.size()) {
        const char c = raw[p++];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        out += c;
        if (c != '"') continue;
        StringReader reader{raw,p};
        unsigned code;
        while (reader.next(code) && code != StringReader::End) {
            const char *escape = code == '"' ? "\\\"" : code == '\\' ? "\\\\" : code == '\n' ? "\\n" : code == '\r' ? "\\r" : code == '\t' ? "\\t" : nullptr;
            if (escape) { out.append(escape,2); continue; }
            char bytes[6];
            size_t n = 0;
            if (code < 32) {
                const char *hex = "0123456789abcdef";
                bytes[0]='\\'; bytes[1]='u'; bytes[2]=bytes[3]='0'; bytes[4]=hex[code>>4]; bytes[5]=hex[code&15]; n=6;
            } else if (code < 128) { bytes[0]=char(code); n=1; }
            else if (code < 2048) { bytes[0]=char(192|(code>>6)); bytes[1]=char(128|(code&63)); n=2; }
            else if (code < 65536) { bytes[0]=char(224|(code>>12)); bytes[1]=char(128|((code>>6)&63)); bytes[2]=char(128|(code&63)); n=3; }
            else { bytes[0]=char(240|(code>>18)); bytes[1]=char(128|((code>>12)&63)); bytes[2]=char(128|((code>>6)&63)); bytes[3]=char(128|(code&63)); n=4; }
            out.append(bytes,n);
        }
        out += '"';
        p = reader.p;
    }
    return out;
}
// Round-trip any finite sensitivity, without quantizing loaded Module values.
inline std::string sensitivity(double value) {
    char text[64];
    std::snprintf(text,sizeof text,"%.*g",std::numeric_limits<double>::max_digits10,value);
    std::string out = text;
    const char *decimal = std::localeconv()->decimal_point;
    if (decimal && std::strcmp(decimal,".") != 0) {
        const size_t at = out.find(decimal);
        if (at != std::string::npos) out.replace(at,std::strlen(decimal),".");
    }
    return out;
}
inline std::string patchFields(const std::string &raw, const Document &root, const std::string *values, unsigned count) {
    std::string out;
    size_t cursor = 0;
    for (unsigned done = 0; done < count; ++done) {
        unsigned next = count;
        for (unsigned i = 0; i < count; ++i) if (root.buses[i].present && root.buses[i].begin >= cursor && (next == count || root.buses[i].begin < root.buses[next].begin)) next = i;
        if (next == count) break;
        const Span &span = root.buses[next];
        out.append(raw,cursor,span.begin-cursor);
        out += values[next];
        cursor = span.end;
    }
    out.append(raw,cursor,root.close-cursor);
    bool comma = root.members;
    for (unsigned i = 0; i < count; ++i) if (!root.buses[i].present) {
        if (comma) out += ',';
        comma = true;
        out += '"'; out += fieldKey(i); out += "\":"; out += values[i];
    }
    out.append(raw,root.close,std::string::npos);
    return out.size() > MaxBytes ? compact(out) : out;
}
inline std::string patch(const std::string &raw, const Document &root, unsigned master, unsigned music, unsigned sounds) {
    const std::string values[3] = {volume(master),volume(music),volume(sounds)};
    return patchFields(raw,root,values,3);
}
inline std::string patchAll(const std::string &raw, const Document &root, const Values &settings) {
    const std::string values[6] = {volume(settings.master),volume(settings.music),volume(settings.sounds),
        sensitivity(settings.sensitivity),settings.invert ? "true" : "false",settings.buffer ? "true" : "false"};
    return patchFields(raw,root,values,6);
}

inline bool validPath(const std::string &path) {
    return !path.empty() && path.find('\0') == std::string::npos;
}
inline Load read(const std::string &path, std::string &raw, bool *oversized = nullptr) {
    if (oversized) *oversized = false;
    if (!validPath(path)) return Load::Invalid;
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) return errno == ENOENT ? Load::Missing : Load::Invalid;
    raw.clear();
    char block[1024];
    bool ok = true;
    for (;;) {
        const size_t n = std::fread(block, 1, sizeof block, f);
        if (n > MaxBytes - raw.size()) { if (oversized) *oversized = true; ok = false; break; }
        raw.append(block, n);
        if (n < sizeof block) { ok = !std::ferror(f); break; }
    }
    if (std::ferror(f)) { ok = false; if (oversized) *oversized = false; }
    if (std::fclose(f) != 0) { ok = false; if (oversized) *oversized = false; }
    return ok ? Load::Valid : Load::Invalid;
}
// This reserved sidecar contains the unchanged old JSON while FAT renames are staged.
inline bool recover(const std::string &path, std::string &error) {
    if (!validPath(path)) return false;
    const std::string backup = path + ".startup-bus-backup";
    std::string old, current;
    const Load saved = read(backup, old);
    if (saved == Load::Missing) return true;
    detail::Document root;
    if (saved != Load::Valid || !parse(old, root, error)) {
        error = "Cannot recover invalid editor bus backup; files preserved";
        return false;
    }
    const Load target = read(path, current);
    if (target == Load::Missing) {
        if (std::rename(backup.c_str(), path.c_str()) == 0) return true;
    } else if (target == Load::Valid && parse(current, root, error)) {
        if (std::remove(backup.c_str()) == 0) return true;
    }
    error = "Cannot recover editor bus transaction; original retained at " + backup;
    return false;
}
inline bool same(const Document &root, unsigned i, unsigned step) {
    return root.buses[i].number == step / 20.0;
}
// Shared checked transaction; caller has validated and prepared the complete raw JSON.
inline bool commit(const std::string &path, Load status, const std::string &raw, std::string &error) {
    if (raw.size() > detail::MaxBytes) { error = "Merged editor settings exceed size limit"; return false; }
    std::string temp;
    int fd = -1;
    for (unsigned i = 0; i < 32; ++i) {
        temp = path + ".startup-bus-tmp" + std::to_string(i);
        fd = ::open(temp.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (fd >= 0 || errno != EEXIST) break;
    }
    if (fd < 0) { error = "Cannot create private editor bus transaction"; return false; }
    FILE *f = ::fdopen(fd, "wb");
    if (!f) {
        ::close(fd);
        std::remove(temp.c_str());
        error = "Cannot open editor bus transaction stream";
        return false;
    }
    bool ok = std::fwrite(raw.data(), 1, raw.size(), f) == raw.size();
    if (std::fflush(f) != 0) ok = false;
    if (std::fclose(f) != 0) ok = false;
    std::string verified;
    detail::Document check;
    if (ok) ok = detail::read(temp, verified) == Load::Valid && verified == raw && detail::parse(verified, check, error);
    if (ok && std::rename(temp.c_str(), path.c_str()) != 0) {
        ok = false;
        const std::string backup = path + ".startup-bus-backup";
        if (status == Load::Valid && std::rename(path.c_str(), backup.c_str()) == 0) {
            if (std::rename(temp.c_str(), path.c_str()) == 0) {
                // Commit is complete. A leftover valid backup is recoverable next time.
                std::remove(backup.c_str());
                ok = true;
            } else if (std::rename(backup.c_str(), path.c_str()) != 0) {
                std::remove(temp.c_str());
                error = "Editor bus commit and rollback failed; original retained at " + backup;
                return false;
            }
        }
    }
    if (!ok) {
        std::remove(temp.c_str());
        error = "Editor bus transaction failed; original preserved";
        return false;
    }
    error.clear();
    return true;
}
} // namespace detail
// Missing/invalid never changes caller values. Valid omitted buses use editor's 1.0 defaults.
inline Load load(const std::string &path, unsigned &master, unsigned &music, unsigned &sounds) {
    std::string raw, error;
    if (!detail::recover(path, error)) return Load::Invalid;
    const Load status = detail::read(path, raw);
    if (status != Load::Valid) return status;
    detail::Document root;
    if (!detail::parse(raw, root, error)) return Load::Invalid;
    master = detail::step(root, 0);
    music = detail::step(root, 1);
    sounds = detail::step(root, 2);
    return Load::Valid;
}
// Missing/invalid leaves the caller unchanged; omitted fields use frozen Module defaults.
inline Load loadAll(const std::string &path, Values &values) {
    std::string raw, error;
    if (!detail::recover(path,error)) return Load::Invalid;
    const Load status = detail::read(path,raw);
    if (status != Load::Valid) return status;
    detail::Document root;
    if (!detail::parse(raw,root,error)) return Load::Invalid;
    values = Values{detail::step(root,0),detail::step(root,1),detail::step(root,2),
        root.buses[3].number,root.buses[4].number != 0,root.buses[5].number != 0};
    return Load::Valid;
}
inline bool mergeAll(const std::string &path, const Values &values, std::string &error) {
    error.clear();
    if (!detail::validPath(path) || !valid(values)) {
        error = "Invalid editor settings path or values";
        return false;
    }
    if (!detail::recover(path,error)) return false;
    std::string previous;
    const Load status = detail::read(path,previous);
    detail::Document root;
    if (status == Load::Invalid || (status == Load::Valid && !detail::parse(previous,root,error))) {
        if (error.empty()) error = "Cannot read existing editor settings; original preserved";
        return false;
    }
    if (status == Load::Valid && detail::same(root,0,values.master) && detail::same(root,1,values.music) && detail::same(root,2,values.sounds) &&
        root.buses[3].number == values.sensitivity && root.buses[4].number == double(values.invert) && root.buses[5].number == double(values.buffer)) return true;
    if (status == Load::Missing) previous = "{}";
    if (status == Load::Missing && !detail::parse(previous,root,error)) return false;
    const std::string raw = detail::patchAll(previous,root,values);
    return detail::commit(path,status,raw,error);
}
// Standalone synchronous transaction. Caller serializes access with the frozen editor.
inline bool merge(const std::string &path, unsigned master, unsigned music, unsigned sounds, std::string &error) {
    error.clear();
    if (!detail::validPath(path) || master > 20 || music > 20 || sounds > 20) {
        error = "Invalid editor bus path or step";
        return false;
    }
    if (!detail::recover(path, error)) return false;
    std::string previous;
    const Load status = detail::read(path, previous);
    detail::Document root;
    if (status == Load::Invalid || (status == Load::Valid && !detail::parse(previous, root, error))) {
        if (error.empty()) error = "Cannot read existing editor settings; original preserved";
        return false;
    }
    if (status == Load::Valid && detail::same(root, 0, master) && detail::same(root, 1, music) && detail::same(root, 2, sounds)) return true;
    if (status == Load::Missing) previous = "{}";
    if (status == Load::Missing && !detail::parse(previous, root, error)) return false;
    const std::string raw = detail::patch(previous, root, master, music, sounds);
    return detail::commit(path,status,raw,error);
}

// Only an explicit fresh intro Start, after staging StartupPending, may call this.
// Ordinary merge remains non-destructive for malformed or unreadable input.
inline bool resetInvalid(const std::string &path, unsigned master, unsigned music, unsigned sounds, std::string &error) {
    error.clear();
    if (!detail::validPath(path) || master > 20 || music > 20 || sounds > 20) {
        error = "Invalid editor bus reset path or step";
        return false;
    }
    const std::string files[2] = {path, path + ".startup-bus-backup"};
    bool invalid[2] = {};
    // Inspect both inputs before any mutation. I/O failures are never data corruption.
    for (unsigned i = 0; i < 2; ++i) {
        std::string raw;
        bool oversized = false;
        const Load status = detail::read(files[i], raw, &oversized);
        if (status == Load::Missing) continue;
        if (status == Load::Invalid && !oversized) {
            error = "Cannot read editor bus reset input; files preserved";
            return false;
        }
        detail::Document root;
        invalid[i] = oversized || !detail::parse(raw, root, error);
    }
    std::string saved[2];
    auto rollback = [&]() {
        for (unsigned i = 0; i < 2; ++i) if (!saved[i].empty()) {
            struct stat info;
            if (::stat(files[i].c_str(), &info) != 0 && errno == ENOENT && std::rename(saved[i].c_str(), files[i].c_str()) == 0) continue;
            error += "; raw configuration retained at " + saved[i];
        }
    };
    // Reserve only unused names. The caller serializes these private sidecar names.
    for (int i = 1; i >= 0; --i) if (invalid[i]) {
        std::string candidate;
        for (unsigned n = 0; n < 32; ++n) {
            const std::string name = path + ".startup-bus-invalid" + std::to_string(n);
            struct stat info;
            if (::stat(name.c_str(), &info) == 0) continue;
            if (errno != ENOENT) break;
            candidate = name;
            break;
        }
        if (candidate.empty() || std::rename(files[i].c_str(), candidate.c_str()) != 0) {
            error = "Cannot preserve invalid editor bus configuration";
            rollback();
            return false;
        }
        saved[i] = candidate;
    }
    if (merge(path, master, music, sounds, error)) return true;
    rollback();
    return false;
}
} // namespace StartupEditorBus
