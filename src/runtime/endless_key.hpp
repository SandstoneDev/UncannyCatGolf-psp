#pragma once
#include "game.hpp"

namespace EndlessKey {
inline bool touch(Vec2 previous,Vec2 position,Vec2 center,float radius) {
    const Vec2 motion=position-previous;
    const float squared=dot(motion,motion);
    const float at=squared>0?std::clamp(dot(center-previous,motion)/squared,0.0f,1.0f):0;
    return length(previous+motion*at-center)<=radius;
}

struct Trap {
    enum class Stage { Sensor, Approach, KeyOne, KeyTwo, Final, Cleared };
    Stage stage=Stage::Sensor;
    Vec2 crusher1{288,-360},crusher2{136,-360}, key{-408,96};
    unsigned frames=0,sweep=0,secondFrame=0;
    float firstTarget=0,secondTarget=0;
    bool secondUp=false;

    enum class Event { None, Started, Fake, Dead, Award };
    bool visible() const { return stage!=Stage::Cleared; }
    bool freezesClock() const { return stage!=Stage::Sensor && stage!=Stage::Cleared; }

    static float motion(float from,float to,unsigned frames,unsigned duration) {
        const float a=std::min(1.0f,static_cast<float>(frames)/duration);
        return from+(to-from)*a;
    }

    static bool collides(Vec2 a,Vec2 b,Vec2 c,float hw,float hh,float radius) {
        // Continuous contact against the expanded axis-aligned crusher.
        const Vec2 d=b-a;
        float enter=0,exit=1;
        const auto axis=[&](float pos,float dir,float center,float limit) {
            if (std::abs(dir)<0.0001f) return std::abs(pos-center)<=limit;
            const float t1=(center-limit-pos)/dir,t2=(center+limit-pos)/dir;
            enter=std::max(enter,std::min(t1,t2));
            exit=std::min(exit,std::max(t1,t2));
            return enter<=exit;
        };
        return axis(a.x,d.x,c.x,hw+radius) && axis(a.y,d.y,c.y,hh+radius);
    }

    Event tick(Vec2 previous,Vec2 at,float radius,bool grounded,float velocityX) {
        const Vec2 before1=crusher1,before2=crusher2;
        Event event=Event::None;
        if (stage==Stage::Sensor && grounded && velocityX>0 &&
            collides(previous,at,{322.5f,101.5f},22.5f,41.5f,radius)) {
            stage=Stage::Approach;
            firstTarget=at.y+32;
            frames=0;
            event=Event::Started;
        }
        if (stage==Stage::Approach) {
            ++frames;
            crusher1.y=frames<120?motion(-360,firstTarget,frames,18):
                                   motion(firstTarget,-360,frames-120,60);
            if (!secondFrame && frames>=60 && (at.x<136 || frames>=150)) {
                secondFrame=frames;
                crusher2.x=at.x;
                secondUp=at.x>=136;
                secondTarget=at.y+200;
            }
            if (secondFrame) {
                const unsigned elapsed=frames-secondFrame;
                if (secondUp) crusher2.y=motion(640,-360,elapsed,54);
                else if (elapsed<=42) crusher2.y=motion(-360,secondTarget,elapsed,12);
                else crusher2.y=motion(secondTarget,-360,elapsed-42,60);
            }
            if (frames>=210) { stage=Stage::KeyOne; key={-232,224}; }
        }
        if (stage==Stage::KeyOne && grounded && touch(previous,at,key,radius+10)) {
            stage=Stage::KeyTwo;
            key={112,96};
            sweep=1;
            event=Event::Fake;
        } else if (stage==Stage::KeyTwo && grounded && touch(previous,at,key,radius+10)) {
            stage=Stage::Final;
            frames=0;
            event=Event::Fake;
        }
        if (stage==Stage::KeyTwo && sweep && sweep<=540) {
            const float x=motion(-488,440,sweep,540);
            const float prior=motion(-488,440,sweep-1,540);
            ++sweep;
            if (grounded && collides(previous+Vec2{x-prior,0},at,{x-56,-151},115,974,radius)) return Event::Dead;
        }
        if (stage==Stage::Final) {
            ++frames;
            if (frames==90) { crusher2.x=at.x; secondTarget=at.y+200; }
            if (frames>=90 && frames<=138) crusher2.y=motion(-360,secondTarget,frames-90,48);
            if (frames>168) crusher2.y=motion(secondTarget,-360,frames-168,60);
        }
        const auto swept=[&](Vec2 old,Vec2 now,float width) {
            // Ignore the placement step when shifting a retracted crusher horizontally.
            if (old.x!=now.x || std::abs(old.y-now.y)>100) old=now;
            return collides(previous+now-old,at,now+Vec2{0,1},width,9,radius);
        };
        if (grounded && (swept(before1,crusher1,32) || swept(before2,crusher2,48))) return Event::Dead;
        if (stage==Stage::Final && frames>=240) { stage=Stage::Cleared; return Event::Award; }
        return event;
    }
};
} // namespace EndlessKey
