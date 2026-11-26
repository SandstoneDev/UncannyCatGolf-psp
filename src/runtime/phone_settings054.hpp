#pragma once
#include "startup_commit.hpp"
#ifdef __PSP__
#include <pspiofilemgr.h>
#endif

namespace PhoneSettings054 {
using StartupSettings::Values;
namespace detail {
// Same 16-byte sequence/checksum layout as StartupSettings. A separate magic
// and pair of files keep replay retries independent of first-run completion.
constexpr uint32_t Phone = 0x31504355, Locale = 0x394c4355, Done = 1u << 16;
struct Record {
    uint32_t magic=0, sequence=0, value=0, check=0;
    uint32_t checksum() const { return ((magic ^ sequence)*16777619u ^ value)*16777619u; }
};
static_assert(sizeof(Record)==16);
inline Values decode(uint32_t payload) {
    return {payload & 31,(payload >> 5) & 31,(payload >> 10) & 31,Language((payload >> 15) & 1)};
}
inline uint32_t encode(const Values& v) {
    return v.master | (v.music << 5) | (v.sounds << 10) | (uint32_t(v.language) << 15);
}
inline std::string slot(const std::string& base,unsigned i) { return base+(i ? ".1" : ".0"); }
struct Latest {
    Record record{};
    unsigned index=1;
    bool present=false, readable=true;
};
inline Latest latest(const std::string& base,uint32_t magic) {
    Latest out;
    for (unsigned i=0;i<2;++i) {
        FILE* file=std::fopen(slot(base,i).c_str(),"rb");
        if (!file) { if (errno!=ENOENT) out.readable=false; continue; }
        out.present=true;
        Record r{};
        bool full=std::fread(&r,1,sizeof r,file)==sizeof r && std::fgetc(file)==EOF;
        const bool readError=std::ferror(file)!=0;
        if (std::fclose(file)!=0 || readError) {out.readable=false;continue;}
        if (!full || !r.sequence || r.magic!=magic || r.check!=r.checksum()) continue;
        if (magic==Phone ? ((r.value & ~0x1ffffu) || !decode(r.value).valid()) : !validLanguage(r.value)) continue;
        if (r.sequence>out.record.sequence) {out.record=r;out.index=i;}
    }
    return out;
}

// Flush both the file contents and directory entry before crossing each write
// barrier. PSP's device-wide sync also covers renamed JSON and its sidecar.
inline bool syncFile(const std::string& path) {
#ifdef __PSP__
    std::string location=path;
    if (location.find(':')==std::string::npos) {
        char cwd[1024];
        if (!::getcwd(cwd,sizeof cwd)) return false;
        location=cwd;
    }
    auto colon=location.find(':');
    return colon!=std::string::npos && sceIoSync(location.substr(0,colon+1).c_str(),0)>=0;
#else
    const auto flush=[](const std::string& name) {
        int fd=::open(name.c_str(),O_RDONLY);
        if (fd<0) return false;
        bool ok=::fsync(fd)==0;
        return ::close(fd)==0 && ok;
    };
    auto slash=path.find_last_of('/');
    return flush(path) && flush(slash==std::string::npos ? "." : slash==0 ? "/" : path.substr(0,slash));
#endif
}
inline bool append(const std::string& base,Latest& previous,uint32_t payload) {
    Record record{Phone,previous.record.sequence+1,payload,0};
    if (!previous.readable || !record.sequence) return false;
    record.check=record.checksum();
    const auto target=slot(base,1-previous.index);
    FILE* file=std::fopen(target.c_str(),"wb");
    if (!file) return false;
    bool ok=std::fwrite(&record,1,sizeof record,file)==sizeof record;
    if (std::fclose(file)!=0 || !ok || !syncFile(target)) return false;
    previous=latest(base,Phone);
    return previous.readable && previous.record.sequence==record.sequence && previous.record.value==payload;
}
}

class Files {
public:
    Files(const std::string& journal,const std::string& editor,const std::string& language):
        journal(journal),editor(editor),language(language) {}

    bool save(const Values& values) const {
        if (!values.valid()) return false;
        auto staged=detail::latest(journal,detail::Phone);
        if (!staged.readable) return false;
        const uint32_t proposal=detail::encode(values);
        if (!staged.record.sequence || staged.record.value!=proposal) {
            if (!detail::append(journal,staged,proposal)) return false;
        } else if (!detail::syncFile(detail::slot(journal,staged.index))) {
            // A previously failed media sync may still leave readable cached bytes.
            return false;
        }
        std::string why;
        if (!StartupEditorBus::merge(editor,values.master,values.music,values.sounds,why) ||
            !detail::syncFile(editor)) return false;
        StartupEditorBus::Values verified;
        if (StartupEditorBus::loadAll(editor,verified)!=StartupEditorBus::Load::Valid ||
            verified.master!=values.master || verified.music!=values.music || verified.sounds!=values.sounds) return false;
        auto saved=detail::latest(language,detail::Locale);
        if (!saved.readable) return false;
        if (!saved.record.sequence || saved.record.value!=uint32_t(values.language)) {
            if (!LanguageFile(language).write(values.language)) return false;
            saved=detail::latest(language,detail::Locale);
        }
        if (!saved.readable || !saved.record.sequence || saved.record.value!=uint32_t(values.language) ||
            !detail::syncFile(detail::slot(language,saved.index))) return false;
        return detail::append(journal,staged,proposal | detail::Done);
    }

    StartupCommit::Boot boot(const StartupSettings::File& startup,const std::string& background,
                             bool returning,bool& retry) const {
        const auto stored=detail::latest(journal,detail::Phone);
        retry=false;
        const auto& record=stored.record;
        const Values proposal=record.sequence ? detail::decode(record.value) : Values{};
        if (!stored.readable || (stored.present && !record.sequence) ||
            (record.sequence && !(record.value & detail::Done) && !save(proposal))) {
            retry=true;
            return {proposal,true};
        }
        return StartupCommit::inspect(startup,language,editor,background,returning);
    }
private:
    std::string journal,editor,language;
};
}
