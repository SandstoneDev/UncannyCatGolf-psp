#pragma once
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <string>

namespace WorldLegacy054 {
enum class Status {Missing,Valid,Invalid,IOError};
struct Value {Status status=Status::Missing;uint32_t value=0;};
template<uint32_t Magic,bool (*Valid)(uint32_t)>
Value read(const std::string& base) {
    struct Record {uint32_t magic=0,sequence=0,value=0,check=0;};
    Record latest{};bool present=false;
    for (unsigned i=0;i<2;++i) {
        auto* f=std::fopen((base+(i?".1":".0")).c_str(),"rb");
        if (!f) {if (errno!=ENOENT) return {Status::IOError,0};continue;}
        present=true;Record r;
        const bool full=std::fread(&r,sizeof r,1,f)==1 && std::fgetc(f)==EOF;
        const bool error=std::ferror(f)!=0;
        if (std::fclose(f)!=0 || error) return {Status::IOError,0};
        const uint32_t check=((r.magic^r.sequence)*16777619u^r.value)*16777619u;
        if (!full || r.magic!=Magic || !r.sequence || !Valid(r.value) || r.check!=check) continue;
        if (r.sequence==latest.sequence && r.value!=latest.value) return {Status::Invalid,0};
        if (r.sequence>latest.sequence) latest=r;
    }
    if (latest.sequence) return {Status::Valid,latest.value};
    return {present?Status::Invalid:Status::Missing,0};
}
inline bool readable(const Value& v) {return v.status==Status::Missing || v.status==Status::Valid;}
} // namespace WorldLegacy054
