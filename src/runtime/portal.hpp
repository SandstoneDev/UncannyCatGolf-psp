#pragma once

#include "game.hpp"

struct PortalNode {
    static constexpr unsigned NoTarget=~0u;
    Vec2 position;
    unsigned target=NoTarget;
};

struct PortalStage {
    static constexpr unsigned MaxPortals=32;
    const PortalNode* nodes=nullptr;
    unsigned count=0;
};

class PortalRun {
public:
    void restart(PortalStage next) {
        stage=next.nodes && next.count<=PortalStage::MaxPortals?next:PortalStage{};
        occupied=0;
        sickness=0;
    }

    bool step(Game& game,float radius,bool grounded=true) {
        if (!stage.count || game.won) return false;
        const uint32_t now=grounded?overlaps(game.position,radius):0;
        const uint32_t entered=now&~occupied;
        occupied=now;
        if (sickness) {
            --sickness;
            return false;
        }
        for (unsigned i=0;i<stage.count;++i) {
            const unsigned target=stage.nodes[i].target;
            if (!(entered&(uint32_t{1}<<i)) || target>=stage.count) continue;
            game.position=stage.nodes[target].position;
            occupied=overlaps(game.position,radius);
            sickness=2;
            return true;
        }
        return false;
    }

private:
    uint32_t overlaps(Vec2 position,float radius) const {
        constexpr float halfSize=8.0f;
        uint32_t result=0;
        for (unsigned i=0;i<stage.count;++i) {
            const Vec2 local=position-stage.nodes[i].position;
            const Vec2 offset{std::max(std::abs(local.x)-halfSize,0.0f),
                              std::max(std::abs(local.y)-halfSize,0.0f)};
            if (dot(offset,offset)<=radius*radius) result|=uint32_t{1}<<i;
        }
        return result;
    }

    PortalStage stage{};
    uint32_t occupied=0;
    unsigned sickness=0;
};
