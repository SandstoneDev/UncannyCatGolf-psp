#pragma once

inline Vec2 rotateCosmic053(Vec2 point,float cosine,float sine) {
    return {cosine*point.x-sine*point.y,sine*point.x+cosine*point.y};
}

class LateCosmicWalls053 {
public:
    static constexpr unsigned Count=6;
    static constexpr unsigned Bytes=Count*sizeof(TileField);

    bool load(uint8_t* memory,const char* const* paths,unsigned count) {
        if (count>Count) return false;
        fields=reinterpret_cast<TileField*>(memory);
        fieldCount=0;
        for (unsigned i=0;i<count;++i) {
            const auto file=openAssetFile(paths[i]);
            auto* field=new (fields+i) TileField;
            if (!file || !field->read(file.get())) return false;
        }
        fieldCount=count;
        restart();
        return true;
    }

    void restart() {
        rotationCosine=1;
        rotationSine=0;
        trapOpen=dogOpen=false;
        rhythm=Metronome048::Both;
    }

    void setRotation(unsigned tick) {
        const float radians=tick/600.0f;
        rotationCosine=std::cos(radians);
        rotationSine=std::sin(radians);
    }

    WallContact sweep(Vec2 position,Vec2 motion,float radius) const {
        WallContact contact;
        if (fieldCount==2) {
            for (unsigned i=0;i<2;++i) {
                const float sine=i?-rotationSine:rotationSine;
                const auto local=rotateCosmic053(position,rotationCosine,-sine);
                const auto delta=rotateCosmic053(motion,rotationCosine,-sine);
                auto hit=fields[i].sweep(local,delta,radius);
                if (hit.time<contact.time) {
                    hit.normal=rotateCosmic053(hit.normal,rotationCosine,sine);
                    contact=hit;
                }
            }
        } else if (fieldCount==4) {
            for (unsigned i=0;i<4;++i) {
                if ((i==0 && trapOpen) || (i==1 && dogOpen) ||
                    (i>=2 && rhythm!=Metronome048::Both && rhythm!=i-2)) continue;
                const auto hit=fields[i].sweep(position,motion,radius);
                if (hit.time<contact.time) contact=hit;
            }
        }
        return contact;
    }

    const TileField* phase(unsigned index) const {
        return fieldCount==4 && index<2?fields+2+index:nullptr;
    }

    bool trapOpen=false,dogOpen=false;
    unsigned rhythm=Metronome048::Both;

private:
    TileField* fields=nullptr;
    unsigned fieldCount=0;
    float rotationCosine=1,rotationSine=0;
};

struct LastHurrah053 {
    unsigned revealed=0;
    GhostMotion hunter{};
    bool hunterActive=false;

    void restart(Vec2 spawn) {
        *this={};
        hunter.position=spawn;
    }

    bool chase(Vec2 target,float playerRadius) {
        if (hunterActive) {
            const Vec2 offset=target-hunter.position;
            const float distance=length(offset);
            if (distance>0.001f) hunter.velocity=hunter.velocity+offset*(0.875f/distance);
            const float speed=length(hunter.velocity);
            if (speed>100) hunter.velocity=hunter.velocity*(100/speed);
            hunter.position=hunter.position+hunter.velocity*(1.0f/60.0f);
        }
        const Vec2 gap=target-hunter.position;
        const float radius=90+playerRadius;
        return dot(gap,gap)<=radius*radius;
    }
};

inline bool touchesCosmicExit053(Vec2 position,Vec2 goal,float playerRadius) {
    // Closest point on the goal's 16x64 authored elliptical trigger.
    const Vec2 delta{std::abs(position.x-goal.x),std::abs(position.y-goal.y)};
    if (delta.x>16+playerRadius || delta.y>64+playerRadius) return false;
    const float x2=16*16,y2=64*64;
    if (delta.x*delta.x/x2+delta.y*delta.y/y2<=1) return true;
    float low=0,high=64*playerRadius;
    for (unsigned i=0;i<16;++i) {
        const float mid=(low+high)*0.5f;
        const float x=16*delta.x/(x2+mid),y=64*delta.y/(y2+mid);
        if (x*x+y*y>1) low=mid;
        else high=mid;
    }
    const Vec2 edge{x2*delta.x/(x2+high),y2*delta.y/(y2+high)};
    const Vec2 gap=delta-edge;
    return dot(gap,gap)<=playerRadius*playerRadius;
}
