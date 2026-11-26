#pragma once

#include "world_transition.hpp"
#include "visual.hpp"
#include "selector_gifts.hpp"

namespace VillaUnlocks {
using Flags = uint64_t;

// Preserve bit indices when adding later unlocks to a persisted save.
enum class Id : unsigned {
    Villa, CatArtist, Phone, Meowls, UncannyDash, BortBash, SoundTest,
    Copycat, MsCanny, Retro, Dog, Veteran, Cannydigo, Rainbow,
    Wires, Bingus, Minnie, Robert, Rudy, Ploobie, Pig, Chen, Swag, Arleling, True, King,
    Catfish, Builder, Nothing,
    PeakHunter, Endless, Editor, Chill, Panic, Buddy, Parry, Jumpy, Burst, Dizzy,
    Sticky, Missiles, Telekinesis, Godmode,
    SongsFields, SongsCosmic, Count
};

enum class Section : unsigned { Extras, Costumes, Main };
enum class Status : unsigned { Complete, Shown, Hidden };

struct Text {
    const char* en;
    const char* ru;

    constexpr const char* get(bool russian) const { return russian ? ru : en; }
};

struct Row {
    Id id;
    Section section;
    const char* key;
    Text name;
    Text description;
    Text hint;
    Id prerequisite;
    bool obscured;
    bool available;
};

constexpr Text ChecklistTitle{"Villa Checklist", "Список задач виллы"};
constexpr Text ChecklistHeader{
    "This is the objectives board! Complete tasks to fill up your Summer Villa with more residents and trinkets!",
    "Выполняйте задания, чтобы на летней вилле появлялись новые жители и вещи!"};
constexpr Text UnlockPrefix{"TO UNLOCK...", "КАК ОТКРЫТЬ..."};
constexpr Text SectionTitles[] = {
    {"Minigames and Extras", "Мини-игры и бонусы"},
    {"Costumes", "Костюмы"},
    {"Main Game", "Основная игра"},
};

constexpr Text ClearTutorial{"Make it through World 0.", "Пройдите мир 0."};
constexpr Text ClearGolfCentral{"Make it through World 1.", "Пройдите мир 1."};
constexpr Text ClearGlowstick{"Make it through World 2.", "Пройдите мир 2."};
constexpr Text ClearChuckle{"Make it through World 3.", "Пройдите мир 3."};
constexpr Text ClearFrontier{"Make it through World 4.", "Пройдите мир 4."};
constexpr Text ClearUnknown{"Complete a certain world.", "Пройдите один из миров."};
constexpr Text PeakTutorial{"Show mastery of World 0.", "Получите PEAK на всех лунках мира 0."};
constexpr Text PeakUnknownGlowstick{"Peak a certain level in World 2.", "Получите PEAK на одной из лунок мира 2."};
constexpr Text PeakUnknownFrontier{"Peak a certain level in World 4.", "Получите PEAK на одной из лунок мира 4."};

// Windows 1.0.3b: res://assetsNEW/scenes/game/menus/extras/house_list.tscn
// PCK: .godot/exported/133200997/export-324d2a6b052e3032c5d59a8694276724-house_list.scn
// Status and masking: res://assetsNEW/scenes/menu_objects/house_list_item.gd (.gdc in PCK).
// English strings: res://assetsNEW/translations/UCG_T.csv (UCG_T.en.translation in PCK).
// The original CSV leaves these Russian entries empty. Use PSP translations for the checklist.
constexpr Row Rows[] = {
    {Id::Villa, Section::Extras, "extra_costumes", {"Summer Villa", "Летняя вилла"},
     ClearTutorial, {}, Id::Count, false, true},
    {Id::CatArtist, Section::Extras, "extra_catartist", {"Cat Artist", "Кот-художник"},
     PeakTutorial, {}, Id::Villa, false, false},
    {Id::Phone, Section::Extras, "extra_mystery_phone", {"Mystery Phone", "Таинственный телефон"},
     ClearGolfCentral, {}, Id::Count, false, false},
    {Id::Meowls, Section::Extras, "minigame_meowl", {"Meowls", "Meowls"},
     ClearGolfCentral, {}, Id::Count, true, false},
    {Id::UncannyDash, Section::Extras, "minigame_uncannydash", {"UNCANNY_DASH", "UNCANNY_DASH"},
     ClearGlowstick, {}, Id::Phone, true, false},
    {Id::BortBash, Section::Extras, "minigame_bartbash", {"Bort Bash", "Bort Bash"},
     ClearChuckle, {}, Id::UncannyDash, true, false},
    {Id::SoundTest, Section::Extras, "extra_sound_test", {"Sound Test", "Музыкальный проигрыватель"},
     ClearFrontier, {}, Id::BortBash, false, false},

    {Id::Copycat, Section::Costumes, "costume_copycat", {"Copy Cat", "Кот-копирка"},
     PeakTutorial, {}, Id::CatArtist, true, false},
    {Id::MsCanny, Section::Costumes, "costume_ms_canny", {"Ms. Canny", "Мисс Кэнни"},
     {"Show mastery of World 1.", "Получите PEAK на всех лунках мира 1."}, {}, Id::Phone, false, false},
    {Id::Retro, Section::Costumes, "costume_retro", {"Retro Canny", "Ретро Кэнни"},
     {"Show mastery of World 2.", "Получите PEAK на всех лунках мира 2."}, {}, Id::UncannyDash, false, false},
    {Id::Dog, Section::Costumes, "costume_dog", {"Shiver Dog", "Дрожащий пёс"},
     {"Show mastery of World 3.", "Получите PEAK на всех лунках мира 3."}, {}, Id::BortBash, false, false},
    {Id::Veteran, Section::Costumes, "costume_veteran", {"TD Veteran", "Ветеран TD"},
     {"Show mastery of World 4.", "Получите PEAK на всех лунках мира 4."}, {}, Id::SoundTest, false, false},
    {Id::Cannydigo, Section::Costumes, "costume_cannydigo", {"Cannydigo", "Кэннидиго"},
     {"Show mastery of World 5.", "Получите PEAK на всех лунках мира 5."}, {}, Id::Panic, true, false},
    {Id::Rainbow, Section::Costumes, "costume_rainbow", {"The Rainbow", "Радуга"},
     {"Show mastery of World P.", "Получите PEAK на всех лунках мира P."}, {}, Id::Buddy, true, false},
    {Id::Wires, Section::Costumes, "costume_wires", {"Wires", "Провода"},
     {"Collect 100 canny prisms.", "Соберите 100 призм Кэнни."}, {}, Id::Count, true, false},
    {Id::Bingus, Section::Costumes, "costume_bingus", {"Bingus", "Бингус"},
     {"Collect 150 canny prisms.", "Соберите 150 призм Кэнни."}, {}, Id::Wires, true, false},
    {Id::Minnie, Section::Costumes, "costume_minnie", {"Minnie", "Минни"},
     {"Collect 200 canny prisms.", "Соберите 200 призм Кэнни."}, {}, Id::Bingus, true, false},
    {Id::Robert, Section::Costumes, "costume_robert", {"Robert", "Роберт"},
     {"Collect 250 canny prisms.", "Соберите 250 призм Кэнни."}, {}, Id::Minnie, true, false},
    {Id::Rudy, Section::Costumes, "costume_rudy", {"Rudy", "Руди"},
     {"Collect 300 canny prisms.", "Соберите 300 призм Кэнни."}, {}, Id::Robert, true, false},
    {Id::Ploobie, Section::Costumes, "costume_ploobie", {"Ploobie", "Плуби"},
     {"Collect 325 canny prisms.", "Соберите 325 призм Кэнни."}, {}, Id::Rudy, true, false},
    {Id::Pig, Section::Costumes, "costume_pig", {"Pig", "Свинка"},
     {"Collect 375 canny prisms.", "Соберите 375 призм Кэнни."}, {}, Id::Ploobie, true, false},
    {Id::Chen, Section::Costumes, "costume_chen", {"Chen", "Чен"},
     {"Collect 400 canny prisms.", "Соберите 400 призм Кэнни."}, {}, Id::Pig, true, false},
    {Id::Swag, Section::Costumes, "costume_swag", {"Swag", "Свэг"},
     {"Collect 450 canny prisms.", "Соберите 450 призм Кэнни."}, {}, Id::Chen, true, false},
    {Id::Arleling, Section::Costumes, "costume_arleling", {"Arleling", "Арлелинг"},
     {"Collect 500 canny prisms.", "Соберите 500 призм Кэнни."}, {}, Id::Swag, true, false},
    {Id::True, Section::Costumes, "costume_true", {"True...", "Правда..."},
     {"Collect 525 canny prisms.", "Соберите 525 призм Кэнни."}, {}, Id::Arleling, true, false},
    {Id::King, Section::Costumes, "costume_king", {"King Canny", "Король Кэнни"},
     {"Become a TRUE champion.", "Станьте НАСТОЯЩИМ чемпионом."}, {}, Id::True, true, false},
    {Id::Catfish, Section::Costumes, "costume_catfish", {"Catfish", "Сом"},
     {"Master eternity.", "Покорите вечность."}, {}, Id::Endless, true, false},
    {Id::Builder, Section::Costumes, "costume_builder", {"Hard Hat", "Каска"},
     {"Clear the example custom world.", "Пройдите пример пользовательского мира."}, {}, Id::Editor, true, false},
    {Id::Nothing, Section::Costumes, "costume_nothing", {"Nothing", "Ничто"},
     {"Solve the maze...", "Разгадайте загадку лабиринта..."}, {}, Id::Nothing, true, false},

    {Id::PeakHunter, Section::Main, "game_peak_hunter", {"Peak Hunter", "Охотник за PEAK"},
     ClearFrontier, ClearUnknown, Id::Count, true, false},
    {Id::Endless, Section::Main, "mode_endless", {"Endless Shuffle", "Бесконечное перемешивание"},
     ClearFrontier, ClearUnknown, Id::Count, true, false},
    {Id::Editor, Section::Main, "mode_editor", {"Custom Levels", "Пользовательские уровни"},
     ClearFrontier, ClearUnknown, Id::Count, true, false},
    {Id::Chill, Section::Main, "mod_easy_mode", {"Chill Mode", "Режим отдыха"},
     ClearTutorial, {}, Id::Count, false, true},
    {Id::Panic, Section::Main, "mod_hard_mode", {"Panic Mode", "Режим паники"},
     {"Make it through World 5.", "Пройдите мир 5."}, {}, Id::Robert, true, false},
    {Id::Buddy, Section::Main, "mod_buddy", {"Little Buddy", "Маленький друг"},
     {"Make it through World P.", "Пройдите мир P."}, ClearUnknown, Id::Panic, true, false},
    {Id::Parry, Section::Main, "mod_parry", {"Parry", "Парирование"},
     {"Peak level 2-16.", "Получите PEAK на лунке 2-16."}, {}, Id::UncannyDash, false, false},
    {Id::Jumpy, Section::Main, "mod_jumpy", {"Jumpy", "Прыгучесть"},
     {"Peak level 2-10.", "Получите PEAK на лунке 2-10."}, PeakUnknownGlowstick, Id::UncannyDash, false, false},
    {Id::Burst, Section::Main, "mod_burst", {"Burst", "Всплеск"},
     {"Peak level 3-8.", "Получите PEAK на лунке 3-8."}, {}, Id::BortBash, false, false},
    {Id::Dizzy, Section::Main, "mod_dizzy", {"Dizzy", "Головокружение"},
     {"Peak level 4-6.", "Получите PEAK на лунке 4-6."}, PeakUnknownFrontier, Id::Panic, false, false},
    {Id::Sticky, Section::Main, "mod_sticky", {"Sticky", "Липкость"},
     {"Clear 50 levels in endless mode.", "Пройдите 50 лунок в бесконечном режиме."}, {}, Id::Endless, false, false},
    {Id::Missiles, Section::Main, "mod_missiles", {"Missile Strike", "Ракетный удар"},
     {"Clear 100 levels in endless mode.", "Пройдите 100 лунок в бесконечном режиме."}, {}, Id::Sticky, true, false},
    {Id::Telekinesis, Section::Main, "mod_telekinesis", {"Telekinesis", "Телекинез"},
     {"Clear 150 levels in endless mode.", "Пройдите 150 лунок в бесконечном режиме."}, {}, Id::Missiles, true, false},
    {Id::Godmode, Section::Main, "mod_god", {"Godmode", "Режим бога"},
     {"Unlock everything else!", "Откройте все остальные награды!"}, {}, Id::Count, true, false},
};

constexpr unsigned RowCount = sizeof(Rows) / sizeof(Rows[0]);
constexpr Flags bit(Id id) { return id == Id::Count ? 0 : Flags{1} << static_cast<unsigned>(id); }
constexpr Flags KnownFlags = (bit(Id::SongsCosmic) << 1) - 1;
static_assert(RowCount == static_cast<unsigned>(Id::SongsFields));

constexpr bool unlocked(Flags flags, Id id) { return (flags & bit(id)) != 0; }

constexpr Status status(const Row& row, Flags flags) {
    if (unlocked(flags, row.id)) return Status::Complete;
    if (row.prerequisite != Id::Count && !unlocked(flags, row.prerequisite)) return Status::Hidden;
    return Status::Shown;
}

// Hidden content still occupies its original row, with question marks for both fields.
constexpr bool visible(const Row& row, Flags flags) { return status(row, flags) != Status::Hidden; }

constexpr const char* title(const Row& row, Flags flags, bool russian) {
    const Status state = status(row, flags);
    return state == Status::Complete || (state == Status::Shown && !row.obscured) ? row.name.get(russian) : "???";
}

constexpr const char* condition(const Row& row, Flags flags, bool russian) {
    const Status state = status(row, flags);
    if (state == Status::Hidden) return "???";
    if (state == Status::Shown && row.hint.en) return row.hint.get(russian);
    return row.description.get(russian);
}

// Original costume_house.gd counts the two starting costumes and the unlocked poster and mailbox.
// costume_house.tscn specifies 29 total_objects and 11 streams, even though 32 objects can count.
constexpr unsigned villaMusicStage(Flags flags) {
    unsigned objects = 4;
    for (const Row& row : Rows) {
        const bool counts = row.section == Section::Costumes || (row.section == Section::Extras && row.id != Id::Villa);
        if (counts && unlocked(flags, row.id)) ++objects;
    }
    return objects * 10 / 29;
}

// The original exposes awards after opening gifts (master.gd open_presents). Use the
// existing gifts.cfg opened bits for matching rewards, including for pre-054 profiles.
constexpr Flags giftFlags(uint32_t openedGifts) {
    using Reward = SelectorGifts::Reward;
    constexpr struct {
        Reward gift;
        Id id;
    } gifts[] = {
        {Reward::Villa, Id::Villa}, {Reward::Chill, Id::Chill},
        {Reward::CatArtist, Id::CatArtist}, {Reward::Copycat, Id::Copycat},
        {Reward::Phone, Id::Phone}, {Reward::Meowls, Id::Meowls},
        {Reward::MsCanny, Id::MsCanny}, {Reward::Wires, Id::Wires},
        {Reward::Parry, Id::Parry}, {Reward::UncannyDash, Id::UncannyDash},
        {Reward::Retro, Id::Retro}, {Reward::Burst, Id::Burst},
        {Reward::BartBash, Id::BortBash},
    };
    Flags flags = 0;
    for (const auto& gift : gifts)
        if (openedGifts & (uint32_t{1} << static_cast<unsigned>(gift.gift))) flags |= bit(gift.id);
    return flags;
}

inline Flags campaignFlags(const Progress& progress, uint32_t openedGifts) {
    Flags flags = giftFlags(openedGifts);
    const auto award = [&](Id id, bool reached) {
        if (reached) flags |= bit(id);
    };
    const auto peaked = [&](unsigned begin, unsigned end) {
        return std::all_of(progress.best + begin, progress.best + end,
                           [](const LevelBest& best) { return best.rank == 5; });
    };

    // res://assetsNEW/scenes/game/menus/game/pthru.tscn defines the world PEAK awards.
    award(Id::Dog, peaked(ChuckleFirstCourse, FinalFrontierFirstCourse));
    award(Id::Veteran, peaked(FinalFrontierFirstCourse, ElysianFieldsCourse));
    award(Id::Cannydigo, peaked(ElysianFieldsCourse, CosmicFirstCourse));
    award(Id::Rainbow, peaked(CosmicFirstCourse, CourseCount));

    // 4_17.tscn awards the four flags on win, win/retry or skip. The Wall is 4_18.
    if (progress.next > CataclysmCourse || progress.best[CataclysmCourse].time != UINT32_MAX)
        flags |= bit(Id::PeakHunter) | bit(Id::Endless) | bit(Id::Editor) | bit(Id::SoundTest);

    // 5_18.tscn also awards on the Golden Tee, which records a time before the ending.
    if (progress.next >= CosmicFirstCourse || progress.best[Elysian518Course].time != UINT32_MAX)
        flags |= bit(Id::Panic) | bit(Id::SongsFields);

    // P_18.tscn awards on completion. P_18 is unskippable in this port.
    if (progress.next == CourseCount || progress.best[CosmicEighteenthCourse].time != UINT32_MAX)
        flags |= bit(Id::Buddy) | bit(Id::SongsCosmic);

    // The regular 2_10 and 4_06 scene roots attach these to peak_unlocks.
    award(Id::Jumpy, progress.best[GlowstickFirstCourse + 9].rank == 5);
    award(Id::Dizzy, progress.best[FrontierMazeCourse].rank == 5);

    // res://assetsNEW/scenes/master.tscn defines the prism thresholds for one playthrough.
    constexpr struct {
        Id id;
        unsigned count;
    } prismAwards[] = {
        {Id::Bingus, 150}, {Id::Minnie, 200}, {Id::Robert, 250}, {Id::Rudy, 300},
        {Id::Ploobie, 325}, {Id::Pig, 375}, {Id::Chen, 400}, {Id::Swag, 450},
        {Id::Arleling, 500}, {Id::True, 525}, {Id::King, 565},
    };
    const unsigned prisms = progress.totalPrisms();
    for (const auto& prism : prismAwards) award(prism.id, prisms >= prism.count);

    // Endless (50/100/150 slots; Catfish above 500,000 score), Editor (ExampleWorld
    // with zero skips), and the final Robert cutscene (Nothing) require their own events.
    // Godmode depends on every other unlock in master.gd, including the hidden songs.
    return flags;
}
} // namespace VillaUnlocks
