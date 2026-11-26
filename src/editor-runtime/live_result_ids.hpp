#pragma once

#include <algorithm>
#include <iterator>

// Result clips verified against the shared authored results panel and confetti track.
inline constexpr unsigned CommonWinClips040[]={
    4,6,8,10,12,15,17,19,21,23,25,27,29,
    31,33,35,37,39,41,43,45,47,49,53,55,
    57,59,61,64,66,68,70,72,74,76,78,80,
    82,84,86,88,90,94,96,98,100,102,104,106,
    108,110,112,115,117,119,121,123,125,127,129,133,
    135,137,139,141,143,145,147,149,151,153,155,157,
    159,161,163,165,167,172,176,178,180,182,184,186,188,190,192,194,196,198,200,202,204,206,208,210,212,214,216,218,220,222,224,226,228,230,232,234,236,238,240,
};

inline bool isCommonWinClip040(unsigned id) {
    return std::binary_search(std::begin(CommonWinClips040),std::end(CommonWinClips040),id);
}

inline constexpr bool isPilotResultClip040(unsigned id) { return id>=180 && id<=183; }

inline constexpr bool isNativeLossClip041(unsigned id) { return id>=189 && id<=241 && (id&1u); }
