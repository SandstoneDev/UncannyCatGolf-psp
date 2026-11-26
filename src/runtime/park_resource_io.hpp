#pragma once

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include "storage_hooks.hpp"
#ifdef __PSP__
#include <pspiofilemgr.h>
#endif

struct ParkReadStatus {
    const char* phase="not_started";
    int result=0,closeResult=0,error=0;
    unsigned requested=0,completed=0,calls=0;
    unsigned offset=0;
};

// A bounded course-resource read. Preserve the raw kernel result, rather than
// translating non-POSIX PSP errors through newlib errno.
class ParkResourceReader {
public:
    explicit ParkResourceReader(ParkReadStatus& value):status(value) {}
    ParkResourceReader(const ParkResourceReader&)=delete;
    ParkResourceReader& operator=(const ParkResourceReader&)=delete;
    ~ParkResourceReader() { close(); }

    bool open(const char* path) {
        status.phase="park_art_open";
        errno=0;
#ifdef __PSP__
        bool recognized=false;
        virtualFile=ucg_storage::openVirtualOnly(path,recognized);
        if (recognized) {
            status.result=virtualFile?0:-1;
            status.error=errno;
            return virtualFile!=nullptr;
        }
        errno=0;
        descriptor=sceIoOpen(path,PSP_O_RDONLY,0);
        status.result=descriptor;
        status.error=errno;
        return descriptor>=0;
#else
        file=std::fopen(path,"rb");
        status.result=file?0:-1;
        status.error=errno;
        return file!=nullptr;
#endif
    }

    bool seek(unsigned offset) {
        status.phase="park_art_seek";
        status.offset=offset;
        errno=0;
#ifdef __PSP__
        status.result=virtualFile?(std::fseek(virtualFile,offset,SEEK_SET)?-1:static_cast<int>(offset)):
            sceIoLseek32(descriptor,offset,PSP_SEEK_SET);
        status.error=errno;
        return status.result==static_cast<int>(offset);
#else
        status.result=std::fseek(file,offset,SEEK_SET);
        status.error=errno;
        return status.result==0;
#endif
    }

    bool read(void* output,unsigned bytes) {
        status.phase="park_art_read";
        status.requested=bytes;
        auto* destination=static_cast<uint8_t*>(output);
        while (status.completed<bytes) {
            const unsigned chunk=std::min(4096u,bytes-status.completed);
            errno=0;
#ifdef __PSP__
            if (virtualFile) {
                status.result=static_cast<int>(std::fread(destination+status.completed,1,chunk,virtualFile));
                if (!status.result && std::ferror(virtualFile)) status.result=-1;
            } else status.result=sceIoRead(descriptor,destination+status.completed,chunk);
#else
            status.result=static_cast<int>(std::fread(destination+status.completed,1,chunk,file));
            if (!status.result && std::ferror(file)) status.result=-1;
#endif
            status.error=errno;
            ++status.calls;
            if (status.result<=0 || static_cast<unsigned>(status.result)>chunk) return false;
            status.completed+=static_cast<unsigned>(status.result);
        }
        return true;
    }

    bool finish() {
        if (!close()) { status.phase="park_art_close"; return false; }
        status.phase="complete";
        return true;
    }

private:
    ParkReadStatus& status;
#ifdef __PSP__
    SceUID descriptor=-1;
    std::FILE* virtualFile=nullptr;
#else
    std::FILE* file=nullptr;
#endif
    bool close() {
#ifdef __PSP__
        if (virtualFile) {
            auto* value=virtualFile;virtualFile=nullptr;
            status.closeResult=std::fclose(value);
            return status.closeResult>=0;
        }
        if (descriptor<0) return status.closeResult>=0;
        const auto value=descriptor;
        descriptor=-1;
        status.closeResult=sceIoClose(value);
#else
        if (!file) return status.closeResult>=0;
        auto* value=file;
        file=nullptr;
        status.closeResult=std::fclose(value);
#endif
        return status.closeResult>=0;
    }
};
