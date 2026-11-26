#pragma once
#include <cstdio>
#include <string>
namespace ucg_storage {
inline constexpr unsigned MaxFiles=32;
// Configure once before creating worker threads; reconfiguration requires all
// virtual FILE handles closed. Normalizes a nested editor boot to game root.
bool configure(const char* executable);
bool eligible(const char* path);
bool isVirtual(std::FILE* file);
bool virtualExists(const char* path);
bool virtualDirectory(const char* path);
// recognized=false only when no descriptor exists (or path is ineligible).
// Never falls back to an ordinary-file open; useful for raw kernel consumers.
std::FILE* openVirtualOnly(const char* path,bool& recognized);
unsigned liveFiles();
unsigned peakFiles();
unsigned rawOpenFiles();
unsigned rawPeakOpenFiles();
// External fixed program memory; do not also charge the renderer cache budget.
unsigned poolResidentBytes();
unsigned readerShellBytes();
unsigned poolPeakPinned();
unsigned poolPinned();
unsigned poolInvalidReleases();
}
