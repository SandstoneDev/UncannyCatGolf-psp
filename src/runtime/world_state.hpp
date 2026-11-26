#pragma once
#include "world_awards.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace WorldState {
enum class Status { Missing, Valid, Invalid, IOError };

namespace journal {
using Bytes=std::array<uint8_t,89>;
constexpr std::size_t CheckOffset=85;
inline uint64_t readLE(const Bytes& b,std::size_t offset,unsigned width) {
    uint64_t value=0;
    for(unsigned i=0;i<width;++i) value |= uint64_t{b[offset+i]}<<(8*i);
    return value;
}
inline void writeLE(Bytes& b,std::size_t offset,unsigned width,uint64_t value) {
    for(unsigned i=0;i<width;++i) b[offset+i]=static_cast<uint8_t>(value>>(8*i));
}
inline uint32_t checksum(const Bytes& b) {
    uint32_t hash=2166136261u;
    for(std::size_t i=0;i<CheckOffset;++i) hash=(hash^b[i])*16777619u;
    return hash;
}
inline Bytes encode(const Data& d,uint32_t sequence) {
    Bytes b{'U','C','W','S'};
    writeLE(b,4,2,1);
    writeLE(b,6,2,b.size());
    writeLE(b,8,4,sequence);
    writeLE(b,12,8,d.owned);
    b[20]=d.pendingCount;
    std::copy_n(d.pending,d.pendingCount,b.begin()+21);
    b[64]=d.friendPhase;
    b[65]=d.questKeys;
    b[66]=static_cast<uint8_t>(d.worldProgress+1);
    b[67]=static_cast<uint8_t>(d.pennyHi | (d.appleHasDied<<1));
    writeLE(b,68,8,static_cast<uint64_t>(d.deathUnix));
    const auto& t=d.deathLocal;
    writeLE(b,76,2,t.year);
    b[78]=t.month; b[79]=t.day; b[80]=t.weekday;
    b[81]=t.hour; b[82]=t.minute; b[83]=t.second; b[84]=t.dst;
    writeLE(b,CheckOffset,4,checksum(b));
    return b;
}
inline bool decode(const Bytes& b,Data& d) {
    if (b[0]!='U' || b[1]!='C' || b[2]!='W' || b[3]!='S' || readLE(b,4,2)!=1 ||
        readLE(b,6,2)!=b.size() || !readLE(b,8,4) || readLE(b,CheckOffset,4)!=checksum(b)) return false;
    if (b[20]>VisibleCount || b[66]>7 || b[67]>3 || b[84]>1 || b[75]>=128) return false;
    for(unsigned i=b[20];i<VisibleCount;++i) if (b[21+i]) return false;
    d.owned=readLE(b,12,8);
    d.pendingCount=b[20];
    std::copy_n(b.begin()+21,VisibleCount,d.pending);
    d.friendPhase=b[64];
    d.questKeys=b[65];
    d.worldProgress=static_cast<int8_t>(static_cast<int>(b[66])-1);
    d.pennyHi=(b[67]&1)!=0;
    d.appleHasDied=(b[67]&2)!=0;
    d.deathUnix=static_cast<int64_t>(readLE(b,68,8));
    d.deathLocal={static_cast<uint16_t>(readLE(b,76,2)),b[78],b[79],b[80],b[81],b[82],b[83],b[84]!=0};
    return valid(d);
}
struct Slot {
    Status status=Status::Missing;
    Bytes bytes{};
    Data data{};
    uint32_t sequence() const { return static_cast<uint32_t>(readLE(bytes,8,4)); }
};
inline Slot read(const std::string& path) {
    Slot out;
    std::FILE* f=std::fopen(path.c_str(),"rb");
    if (!f) {
        out.status=errno==ENOENT ? Status::Missing : Status::IOError;
        return out;
    }
    const auto count=std::fread(out.bytes.data(),1,out.bytes.size(),f);
    const int trailing=std::fgetc(f);
    const bool error=std::ferror(f)!=0;
    const bool closed=std::fclose(f)==0;
    if (error || !closed) out.status=Status::IOError;
    else if (count==out.bytes.size() && trailing==EOF && decode(out.bytes,out.data)) out.status=Status::Valid;
    else out.status=Status::Invalid;
    return out;
}
inline bool stage(const std::string& path,const Bytes& bytes) {
    auto* f=std::fopen(path.c_str(),"wb");
    if (!f) return false;
    const bool wrote=std::fwrite(bytes.data(),1,bytes.size(),f)==bytes.size();
    const bool flushed=std::fflush(f)==0;
    const bool closed=std::fclose(f)==0;
    if (!wrote || !flushed || !closed) return false;
    const auto verified=read(path);
    return verified.status==Status::Valid && verified.bytes==bytes;
}
} // namespace journal

