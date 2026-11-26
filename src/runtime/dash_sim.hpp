#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace DashSim {

constexpr unsigned Size=24,Primary=1,Secondary=2;
enum class Cue { Jump, Skip, Time, Death, StopDeath, StopMove };
struct Run {
    std::array<bool,Size> board{};
    uint32_t random=1;
    unsigned position=0,score=0,best=0,rounds=0,remaining=4,shownScore=0;
    bool inactive=false,firstInput=false,scoreVisible=false,initialBar=true;
    uint64_t periodUs=4000000,timerUs=4000000,continueUs=0,toneUs=0;
    uint32_t cues=0;

    uint32_t draw() {
        random^=random<<13;
        random^=random>>17;
        return random^=random<<5;
    }
    void signal(Cue cue) { cues|=1u<<static_cast<unsigned>(cue); }
    bool signaled(Cue cue) const { return (cues&(1u<<static_cast<unsigned>(cue)))!=0; }

    void generate() {
        board[0]=board[Size-1]=true;
        for (unsigned i=0;i<Size;++i) {
            const bool land=draw()&1u;
            if (i>1 && i<Size-1) board[i]=!board[i-1] || land;
        }
    }

    void reset(uint32_t seed) {
        *this={};
        random=seed?seed:1;
        board[1]=true;
        generate();
    }

    void addScore(unsigned points) {
        score+=points;
        best=std::max(best,score);
        shownScore=score;
        scoreVisible=true;
    }

    void die() {
        inactive=true;
        continueUs=100000;
        timerUs=0;
        periodUs=10000000;
        remaining=4;
        initialBar=false;
        position=0;
        best=std::max(best,score);
        score=0;
        firstInput=true;
        signal(Cue::Death);
    }

    void next() {
        if (periodUs>3000000) periodUs-=500000;
        timerUs=periodUs;
        remaining=4;
        initialBar=false;
        addScore(10);
        position=0;
        ++rounds;
        generate();
    }

    void action(unsigned buttons) {
        if (!(buttons&(Primary|Secondary))) return;
        // Parent big_game runs before the child game and resets the timer
        // on every accepted mouse action, including normal play.
        if (continueUs) return;
        inactive=false;
        timerUs=periodUs;
        signal(Cue::StopDeath);
        if (firstInput) { firstInput=false; return; }
        const bool shortJump=buttons&Primary;
        position+=shortJump?1:2;
        if (!toneUs) {
            signal(Cue::StopMove);
            signal(shortJump?Cue::Jump:Cue::Skip);
        }
        addScore(1);
        if (position>=Size) next();
        if (!board[position]) die();
    }

    void step(unsigned buttons,uint64_t us) {
        cues=0;
        toneUs=us>=toneUs?0:toneUs-us;
        continueUs=us>=continueUs?0:continueUs-us;
        action(buttons);
        if (inactive) return;
        if (!board[position]) { die(); return; }
        // Godot Timer uses idle processing, after the physics input callbacks.
        if (timerUs>us) { timerUs-=us; return; }
        timerUs=periodUs;
        if (remaining) --remaining;
        signal(Cue::Time);
        toneUs=252948;
        if (!remaining) die();
    }
};

} // namespace DashSim
