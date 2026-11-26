#pragma once

#include "club_geometry054.hpp"
#include "club_events054.hpp"

namespace Club054 {

namespace Geometry=ucg_club054;
constexpr float Radius=Geometry::playerRadius;
constexpr int Side=Geometry::roomSize;

inline int roomIndex(float coordinate) {
    return static_cast<int>(std::floor(coordinate/Side));
}

inline Vec2 point(Geometry::Point value) { return {value.x,value.y}; }
inline float cross(Vec2 a,Vec2 b) { return a.x*b.y-a.y*b.x; }

inline bool intersects(const Geometry::Shape& shape,Vec2 center,float radius=Radius) {
    const auto* vertices=Geometry::shapePoints+shape.first;
    if (shape.radius>0) {
        const Vec2 delta=center-point(vertices[0]);
        const float distance=radius+shape.radius;
        return dot(delta,delta)<=distance*distance;
    }
    bool inside=false;
    for (unsigned i=0;i<shape.count;++i) {
        const Vec2 a=point(vertices[i]),b=point(vertices[(i+1)%shape.count]);
        const Vec2 edge=b-a,offset=center-a;
        const float edgeLength=dot(edge,edge);
        if (edgeLength>0) {
            const Vec2 gap=offset-edge*std::clamp(dot(offset,edge)/edgeLength,0.0f,1.0f);
            if (dot(gap,gap)<=radius*radius) return true;
        }
        if ((a.y>center.y)!=(b.y>center.y) && center.x<a.x+(center.y-a.y)*(b.x-a.x)/(b.y-a.y))
            inside=!inside;
    }
    return inside;
}

template<class Vertex>
void sweepOutline(unsigned count,Vertex vertex,Vec2 position,Vec2 motion,WallContact& contact) {
    float winding=0;
    for (unsigned i=0;i<count;++i) winding+=cross(vertex(i)-position,vertex((i+1)%count)-position);
    const float sign=winding>=0?1.0f:-1.0f;
    for (unsigned i=0;i<count;++i) {
        const Vec2 a=vertex(i),edge=vertex((i+1)%count)-a;
        const float span=length(edge);
        if (span<0.001f) continue;
        const Vec2 tangent=edge*(1.0f/span),normal=Vec2{tangent.y,-tangent.x}*sign;
        const Vec2 delta=position-a;
        const float toward=dot(motion,normal);
        if (toward<0) {
            const float t=(Radius-dot(delta,normal))/toward;
            if (t>=-0.0001f && t<=1 && t<contact.time) {
                const float along=dot(delta+motion*t,tangent);
                if (along>=0 && along<=span) contact={std::max(0.0f,t),normal};
            }
        }
        WallContact corner=contact;
        sweepCircle({a,0.0001f},position,motion,Radius,corner);
        if (corner.time<contact.time) {
            const Vec2 previous=a-vertex((i+count-1)%count);
            if (dot(corner.normal,tangent)<=0 && dot(corner.normal,previous)>=0)
                contact=corner;
        }
    }
}

struct Physics {
    Vec2 position=point(Geometry::spawn),velocity{};
    bool breakerDoor=false,breakerOn=false,entranceClosed=false;
    bool uncanny=false,shiverDog=false;

    const ClubEvents::Visit* visit=nullptr;
    unsigned jumpTicks=0;
    float jumpScale=1;

    void launch() {
        position={424,-928};velocity={2010,0};jumpTicks=27;jumpScale=1;
    }
    bool enabled(const Geometry::Shape& shape,Vec2& delta) const {
        const auto index=&shape-Geometry::shapes;
        const auto owner=ClubTracks::shapeOwners[index];
        if(!visit || owner.actor<0)return true;
        const auto& p=visit->people[owner.actor];
        delta={p.x-Geometry::npcs[owner.actor].anchor.x,p.y-Geometry::npcs[owner.actor].anchor.y};
        return p.visible && (owner.part==1?p.talk:owner.part==2?p.ouch:p.solid);
    }
    bool hidden(const char* path) const {
        return (!uncanny && std::strncmp(path,"Level/Uncanny/",14)==0) ||
               (!shiverDog && std::strncmp(path,"Level/ShiverDog/",16)==0);
    }

