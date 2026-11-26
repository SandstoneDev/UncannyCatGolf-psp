#pragma once
#include <cstddef>
#include <cstdint>

namespace WorldState {

// Stable IDs 0..44 follow VillaUnlocks::Id. The three starting costumes were
// absent from that checklist. Catalog follows the original dictionary iteration
// order in assetsNEW/scenes/utils/save_data.gd (including the Cheater call).
struct CatalogRow { uint8_t id; const char* key; };
inline constexpr CatalogRow Catalog[] = {
    {29,"game_peak_hunter"}, {30,"mode_endless"}, {31,"mode_editor"},
    {32,"mod_easy_mode"}, {33,"mod_hard_mode"}, {37,"mod_burst"},
    {41,"mod_telekinesis"}, {34,"mod_buddy"}, {35,"mod_parry"}, {36,"mod_jumpy"},
    {38,"mod_dizzy"}, {40,"mod_missiles"}, {39,"mod_sticky"}, {42,"mod_god"},
    {1,"extra_catartist"}, {5,"minigame_bartbash"}, {3,"minigame_meowl"},
    {4,"minigame_uncannydash"}, {0,"extra_costumes"}, {2,"extra_mystery_phone"},
    {6,"extra_sound_test"}, {45,"costume__blank"}, {46,"costume_canny"},
    {47,"costume_uncanny"}, {8,"costume_ms_canny"}, {9,"costume_retro"},
    {10,"costume_dog"}, {11,"costume_veteran"}, {12,"costume_cannydigo"},
    {13,"costume_rainbow"}, {25,"costume_king"}, {26,"costume_catfish"},
    {27,"costume_builder"}, {28,"costume_nothing"}, {7,"costume_copycat"},
    {23,"costume_arleling"}, {15,"costume_bingus"}, {16,"costume_minnie"},
    {18,"costume_rudy"}, {21,"costume_chen"}, {20,"costume_pig"},
    {19,"costume_ploobie"}, {17,"costume_robert"}, {22,"costume_swag"},
    {24,"costume_true"}, {14,"costume_wires"}, {43,"songs_fields"}, {44,"songs_cosmic"}
};
inline constexpr unsigned KeyCount=48, VisibleCount=43, God=42;
inline constexpr uint64_t KnownMask=(uint64_t{1}<<KeyCount)-1, DefaultOwned=uint64_t{7}<<45;
constexpr uint64_t bit(unsigned id) { return id<KeyCount ? uint64_t{1}<<id : 0; }
constexpr uint64_t catalogMask() {
    uint64_t seen=0;
    for (auto row:Catalog) seen |= bit(row.id);
    return seen;
}
static_assert(sizeof Catalog/sizeof Catalog[0]==KeyCount && catalogMask()==KnownMask);

struct CivilDateTime {
    uint16_t year=0;
    uint8_t month=0, day=0, weekday=0, hour=0, minute=0, second=0;
    bool dst=false;
};
struct Data {
    uint64_t owned=DefaultOwned;
    uint8_t pending[VisibleCount]{};
    uint8_t pendingCount=0, friendPhase=0, questKeys=0;
    int8_t worldProgress=-1;
    bool pennyHi=false, appleHasDied=false;
    int64_t deathUnix=0;
    CivilDateTime deathLocal{};
};
inline bool owns(const Data& d,unsigned id) { return (d.owned&bit(id))!=0; }
inline bool valid(const CivilDateTime& t) {
    constexpr unsigned days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if (!t.year || t.year>9999 || !t.month || t.month>12) return false;
    const bool leap=t.year%4==0 && (t.year%100!=0 || t.year%400==0);
    const unsigned last=days[t.month-1]+(t.month==2 && leap);
    return t.day>=1 && t.day<=last && t.weekday<7 && t.hour<24 && t.minute<60 && t.second<60;
}
inline bool valid(const Data& d) {
    if ((d.owned&~KnownMask) || (d.owned&DefaultOwned)!=DefaultOwned ||
        d.pendingCount>VisibleCount || d.friendPhase>6 || d.worldProgress < -1 || d.worldProgress>6) return false;
    uint64_t seen=d.owned;
    for (unsigned i=0;i<d.pendingCount;++i) {
        const unsigned id=d.pending[i];
        if (id>=VisibleCount || (seen&bit(id))) return false;
        seen |= bit(id);
    }
    const auto& t=d.deathLocal;
    if (d.appleHasDied) return d.deathUnix>=0 && d.deathUnix<=INT64_C(253402300799) && valid(t);
    return d.deathUnix==0 && !t.year && !t.month && !t.day && !t.weekday &&
           !t.hour && !t.minute && !t.second && !t.dst;
}
inline uint64_t pendingMask(const Data& d) {
    uint64_t mask=0;
    for (unsigned i=0;i<d.pendingCount && i<VisibleCount;++i) mask |= bit(d.pending[i]);
    return mask;
}
// master.gd award_unlock: duplicate and invisible awards return before the
// Godmode check. Its prerequisite includes both hidden songs.
inline bool award(Data& d,unsigned id) {
    if (id>=KeyCount || !valid(d)) return false;
    uint64_t earned=d.owned|pendingMask(d);
    if (earned&bit(id)) return false;
    if (id>=VisibleCount) {
        d.owned |= bit(id);
        return true;
    }
    d.pending[d.pendingCount++]=static_cast<uint8_t>(id);
    earned |= bit(id);
    if (earned==(KnownMask&~bit(God))) d.pending[d.pendingCount++]=God;
    return true;
}
inline bool cheater(Data& d) {
    bool changed=false;
    for (auto row:Catalog) changed=award(d,row.id) || changed;
    return changed;
}
inline int nextPresent(const Data& d) {
    return valid(d) && d.pendingCount ? d.pending[d.pendingCount-1] : -1;
}
// Called by the presenter only after the displayed reward has actually opened.
inline bool markOpened(Data& d,unsigned id) {
    if (id>=VisibleCount || nextPresent(d)!=static_cast<int>(id)) return false;
    d.owned |= bit(id);
    d.pending[--d.pendingCount]=0;
    return true;
}
// Restore a snapshot. Confirmed old openings supersede queued copies. Preserve
// insertion order for surviving/new pending entries. Restoration is not a new
// earning event (e.g. restoring the two songs must not synthesize a God gift).
inline bool mergeLegacy(Data& d,uint64_t opened,const uint8_t* orderedPending,std::size_t count) {
    if (!valid(d) || (opened&~KnownMask) || (count && !orderedPending)) return false;
    for (std::size_t i=0;i<count;++i) if (orderedPending[i]>=KeyCount) return false;
    bool changed=(opened&~d.owned)!=0;
    d.owned |= opened;
    unsigned kept=0;
    for (unsigned i=0;i<d.pendingCount;++i) {
        if (!owns(d,d.pending[i])) d.pending[kept++]=d.pending[i];
    }
    for (unsigned i=kept;i<d.pendingCount;++i) d.pending[i]=0;
    d.pendingCount=static_cast<uint8_t>(kept);
    uint64_t seen=d.owned|pendingMask(d);
    for (std::size_t i=0;i<count;++i) {
        const auto id=orderedPending[i];
        if (seen&bit(id)) continue;
        if (id>=VisibleCount) d.owned |= bit(id);
        else d.pending[d.pendingCount++]=id;
        seen |= bit(id);
        changed=true;
    }
    return changed;
}
} // namespace WorldState
