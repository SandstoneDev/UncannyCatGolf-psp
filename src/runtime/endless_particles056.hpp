#pragma once
#include "game.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace EndlessParticles056 {
constexpr unsigned Amount=32,FixedFps=30,TextureSide=128,NoiseSide=32,NoiseTimes=8;
constexpr unsigned NoiseLayerBytes=NoiseSide*NoiseSide*NoiseTimes*3,NoiseBytes=NoiseLayerBytes*2;
constexpr float BlackAlpha=.203922f,ColorAlpha=.313726f,BlackStrength=14.04f;
inline const int8_t* noiseField=nullptr;
struct Direction {float x,y,z;};
// Zero endpoint tangents in the original Curve resource: cubic Hermite.
inline float alphaCurve(float t){t=std::clamp(t,0.f,1.f);return 1-t*t*(3-2*t);}
inline float resultGridAlpha(uint64_t us){const float t=std::min(us/8000000.f,1.f);const float back=2-2*t;return t<.5f?4*t*t*t:1-back*back*back*.5f;}
inline Direction noise(unsigned layer,Vec2 position,uint64_t us){
    if(!noiseField)return {1,0,0};
    const float x=(position.x+128)*.125f,y=(position.y+128)*.125f,t=(us%4000000)*.000002f;
    const int ix=static_cast<int>(std::floor(x)),iy=static_cast<int>(std::floor(y)),it=static_cast<int>(t);
    const float fx=x-ix,fy=y-iy,ft=t-it;
    Direction out{};
    for(unsigned dt=0;dt<2;++dt)for(unsigned dy=0;dy<2;++dy)for(unsigned dx=0;dx<2;++dx){
        const float weight=(dx?fx:1-fx)*(dy?fy:1-fy)*(dt?ft:1-ft)/127.f;
        const unsigned index=layer*NoiseLayerBytes+((((it+dt)&7)*32+((iy+dy)&31))*32+((ix+dx)&31))*3;
        out.x+=noiseField[index]*weight;out.y+=noiseField[index+1]*weight;out.z+=noiseField[index+2]*weight;
    }
    const float magnitude=out.x*out.x+out.y*out.y+out.z*out.z;
    if(magnitude<.000001f)return {1,0,0};
    const float inverse=1/std::sqrt(magnitude);return {out.x*inverse,out.y*inverse,out.z*inverse};
}
struct Particle {
    Vec2 position{},velocity{},previousPosition{};
    float velocityZ=0,scale=0,lifetime=0,age=0,initialSpeed=0,previousAge=0;
    uint64_t nextBirthUs=0;
    unsigned births=0;
    float opacity()const{return lifetime>0 && age<lifetime?alphaCurve(age/lifetime):0;}
};
struct State {
    std::array<Particle,Amount*2> particles{};
    uint64_t clockUs=0,accumulator=0,ticks=0;
    uint32_t random=1;
    bool radical=false;
    float unit(){random^=random<<13;random^=random>>17;random^=random<<5;return (random>>8)*(1.f/16777216);}
    void reset(uint32_t seed,bool rad){
        *this={};random=seed?seed:1;radical=rad;
        for(unsigned i=0;i<particles.size();++i)particles[i].nextBirthUs=(i%Amount)*1000000/Amount;
    }
    void spawn(Particle& p,Vec2 origin){
        p.previousPosition=p.position=origin;p.initialSpeed=4+unit()*96;
        const float angle=(unit()*2-1)*3.14159265358979323846f;
        p.velocity={std::cos(angle)*p.initialSpeed,std::sin(angle)*p.initialSpeed};p.velocityZ=0;
        p.scale=.1f+unit()*.5f;p.lifetime=std::max(1.f/16777216,1-unit());p.previousAge=p.age=0;++p.births;
    }
    void advance(uint64_t delta,Vec2 origin,bool paused){
        if(paused)return;
        accumulator+=delta*FixedFps;
        while(accumulator>=1000000){
            accumulator-=1000000;
            const uint64_t end=(++ticks)*1000000/FixedFps;
            const float step=(end-clockUs)*.000001f;
            for(unsigned i=0;i<particles.size();++i){
                auto& p=particles[i];float dt=step;
                p.previousPosition=p.position;p.previousAge=p.age;
                if(end>=p.nextBirthUs){
                    spawn(p,origin);dt=(end-p.nextBirthUs)*.000001f;p.nextBirthUs+=1000000;
                }
                p.age+=dt;
                if(p.opacity()<=0)continue;
                if(i<Amount || radical){
                    // Godot's authored turbulence uses .1 velocity influence,
                    // target speed *1.18. The field is baked from its curl code.
                    const auto d=noise(i<Amount?0:1,p.position,end);
                    const float speed=std::sqrt(p.velocity.x*p.velocity.x+p.velocity.y*p.velocity.y+p.velocityZ*p.velocityZ);
                    p.velocity={p.velocity.x*.9f+d.x*speed*.118f,p.velocity.y*.9f+d.y*speed*.118f};
                    p.velocityZ=p.velocityZ*.9f+d.z*speed*.118f;
                }
                p.position=p.position+p.velocity*dt;
            }
            clockUs=end;
        }
    }
    unsigned alive()const{unsigned count=0;for(const auto& p:particles)count+=p.opacity()>0;return count;}
    Vec2 renderPosition(const Particle& p)const{return p.previousPosition+(p.position-p.previousPosition)*(accumulator/1000000.f);}
    float renderOpacity(const Particle& p)const{
        const float age=p.previousAge+(p.age-p.previousAge)*(accumulator/1000000.f);
        return p.lifetime>0 && age<p.lifetime?alphaCurve(age/p.lifetime):0;
    }
};
}
