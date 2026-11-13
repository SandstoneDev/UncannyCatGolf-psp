#pragma once

#include <algorithm>
#include <cstdint>

enum class SlotOutcome : uint8_t {
    Default, Jackpot, Happy, Ok, Question, Phone, Cherry, Eye, Blue, Sad, Skull,
    Close, Good, Bad, Lose, Count
};

struct SlotMachineRun {
    static constexpr unsigned ReelCount = 3;
    static constexpr unsigned SymbolCount = 10;
    static constexpr unsigned TicksPerSymbol = 15;
    uint8_t frames[ReelCount]{};
    uint8_t stopped = 0;
    SlotOutcome outcome = SlotOutcome::Default;
    unsigned ticks = 0, outcomeTick = 0, bonus = 0;

    void advance() {
        ++ticks;
        if (stopped == AllStopped || ticks % TicksPerSymbol != 0) return;
        for (unsigned reel = 0; reel < ReelCount; ++reel) {
            if (!(stopped & (1u << reel))) frames[reel] = (frames[reel] + 1) % SymbolCount;
        }
    }

    bool stop(unsigned reel, unsigned entropy) {
        if (reel >= ReelCount || (stopped & (1u << reel))) return false;
        stopped |= 1u << reel;
        if (stopped != AllStopped) return false;
        outcome = match();
        outcomeTick = ticks;
        constexpr unsigned points[]{0, 10000, 4000, 2000, 0, 2000, 5000, 2000,
                                    2000, 1500, 2000, 1000, 3000, 500, 1};
        static_assert(sizeof points / sizeof points[0] == static_cast<unsigned>(SlotOutcome::Count));
        bonus = outcome == SlotOutcome::Question ? entropy % 10000 + 1 :
                points[static_cast<unsigned>(outcome)];
        return true;
    }

    unsigned blueAlpha() const {
        // The 1 s animation multiplies the polygon's fixed alpha of 100/255.
        return outcome == SlotOutcome::Blue ? std::min(ticks - outcomeTick, 60u) * 100 / 60 : 0;
    }

private:
    static constexpr uint8_t AllStopped = (1u << ReelCount) - 1;

    static bool happy(unsigned frame) {
        return frame == 0 || frame == 1 || frame == 2 || frame == 5 || frame == 6;
    }

    SlotOutcome match() const {
        const unsigned left = frames[0], middle = frames[1], right = frames[2];
        if (left == middle && middle == right) return static_cast<SlotOutcome>(left + 1);
        if (left == 0 && middle == 0) return SlotOutcome::Close;
        if (happy(left) && happy(middle) && happy(right)) return SlotOutcome::Good;
        return left == middle ? SlotOutcome::Bad : SlotOutcome::Lose;
    }
};
