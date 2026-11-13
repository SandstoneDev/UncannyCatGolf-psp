#pragma once

struct SantaRun052 {
    static constexpr float Top=-1824;
    float cameraY=0,highestY=200;
    bool boundsArmed=false,keyRingStarted=false;
    unsigned ringTick=0,ringFinished=0;
    bool ringPassed=false;

    static float eased(float t) {
        return t<0.5f?2*t*t:1-2*(1-t)*(1-t);
    }

    static Vec2 goal(unsigned tick) {
        const float t=std::clamp((tick-84.0f)/96,0.0f,1.0f);
        return {0,-144-1806*eased(t)};
    }

    static float ufoY(unsigned tick) {
        if (tick<60) return -290+30*eased(tick/60.0f);
        if (tick<84) return -260;
        if (tick<184) return -260-1830*eased((tick-84)/100.0f);
        return -2090-7909*std::pow(std::clamp((tick-184)/40.0f,0.0f,1.0f),2.0f);
    }

    static float beamAlpha(unsigned tick) {
        if (tick<20) return std::pow(std::max(0.0f,(tick-1.333332f)/(20-1.333332f)),2.0f);
        return std::clamp((180.0f-tick)/12,0.0f,1.0f);
    }

    void track(Vec2 player) {
        highestY=std::min(highestY,player.y);
        if (cameraY>highestY) cameraY=std::max(Top,cameraY+(highestY-cameraY)*0.1f);
        const float relativeY=player.y-cameraY;
        boundsArmed|=player.x<-256 || player.x>272 || relativeY>272 || relativeY<-272;
    }

    bool keyActive(unsigned tick) const {
        return keyRingStarted && !ringFinished && tick-ringTick<240;
    }

    bool startRing(Vec2 player,float radius,unsigned tick) {
        const auto offset=player-Vec2{64,-1240};
        if (keyRingStarted || dot(offset,offset)>(radius+24)*(radius+24)) return false;
        keyRingStarted=true;
        ringTick=tick;
        return true;
    }

    bool finishRing(bool gotKey,unsigned tick) {
        if (!keyRingStarted || ringFinished || (!gotKey && tick-ringTick<240)) return false;
        ringFinished=tick;
        ringPassed=gotKey;
        return true;
    }
};

struct FakeGoals052 {
    unsigned triggered[13]{};
    unsigned bonus=0;

    bool step(const Vec2* goals,unsigned count,Game& game,float playerRadius,unsigned tick) {
        const float radius=playerRadius+16;
        bool picked=false;
        for (unsigned i=0;i<count;++i) {
            const Vec2 offset=game.position-goals[i];
            if (triggered[i] || dot(offset,offset)>radius*radius) continue;
            triggered[i]=tick;
            game.velocity={};
            bonus+=500;
            picked=true;
        }
        return picked;
    }
};
