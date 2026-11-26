#pragma once
// Narrow Godot 4.4 ColorPalette interchange. No ResourceLoader or script execution.
#include "ucg_editor_model.hpp"
#include "editor_allocation_probe.hpp"
#include <array>
#include <cstdint>
#include <cerrno>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <fcntl.h>
#include <unistd.h>
namespace EditorPaletteFiles {
using Color=ucg::editor::Color;
inline constexpr unsigned MaxColors=4096,MaxFileBytes=512*1024;
struct FileOps {
    size_t (*write)(const void*,size_t,size_t,FILE*)=std::fwrite;
    int (*rename)(const char*,const char*)=std::rename;
};
namespace detail {
struct File {FILE*f=nullptr;~File(){if(f)std::fclose(f);}bool close(){if(!f)return true;FILE*x=f;f=nullptr;return std::fclose(x)==0;}};
inline bool pathValid(const std::string&p){return !p.empty()&&p.size()<2048&&p.find('\0')==std::string::npos;}
inline int extension(const std::string&p){auto x=p.find_last_of('.');if(x==std::string::npos)return 0;auto s=p.substr(x);for(char&c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s==".tres"?1:s==".res"?2:0;}
inline bool channel(double v){return std::isfinite(v)&&v<=std::numeric_limits<float>::max()&&v>=-std::numeric_limits<float>::max();}
inline bool reserve(std::vector<Color>&v,unsigned n,std::string&e){
    if(n>MaxColors){e="palette color count exceeds4096";return false;}
    if(!n)return true;
#ifdef __cpp_exceptions
    try {v.reserve(n);}catch(const std::bad_alloc&){e="palette allocation failed";return false;}
#else
    // std::vector's required public allocator cannot report failure without exceptions.
    // Preflight the exact bounded block plus margin before its single allocation.
    const size_t bytes=static_cast<size_t>(n)*sizeof(Color)+64;
    if(!EditorMemory::probe(bytes,bytes,1)){e="palette allocation failed";return false;}
    v.reserve(n);
#endif
    return true;
}
struct Reader {
    FILE*f;unsigned size,pos=0;bool ok=true,big=false;
    bool read(void*p,unsigned n){if(!ok||n>size-pos){ok=false;return false;}if(std::fread(p,1,n,f)!=n){ok=false;return false;}pos+=n;return true;}
    unsigned u32(){unsigned char b[4]{};read(b,4);return big?(unsigned(b[0])<<24|unsigned(b[1])<<16|unsigned(b[2])<<8|b[3]):(unsigned(b[3])<<24|unsigned(b[2])<<16|unsigned(b[1])<<8|b[0]);}
    uint64_t u64(){uint64_t a=u32(),b=u32();return big?(a<<32)|b:a|(b<<32);}
    bool string(char*out,unsigned cap){unsigned n=u32();if(!ok||!n||n>cap||!read(out,n)){ok=false;return false;}if(out[n-1]!=0||std::memchr(out,0,n-1)){ok=false;return false;}return true;}
    bool seek(unsigned n){if(n>size||std::fseek(f,n,SEEK_SET)){ok=false;return false;}pos=n;return true;}
    float scalar(){unsigned b=u32();float x;std::memcpy(&x,&b,4);if(!std::isfinite(x))ok=false;return x;}
};
inline bool binary(Reader&r,std::vector<Color>&out,std::string&e){
    char magic[4]{};if(!r.read(magic,4))return false;if(!std::memcmp(magic,"RSCC",4)){e="compressed RSCC palettes unsupported; save normal .res or .tres";return false;}
    if(std::memcmp(magic,"RSRC",4)){e="not a Godot RSRC ColorPalette";return false;}
    unsigned endian=r.u32();if(endian>1)return false;r.big=endian!=0;
    unsigned real64=r.u32(),major=r.u32(),minor=r.u32(),version=r.u32();(void)minor;
    char str[256]{};if(real64>1||major!=4||version!=6||!r.string(str,sizeof str)||std::strcmp(str,"ColorPalette"))return false;
    if(r.u64()!=0)return false;
    unsigned flags=r.u32();r.u64();if(flags&~7u)return false;
    for(unsigned i=0;i<11;++i)if(r.u32()!=0)return false;
    unsigned names=r.u32();if(names>16)return false;
    char table[16][256]{};for(unsigned i=0;i<names;++i)if(!r.string(table[i],sizeof table[i]))return false;
    if(r.u32()!=0||r.u32()!=1||!r.string(str,sizeof str))return false;
    uint64_t offset=r.u64();if(!r.ok||offset<r.pos||offset>r.size||!r.seek(static_cast<unsigned>(offset))||!r.string(str,sizeof str)||std::strcmp(str,"ColorPalette"))return false;
    unsigned properties=r.u32();if(properties>4)return false;
    unsigned seen=0;
    for(unsigned p=0;p<properties;++p){unsigned id=r.u32(),kind=r.u32();if(!r.ok||id>=names)return false;
        const char*name=table[id];
        if(!std::strcmp(name,"colors")){
            if((seen&1)||kind!=36)return false;
            seen|=1;unsigned n=r.u32();if(!r.ok||n>MaxColors||n>(r.size-r.pos)/16)return false;
            if(!reserve(out,n,e))return false;
            for(unsigned i=0;i<n;++i){Color c;c.r=r.scalar();c.g=r.scalar();c.b=r.scalar();c.a=r.scalar();if(!r.ok)return false;out.push_back(c);}
        }else if(!std::strcmp(name,"script")){if((seen&2)||kind!=1)return false;
            seen|=2;}
        else if(!std::strcmp(name,"resource_name")){if((seen&4)||kind!=5||!r.string(str,sizeof str))return false;
            seen|=4;}
        else if(!std::strcmp(name,"resource_local_to_scene")){if((seen&8)||kind!=2||r.u32()>1)return false;
            seen|=8;}
        else return false;
    }
    r.read(magic,4);return r.ok&&r.pos==r.size&&!std::memcmp(magic,"RSRC",4);
}
// Streaming tokenizer: no file-sized allocation, identifiers/quoted metadata bounded.
struct Text {
    FILE*f;int ch;char token[256]{};bool ok=true;unsigned used=0;
    explicit Text(FILE*x):f(x),ch(std::fgetc(f)){if(ch==0xef){if(std::fgetc(f)!=0xbb||std::fgetc(f)!=0xbf)ok=false;ch=std::fgetc(f);}}
    void step(){ch=std::fgetc(f);if(++used>MaxFileBytes)ok=false;}
    void next(){token[0]=0;if(!ok)return;for(;;){while(ch!=EOF&&std::isspace(static_cast<unsigned char>(ch)))step();if(ch!='#'&&ch!=';')break;while(ch!=EOF&&ch!='\n')step();}
        unsigned n=0;if(ch==EOF){if(std::ferror(f))ok=false;return;}
        if(ch=='"'){token[n++]='"';step();while(ok&&ch!=EOF&&ch!='"'){if(ch=='\\'){step();if(ch==EOF){ok=false;break;}}if(n+2>=sizeof token){ok=false;break;}token[n++]=static_cast<char>(ch);step();}if(ch!='"')ok=false;else step();token[n++]='"';}
        else if(std::isalnum(static_cast<unsigned char>(ch))||ch=='_'||ch=='+'||ch=='-'||ch=='.'){
            do{if(n+1>=sizeof token){ok=false;break;}token[n++]=static_cast<char>(ch);step();}while(ch!=EOF&&(std::isalnum(static_cast<unsigned char>(ch))||ch=='_'||ch=='+'||ch=='-'||ch=='.'));
        }else{token[n++]=static_cast<char>(ch);step();}token[n]=0;
    }
    bool is(const char*s)const{return ok&&!std::strcmp(token,s);}
    bool take(const char*s){if(!is(s))return false;next();return true;}
    bool number(float&v){if(!ok||!token[0])return false;for(const char*p=token;*p;++p)if(!std::isdigit(static_cast<unsigned char>(*p))&&*p!='+'&&*p!='-'&&*p!='.'&&*p!='e'&&*p!='E')return false;char*end=nullptr;float x=std::strtof(token,&end);if(end==token||*end||!std::isfinite(x))return false;v=x;next();return true;}
};
inline bool text(FILE*f,std::vector<Color>*out,unsigned&count){
    Text t(f);t.next();if(!t.take("[")||!t.take("gd_resource"))return false;unsigned head=0;
    while(!t.is("]")){
        if(t.take("type")){if((head&1)||!t.take("=")||!t.take("\"ColorPalette\""))return false;head|=1;}
        else if(t.take("format")){if((head&2)||!t.take("=")||!t.take("3"))return false;head|=2;}
        else if(t.take("load_steps")){if(!t.take("=")||!t.take("1"))return false;}
        else if(t.take("uid")){if(!t.take("=")||t.token[0]!='"')return false;t.next();}
        else return false;
    }
    if(head!=3||!t.take("]")||!t.take("[")||!t.take("resource")||!t.take("]"))return false;
    unsigned seen=0;count=0;
    while(t.ok&&t.token[0]){
        if(t.take("colors")){
            if((seen&1)||!t.take("=")||!t.take("PackedColorArray")||!t.take("("))return false;
            seen|=1;
            if(!t.is(")"))for(;;){float v[4];for(unsigned j=0;j<4;++j){if(!t.number(v[j])||(j<3&&!t.take(",")))return false;}if(++count>MaxColors)return false;if(out){if(count>out->capacity())return false;out->push_back({v[0],v[1],v[2],v[3]});}if(t.is(")"))break;if(!t.take(",")||t.is(")"))return false;}
            if(!t.take(")"))return false;
        }else if(t.take("script")){if((seen&2)||!t.take("=")||!t.take("null"))return false;
            seen|=2;}
        else if(t.take("resource_name")){if((seen&4)||!t.take("=")||t.token[0]!='"')return false;
            seen|=4;t.next();}
        else if(t.take("resource_local_to_scene")){if((seen&8)||!t.take("=")||(!t.is("true")&&!t.is("false")))return false;
            seen|=8;t.next();}
        else return false;
    }
    return t.ok;
}
struct Writer {
    FILE*f;const FileOps&ops;bool ok=true;unsigned size=0;
    void bytes(const void*p,unsigned n){if(!ok)return;if(n>MaxFileBytes-size||ops.write(p,1,n,f)!=n){ok=false;return;}size+=n;}
    void u32(unsigned n){unsigned char b[4]={static_cast<unsigned char>(n),static_cast<unsigned char>(n>>8),static_cast<unsigned char>(n>>16),static_cast<unsigned char>(n>>24)};bytes(b,4);}
    void u64(uint64_t n){u32(static_cast<unsigned>(n));u32(static_cast<unsigned>(n>>32));}
    void string(const char*s){unsigned n=static_cast<unsigned>(std::strlen(s))+1;u32(n);bytes(s,n);}
    void plain(const char*s){bytes(s,static_cast<unsigned>(std::strlen(s)));}
    void scalar(double v){float f32=static_cast<float>(v);unsigned n;std::memcpy(&n,&f32,4);u32(n);}
};
inline void binary(Writer&w,const std::vector<Color>&c){
    w.plain("RSRC");w.u32(0);w.u32(0);w.u32(4);w.u32(4);w.u32(6);w.string("ColorPalette");w.u64(0);w.u32(3);w.u64(UINT64_MAX);for(unsigned i=0;i<11;++i)w.u32(0);
    w.u32(1);w.string("colors");w.u32(0);w.u32(1);w.string("local://ColorPalette");unsigned patch=w.size;w.u64(0);unsigned offset=w.size;
    w.string("ColorPalette");w.u32(1);w.u32(0);w.u32(36);w.u32(static_cast<unsigned>(c.size()));for(const auto&v:c){w.scalar(v.r);w.scalar(v.g);w.scalar(v.b);w.scalar(v.a);}w.plain("RSRC");
    unsigned end=w.size;if(w.ok&&std::fseek(w.f,patch,SEEK_SET)==0){w.u64(offset);if(std::fseek(w.f,end,SEEK_SET))w.ok=false;}else w.ok=false;
}
inline void text(Writer&w,const std::vector<Color>&c){
    w.plain("[gd_resource type=\"ColorPalette\" format=3]\n\n[resource]\ncolors = PackedColorArray(");bool first=true;
    for(const auto&v:c)for(double d:{v.r,v.g,v.b,v.a}){if(!first)w.plain(", ");first=false;float n=static_cast<float>(d);char b[48];if(n==0&&std::signbit(n))std::strcpy(b,"-0.0");else std::snprintf(b,sizeof b,"%.9g",static_cast<double>(n));w.plain(b);}
    w.plain(")\n");
}
inline bool exists(const std::string&p){File f;f.f=std::fopen(p.c_str(),"rb");return f.f!=nullptr;}
inline std::string unused(const std::string&p,const char*s){for(unsigned i=0;i<32;++i){std::string v=p+s+std::to_string(i);if(!exists(v))return v;}return {};}
} // detail
inline bool load(const std::string&path,std::vector<Color>&colors,std::string&error){
    error.clear();int ext=detail::extension(path);if(!detail::pathValid(path)||!ext){error="palette requires .tres or .res filename";return false;}detail::File f;f.f=std::fopen(path.c_str(),"rb");if(!f.f){error="cannot open palette";return false;}
    if(std::fseek(f.f,0,SEEK_END)){error="cannot size palette";return false;}long length=std::ftell(f.f);if(length<0||static_cast<unsigned long>(length)>MaxFileBytes||std::fseek(f.f,0,SEEK_SET)){error="palette file exceeds512KiB or cannot seek";return false;}
    std::vector<Color> result;bool ok=false;
    if(ext==2){detail::Reader r{f.f,static_cast<unsigned>(length)};ok=detail::binary(r,result,error);}
    else{unsigned n=0;ok=detail::text(f.f,nullptr,n);if(ok)ok=detail::reserve(result,n,error);if(ok)ok=std::fseek(f.f,0,SEEK_SET)==0&&detail::text(f.f,&result,n);}
    if(!ok){if(error.empty())error="invalid or unsupported ColorPalette data";return false;}if(!f.close()){error="palette close failed";return false;}colors.swap(result);return true;
}
inline bool save(const std::string&path,const std::vector<Color>&colors,std::string&error,const FileOps&ops={}){
    error.clear();int ext=detail::extension(path);if(!detail::pathValid(path)||!ext){error="palette requires .tres or .res filename";return false;}if(colors.size()>MaxColors){error="palette color count exceeds4096";return false;}
    for(const auto&c:colors)if(!detail::channel(c.r)||!detail::channel(c.g)||!detail::channel(c.b)||!detail::channel(c.a)){error="palette channels must be finite F32";return false;}
    std::string temp=detail::unused(path,".palette-tmp-"),backup=detail::unused(path,".palette-bak-");if(temp.empty()||backup.empty()){error="no palette transaction filename";return false;}
    int fd=::open(temp.c_str(),O_WRONLY|O_CREAT|O_EXCL,0666);if(fd<0){error="cannot create palette temporary file";return false;}detail::File f;f.f=::fdopen(fd,"wb");if(!f.f){::close(fd);std::remove(temp.c_str());error="cannot open palette temporary stream";return false;}
    detail::Writer w{f.f,ops};if(ext==1)detail::text(w,colors);else detail::binary(w,colors);bool ok=w.ok;if(std::fflush(f.f))ok=false;if(!f.close())ok=false;
    if(!ok){std::remove(temp.c_str());error="palette write failed; original preserved";return false;}
    if(ops.rename(temp.c_str(),path.c_str())==0)return true;
    if(!detail::exists(path)){std::remove(temp.c_str());error="palette commit failed";return false;}
    if(ops.rename(path.c_str(),backup.c_str())!=0){std::remove(temp.c_str());error="palette backup failed; original preserved";return false;}
    if(ops.rename(temp.c_str(),path.c_str())!=0){bool restored=ops.rename(backup.c_str(),path.c_str())==0;std::remove(temp.c_str());error=restored?"palette commit failed; original restored":"palette rollback failed; original retained at "+backup;return false;}
    std::remove(backup.c_str());return true;
}
} // EditorPaletteFiles
