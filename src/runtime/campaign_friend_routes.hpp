#pragma once

#include "routes037.hpp"

#include <initializer_list>

// Include after the campaign runtime definitions.
namespace CampaignFriendRoute {
bool startShot(Round& round, int x, int y) {
    round.updateControls({x / 127.0f, y / 127.0f}, true, false);
    return round.updateControls({}, false, false) == ControlEvent::Shot;
}

bool settle(Round& round, unsigned limit) {
    for (unsigned tick = 0; tick < limit && !round.caught && !round.game.won &&
         round.game.moving(); ++tick) round.step();
    return !round.caught && (round.game.won || !round.game.moving());
}

bool playOpening(const char* executable, Course& course, Round& round, SaveFile& save, const char* log) {
    // Authored stick vectors from runVisualTest. Each shot starts at normal game rest.
    static const std::initializer_list<Vec2> routes[]{
        {{0.57f,0.84f}}, {{0.503f,0.308f}}, {{0.036913f,0.845445f}}, {{0,-0.59f}},
        {{0.103510f,0.736512f}},
        {{0.593144f,0.603588f}}, {{-0.729063f,-0.317006f}},
        {{-0.672850f,-0.507029f}}, {{-0.9475f,0},{0.475f,0}},
        {{-0.90445805f,-0.06324580f},{-0.96126169f,0.27563736f}},
        {{-0.32605916f,0.89584023f},{0.75123686f,0.58693057f},{0.30000001f,0}},
        {{-0.97814763f,0.20791161f},{0.49999997f,0.86602545f},{0.86602539f,0.5f},
         {0.64278764f,0.76604444f},{0.87154388f,0.24991120f},{0.62628490f,-0.02187042f},
         {0.99756408f,-0.06975638f},{0.95333326f,0},{0.10452836f,-0.99452192f},
         {-0.13917325f,-0.99026805f},{-0.99939084f,-0.03489963f},{-0.58730084f,0.49280369f}},
        {{0.62982357f,0.65220141f}},
        {{0.05574849f,0.53041166f},{-0.40673664f,-0.91354543f}},
        {{0.71813065f,0.38183686f}}, {{-0.79737806f,-0.32216170f}},
        {{-0.50549519f,-0.69575471f}},
        {{-0.47879738f,-0.76623654f},{0.80656719f,0.02816595f}},
        {{-0.63685477f,-0.70729905f},{0.97029573f,0.24192192f}},
        {{-0.16527222f,0.93730521f},{0.55943346f,-0.76999366f},
         {-0.03321608f,0.95118493f},{-0.06639187f,0.94944620f}},
        {{0.54977250f,0.65519327f},{0.95176470f,0},{0.90132850f,0.06302702f}},
        {{0.84697044f,0.11903394f},{-0.94944626f,0.06639152f}},
        {{0.55995119f,0.43748182f}}
    };
    static_assert(std::size(routes) == GlowstickFirstCourse);
    if (progress.next || progress.quest || !enterCourse(executable, 0, course, round)) return false;
    for (unsigned index = 0; index < GlowstickFirstCourse; ++index) {
        if (currentCourse != index || round.caught || round.ticks) return false;
        for (Vec2 aim : routes[index]) {
            round.updateControls(aim, true, false);
            if (round.updateControls({}, false, false) != ControlEvent::Shot) return false;
            round.coins.multiplier = 0;
            if (!settle(round, 480)) return false;
        }
        if (!round.game.won) return false;
        logEvent(log, "friend_opening_goal", round.game);
        const auto outcome = advanceCampaign(executable, course, round, save, log);
        if (outcome == Advance::NextHole) continue;
        if (outcome != Advance::WorldCleared) return false;
        Flow results;
        results.screen = Screen::WorldResults;
        leaveWorldResults(results, true);
        if (results.screen != Screen::Hole || menuCourse != index + 1 ||
            !enterCourse(executable, menuCourse, course, round)) return false;
    }
    Progress saved;
    return save.read(saved) && saved.next == GlowstickFirstCourse &&
        !saved.quest && !saved.terryKey && !saved.golfKey;
}

bool talkToTerry(Round& round, SaveFile& save, const char* log) {
    if (currentCourse != 1 || progress.quest < 2 || progress.terryKey) return false;
    for (unsigned click = 0; click < 15; ++click) {
        if (interactTerry(round, save, log) != TerryEvent::Grow) return false;
        round.step();
    }
    return interactTerry(round, save, log) == TerryEvent::Key && progress.terryKey == 1;
}

bool swimToFriend(Round& round) {
    if (currentCourse != PoolCourse || !canSwim || progress.quest) return false;
    if (!startShot(round, -79, -119) || !settle(round, 480) || !startShot(round, 121, 49)) return false;
    for (unsigned tick = 0; tick < 480 && !round.caught && !round.game.won &&
         !progress.quest && round.game.moving(); ++tick) round.step();
    return !round.caught && round.pool.swimming && round.pool.showingSpeech && progress.quest == 1;
}

bool finishJumpyChallenge(Round& round) {
    return currentCourse == JumpCourse && startShot(round, -124, 19) &&
        settle(round, 420) && round.game.won && round.game.strokes == 1 && round.speedRun.collected == 0x1d;
}

bool collectBunker(Round& round) {
    if (currentCourse != TutorialCount + 6 || progress.quest < 2 || progress.golfKey) return false;
    if (!startShot(round, -31, 105)) return false;
    for (unsigned tick = 0; tick < 240 && !round.caught && !round.game.won &&
         !progress.golfKey; ++tick) round.step();
    return !round.caught && progress.golfKey == 1 && round.questKeyTick;
}

bool collectMarcy(Round& round) {
    if (currentCourse != MartCourse || !round.marcy.hintVisible() || progress.parkKey) return false;
    constexpr int shots[][2]{{-106,-99}, {69,125}};
    for (const auto& shot : shots) {
        if (!startShot(round, shot[0], shot[1]) || !settle(round, 480)) return false;
    }
    if (round.marcy.cleans != MarcyRun::MaxCleans || !round.marcy.finishedAt) return false;
    for (unsigned tick = 0; tick < 180 && !round.caught && !progress.parkKey; ++tick) round.step();
    return !round.caught && progress.parkKey == 1 && round.questKeyTick;
}

bool collectCosmic(Round& round) {
    if (currentCourse != CosmicThirteenthCourse || progress.quest < 2 || progress.cosmicKey) return false;
    constexpr int shots[][2]{{-127,5}, {-115,-59}, {-115,53}};
    for (const auto& shot : shots) {
        if (!startShot(round, shot[0], shot[1]) || !settle(round, 480)) return false;
    }
    if (!startShot(round, 81, 97)) return false;
    for (unsigned tick = 0; tick < 480 && !round.caught && !round.game.won &&
         round.game.moving() && !progress.cosmicKey; ++tick) round.step();
    return !round.caught && progress.cosmicKey == 1 &&
        round.movingGoal.face == MovingGoal051::Face::Cry && round.questKeyTick;
}

bool collectCity(Round& round) {
    if (currentCourse != CityCourse || !round.secret.available || progress.glowstickKey) return false;
    constexpr int shots[][2]{{127, 0}, {-30, 127}, {-48, -99}};
    for (const auto& shot : shots) {
        if (!startShot(round, shot[0], shot[1]) || !settle(round, 480)) return false;
    }
    if (round.secret.startedAt != 53 || round.game.won || round.ticks != 168 || progress.glowstickKey) return false;
    for (unsigned tick = 0; tick < 28 && !progress.glowstickKey && !round.caught; ++tick) round.step();
    return !round.caught && round.ticks == 196 && round.secret.collected && progress.glowstickKey == 1;
}

bool collectFrontier(Round& round) {
    if (currentCourse != CataclysmCourse || !round.finale417.keyAvailable || progress.frontierKey) return false;
    for (unsigned shot = 0; shot < Route417.count; ++shot) {
        const auto input = Route417.shots[shot];
        for (unsigned tick = 0; tick < input.wait && !round.caught && !round.game.won; ++tick) round.step();
        if (round.caught || !startShot(round, input.x, input.y) || !settle(round, 2400)) return false;
    }
    return round.game.won && round.finale417.ufoAt && progress.frontierKey == 1;
}

bool defeatHorse(Round& round) {
    // Strokes from the proven focused045 route. Use round ticks, which pause for the cutscene.
    constexpr struct Stroke { unsigned tick; int x, y; } shots[]{
        {62,33,-14}, {118,-36,-20}, {175,-5,-87}, {321,20,51}, {374,75,-44},
        {462,-38,-61}, {631,26,43}, {681,5,-89}, {813,3,-53}, {878,1,-49},
        {953,-5,-76}, {1075,49,46}, {1190,-51,-8}
    };
    if (currentCourse != Elysian518Course || round.ticks || round.finale518.phase != FinalePhase518::Hunt) return false;
    unsigned shot = 0;
    for (unsigned frame = 0; frame < 1500 && !round.caught &&
         round.finale518.phase == FinalePhase518::Hunt; ++frame) {
        if (shot < std::size(shots) && round.ticks == shots[shot].tick) {
            if (!startShot(round, shots[shot].x, shots[shot].y)) return false;
            ++shot;
        }
        round.step();
    }
    if (round.caught || shot != std::size(shots) || round.ticks != 1300 ||
        round.finale518.phase != FinalePhase518::Defeat || round.horse.health) return false;
    for (unsigned tick = 0; tick < 720; ++tick) round.step();
    return round.game.won && round.finale518.phase == FinalePhase518::Results;
}

bool waitForElysianKey(Round& round) {
    if (progress.quest < 2 || progress.elysianKey || !round.finale518.keyWaiting || !round.game.won) return false;
    for (unsigned tick = 0; tick < HorseFinale518Run::KeyAwardTick && !progress.elysianKey; ++tick) round.step();
    return progress.elysianKey == 1 && round.questKeyTick && !round.finale518.keyWaiting;
}
}
