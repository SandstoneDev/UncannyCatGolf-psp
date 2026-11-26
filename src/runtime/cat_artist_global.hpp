#pragma once
#include "cat_artist_book.hpp"
#include <cerrno>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <utility>
#include <unistd.h>

namespace ArtistGlobal {
using ArtistBook::Book;
using ArtistBook::Header;
enum class Status { Missing, Valid, Invalid, IOError, Ambiguous };
constexpr unsigned Profiles=8, NoProfile=Profiles;

struct Slot {
    Status status=Status::Missing;
    Header header{};
    std::string path;
};

inline Slot inspect(const std::string& path,Book* output=nullptr,const Book* expected=nullptr) {
    Slot slot;slot.path=path;
    FILE* file=std::fopen(path.c_str(),"rb");
    if (!file) {slot.status=errno==ENOENT ? Status::Missing : Status::IOError;return slot;}
    Header& h=slot.header;
    bool valid=ArtistBook::readHeader(file,h);
    uint32_t hash=ArtistBook::headerChecksum(h);
    uint8_t buffer[1024];
    if (expected && (h.active!=expected->active || h.bases!=expected->bases)) valid=false;
    for (unsigned page=0;valid && page<ArtistBook::Slots;++page) {
        static_assert(sizeof(ArtistBook::Canvas)%sizeof buffer==0);
        for (size_t pos=0;pos<sizeof(ArtistBook::Canvas);pos+=sizeof buffer) {
            auto* bytes=output ? output->art[page].data()+pos : buffer;
            if (std::fread(bytes,1,sizeof buffer,file)!=sizeof buffer ||
                (expected && std::memcmp(bytes,expected->art[page].data()+pos,sizeof buffer))) {valid=false;break;}
            for (unsigned i=0;i<sizeof buffer;++i) {
                if (bytes[i]>ArtistBook::Colors) {valid=false;break;}
                hash=(hash^bytes[i])*16777619u;
            }
            if (!valid) break;
        }
    }
    valid=valid && hash==h.checksum && std::fgetc(file)==EOF;
    const bool ioError=std::ferror(file)!=0;
    const bool closed=std::fclose(file)==0;
    slot.status=ioError || !closed ? Status::IOError : valid ? Status::Valid : Status::Invalid;
    if (slot.status==Status::Valid && output) {
        output->generation=h.generation;output->active=h.active;output->bases=h.bases;
    }
    return slot;
}

struct Pair {
    Slot slots[2];unsigned latest=0;Status status=Status::Missing;
    const Slot& selected() const {return slots[latest];}
};

// Compare equal-generation siblings and equal-time legacy books without retaining
// another canvas. Different legacy generations may describe identical artwork.
inline Status sameArt(const Slot& a,const Slot& b) {
    if (a.header.active!=b.header.active || a.header.bases!=b.header.bases) return Status::Ambiguous;
    FILE* fa=std::fopen(a.path.c_str(),"rb");
    if (!fa) return Status::IOError;
    FILE* fb=std::fopen(b.path.c_str(),"rb");
    if (!fb) {std::fclose(fa);return Status::IOError;}
    Status status=Status::Valid;
    if (std::fseek(fa,sizeof(Header),SEEK_SET) || std::fseek(fb,sizeof(Header),SEEK_SET)) status=Status::IOError;
    uint8_t left[512],right[512];
    for (size_t pos=0;status==Status::Valid && pos<sizeof(Book::art);pos+=sizeof left) {
        if (std::fread(left,1,sizeof left,fa)!=sizeof left || std::fread(right,1,sizeof right,fb)!=sizeof right)
            status=Status::IOError;
        else if (std::memcmp(left,right,sizeof left)) status=Status::Ambiguous;
    }
    const bool ca=std::fclose(fa)==0,cb=std::fclose(fb)==0;
    return ca && cb ? status : Status::IOError;
}

inline Pair scan(const std::string& prefix,bool global) {
    Pair pair;
    for (unsigned i=0;i<2;++i) {
        const auto path=prefix+(i?".1":".0");
        auto& s=pair.slots[i];s=inspect(path);
        if (global && s.status==Status::Missing) {
            const auto parked=inspect(path+".prev");
            if (parked.status!=Status::Missing) s=parked;
        }
    }
    const auto& a=pair.slots[0];const auto& b=pair.slots[1];
    if (a.status==Status::IOError || b.status==Status::IOError) pair.status=Status::IOError;
    else if (a.status==Status::Valid && b.status==Status::Valid) {
        const uint32_t delta=b.header.generation-a.header.generation;
        pair.status=delta==0x80000000u ? Status::Ambiguous : delta==0 ? sameArt(a,b) : Status::Valid;
        pair.latest=delta && delta<0x80000000u;
    } else if (a.status==Status::Valid || b.status==Status::Valid) {
        pair.latest=(b.status==Status::Valid);pair.status=Status::Valid;
    } else if (a.status!=Status::Missing || b.status!=Status::Missing) pair.status=Status::Invalid;
    return pair;
}

inline bool validData(const Book& book) {
    if (book.active>=ArtistBook::Slots) return false;
    for (auto b:book.bases) if (b>ArtistBook::Blank || b==ArtistBook::Copycat) return false;
    for (const auto& art:book.art)
        if (std::any_of(art.begin(),art.end(),[](uint8_t c){return c>ArtistBook::Colors;})) return false;
    return true;
}

struct Scratch {
    std::string path;
    FILE* create(const std::string& prefix) {
        for (unsigned i=0;;++i) {
            const std::string name=prefix+".tmp"+(i?std::to_string(i):"");
            const int fd=::open(name.c_str(),O_CREAT|O_EXCL|O_WRONLY,0600);
            if (fd>=0) {
                path=name;
                auto* f=::fdopen(fd,"wb");
                if (!f) ::close(fd);
                return f;
            }
            if (errno!=EEXIST || i==UINT32_MAX) return nullptr;
        }
    }
    ~Scratch() {if (!path.empty()) ::unlink(path.c_str());}
};

inline bool save(const std::string& prefix,const Book& previous,Book& next) {
    if (!validData(next)) return false;
    const auto pair=scan(prefix,true);
    unsigned target=1;
    if (pair.status==Status::Valid) {
        const auto& old=pair.selected();
        if (old.header.generation!=previous.generation || inspect(old.path,nullptr,&previous).status!=Status::Valid)
            return false;
        target=1-pair.latest;
        // A damaged or unknown sibling is retained byte-for-byte for recovery.
        if (pair.slots[target].status==Status::Invalid) return false;
    } else if (pair.status!=Status::Missing || previous.generation) return false;
    Header h;h.generation=previous.generation+1;
    if (!h.generation) h.generation=2; // preserve the released UCA1 wrap convention
    h.active=static_cast<uint8_t>(next.active);h.bases=next.bases;h.checksum=ArtistBook::checksum(h,next);
    const std::string dest=prefix+(target?".1":".0");
    Scratch temp;
    FILE* f=temp.create(prefix);
    if (!f) return false;
    const bool wrote=std::fwrite(&h,sizeof h,1,f)==1 && std::fwrite(next.art.data(),sizeof next.art,1,f)==1;
    const bool flushed=std::fflush(f)==0,closed=std::fclose(f)==0;
    if (!wrote || !flushed || !closed) return false;
    const auto verified=inspect(temp.path,nullptr,&next);
    if (verified.status!=Status::Valid || verified.header.generation!=h.generation) return false;
    if (std::rename(temp.path.c_str(),dest.c_str())!=0) {
        if (errno!=EEXIST || pair.slots[target].status!=Status::Valid || pair.slots[target].path!=dest) return false;
        const std::string parked=dest+".prev";
        const auto backup=inspect(parked);
        if (backup.status!=Status::Missing &&
            (backup.status!=Status::Valid || ::unlink(parked.c_str())!=0)) return false;
        if (std::rename(dest.c_str(),parked.c_str())!=0) return false;
        if (std::rename(temp.path.c_str(),dest.c_str())!=0) {
            std::rename(parked.c_str(),dest.c_str()); // scan also recognizes a failed restoration
            return false;
        }
    }
    temp.path.clear();
    next.generation=h.generation;
    return true;
}

struct Choice {
    unsigned profile=NoProfile;
    uint32_t sourceGeneration=0,sourceChecksum=0;
    unsigned validProfiles=0;
};
inline std::string legacy(const std::string& root,unsigned profile) {
    return root+"cat-artist-"+std::to_string(profile)+".art";
}
inline std::string shared(const std::string& root) {return root+"cat-artist-global.art";}

inline Status load(const std::string& root,unsigned preferred,Book& output,Choice& choice) {
    choice={};
    const auto prefix=shared(root);
    const auto global=scan(prefix,true);
    if (global.status==Status::Valid) return inspect(global.selected().path,&output).status;
    if (global.status!=Status::Missing) return global.status;
    Slot selected;std::pair<time_t,long> newest{};bool tied=false;
    for (unsigned i=0;i<Profiles;++i) {
        // An explicit profile with a valid drawing determines the selection even
        // when another profile is unavailable. Its book is the only input needed.
        unsigned profile=(preferred<Profiles ? (preferred+i)%Profiles : i);
        const auto candidate=scan(legacy(root,profile),false);
        if (candidate.status==Status::Missing) continue;
        if (candidate.status!=Status::Valid) return candidate.status;
        choice.validProfiles|=1u<<profile;
        const auto& slot=candidate.selected();
        if (profile==preferred) {selected=slot;choice.profile=profile;break;}
        struct stat st{};
        if (::stat(slot.path.c_str(),&st)!=0) return Status::IOError;
        std::pair<time_t,long> stamp{st.st_mtime,0};
#ifdef __linux__
        stamp.second=st.st_mtim.tv_nsec;
#endif
        if (choice.profile==NoProfile || stamp>newest) {
            newest=stamp;selected=slot;choice.profile=profile;tied=false;
        } else if (stamp==newest) {
            const auto equal=sameArt(selected,slot);
            if (equal==Status::IOError) return equal;
            if (equal!=Status::Valid) tied=true;
        }
    }
    if (tied) return Status::Ambiguous;
    if (choice.profile==NoProfile) {
        output.generation=output.active=0;output.bases={0,1,ArtistBook::Blank};
        for (auto& art:output.art) art.fill(0);
        return Status::Valid;
    }
    const auto loaded=inspect(selected.path,&output);
    if (loaded.status!=Status::Valid) return loaded.status;
    if (loaded.header.generation!=selected.header.generation || loaded.header.checksum!=selected.header.checksum)
        return Status::Ambiguous;
    choice.sourceGeneration=output.generation;choice.sourceChecksum=loaded.header.checksum;
    // A new global journal starts at generation one; generations from different
    // legacy paths carry no cross-profile ordering or concurrency information.
    output.generation=0;
    return save(prefix,output,output) ? Status::Valid : Status::IOError;
}
} // namespace ArtistGlobal
