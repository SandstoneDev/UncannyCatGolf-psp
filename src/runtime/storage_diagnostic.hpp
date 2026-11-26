#pragma once
#include <cerrno>
#include <cstdint>
namespace ucg_storage { namespace diagnostic {
// Enabled only around the diagnostic executable's serial startup Clip open.
// No allocation or file output; preserve the first cause before cleanup.
struct Failure {const char* phase=nullptr;int code=0;uint32_t wanted=0,got=0;};
inline Failure first;
inline bool active=false;
inline void begin(){first={};active=true;}
inline void end(){active=false;}
inline void record(const char* phase,int code,uint32_t wanted=0,uint32_t got=0){
    if(active && !first.phase)first={phase,code,wanted,got};
}
inline bool fail(const char* phase,uint32_t wanted=0,uint32_t got=0){record(phase,errno,wanted,got);return false;}
} }
