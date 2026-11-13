#pragma once

struct Metronome048 {
    static constexpr unsigned Both=2;
    float particleSeconds=0,particleSpeed=0.8f;
    unsigned beats=0, phase=Both, sinceBeat=0, sinceTrigger=0, sincePoof=0, labelFadeTicks=0;

    static unsigned musicBeat(unsigned samples) {
        // 135 BPM at the mixer's 44100 Hz. Reduce the fraction before multiplying.
        return 3*(samples/58800)+(samples%58800)*3/58800;
    }

    static unsigned testBeat(unsigned tick) { return tick/80*3+(tick%80)*3/80; }

    void restart(unsigned songBeat) {
        *this={};
        lastBeat=songBeat;
    }

    bool step(unsigned songBeat) {
        particleSpeed+=(0.1f-particleSpeed)*0.05f;
        particleSeconds+=particleSpeed/60;
        if (beats>=15) ++sincePoof;
        if (beats>6 && labelFadeTicks<10) ++labelFadeTicks;
        ++sinceBeat;
        ++sinceTrigger;
        if (lastBeat==songBeat) return false;
        lastBeat=songBeat;
        ++beats;
        sinceBeat=0;
        if (beats%2) return true;
        phase=(beats/2-1)%2;
        sinceTrigger=0;
        particleSpeed=1;
        return true;
    }

    bool ghostActive(unsigned index,unsigned count) const {
        return index<count && (phase==Both || index%2==phase);
    }

    unsigned fieldIndex() const { return phase==Both?0:phase+1; }

private:
    unsigned lastBeat=0;
};
