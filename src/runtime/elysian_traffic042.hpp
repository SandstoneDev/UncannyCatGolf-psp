#pragma once

#include "game.hpp"
#include "park_late031.hpp"
#include "chuckle_run.hpp"

struct ElysianTraffic513 {
    static constexpr unsigned DogCount=4;
    DogGolfRun dogs[DogCount]{};
    CircleCollider bodies[DogCount]{};

    template<std::size_t Sources>
    void restart(const DogGolfScene (&scenes)[DogCount],ChuckleRun& chuckles,
                 const ChuckleSpawner (&sources)[Sources],float despawn) {
        for (unsigned i=0;i<DogCount;++i) {
            dogs[i].restart(scenes[i]);
            bodies[i]=dogs[i].body;
        }
        chuckles.startSpawners(sources,Sources,despawn,true);
    }

    struct Events { bool bounced=false, defeated=false; };

    Events step(const DogGolfScene (&scenes)[DogCount],ChuckleRun& chuckles,
                const TileField& walls,Vec2 target,unsigned tick) {
        Events events;
        for (unsigned i=0;i<DogCount;++i) {
            const auto hit=dogs[i].step(scenes[i],walls,nullptr,nullptr,nullptr,0,
                                       chuckles.bodies,chuckles.count,{nullptr,0,{},bodies,DogCount,i});
            bodies[i]=dogs[i].body;
            events.bounced|=hit.bounced;
            if (hit.baddie>=0)
                events.defeated|=chuckles.defeat(static_cast<unsigned>(hit.baddie),tick);
            if (hit.dog>=0) struckByPlayer(scenes,static_cast<unsigned>(hit.dog),bodies[i].center);
        }
        chuckles.step(walls,target,nullptr,0,{},nullptr,0,0,nullptr,nullptr,nullptr,0,bodies,DogCount);
        return events;
    }

    void struckByPlayer(const DogGolfScene (&scenes)[DogCount],unsigned i,Vec2 player) {
        dogs[i].struckByPlayer(scenes[i],player);
    }
};
