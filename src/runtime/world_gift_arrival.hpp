#pragma once
#include "world_awards.hpp"

namespace WorldGift {
// master.gd add_present: .25 -> .2 in .5s, TRANS_CUBIC with default EASE_IN_OUT.
// Pure presentation state. Stable reward IDs keep a pulse with its gift when
// the pending queue shifts; no save/acknowledgement state is modified here.
struct ArrivalPulse {
    uint64_t seen=0;
    uint64_t born[WorldState::VisibleCount]{};
    void observe(const WorldState::Data& data,uint64_t now) {
        uint64_t pending=0;
        for(unsigned i=0;i<data.pendingCount && i<WorldState::VisibleCount;++i) {
            const unsigned id=data.pending[i];
            if(id>=WorldState::VisibleCount)continue;
            const auto bit=WorldState::bit(id);
            if(!(seen&bit))born[id]=now;
            pending|=bit;
        }
        seen=pending;
    }
    float scale(unsigned id,uint64_t now) const {
        if(id>=WorldState::VisibleCount || !(seen&WorldState::bit(id)))return .2f;
        const uint64_t age=now>=born[id]?now-born[id]:0;
        if(age>=500000)return .2f;
        const float t=age/500000.f;
        const float u=1-t;
        const float eased=t<.5f?4*t*t*t:1-4*u*u*u;
        return .25f-.05f*eased;
    }
};
}
