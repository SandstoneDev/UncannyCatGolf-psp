#pragma once
#include "locale.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
namespace StartupSettings {
enum class Status { Missing, Valid, Invalid, Pending };
struct Values {
    unsigned master=20,music=20,sounds=20;
    Language language=Language::English;
    bool valid() const { return master<=20 && music<=20 && sounds<=20 && validLanguage(static_cast<uint32_t>(language)); }
    bool operator==(const Values& b) const { return master==b.master && music==b.music && sounds==b.sounds && language==b.language; }
};
struct Result { Status status=Status::Missing; Values values{}; uint32_t sequence=0; };
inline bool showIntro(Status status,bool legacyValid,bool editorReturning) {
    return !editorReturning && status!=Status::Valid && (status==Status::Invalid || status==Status::Pending || !legacyValid);
}
class File {
public:
    explicit File(const std::string& path):base(path) {}
    Result read() const {
        const auto a=slot(0),b=slot(1);
        if (a.valid || b.valid) {
            const auto& r=b.record.sequence>a.record.sequence?b:a;
            return {r.record.value&Completed?Status::Valid:Status::Pending,decode(r.record.value),r.record.sequence};
        }
        return {a.present || b.present?Status::Invalid:Status::Missing,{},0};
    }
    bool stage(const Values& values) const {return write(values,Pending);}
    bool commit(const Values& values) const {return write(values,Completed);}
private:
    bool write(const Values& values,uint32_t state) const {
        if (!values.valid()) return false;
        const auto a=slot(0),b=slot(1);
        const bool latestB=b.record.sequence>a.record.sequence;
        const uint32_t sequence=(latestB?b.record.sequence:a.record.sequence)+1;
        if (!sequence) return false;
        Record record{Magic,sequence,encode(values)|state,0};record.check=checksum(record);
        const unsigned target=latestB?0:1;
        // Both-empty starts at slot0; later updates only overwrite the older slot.
        const unsigned index=!a.valid && !b.valid?0:target;
        auto* file=std::fopen(path(index).c_str(),"wb");
        if (!file) return false;
        const bool written=std::fwrite(&record,sizeof record,1,file)==1;
        const bool closed=std::fclose(file)==0;
        const auto verified=slot(index);
        return written && closed && verified.valid && verified.record.sequence==sequence && verified.record.value==record.value;
    }
private:
    struct Record { uint32_t magic,sequence,value,check; };
    struct Slot { Record record{}; bool present=false,valid=false; };
    static constexpr uint32_t Magic=0x31494355,Completed=1u<<16,Pending=1u<<17,Allowed=Completed|Pending|0xffffu;
    std::string base;
    std::string path(unsigned index) const {return base+(index?".1":".0");}
    static uint32_t checksum(const Record& r) {return ((r.magic^r.sequence)*16777619u^r.value)*16777619u;}
    static uint32_t encode(const Values& v) {return v.master|(v.music<<5)|(v.sounds<<10)|(static_cast<uint32_t>(v.language)<<15);}
    static Values decode(uint32_t v) {return {v&31,(v>>5)&31,(v>>10)&31,static_cast<Language>((v>>15)&1)};}
    Slot slot(unsigned index) const {
        Slot out;auto* file=std::fopen(path(index).c_str(),"rb");
        if (!file) return out;
        out.present=true;Record r{};
        const bool valid=std::fread(&r,sizeof r,1,file)==1 && std::fgetc(file)==EOF && !std::ferror(file) &&
            r.magic==Magic && r.sequence && ((r.value&(Completed|Pending))==Completed || (r.value&(Completed|Pending))==Pending) && !(r.value&~Allowed) &&
            decode(r.value).valid() && r.check==checksum(r);
        std::fclose(file);out.valid=valid;if (valid) out.record=r;return out;
    }
};
inline bool legacyPreference(const std::string& base,uint32_t magic,bool (*valid)(uint32_t),uint32_t& value) {
    uint32_t newest=0;
    for (const char* suffix:{".0",".1"}) {
        auto* f=std::fopen((base+suffix).c_str(),"rb");if (!f) continue;
        uint32_t r[4]{};
        const bool ok=std::fread(r,sizeof r,1,f)==1 && std::fgetc(f)==EOF && !std::ferror(f) &&
            r[0]==magic && r[1] && valid(r[2]) && r[3]==((r[0]^r[1])*16777619u^r[2])*16777619u;
        std::fclose(f);if (ok && r[1]>newest) {newest=r[1];value=r[2];}
    }
    return newest!=0;
}
inline bool legacyLanguage(const std::string& base,Values& values) {
    uint32_t selected=0;
    if (!legacyPreference(base,0x394c4355,validLanguage,selected)) return false;
    values.language=static_cast<Language>(selected);return true;
}
inline bool legacyBackground(const std::string& base) {
    uint32_t ignored=0;return legacyPreference(base,0x42464355,validBackground,ignored);
}
// Conservative compatibility reader for the old six-field JSON. It never writes
// the file. Unknown fields are left to the shared Editor bridge's full parser.
inline bool legacyEditor(const std::string& path,Values& values) {
    auto* f=std::fopen(path.c_str(),"rb");if (!f) return false;
    char text[4097]{};const auto bytes=std::fread(text,1,4096,f);
    const bool ok=bytes<4096 && !std::ferror(f);std::fclose(f);if (!ok) return false;
    const char* p=text;Values candidate=values;
    const auto space=[&]() {while (*p==' ' || *p=='\n' || *p=='\r' || *p=='\t') ++p;};
    space();if (*p++!='{') return false;space();
    while (*p!='}') {
        if (*p++!='"') return false;
        const char* first=p;while (*p && *p!='"' && *p!='\\') ++p;if (*p!='"') return false;
        const std::string key(first,p++);space();if (*p++!=':') return false;space();
        if (key=="invert_shot" || key=="buffer_shot") {
            if (std::strncmp(p,"true",4)==0) p+=4;else if (std::strncmp(p,"false",5)==0) p+=5;else return false;
        } else {
            if (key!="master_volume" && key!="music_volume" && key!="sound_volume" && key!="power_sensitivity") return false;
            const char* number=p;
            if (*number=='-') ++number;
            if (*number=='0') ++number;
            else if (*number>='1' && *number<='9') {while (*number>='0' && *number<='9') ++number;}
            else return false;
            if (*number=='.') {++number;if (*number<'0' || *number>'9') return false;while (*number>='0' && *number<='9') ++number;}
            if (*number=='e' || *number=='E') {
                ++number;if (*number=='-' || *number=='+') ++number;
                if (*number<'0' || *number>'9') return false;
                while (*number>='0' && *number<='9') ++number;
            }
            char* end=nullptr;const double value=std::strtod(p,&end);
            if (end!=number || !std::isfinite(value) || value<0 || value>(key=="power_sensitivity"?2:1)) return false;
            p=end;const unsigned step=static_cast<unsigned>(std::round(value*20));
            if (key=="master_volume") candidate.master=step;else if (key=="music_volume") candidate.music=step;
            else if (key=="sound_volume") candidate.sounds=step;
        }
        space();if (*p=='}') break;if (*p++!=',') return false;space();if (*p=='}') return false;
    }
    ++p;space();if (*p) return false;values=candidate;return true;
}
} // namespace StartupSettings
