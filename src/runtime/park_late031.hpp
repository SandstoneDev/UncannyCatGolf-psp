#pragma once

#include "smiley_ring.hpp"

struct DogGolfScene {
    Vec2 spawn,velocity,goal;
    float radius,goalRadius,graphicScale,winnerLerp,winnerShrink;
    bool bounceOffPlayer;
    bool hasGoal=true;
};

struct DogGolfPeers {
    const SolidBlock* blocks=nullptr;
    unsigned blockCount=0;
    BlockMask broken;
    const CircleCollider* dogs=nullptr;
    unsigned dogCount=0,self=0;
};

struct DogGolfUpdate {
    bool bounced=false,switched=false,won=false;
    int baddie=-1,block=-1,dog=-1;
};

class DogGolfRun {
public:
    CircleCollider body{};
    Vec2 velocity{};
    float graphicScale=0;
    bool winner=false;

    void restart(const DogGolfScene& scene) {
        body={scene.spawn,scene.radius};
        velocity=scene.velocity;
        graphicScale=scene.graphicScale;
        winner=false;
        previousSwitchTiles=nullptr;
        recovering=false;
    }

    DogGolfUpdate step(const DogGolfScene& scene,const TileField& walls,
                       const TileField* gate,SwitchRun* switches,
                       const CircleCollider* skulls=nullptr,unsigned skullCount=0,
                       const CircleCollider* chuckles=nullptr,unsigned chuckleCount=0,
                       DogGolfPeers peers={},
                       WallContact (*otherWalls)(Vec2,Vec2,float)=nullptr) {
        DogGolfUpdate event;
        if (winner) {
            body.center=body.center+(scene.goal-body.center)*scene.winnerLerp;
            if (graphicScale>0.001f) graphicScale-=scene.winnerShrink;
            return event;
        }

        const TileField* active=switches?switches->activeTiles():nullptr;
        if (active!=previousSwitchTiles) {
            previousSwitchTiles=active;
            recovering=active;
        }
        if (recovering) recover(active,walls,gate);

        const Vec2 motion=velocity*(1.0f/60.0f);
        WallContact contact=walls.sweep(body.center,motion,body.radius);
        if (gate) {
            const auto door=gate->sweep(body.center,motion,body.radius);
            if (door.time<contact.time) contact=door;
        }
        if (otherWalls) {
            const auto other=otherWalls(body.center,motion,body.radius);
            if (other.time<contact.time) contact=other;
        }
        int block=switches?switches->sweep(body.center,motion,body.radius,contact):-1;
        for (unsigned i=0;i<peers.blockCount;++i) {
            if (peers.broken.has(i)) continue;
            const float previous=contact.time;
            TileField::sweepBlock(peers.blocks[i],body.center,motion,body.radius,contact);
            if (contact.time<previous) {
                event.block=static_cast<int>(i);
                block=-1;
            }
        }
        for (unsigned i=0;i<skullCount+chuckleCount;++i) {
            const auto& enemy=i<skullCount?skulls[i]:chuckles[i-skullCount];
            const float previous=contact.time;
            sweepCircle(enemy,body.center,motion,body.radius,contact);
            if (contact.time<previous) {
                event.baddie=static_cast<int>(i);
                event.block=-1;
                block=-1;
            }
        }
        for (unsigned i=0;i<peers.dogCount;++i) {
            if (i==peers.self) continue;
            const float previous=contact.time;
            sweepCircle(peers.dogs[i],body.center,motion,body.radius,contact);
            if (contact.time<previous) {
                event.dog=static_cast<int>(i);
                event.block=event.baddie=-1;
                block=-1;
            }
        }
        body.center=body.center+motion*std::min(contact.time,1.0f);
        if (contact.time<=1.0f) {
            velocity=velocity-contact.normal*(2.0f*dot(velocity,contact.normal));
            event.bounced=true;
            if (block>=0) {
                switches->strike(static_cast<unsigned>(block));
                event.switched=true;
            }
        }

        const Vec2 toGoal=scene.goal-body.center;
        const float range=body.radius+scene.goalRadius;
        if (scene.hasGoal && dot(toGoal,toGoal)<=range*range) {
            body.radius=0;
            velocity={};
            winner=true;
            event.won=true;
        }
        return event;
    }

    void struckByPlayer(const DogGolfScene& scene,Vec2 player) {
        if (!scene.bounceOffPlayer || winner) return;
        const Vec2 away=body.center-player;
        constexpr float QuarterPi=0.7853981633974483f, Diagonal=0.7071067811865475f;
        constexpr Vec2 directions[]{
            {1,0}, {Diagonal,Diagonal}, {0,1}, {-Diagonal,Diagonal},
            {-1,0}, {-Diagonal,-Diagonal}, {0,-1}, {Diagonal,-Diagonal}
        };
        const int turn=static_cast<int>(std::floor(std::atan2(away.y,away.x)/QuarterPi+0.5f));
        velocity=directions[(turn+8)%8]*length(velocity);
    }

private:
    const TileField* previousSwitchTiles=nullptr;
    bool recovering=false;

    void recover(const TileField* active,const TileField& walls,const TileField* gate) {
        constexpr float safeRadiusPadding=0.08f;
        const float safeRadius=body.radius+safeRadiusPadding;
        for (unsigned attempt=0;attempt<4;++attempt) {
            Vec2 shift=active->recoverCircle(body.center,safeRadius);
            if (dot(shift,shift)<1.0e-8f) break;
            shift=walls.recoverCircle(body.center,safeRadius,shift);
            if (gate) shift=gate->recoverCircle(body.center,safeRadius,shift);
            body.center=body.center+shift;
        }
        const Vec2 overlap=active->recoverCircle(body.center,body.radius);
        recovering=dot(overlap,overlap)>1.0e-8f;
    }
};

#include "park_late031_data.hpp"

inline bool onParkPursuitSand(Vec2 position,float radius) {
    const Vec2 local=position-ParkPursuitSand.center;
    const Vec2 half=ParkPursuitSand.halfSize;
    const Vec2 closest{std::clamp(local.x,-half.x,half.x),std::clamp(local.y,-half.y,half.y)};
    const Vec2 offset=local-closest;
    return dot(offset,offset)<=radius*radius;
}
