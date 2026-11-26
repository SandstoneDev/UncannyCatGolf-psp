#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>

namespace CampaignMods {
enum Mod : uint16_t {
    Chill=1<<0, Panic=1<<1, Burst=1<<2, Telekinesis=1<<3, Buddy=1<<4,
    Parry=1<<5, Jumpy=1<<6, Dizzy=1<<7, Missiles=1<<8, Sticky=1<<9, God=1<<10
};
constexpr uint16_t All=(1<<11)-1;
constexpr bool valid(uint32_t mask) {
    return !(mask&~uint32_t(All)) && (mask&(Chill|Panic))!=(Chill|Panic);
}
constexpr uint16_t toggle(uint16_t mask,Mod mod) {
    const bool on=!(mask&mod);
    if (on && mod==Chill) mask&=~Panic;
    if (on && mod==Panic) mask&=~Chill;
    return uint16_t(mask^mod);
}
struct Options {
    uint16_t modifiers=0;
    // Original PThru.peak_hunter is a session-static flag, never serialized.
    bool peakHunter=false;
    constexpr bool enabled(Mod mod) const { return (modifiers&mod)!=0; }
};

// Optional extension; the original Progress and UCP1 profile records stay exact.
class Store {
    struct Record { uint32_t magic,sequence,mask,check; };
    static_assert(sizeof(Record)==16);
    static constexpr uint32_t Magic=0x314d4355; // UCM1
    static uint32_t hash(const Record& record) {
        const auto* bytes=reinterpret_cast<const uint8_t*>(&record);
        uint32_t value=2166136261u;
        for (unsigned i=0;i<12;++i) value=(value^bytes[i])*16777619u;
        return value;
    }
    static bool newer(uint32_t a,uint32_t b) { return a!=b && uint32_t(a-b)<0x80000000u; }
    bool load(unsigned index,Record& value) const {
        auto* file=std::fopen((path+"."+char('0'+index)).c_str(),"rb");
        if (!file) return false;
        const bool ok=std::fread(&value,sizeof value,1,file)==1 &&
            std::fgetc(file)==EOF && !std::ferror(file);
        std::fclose(file);
        return ok && value.magic==Magic && valid(value.mask) && value.check==hash(value);
    }
    int latest(Record& record) const {
        Record a{},b{};
        const bool haveA=load(0,a),haveB=load(1,b);
        if (!haveA && !haveB) return -1;
        if (haveB && (!haveA || newer(b.sequence,a.sequence))) { record=b;return 1; }
        record=a;return 0;
    }
public:
    explicit Store(std::string name):path(std::move(name)) {}
    uint16_t read(bool oldChill=false) const {
        Record record{};
        return latest(record)<0 ? (oldChill?Chill:0) : uint16_t(record.mask);
    }
    bool write(uint16_t mask) const {
        if (!valid(mask)) return false;
        Record previous{};
        const int old=latest(previous);
        const unsigned index=old<0?0:unsigned(old)^1u;
        Record record{Magic,old<0?1u:previous.sequence+1,mask,0};
        record.check=hash(record);
        auto* file=std::fopen((path+"."+char('0'+index)).c_str(),"wb");
        if (!file) return false;
        const bool wrote=std::fwrite(&record,sizeof record,1,file)==1;
        const bool closed=std::fclose(file)==0;
        Record check{};
        return wrote && closed && load(index,check) && check.sequence==record.sequence && check.mask==mask;
    }
private:
    std::string path;
};
}