// One serialized writer in the PSP main loop. A commit refreshes both slots
// first and refuses stale snapshots from other sequential Store instances.
class Store {
public:
    explicit Store(const std::string& gameRoot):root(gameRoot) {}
    Status load() {
        const auto snapshot=scan();
        status=snapshot.status;
        if (status==Status::Valid) {
            const auto& latest=snapshot.slots[snapshot.latest];
            value=latest.data;
            sequence=latest.sequence();
        } else if (status==Status::Missing) {
            value=Data{};
            sequence=0;
        }
        return status;
    }
    const Data& data() const { return value; }
    bool commit(const Data& next) {
        if ((status!=Status::Valid && status!=Status::Missing) || !valid(next) || !preservesProgress(next)) return false;
        const auto current=scan();
        if (current.status!=status) return false;
        unsigned target=0;
        if (status==Status::Valid) {
            const auto& latest=current.slots[current.latest];
            if (latest.bytes!=journal::encode(value,sequence)) return false;
            if (journal::encode(next,sequence)==latest.bytes) return true;
            target=1-current.latest;
        }
        if (sequence==UINT32_MAX) return false;
        const auto bytes=journal::encode(next,sequence+1);
        const auto scratch=path(".tmp");
        if (!journal::stage(scratch,bytes)) return false;
        const auto dest=path(target?".1":".0");
        if (std::rename(scratch.c_str(),dest.c_str())!=0) {
            if (errno!=EEXIST || current.slots[target].status==Status::Missing ||
                !publishAfterCollision(scratch,dest)) return false;
        }
        value=next;
        ++sequence;
        status=Status::Valid;
        return true;
    }
    // Callback must return true to accept its candidate for validation and
    // checked publication. Returning false discards all candidate mutations.
    template<class Fn> bool update(Fn mutate) {
        if (status!=Status::Valid && status!=Status::Missing) return false;
        Data next=value;
        return mutate(next) && commit(next);
    }
private:
    bool publishAfterCollision(const std::string& scratch,const std::string& dest) {
        // Some FAT drivers refuse to replace an existing name. Move the old
        // standby aside only for that error, after confirming it exists.
        struct stat info{};
        if (::stat(dest.c_str(),&info)!=0 || !S_ISREG(info.st_mode)) return false;
        const auto parked=dest+".prev";
        // A leftover from a prior successful replacement is superseded by the
        // actual standby in dest. Never unlink the only copy of that standby.
        if (::unlink(parked.c_str())!=0 && errno!=ENOENT) return false;
        if (std::rename(dest.c_str(),parked.c_str())!=0) return false;
        if (std::rename(scratch.c_str(),dest.c_str())==0) return true;
        // On a second I/O failure keep the parked bytes. load() can read them
        // through their recovery name. Require a fresh scan before retrying.
        if (std::rename(parked.c_str(),dest.c_str())!=0) status=Status::IOError;
        return false;
    }
    bool preservesProgress(const Data& next) const {
        if ((value.owned&~next.owned) || (value.questKeys&~next.questKeys) ||
            next.friendPhase<value.friendPhase || next.worldProgress<value.worldProgress ||
            (value.pennyHi && !next.pennyHi)) return false;
        if (value.appleHasDied) {
            const auto& a=value.deathLocal;
            const auto& b=next.deathLocal;
            if (!next.appleHasDied || value.deathUnix!=next.deathUnix ||
                a.year!=b.year || a.month!=b.month || a.day!=b.day || a.weekday!=b.weekday ||
                a.hour!=b.hour || a.minute!=b.minute || a.second!=b.second || a.dst!=b.dst) return false;
        }
        unsigned kept=0;
        for (unsigned i=0;i<value.pendingCount;++i) {
            const unsigned id=value.pending[i];
            if (owns(next,id)) continue;
            if (kept==next.pendingCount || next.pending[kept++]!=id) return false;
        }
        return true;
    }
    struct Snapshot {
        journal::Slot slots[2];
        unsigned latest=0;
        Status status=Status::IOError;
    };
    Snapshot scan() const {
        Snapshot out;
        struct stat info{};
        if (root.empty() || root.find('\0')!=std::string::npos ||
            ::stat(root.c_str(),&info)!=0 || !S_ISDIR(info.st_mode)) return out;
        for (unsigned i=0;i<2;++i) {
            const auto primary=path(i?".1":".0");
            out.slots[i]=journal::read(primary);
            if (out.slots[i].status==Status::Missing) out.slots[i]=journal::read(primary+".prev");
        }
        const auto& a=out.slots[0];
        const auto& b=out.slots[1];
        if (a.status==Status::IOError || b.status==Status::IOError) return out;
        if (a.status==Status::Valid || b.status==Status::Valid) {
            if (a.status==Status::Valid && b.status==Status::Valid && a.sequence()==b.sequence() && a.bytes!=b.bytes) {
                out.status=Status::Invalid;
                return out;
            }
            out.latest=(a.status!=Status::Valid || (b.status==Status::Valid && b.sequence()>a.sequence()));
            out.status=Status::Valid;
        } else {
            out.status=a.status==Status::Missing && b.status==Status::Missing ? Status::Missing : Status::Invalid;
        }
        return out;
    }
    std::string path(const char* suffix) const {
        const bool slash=root.back()=='/' || root.back()=='\\';
        return root+(slash?"":"/")+"world-state.cfg"+suffix;
    }
    std::string root;
    Data value{};
    uint32_t sequence=0;
    Status status=Status::IOError;
};
} // namespace WorldState
