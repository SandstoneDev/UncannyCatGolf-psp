#pragma once

namespace VillaButtons060 {

struct Target {
    int id;
    float x;
    bool eligible;
};

inline bool before(const Target& a,const Target& b) {
    return a.x<b.x || (a.x==b.x && a.id<b.id);
}

// A stable authored-world-X cycle makes every unlocked object reachable even
// while residents move. Up/Left go backward; Down/Right go forward.
inline int focus(const Target* targets,unsigned count,int current,int direction) {
    const Target* selected=nullptr;
    for (unsigned i=0;i<count;++i)
        if (targets[i].eligible && targets[i].id==current) selected=&targets[i];
    const Target* edge=nullptr;
    const Target* next=nullptr;
    for (unsigned i=0;i<count;++i) {
        const auto& item=targets[i];
        if (!item.eligible) continue;
        if (!edge || (direction<0?before(*edge,item):before(item,*edge))) edge=&item;
        if (!selected || !direction) continue;
        const bool ahead=direction<0?before(item,*selected):before(*selected,item);
        if (ahead && (!next || (direction<0?before(*next,item):before(item,*next)))) next=&item;
    }
    if (selected && !direction) return current;
    return next?next->id:edge?edge->id:-1;
}

inline float camera(float scaledWorldX,float maxPan) {
    const float desired=scaledWorldX-272.0f;
    return desired<0?0:desired>maxPan?maxPan:desired;
}

} // namespace VillaButtons060