    WallContact sweep(Vec2 motion) const {
        WallContact contact;
        const Vec2 end=position+motion;
        const Vec2 lo{std::min(position.x,end.x)-Radius,std::min(position.y,end.y)-Radius};
        const Vec2 hi{std::max(position.x,end.x)+Radius,std::max(position.y,end.y)+Radius};
        for (const auto& room:Geometry::rooms) {
            if (room.x<roomIndex(lo.x) || room.x>roomIndex(hi.x) ||
                room.y<roomIndex(lo.y) || room.y>roomIndex(hi.y)) continue;
            for (unsigned j=room.first;j<room.first+room.count;++j) {
                const auto& tile=Geometry::tiles[j];
                if ((tile.layer==Geometry::NTilesLOCK && breakerOn) ||
                    (tile.layer==Geometry::BasementTilesFinal && !entranceClosed)) continue;
                const Vec2 anchor{float(tile.x*16),float(tile.y*16)};
                if (anchor.x>hi.x || anchor.y>hi.y || anchor.x+16<lo.x || anchor.y+16<lo.y) continue;
                const auto& outline=Geometry::tileOutlines[tile.outline];
                sweepOutline(outline.count,[&](unsigned i) {
                    const auto p=outline.points[i];
                    return anchor+Vec2{float(p.x),float(p.y)};
                },position,motion,contact);
            }
        }
        for (const auto& shape:Geometry::shapes) {
            if ((shape.flags&(Geometry::staticBody|Geometry::initiallyDisabled))!=Geometry::staticBody ||
                !(shape.layer&1) || hidden(shape.path) ||
                (breakerDoor && std::strcmp(shape.path,"Level/BreakerDoor/CollisionShape2D")==0)) continue;
            Vec2 delta{};
            if(!enabled(shape,delta))continue;
            const auto* vertices=Geometry::shapePoints+shape.first;
            if (shape.radius>0) {
                sweepCircle({point(vertices[0])+delta,shape.radius},position,motion,Radius,contact);
                continue;
            }
            const auto* last=vertices+shape.count;
            if (std::all_of(vertices,last,[&](auto p){ return p.x+delta.x<lo.x; }) ||
                std::all_of(vertices,last,[&](auto p){ return p.x+delta.x>hi.x; }) ||
                std::all_of(vertices,last,[&](auto p){ return p.y+delta.y<lo.y; }) ||
                std::all_of(vertices,last,[&](auto p){ return p.y+delta.y>hi.y; })) continue;
            sweepOutline(shape.count,[&](unsigned i){return point(vertices[i])+delta;},position,motion,contact);
        }
        return contact;
    }

    bool tick() {
        if(jumpTicks) {
            // credits.tscn has no layer-65536 airborne obstacle. PJump changes
            // back to PMoving on its 27th update, before this tick's motion.
            if(--jumpTicks) {
                jumpScale+=(0.15f/0.064f-jumpScale)*(jumpTicks>=14?0.13f:-0.13f);
                position=position+velocity*(1.0f/60.0f);
                return false;
            }
            jumpScale=1;
        }
        const float speed=length(velocity);
        if (speed<=20) { velocity={}; return false; }
        const Vec2 motion=velocity*(1.0f/60.0f);
        const auto hit=sweep(motion);
        const bool bounced=hit.time<=1;
        position=position+motion*(bounced?hit.time:1.0f);
        if (bounced) {
            position=position+hit.normal*0.01f;
            velocity=velocity-hit.normal*(2.0f*dot(velocity,hit.normal));
        }
        velocity=velocity*(1.0f-175.0f*std::pow(0.04f/speed,1.06f));
        return bounced;
    }
};

} // namespace Club054
