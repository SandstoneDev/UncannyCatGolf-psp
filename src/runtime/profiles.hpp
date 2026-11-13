#pragma once

#include "campaign.hpp"

#include <array>
#include <cstddef>
#include <cstring>

constexpr unsigned ProfileNameLength=16;
using ProfileName=std::array<char,3*ProfileNameLength+1>;

struct ProfileEntry {
    ProfileName title{},player{};
    unsigned completed=0,prisms=0;
    bool found=false,chill=false;
};

class Profiles {
public:
    static constexpr unsigned Count=8, Legacy=0, None=Count;

    explicit Profiles(std::string directory): root(std::move(directory)) {}

    SaveFile save(unsigned slot) const { return SaveFile(path(slot,".sav")); }

    static bool validName(const ProfileName& name) {
        unsigned letters=0;
        bool content=false;
        for (unsigned i=0;i<name.size();) {
            const auto ch=static_cast<unsigned char>(name[i++]);
            if (!ch) return letters && content;
            if (++letters>ProfileNameLength || ch<0x20 || ch==0x7f) return false;
            if (ch<0x80) {
                content|=ch!=' ';
            } else {
                if (ch<0xc2 || ch>0xef) return false;
                const unsigned trailing=ch<0xe0?1:2;
                if (i+trailing>=name.size()) return false;
                for (unsigned j=0;j<trailing;++j) {
                    const auto next=static_cast<unsigned char>(name[i++]);
                    if ((next&0xc0)!=0x80 || (j==0 && ch==0xe0 && next<0xa0) ||
                        (j==0 && ch==0xed && next>=0xa0)) return false;
                }
                content=true;
            }
        }
        return false;
    }

    void refresh() {
        entries={};
        order={};
        visible=0;
        for (unsigned index=Count;index>0;--index) {
            const unsigned slot=index-1;
            Progress progress;
            if (!save(slot).read(progress)) continue;
            auto& entry=entries[slot];
            entry.found=true;
            for (unsigned i=0;i<CourseCount;++i) entry.completed+=progress.best[i].score!=0;
            entry.prisms=progress.totalPrisms();
            if (slot!=Legacy) {
                Record record{};
                if (read(path(slot,".meta"),record) || read(path(slot,".meta.tmp"),record)) {
                    entry.title=record.title;
                    entry.player=record.player;
                    entry.chill=record.flags&1;
                }
            }
            order[visible++]=slot;
        }
    }

    unsigned size() const { return visible; }
    unsigned slot(unsigned index) const { return index<visible?order[index]:None; }
    const ProfileEntry& entry(unsigned slot) const { return entries[slot]; }

    unsigned create(const ProfileName& title,const ProfileName& player,bool chill=false) {
        if (!validName(title) || !validName(player)) return None;
        refresh();
        unsigned slot=1;
        while (slot<Count && entries[slot].found) ++slot;
        if (slot==Count) return None;

        Record record{};
        record.magic=Magic;
        record.title=title;
        record.player=player;
        record.flags=chill?1:0;
        record.check=hash(record);
        const std::string temporary=path(slot,".meta.tmp");
        const std::string metadata=path(slot,".meta");
        auto* file=std::fopen(temporary.c_str(),"wb");
        if (!file) return None;
        const bool wrote=std::fwrite(&record,sizeof record,1,file)==1;
        const bool closed=std::fclose(file)==0;
        if (!wrote || !closed || !save(slot).write({})) return None;
        // The temporary metadata also remains readable after a failed FAT rename.
        std::rename(temporary.c_str(),metadata.c_str());
        refresh();
        return entries[slot].found?slot:None;
    }

private:
    static constexpr uint32_t Magic=0x31504355; // UCP1
    struct Record {
        uint32_t magic;
        ProfileName title,player;
        uint8_t flags,reserved;
        uint32_t check;
    };
    static_assert(sizeof(Record)==108);

    static uint32_t hash(const Record& record) {
        const auto* bytes=reinterpret_cast<const uint8_t*>(&record);
        uint32_t value=2166136261u;
        for (unsigned i=0;i<offsetof(Record,check);++i) value=(value^bytes[i])*16777619u;
        return value;
    }

    static bool read(const std::string& path,Record& record) {
        auto* file=std::fopen(path.c_str(),"rb");
        if (!file) return false;
        const bool ok=std::fread(&record,sizeof record,1,file)==1 && std::fgetc(file)==EOF &&
                      !std::ferror(file);
        std::fclose(file);
        return ok && record.magic==Magic && record.check==hash(record) &&
               record.flags<=1 && validName(record.title) && validName(record.player);
    }

    std::string path(unsigned slot,const char* suffix) const {
        if (slot==Legacy) return root+"progress.sav";
        char name[36];
        std::snprintf(name,sizeof name,"profile-%02u%s",slot,suffix);
        return root+name;
    }

    std::string root;
    std::array<ProfileEntry,Count> entries{};
    std::array<unsigned,Count> order{};
    unsigned visible=0;
};
