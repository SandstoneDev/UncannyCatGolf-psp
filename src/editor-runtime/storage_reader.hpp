#pragma once
// UCGSTV1 bounded random-access reader. No asset semantics or global state.
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <zlib.h>
#include "storage_diagnostic.hpp"

namespace ucg_storage {
struct DecodedWindow {unsigned char* data=nullptr;uint64_t serial=0;};
// Reader operations require caller serialization; hook providers must outlive
// every Reader that installs them, including shutdown/early-failure destructors.
struct WindowHooks {
 bool(*acquire)(void*,const void*,uint64_t,DecodedWindow&)=nullptr;
 void(*release)(void*,const void*,uint64_t,const DecodedWindow&)=nullptr;
 void(*invalidate)(void*,const void*,uint64_t)=nullptr;void* context=nullptr;
 bool complete()const{return acquire&&release&&invalidate;}
 bool empty()const{return !acquire&&!release&&!invalidate;}
};
class Reader {
public:
    static constexpr uint32_t Block=65536;
    using Opener=std::FILE*(*)(const char*,const char*);
    Reader()=default;
    Reader(const Reader&)=delete;
    Reader& operator=(const Reader&)=delete;
    ~Reader() { close(); }
    // Install only before opening. Misuse latches closed until a valid install
    // after close; it cannot silently switch an active FILE to owned storage.
    bool setWindowHooks(WindowHooks hooks) {
        if(index_||data_||owned_||activeRead_||(!hooks.empty()&&!hooks.complete())) {
            hookError_=failed_=true;return false;
        }
        windows_=hooks;hookError_=false;return true;
    }
    static constexpr size_t ownedWindowBytes(){return Block;}
    void close() {
        if(windows_.complete())windows_.invalidate(windows_.context,this,epoch_);
        if(owned_)std::free(owned_);
        owned_=decoded_=nullptr;cachedSerial_=0;
        if(!++epoch_)++epoch_;
        if (index_) std::fclose(index_);
        if (data_) std::fclose(data_);
        index_=data_=nullptr;length_=count_=position_=0;cached_=UINT32_MAX;failed_=false;decodedValid_=false;smallRaw_=false;
    }
    bool open(const char* indexPath,const char* dataPath,Opener opener=std::fopen) {
        return openOwned(opener(indexPath,"rb"),dataPath,opener);
    }
    // Always consumes descriptor, including on failure. Explicit opener permits
    // --wrap=fopen integration without recursion or probing a descriptor twice.
    bool openOwned(std::FILE* descriptor,const char* dataPath,Opener opener=std::fopen) {
        close();
        if(hookError_){diagnostic::record("window_hooks",errno);if(descriptor)std::fclose(descriptor);return false;}
        index_=descriptor;data_=index_?opener(dataPath,"rb"):nullptr;
        if (!index_) return reject("descriptor_open");
        if (!data_) return reject("payload_open");
        unsigned char header[32];
        const auto headerBytes=std::fread(header,1,32,index_);
        if(headerBytes!=32)return reject("index_header_read",32,headerBytes);
        if (std::memcmp(header,"UCGSTV1\0",8) ||
            word(header+16)!=Block || word(header+24) ||
            crc32(0,header,28)!=word(header+28)) return reject("index_header",word(header+28),crc32(0,header,28));
        length_=word(header+8);count_=word(header+12);
        if (length_>INT32_MAX || count_>length_ || (!length_ && count_) || (length_ && !count_)) return reject("index_counts",length_,count_);
        if (std::fseek(index_,0,SEEK_END)) return reject("index_seek_end");
        const long indexBytes=std::ftell(index_);
        if (indexBytes<32 || uint64_t(count_)*24+32!=static_cast<uint64_t>(indexBytes)) return reject("index_size",count_*24+32,indexBytes);
        if (std::fseek(data_,0,SEEK_END)) return reject("payload_seek_end");
        const long dataBytes=std::ftell(data_);
        if (dataBytes<0 || dataBytes>INT32_MAX) return reject("payload_size",0,dataBytes);
        if (std::fseek(index_,32,SEEK_SET)) return reject("index_seek_entries");
        uint32_t logical=0;uLong crc=0;bool smallRaw=true;
        for (uint32_t i=0;i<count_;++i) {
            unsigned char raw[24];Entry e;
            const auto read=std::fread(raw,1,24,index_);
            if (read!=24) return reject("index_entry_read",24,read);
            crc=crc32(crc,raw,24);parse(raw,e);
            if (e.logical!=logical || !e.raw || e.raw>Block || !e.stored || e.stored>Block ||
                e.codec>6 || (!e.codec && e.raw!=e.stored) || e.offset>static_cast<uint32_t>(dataBytes) ||
                e.stored>static_cast<uint32_t>(dataBytes)-e.offset || e.raw>length_-logical) return reject("index_entry_bounds",length_,e.logical);
            smallRaw=smallRaw&&!e.codec&&e.raw<=sizeof input_;
            logical+=e.raw;
        }
        if (logical!=length_ || crc!=word(header+20)) return reject("index_crc",word(header+20),crc);
        smallRaw_=smallRaw;return true;
    }
    uint32_t size() const { return length_; }
    uint32_t tell() const { return position_; }
    bool error() const { return failed_; }
    bool seek(int64_t offset,int origin) {
        if (!index_) return false;
        int64_t base=0;
        if (origin==SEEK_CUR) base=position_;
        else if (origin==SEEK_END) base=length_;
        else if (origin!=SEEK_SET) return false;
        if (offset < -base || offset>INT32_MAX-base) return false;
        position_=static_cast<uint32_t>(base+offset);
        return true;
    }
    // On corruption returns -1 even if earlier blocks of this call were copied.
    // Callers must discard the destination after any failed read.
    int read(void* destination,uint32_t wanted) {return readInternal(destination,wanted,false);}
    // A positive short callback is not EOF. Stdio refills as needed; only a
    // proved small-RAW file stops at its current record to avoid read-ahead IO.
    int readForStdio(void* destination,uint32_t wanted) {return readInternal(destination,wanted,true);}
private:
    int readInternal(void* destination,uint32_t wanted,bool oneSmallRawBlock) {
        if (!index_ || failed_ || (wanted && !destination) || wanted>INT_MAX) return -1;
        auto* output=static_cast<unsigned char*>(destination);
        uint32_t completed=0;
        const uint32_t available=position_<length_?length_-position_:0;
        wanted=std::min(wanted,available);
        if(!wanted)return 0;
        DecodedWindow window;
        const bool pooled=!smallRaw_&&windows_.complete();
        if(smallRaw_)window={input_,epoch_};
        else if(pooled){
            if(!windows_.acquire(windows_.context,this,epoch_,window)){diagnostic::record("window_acquire",errno);failed_=true;return -1;}
        }else{
            if(!owned_)owned_=static_cast<unsigned char*>(std::malloc(Block));
            if(!owned_){diagnostic::record("window_alloc",errno,Block,0);failed_=true;return -1;}
            window={owned_,epoch_};
        }
        ReadPin pin{this,windows_,epoch_,window,pooled};activeRead_=true;decoded_=window.data;
        if(!decoded_||!window.serial||failed_){diagnostic::record("window_token",errno);failed_=true;return -1;}
        if(cachedSerial_!=window.serial){decodedValid_=false;cachedSerial_=window.serial;}
        while (completed<wanted) {
            if (!decodedValid_ || cached_==UINT32_MAX || position_<entry_.logical || position_-entry_.logical>=entry_.raw) {
                if (!load()) { failed_=true;return -1; }
            }
            const uint32_t within=position_-entry_.logical;
            const uint32_t amount=std::min(wanted-completed,entry_.raw-within);
            std::memcpy(output+completed,decoded_+within,amount);
            completed+=amount;position_+=amount;
            if(oneSmallRawBlock&&smallRaw_)break;
        }
        return static_cast<int>(completed);
    }
private:
    struct Entry { uint32_t logical,offset,stored,raw,crc,codec; } entry_{};
    std::FILE* index_=nullptr;std::FILE* data_=nullptr;
    uint32_t length_=0,count_=0,position_=0,cached_=UINT32_MAX;
    bool failed_=false,hookError_=false,activeRead_=false,decodedValid_=false,smallRaw_=false;
    WindowHooks windows_{};unsigned char* owned_=nullptr;unsigned char* decoded_=nullptr;
    uint64_t epoch_=1,cachedSerial_=0;
    // Only the small encoded-input buffer is per Reader. Decoded storage is
    // owned lazily or borrowed and pinned across the complete read operation.
    alignas(64) unsigned char input_[4096];
    struct ReadPin {
        Reader* reader;WindowHooks hooks;uint64_t epoch;DecodedWindow window;bool pooled;
        ~ReadPin(){reader->decoded_=nullptr;reader->activeRead_=false;if(pooled)hooks.release(hooks.context,reader,epoch,window);}
    };
    static uint32_t word(const unsigned char* p) {
        return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
    }
    static void parse(const unsigned char* p,Entry& e) {
        e={word(p),word(p+4),word(p+8),word(p+12),word(p+16),word(p+20)};
    }
    bool reject(const char* phase,uint32_t wanted=0,uint32_t got=0) {diagnostic::record(phase,errno,wanted,got);close();return false;}
    bool record(uint32_t index,Entry& e) {
        unsigned char raw[24];
        if (index>=count_ || std::fseek(index_,32+static_cast<long>(index)*24,SEEK_SET) ||
            std::fread(raw,1,24,index_)!=24) return diagnostic::fail("record_io",24,0);
        parse(raw,e);return true;
    }
    struct Budget { size_t remaining=48*1024; };
    static voidpf allocate(voidpf opaque,uInt count,uInt bytes) {
        auto& budget=*static_cast<Budget*>(opaque);
        if (!count || bytes>budget.remaining/count) return nullptr;
        void* memory=std::calloc(count,bytes);
        if (!memory) diagnostic::record("zalloc",errno,count*bytes,0);
        if (memory) budget.remaining-=static_cast<size_t>(count)*bytes;
        return memory;
    }
    static void release(voidpf,voidpf memory) { std::free(memory); }
    struct BackIO {
        Reader* owner;
        unsigned encoded,expected,supplied=0,produced=0,calls=0;
        bool readFailed=false;
    };
    static unsigned backInput(void* context,z_const unsigned char** buffer) {
        auto& io=*static_cast<BackIO*>(context);
        const unsigned bytes=std::min<unsigned>(sizeof io.owner->input_,io.encoded-io.supplied);
        if (!bytes) return 0;
        const auto got=std::fread(io.owner->input_,1,bytes,io.owner->data_);
        if (got!=bytes) {
            diagnostic::record("encoded_read",errno,bytes,got);
            io.readFailed=true;return 0;
        }
        io.supplied+=bytes;*buffer=io.owner->input_;return bytes;
    }
    static int backOutput(void* context,unsigned char* buffer,unsigned bytes) {
        auto& io=*static_cast<BackIO*>(context);
        if (buffer!=io.owner->decoded_ || !bytes || bytes>32768 || io.calls>=2 ||
            bytes>io.expected-io.produced) return 1;
        // inflateBack forbids changing its 32 KiB window from either callback.
        // Preserve the first full chunk outside that window; later output stays
        // in the first half until inflateBackEnd has released the stream state.
        if (!io.calls && io.expected>32768) {
            if (bytes!=32768) return 1;
            std::memcpy(io.owner->decoded_+32768,buffer,32768);
        }
        io.produced+=bytes;++io.calls;return 0;
    }
    bool inflateBackBlock(const Entry& e) {
        Budget budget;z_stream stream{};
        stream.zalloc=allocate;stream.zfree=release;stream.opaque=&budget;
        const int initialized=inflateBackInit(&stream,15,decoded_);
        if (initialized!=Z_OK) {diagnostic::record("inflate_back_init",initialized);return false;}
        BackIO io{this,e.stored-6,e.raw};
        stream.next_in=Z_NULL;stream.avail_in=0;
        const int result=inflateBack(&stream,backInput,&io,backOutput,&io);
        // Unlike inflate(), inflateBack() does not promise updated total counts.
        const bool okay=result==Z_STREAM_END && !io.readFailed &&
            io.supplied==io.encoded && !stream.avail_in && io.produced==e.raw;
        const int ended=inflateBackEnd(&stream);
        if (!okay) {diagnostic::record("inflate_back",result,e.raw,io.produced);return false;}
        if (ended!=Z_OK) {diagnostic::record("inflate_back_end",ended);return false;}
        if (e.raw>32768) {
            for (unsigned at=0;at<32768;at+=4) {
                uint32_t first,second;
                std::memcpy(&first,decoded_+at,4);std::memcpy(&second,decoded_+32768+at,4);
                std::memcpy(decoded_+at,&second,4);std::memcpy(decoded_+32768+at,&first,4);
            }
        }
        unsigned char trailer[4];
        const auto trailerBytes=std::fread(trailer,1,sizeof trailer,data_);
        if (trailerBytes!=sizeof trailer) return diagnostic::fail("zlib_trailer",sizeof trailer,trailerBytes);
        const uint32_t expected=uint32_t(trailer[0])<<24 | uint32_t(trailer[1])<<16 |
            uint32_t(trailer[2])<<8 | uint32_t(trailer[3]);
        // The zlib trailer covers filtered bytes, before load() reverses filters.
        const auto actual=adler32(adler32(0,Z_NULL,0),decoded_,e.raw);
        if(actual!=expected)return diagnostic::fail("zlib_adler",expected,actual);
        return true;
    }
    bool inflateBlock(const Entry& e) {
        unsigned char header[2];
        if (e.stored<6) return diagnostic::fail("zlib_size",6,e.stored);
        const auto headerBytes=std::fread(header,1,sizeof header,data_);
        if (headerBytes!=sizeof header) return diagnostic::fail("zlib_header_read",sizeof header,headerBytes);
        const unsigned cmf=header[0],flags=header[1];
        if ((cmf&15)!=Z_DEFLATED || (cmf>>4)>7 || ((cmf<<8|flags)%31) || (flags&32)) return diagnostic::fail("zlib_header",Z_DEFLATED,cmf<<8|flags);
        if (cmf==0x78) return inflateBackBlock(e);
        // Preserve v2's ordinary streaming path for other valid window sizes.
        return !std::fseek(data_,e.offset,SEEK_SET) && inflateStreamBlock(e);
    }
    bool inflateStreamBlock(const Entry& e) {
        Budget budget;z_stream stream{};
        stream.zalloc=allocate;stream.zfree=release;stream.opaque=&budget;
        if (inflateInit(&stream)!=Z_OK) return false;
        unsigned supplied=0;unsigned char overflow=0;bool okay=false;
        for (;;) {
            if (!stream.avail_in && supplied<e.stored) {
                const unsigned bytes=std::min<unsigned>(sizeof input_,e.stored-supplied);
                if (std::fread(input_,1,bytes,data_)!=bytes) break;
                supplied+=bytes;stream.next_in=input_;stream.avail_in=bytes;
            }
            stream.next_out=stream.total_out<e.raw?decoded_+stream.total_out:&overflow;
            stream.avail_out=stream.total_out<e.raw?e.raw-static_cast<unsigned>(stream.total_out):1;
            const auto beforeIn=stream.total_in,beforeOut=stream.total_out;
            const int result=inflate(&stream,Z_NO_FLUSH);
            if (stream.total_out>e.raw) break;
            if (result==Z_STREAM_END) {
                okay=stream.total_in==e.stored && stream.total_out==e.raw && supplied==e.stored && !stream.avail_in;
                break;
            }
            if (result!=Z_OK || (beforeIn==stream.total_in && beforeOut==stream.total_out)) break;
        }
        inflateEnd(&stream);return okay;
    }
    bool restoreBytePlanes(unsigned bytes,unsigned stride) {
        // The inverse shuffle is a permutation from each concatenated channel
        // position to its interleaved output position. Cycles retain one byte;
        // a temporary heap bitmap costs at most 8 KiB, including uneven tails.
        auto* visited=static_cast<unsigned char*>(std::calloc((bytes+7)/8,1));
        if (!visited) return diagnostic::fail("byteplane_alloc",(bytes+7)/8,0);
        const unsigned length=bytes/stride,extra=bytes%stride;
        for (unsigned start=0;start<bytes;++start) {
            if (visited[start/8]&(1u<<(start%8))) continue;
            unsigned current=start;unsigned char value=decoded_[current];
            do {
                visited[current/8]|=static_cast<unsigned char>(1u<<(current%8));
                unsigned channel=0,begin=0;
                while (channel+1<stride) {
                    const unsigned count=length+(channel<extra);
                    if (current<begin+count) break;
                    begin+=count;++channel;
                }
                const unsigned next=(current-begin)*stride+channel;
                const unsigned char displaced=decoded_[next];
                decoded_[next]=value;value=displaced;current=next;
            } while (current!=start);
        }
        std::free(visited);return true;
    }
    bool load() {
        decodedValid_=false;
        uint32_t found=UINT32_MAX;Entry next;
        if (cached_!=UINT32_MAX && position_>=entry_.logical && position_-entry_.logical<entry_.raw) {
            // Eviction invalidates decoded bytes, not the current record hint.
            // Reread that record and apply every existing bounds/CRC check.
            found=cached_;if (!record(found,next)) return false;
        } else if (cached_!=UINT32_MAX && position_==entry_.logical+entry_.raw) {
            found=cached_+1;if (!record(found,next)) return false;
        } else {
            uint32_t lo=0,hi=count_;
            while (lo<hi) {
                const uint32_t mid=lo+(hi-lo)/2;
                if (!record(mid,next)) return false;
                if (next.logical<=position_) lo=mid+1;else hi=mid;
            }
            if (!lo) return false;
            found=lo-1;if (!record(found,next)) return false;
        }
        // The private 4KiB slab must never receive changed compressed/large data.
        if(smallRaw_&&(next.codec||next.raw>sizeof input_))return false;
        // Revalidate bounds after lookup too: do not trust an index changed since open.
        if (position_<next.logical || position_-next.logical>=next.raw || !next.raw || next.raw>Block ||
            !next.stored || next.stored>Block || next.codec>6 || (!next.codec && next.raw!=next.stored) ||
            next.offset>INT32_MAX) return diagnostic::fail("block_bounds");
        if (std::fseek(data_,next.offset,SEEK_SET)) return diagnostic::fail("block_seek",next.offset,0);
        if (next.codec) { if (!inflateBlock(next)) return diagnostic::fail("block_inflate"); }
        else {
            const auto got=std::fread(decoded_,1,next.raw,data_);
            if (got!=next.raw) return diagnostic::fail("raw_read",next.raw,got);
        }
        if (next.codec>=2 && next.codec<=4) {
            const uint32_t stride=next.codec==2?4:next.codec==3?2:1;
            for (uint32_t i=stride;i<next.raw;++i) decoded_[i]+=decoded_[i-stride];
        } else if (next.codec>=5) {
            if (!restoreBytePlanes(next.raw,next.codec==5?4:2)) return false;
        }
        const auto actualCrc=crc32(0,decoded_,next.raw);
        if (actualCrc!=next.crc) return diagnostic::fail("block_crc",next.crc,actualCrc);
        entry_=next;cached_=found;decodedValid_=true;return true;
    }
};
} // namespace ucg_storage
