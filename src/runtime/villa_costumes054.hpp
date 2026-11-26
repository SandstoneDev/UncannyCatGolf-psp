#pragma once

#include "villa_unlocks054.hpp"
#include "villa_costume_art054.hpp"

#include <string_view>

namespace VillaCostumes {

// These values are the stored preference codes. Append future values to keep old saves valid.
enum class Id : unsigned {
    Canny, Uncanny, Copycat, MsCanny, Retro, Dog, Veteran, Cannydigo, Rainbow, King,
    Catfish, Builder, Nothing, Arleling, Minnie, Rudy, Bingus, Chen, Pig, Ploobie,
    Robert, Swag, True, Wires, Count
};
constexpr unsigned Count = static_cast<unsigned>(Id::Count);

enum class Behavior : unsigned { Still, WalkAndJump, Ghost, Fight };

struct Row {
    const char* key;
    const char* nameKey;
    VillaUnlocks::Id unlock;
    unsigned actor;
    unsigned ghost;
    int x;
    int y;
    float scale;
    float skew;
    int z;
    unsigned lightMask;
    Behavior behavior;
    int moveX;
    int moveY;
    float speedX;
    float speedY;
    float jumpHeight;
    float jumpTime;
    float jumpChance;
    float wait;
    Id opponent;
    float attackSpeed;
    float attackPowerMin;
    float attackPowerMax;
};

#include "villa_costume_rows054.inc"

constexpr bool validPreference(uint32_t code) { return code < Count; }

constexpr const Row& row(Id id) {
    const auto code = static_cast<unsigned>(id);
    return Rows[validPreference(code) ? code : 0];
}

constexpr bool available(Id id, VillaUnlocks::Flags flags) {
    if (!validPreference(static_cast<unsigned>(id))) return false;
    const auto unlock = row(id).unlock;
    return unlock == VillaUnlocks::Id::Count || VillaUnlocks::unlocked(flags, unlock);
}

constexpr Id loadPreference(uint32_t code, VillaUnlocks::Flags flags) {
    return validPreference(code) && available(static_cast<Id>(code), flags) ? static_cast<Id>(code) : Id::Canny;
}

constexpr bool select(Id& selected, Id requested, VillaUnlocks::Flags flags) {
    if (!available(requested, flags) || selected == requested) return false;
    selected = requested;
    return true;
}

constexpr Id find(std::string_view key) {
    for (unsigned i = 0; i < Count; ++i)
        if (Rows[i].key == key) return static_cast<Id>(i);
    return Id::Count;
}

constexpr Id cycle(Id id, VillaUnlocks::Flags flags, bool forward) {
    unsigned code = static_cast<unsigned>(loadPreference(static_cast<unsigned>(id), flags));
    do {
        code = (code + (forward ? 1 : Count - 1)) % Count;
    } while (!available(static_cast<Id>(code), flags));
    return static_cast<Id>(code);
}

constexpr const char* name(Id id, bool russian) {
    const auto unlock = row(id).unlock;
    if (unlock != VillaUnlocks::Id::Count) return VillaUnlocks::Rows[static_cast<unsigned>(unlock)].name.get(russian);
    return id == Id::Uncanny ? (russian ? "Анкэнни" : "Uncanny") : (russian ? "Кэнни" : "Canny");
}

struct Pair { unsigned player, ghost; };

constexpr Pair actorPair(Id selected, Id copycatBase = Id::Canny) {
    // Copy Cat has no texture of its own. The original default art slot uses Canny,
    // and its chosen base costume supplies the ghost pair as well as the base art.
    const Row& costume = row(selected == Id::Copycat && copycatBase != Id::Copycat ? copycatBase : selected);
    return {costume.actor, costume.ghost};
}

constexpr const Patch& residentPatch(Id id, Id copycatBase = Id::Canny) {
    return Art::Residents[actorPair(id, copycatBase).player];
}

// Cat Artist owns these values independently of the selected wearable.
struct CopyCatSlots {
    static constexpr unsigned Count = 3;
    static constexpr uint32_t Blank = VillaCostumes::Count;
    uint32_t active = 0;
    uint32_t bases[Count] = {static_cast<uint32_t>(Id::Canny), static_cast<uint32_t>(Id::Uncanny), Blank};

    constexpr unsigned slot() const { return active < Count ? active : 0; }

    constexpr Pair pair(VillaUnlocks::Flags flags) const {
        const uint32_t base = bases[slot()];
        if (base == Blank) return {Art::BlankArt, row(Id::Canny).ghost};
        return actorPair(base == static_cast<uint32_t>(Id::Copycat) ? Id::Canny : loadPreference(base, flags));
    }
};

constexpr Pair actorPair(Id selected, const CopyCatSlots& art, VillaUnlocks::Flags flags) {
    return selected == Id::Copycat ? art.pair(flags) : actorPair(selected);
}

} // namespace VillaCostumes
