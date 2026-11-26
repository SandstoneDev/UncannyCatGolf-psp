#pragma once
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

// Flat, bounded, reversible local-profile transactions. Never enumerates paths
// or touches native SAVEDATA, world/config data, or custom levels.
class ProfileArchive {
public:
    static constexpr unsigned Slots=8,Components=10;
    struct Fingerprint {
        uint64_t bytes=0,hash=0;
        bool operator==(const Fingerprint& b) const {return bytes==b.bytes && hash==b.hash;}
        bool operator!=(const Fingerprint& b) const {return !(*this==b);}
    };
    struct Token {
        unsigned slot=Slots,roster=0,generation=0,mask=0;
        std::array<Fingerprint,Components> files{};
    };
    enum class Result { Archived, Stale, IOError };
    explicit ProfileArchive(std::string directory):root(std::move(directory)){}

    bool recover(unsigned slot) const {
        Journal value{};
        const int status=latest(slot,value);
        if(status<0)return false;
        return !status || value.state!=Prepared || rollback(value);
    }
    bool empty(unsigned slot) const {
        if(slot>=Slots || !recover(slot))return false;
        Token token{};
        return inspect(slot,token) && !token.mask;
    }
    bool capture(unsigned slot,unsigned roster,Token& token) const {
        token={};
        if(slot>=Slots || !recover(slot))return false;
        Journal value{};
        if(latest(slot,value)<0 || !inspect(slot,token) || !token.mask)return false;
        token.roster=roster;
        token.generation=value.generation;
        return true;
    }
    Result archive(const Token& token,unsigned roster) const {
        if(token.slot>=Slots || !token.mask || token.roster!=roster)return Result::Stale;
        if(!recover(token.slot))return Result::IOError;
        Journal previous{};
        if(latest(token.slot,previous)<0)return Result::IOError;
        Token current{};
        if(!inspect(token.slot,current))return Result::IOError;
        if(token.generation!=previous.generation || token.mask!=current.mask || token.files!=current.files)
            return Result::Stale;
        if(previous.generation==UINT32_MAX || previous.sequence>=UINT32_MAX-2)return Result::IOError;
        Journal value{};
        value.slot=token.slot;value.state=Prepared;
        value.sequence=previous.sequence+1;value.generation=previous.generation+1;
        value.mask=token.mask;value.files=token.files;
        for(unsigned i=0;i<Components;++i)if(value.mask&(1u<<i)) {
            Fingerprint ignored{};bool present=false;
            if(!fingerprint(archivePath(value,i),ignored,present) || present)return Result::IOError;
        }
        if(!store(value)) {
            // No moves have started. Retain a valid completed rollback even
            // when the first-ever prepare write tears its only journal copy.
            value.state=RolledBack;++value.sequence;store(value);
            return Result::IOError;
        }
        for(unsigned i=0;i<Components;++i)if(value.mask&(1u<<i)) {
            if(std::rename(component(value.slot,i).c_str(),archivePath(value,i).c_str())!=0) {
                rollback(value);
                return Result::IOError;
            }
        }
        value.state=Committed;++value.sequence;
        if(!store(value)) {
            value.state=Prepared;--value.sequence;
            rollback(value);
            return Result::IOError;
        }
        return Result::Archived;
    }

private:
    enum : uint32_t { Magic=0x44504355,Version=1,Prepared=1,Committed=2,RolledBack=3 };
    struct Journal {
        uint32_t magic=Magic,version=Version,slot=0,state=0,sequence=0,generation=0,mask=0,reserved=0;
        std::array<Fingerprint,Components> files{};
        uint32_t check=0,padding=0;
    };
    static_assert(sizeof(Journal)==200);
    std::string root;

