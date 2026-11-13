#pragma once

#include "game.hpp"

template<unsigned Capacity>
struct SmileyStageData {
    static_assert(Capacity>0 && Capacity<=32);
    static constexpr unsigned MaxCoins=Capacity;
    Vec2 center;
    float ringRadius,coinRadius;
    uint32_t coinCount,timeTicks,bonus,winTicks,lossTicks,redoable;
    Vec2 coins[MaxCoins];
};
using SmileyStage=SmileyStageData<16>;
static_assert(sizeof(SmileyStage)==168);

enum class SmileyPhase : uint8_t { Ready, Active, Passed, Failed, Hidden };

struct SmileyUpdate {
    bool started=false,passed=false,failed=false;
    unsigned newCoins=0;
};

template<unsigned Capacity>
class SmileyRingState {
    using Stage=SmileyStageData<Capacity>;

public:
    SmileyUpdate step(const Stage& stage,Vec2 ball,float playerRadius,unsigned tick,bool canTouch=true) {
        SmileyUpdate event;
        if (phase==SmileyPhase::Passed || phase==SmileyPhase::Failed) {
            const bool failed=phase==SmileyPhase::Failed;
            if (tick-finished >= (failed?stage.lossTicks:stage.winTicks))
                phase=failed && stage.redoable?SmileyPhase::Ready:SmileyPhase::Hidden;
            return event;
        }
        if (phase==SmileyPhase::Hidden) return event;
        if (phase==SmileyPhase::Ready) {
            if (!canTouch || !touches(ball,stage.center,playerRadius+stage.ringRadius)) return event;
            phase=SmileyPhase::Active;
            started=tick;
            if (!everStarted) {
                ufoStart=tick;
                everStarted=true;
            }
            mask=0;
            count=0;
            event.started=true;
        }
        if (tick-started>=stage.timeTicks) {
            phase=SmileyPhase::Failed;
            finished=tick;
            event.failed=true;
            return event;
        }
        if (!canTouch) return event;
        for (unsigned i=0;i<stage.coinCount;++i) {
            const auto bit=1u<<i;
            if (!(mask&bit) && touches(ball,stage.coins[i],playerRadius+stage.coinRadius)) {
                mask|=bit;
                coinTick[i]=tick;
                coinOrder[i]=++count;
                ++event.newCoins;
            }
        }
        if (count==stage.coinCount) {
            phase=SmileyPhase::Passed;
            finished=tick;
            bonus+=stage.bonus;
            event.passed=true;
        }
        return event;
    }

    bool collected(unsigned index) const { return mask&(1u<<index); }
    unsigned remainingTicks(const Stage& stage,unsigned tick) const {
        return phase==SmileyPhase::Active?stage.timeTicks-std::min<unsigned>(stage.timeTicks,tick-started):0;
    }
    unsigned remaining(const Stage& stage,unsigned tick) const {
        return (remainingTicks(stage,tick)+59)/60;
    }
    float timerPitch(const Stage& stage,unsigned tick) const {
        const float timeFraction=stage.timeTicks?float(remainingTicks(stage,tick))/stage.timeTicks:0.0f;
        return 3.0f-2.5f*timeFraction;
    }

    SmileyPhase phase=SmileyPhase::Ready;
    bool everStarted=false;
    unsigned ufoStart=0,bonus=0;
    unsigned coinTick[Capacity]{},coinOrder[Capacity]{};

private:
    static bool touches(Vec2 a,Vec2 b,float radius) {
        const auto delta=a-b;
        return dot(delta,delta)<=radius*radius;
    }
    unsigned started=0,finished=0,count=0;
    uint32_t mask=0;
};

using SmileyRingRun=SmileyRingState<16>;
