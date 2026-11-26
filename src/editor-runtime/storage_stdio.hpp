#pragma once
#include "storage_reader.hpp"
#include <new>

#ifdef __PSP__
// PSPSDK newlib exports funopen in libc.a, but strict -std=c++17 hides its
// declaration behind __BSD_VISIBLE. Declare the same installed ABI explicitly.
extern "C" std::FILE* funopen(const void*,int(*)(void*,char*,int),
    int(*)(void*,const char*,int),fpos_t(*)(void*,fpos_t,int),int(*)(void*));
#endif

namespace ucg_storage {
namespace detail {
inline int close(void* value) { delete static_cast<Reader*>(value);return 0; }
#ifdef __PSP__
inline int read(void* value,char* output,int bytes) {
    if (bytes<0) { errno=EINVAL;return -1; }
    const int result=static_cast<Reader*>(value)->readForStdio(output,bytes);
    if (result<0) errno=EIO;
    return result;
}
inline fpos_t seek(void* value,fpos_t offset,int origin) {
    auto& reader=*static_cast<Reader*>(value);
    if (!reader.seek(offset,origin)) { errno=EINVAL;return static_cast<fpos_t>(-1); }
    return static_cast<fpos_t>(reader.tell());
}
#else
inline ssize_t read(void* value,char* output,size_t bytes) {
    const int result=static_cast<Reader*>(value)->readForStdio(output,static_cast<uint32_t>(std::min<size_t>(bytes,INT_MAX)));
    if (result<0) errno=EIO;
    return result;
}
inline int seek(void* value,off64_t* offset,int origin) {
    auto& reader=*static_cast<Reader*>(value);
    if (!reader.seek(*offset,origin)) { errno=EINVAL;return -1; }
    *offset=reader.tell();return 0;
}
#endif
}
// Explicit descriptor and payload paths. Caller owns the returned FILE as usual.
// Executables and writable user files must continue using ordinary fopen.
inline std::FILE* openIndexedFileOwned(std::FILE* descriptor,const char* payload,Reader::Opener opener=std::fopen) {
    auto* reader=new(std::nothrow) Reader;
    if (!reader) { if (descriptor) std::fclose(descriptor);errno=ENOMEM;return nullptr; }
    if (!reader->openOwned(descriptor,payload,opener)) { delete reader;errno=EIO;return nullptr; }
#ifdef __PSP__
    auto* result=funopen(reader,detail::read,nullptr,detail::seek,detail::close);
#else
    cookie_io_functions_t functions{};
    functions.read=detail::read;functions.seek=detail::seek;functions.close=detail::close;
    auto* result=fopencookie(reader,"rb",functions);
#endif
    if (!result) delete reader;
    return result;
}
inline std::FILE* openIndexedFile(const char* descriptor,const char* payload,Reader::Opener opener=std::fopen) {
    return openIndexedFileOwned(opener(descriptor,"rb"),payload,opener);
}
} // namespace ucg_storage
