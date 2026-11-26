#pragma once
#include <algorithm>
namespace ucg::editor_audio {
// Module-only residency; the main game's original prefix policy is unchanged.
// BortVoices consumes the entire263 clip directly. Master/World menu players
// must work while scene resources load; selected costume audio is shared by
// jumpscare, graze and Buddy nodes and is borrowed by the direct voice pool.
inline constexpr bool moduleResidentEffect(unsigned id,unsigned selectedGhost){
    return id==263||id==312||id==321||id==348||id==359||id==selectedGhost||id==322||id==330||id==332||id==333||id==357||id==358;
}
inline constexpr bool moduleLegacyHudEffect(unsigned id){return (id>=314&&id<=321)||id==329||id==348||id==359||id==360||id==361;}
inline constexpr unsigned moduleEffectFrames(unsigned id,unsigned frames,unsigned selectedGhost){
    return moduleResidentEffect(id,selectedGhost)?frames:std::min(frames,moduleLegacyHudEffect(id)?2048u:512u);
}
}