    std::string component(unsigned slot,unsigned index) const {
        static constexpr const char* Suffix[]={".sav",".sav.0",".sav.1",".meta",".meta.tmp",".mods.0",".mods.1",".clicks",".clicks.0",".clicks.1"};
        if(slot>=Slots || index>=Components)return {};
        // Legacy's generic Profiles::path intentionally aliases every suffix.
        // Use this independent allowlist instead; no .cfg or aliased .mods.
        if(!slot) {
            if(index==0)return root+"progress.sav";
            if(index<3)return root+"progress.sav."+char('0'+index-1);
            if(index==7)return root+"progress.clicks";
            if(index>7)return root+"progress.clicks."+char('0'+index-8);
            return {};
        }
        char prefix[24];std::snprintf(prefix,sizeof prefix,"profile-%02u",slot);
        return root+prefix+Suffix[index];
    }
    std::string journalPath(unsigned slot,unsigned side) const {
        char name[48];std::snprintf(name,sizeof name,".profile-delete-%02u.journal.%u",slot,side);
        return root+name;
    }
    std::string archivePath(const Journal& value,unsigned index) const {
        char name[64];std::snprintf(name,sizeof name,"profile-delete-%02u-%08x-%02u.bak",unsigned(value.slot),unsigned(value.generation),index);
        return root+name;
    }
    static uint32_t checksum(const Journal& value) {
        uint32_t hash=2166136261u;
        const auto* bytes=reinterpret_cast<const unsigned char*>(&value);
        for(unsigned i=0;i<offsetof(Journal,check);++i)hash=(hash^bytes[i])*16777619u;
        return hash;
    }
    static bool fingerprint(const std::string& path,Fingerprint& result,bool& present) {
        result={};present=false;
        if(path.empty())return true;
        auto* file=std::fopen(path.c_str(),"rb");
        if(!file)return errno==ENOENT;
        present=true;result.hash=14695981039346656037ull;
        unsigned char buffer[4096];size_t count;
        while((count=std::fread(buffer,1,sizeof buffer,file))!=0) {
            result.bytes+=count;
            if(result.bytes>16*1024*1024) {std::fclose(file);return false;}
            for(size_t i=0;i<count;++i)result.hash=(result.hash^buffer[i])*1099511628211ull;
        }
        const bool good=!std::ferror(file);
        const bool closed=std::fclose(file)==0;
        return good && closed;
    }
    bool inspect(unsigned slot,Token& token) const {
        token={};token.slot=slot;
        for(unsigned i=0;i<Components;++i) {
            bool present=false;
            if(!fingerprint(component(slot,i),token.files[i],present))return false;
            if(present)token.mask|=1u<<i;
        }
        return true;
    }
    // 0 missing,1 valid,-1 invalid/I/O. One valid alternating copy survives a
    // torn write of its peer. A wholly corrupt journal blocks all reuse.
    int read(unsigned slot,unsigned side,Journal& value) const {
        auto* file=std::fopen(journalPath(slot,side).c_str(),"rb");
        if(!file)return errno==ENOENT?0:-1;
        const bool good=std::fread(&value,sizeof value,1,file)==1 && std::fgetc(file)==EOF && !std::ferror(file);
        const bool closed=std::fclose(file)==0;
        if(!good || !closed || value.magic!=Magic || value.version!=Version || value.slot!=slot ||
           value.state<Prepared || value.state>RolledBack || !value.sequence || !value.generation ||
           !value.mask || value.mask>=(1u<<Components) || value.reserved || value.padding || value.check!=checksum(value))return -1;
        return 1;
    }
    int latest(unsigned slot,Journal& value) const {
        if(slot>=Slots)return -1;
        Journal a{},b{};const int left=read(slot,0,a),right=read(slot,1,b);
        if(left==1 || right==1) {
            if(left==1 && right==1 && a.sequence==b.sequence && std::memcmp(&a,&b,sizeof a))return -1;
            value=right==1 && (left!=1 || b.sequence>a.sequence)?b:a;return 1;
        }
        value={};return left<0 || right<0?-1:0;
    }
    bool store(Journal value) const {
        value.check=checksum(value);
        const unsigned side=value.sequence&1u;
        auto* file=std::fopen(journalPath(value.slot,side).c_str(),"wb");
        if(!file)return false;
        const bool wrote=std::fwrite(&value,sizeof value,1,file)==1;
        const bool closed=std::fclose(file)==0;
        Journal check{};
        return wrote && closed && read(value.slot,side,check)==1 && !std::memcmp(&value,&check,sizeof value);
    }
    bool rollback(Journal value) const {
        for(unsigned i=Components;i>0;--i)if(value.mask&(1u<<(i-1))) {
            const unsigned index=i-1;
            Fingerprint source{},archived{};bool haveSource=false,haveArchive=false;
            if(!fingerprint(component(value.slot,index),source,haveSource) ||
               !fingerprint(archivePath(value,index),archived,haveArchive) ||
               haveSource==haveArchive || (haveSource?source:archived)!=value.files[index])return false;
            if(haveArchive && std::rename(archivePath(value,index).c_str(),component(value.slot,index).c_str())!=0)return false;
        }
        value.state=RolledBack;
        if(value.sequence==UINT32_MAX)return false;
        ++value.sequence;return store(value);
    }
};
