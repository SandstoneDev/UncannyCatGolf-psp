#pragma once
#include <cstdint>
namespace EditorGroundNativeProof {
inline constexpr uint32_t Magic=0x47435031,Abi=1,Words=48,KeyBytes=4096;
struct Probe{uint32_t words[Words];uint8_t key[KeyBytes];};
static_assert(sizeof(Probe)==4288,"explicit capture-only ABI");
inline constexpr unsigned FixedBytes=sizeof(Probe);
#ifdef UCG_EDITOR_CAPTURE
inline volatile Probe probe{};
// Word47 is unpublished working state; publish copies it to word37.
inline void outcome(uint32_t n){probe.words[47]=n;}
#endif
}
