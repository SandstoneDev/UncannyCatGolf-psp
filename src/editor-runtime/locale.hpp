#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

enum class Language : uint32_t { English, Russian };

template<uint32_t Magic,bool (*Valid)(uint32_t)>
class PreferenceFile {
public:
    explicit PreferenceFile(const std::string& path) : base(path) {}

    uint32_t read(uint32_t fallback=0) const {
        const Record a=readSlot(0),b=readSlot(1);
        const Record& newest=b.sequence>a.sequence?b:a;
        return newest.sequence ? newest.value : fallback;
    }

    bool write(uint32_t preference) const {
        if (!Valid(preference)) return false;
        const Record a=readSlot(0),b=readSlot(1);
        const bool bNewest=b.sequence>a.sequence;
        const unsigned slot=bNewest ? 0 : 1;
        const uint32_t next=(bNewest?b:a).sequence+1;
        if (!next) return false;
        Record value{Magic,next,preference,0};
        value.check=checksum(value);
        std::FILE* file=std::fopen(path(slot).c_str(),"wb");
        if (!file) return false;
        const bool written=std::fwrite(&value,sizeof value,1,file)==1;
        const bool closed=std::fclose(file)==0;
        return written && closed && readSlot(slot).sequence==next;
    }

private:
    struct Record { uint32_t magic,sequence,value,check; };
    const std::string base;

    std::string path(unsigned slot) const { return base+(slot?".1":".0"); }
    static uint32_t checksum(const Record& r) {
        return ((r.magic^r.sequence)*16777619u^r.value)*16777619u;
    }
    Record readSlot(unsigned slot) const {
        std::FILE* file=std::fopen(path(slot).c_str(),"rb");
        if (!file) return {};
        Record r{};
        const bool valid=std::fread(&r,sizeof r,1,file)==1 && std::fgetc(file)==EOF &&
                         !std::ferror(file) && r.magic==Magic && r.sequence &&
                         Valid(r.value) && r.check==checksum(r);
        std::fclose(file);
        return valid ? r : Record{};
    }
};

inline bool validLanguage(uint32_t value) {
    return value<=static_cast<uint32_t>(Language::Russian);
}

class LanguageFile {
public:
    explicit LanguageFile(const std::string& path) : file(path) {}
    Language read() const { return static_cast<Language>(file.read()); }
    bool write(Language language) const { return file.write(static_cast<uint32_t>(language)); }

private:
    PreferenceFile<0x394c4355,validLanguage> file;
};

constexpr unsigned CatalogBackgroundBase=128;
constexpr unsigned RenderedBackgroundCount=8;
constexpr unsigned BackgroundIds[]={0,1,2,3,5,6,7,8,
    CatalogBackgroundBase+1,CatalogBackgroundBase+10,CatalogBackgroundBase+18,CatalogBackgroundBase+19,CatalogBackgroundBase+20,CatalogBackgroundBase+21,CatalogBackgroundBase+25,CatalogBackgroundBase+49,CatalogBackgroundBase+51,CatalogBackgroundBase+52,CatalogBackgroundBase+55,CatalogBackgroundBase+56,
    CatalogBackgroundBase+57,CatalogBackgroundBase+59,CatalogBackgroundBase+60,CatalogBackgroundBase+61,CatalogBackgroundBase+62,CatalogBackgroundBase+63,CatalogBackgroundBase+66,CatalogBackgroundBase+67,CatalogBackgroundBase+68,CatalogBackgroundBase+82,CatalogBackgroundBase+84,CatalogBackgroundBase+85,
    CatalogBackgroundBase+86,CatalogBackgroundBase+87,CatalogBackgroundBase+88,CatalogBackgroundBase+89,CatalogBackgroundBase+90,CatalogBackgroundBase+91,CatalogBackgroundBase+92,CatalogBackgroundBase+93,CatalogBackgroundBase+94,CatalogBackgroundBase+95,CatalogBackgroundBase+96,CatalogBackgroundBase+97,
    CatalogBackgroundBase+103,CatalogBackgroundBase+104};
constexpr unsigned BackgroundCount=sizeof BackgroundIds/sizeof BackgroundIds[0];

inline unsigned backgroundIndex(unsigned id) {
    for (unsigned i=0;i<BackgroundCount;++i) if (BackgroundIds[i]==id) return i;
    return BackgroundCount;
}

struct BackgroundPreference {
    unsigned chosen=1,last=1;
    bool random=false;

    uint32_t encode() const { return chosen | (last<<8) | (static_cast<uint32_t>(random)<<16); }

    static BackgroundPreference decode(uint32_t value) {
        return {value&255,(value>>8)&255,(value&(1u<<16))!=0};
    }
};

inline bool validBackground(uint32_t value) {
    const auto setting=BackgroundPreference::decode(value);
    return value<=0x1ffff && backgroundIndex(setting.chosen)<BackgroundCount &&
           backgroundIndex(setting.last)<BackgroundCount;
}

using BackgroundFile=PreferenceFile<0x42464355,validBackground>;

constexpr unsigned LocaleGlyphs=167;

inline unsigned nextGlyph(const char*& text) {
    unsigned point=static_cast<unsigned char>(*text++);
    if (point>=0x80) {
        const unsigned lead=point;
        unsigned bytes=0, minimum=0;
        if (lead>=0xc2 && lead<=0xdf) { bytes=1; minimum=0x80; point=lead&0x1f; }
        else if (lead>=0xe0 && lead<=0xef) { bytes=2; minimum=0x800; point=lead&0x0f; }
        else if (lead>=0xf0 && lead<=0xf4) { bytes=3; minimum=0x10000; point=lead&0x07; }
        else return '?' - 32;
        for (unsigned i=0;i<bytes;++i) {
            const unsigned next=static_cast<unsigned char>(*text);
            if ((next&0xc0)!=0x80) return '?' - 32;
            ++text;
            point=(point<<6)|(next&0x3f);
        }
        if (point<minimum || point>0x10ffff || (point>=0xd800 && point<=0xdfff)) return '?' - 32;
    }
    if (point>=32 && point<=126) return point-32;
    if (point>=0x410 && point<=0x44f) return 95+point-0x410;
    switch (point) {
    case 0x401: return 159;
    case 0x451: return 160;
    case 0xd7: return 161;
    case 0x25cb: return 162;
    case 0x25a1: return 163;
    case 0x25b3: return 164;
    case 0x2190: return 165;
    case 0x2192: return 166;
    default: return '?' - 32;
    }
}
