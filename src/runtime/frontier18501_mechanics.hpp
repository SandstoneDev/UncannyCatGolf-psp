#pragma once

#include "game.hpp"
#include "elysian_rocks038.hpp"

inline constexpr SolidBlock WallPrismGate418{{0,-96},{128,32}};
inline constexpr Vec2 WallGoal418{0,-528};
inline constexpr Vec2 WallGoalRadii418{512,16};
inline constexpr Vec2 WallBed418{0,64};

inline WallContact sweepLockedPrismGate418(Vec2 position,Vec2 motion,float radius) {
    WallContact contact;
    TileField::sweepBlock(WallPrismGate418,position,motion,radius,contact);
    return contact;
}

inline bool touchesWallGoal418(Vec2 position,float radius) {
    const Vec2 offset=position-WallGoal418;
    // Godot's circle pair projects along the line between the centers.
    // The scaled Area's broad phase limits the otherwise very wide projection.
    constexpr float broadPhaseHeight=60.5f;
    if (std::abs(offset.x)>WallGoalRadii418.x+radius || std::abs(offset.y)>broadPhaseHeight) return false;
    const float distanceSquared=dot(offset,offset);
    const Vec2 scaled{offset.x*WallGoalRadii418.x,offset.y*WallGoalRadii418.y};
    return distanceSquared<=radius*std::sqrt(distanceSquared)+length(scaled);
}

enum class WallEvent418 { None, TooPoor, Shatter, Goal, Bed };

struct WallRun418 {
    static constexpr unsigned RequiredPrisms=250, EndingTicks=60*60;
    static constexpr unsigned LullabyStart=3*60, LullabyEnd=59*60;
    bool easyMode=false, eligible=false, shattered=false, poor=false, bedActive=false;
    bool insideGate=false;
    unsigned shatteredAt=0,poorAt=0,endingTicks=0;
    float sleepAlpha=0,sleepBlur=0;

    void restart(unsigned prisms,bool easy) {
        *this={};
        easyMode=easy;
        eligible=prisms>=RequiredPrisms;
    }

    auto obstacle() const {
        return !easyMode && !eligible ? sweepLockedPrismGate418 : nullptr;
    }

    WallEvent418 step(Game& game,const Course& course,unsigned tick) {
        if (easyMode) {
            if (bedActive) return WallEvent418::None;
            const Vec2 offset=game.position-WallBed418;
            const Vec2 nearest{offset.x-std::clamp(offset.x,-32.0f,32.0f),offset.y};
            const float reach=8+course.playerRadius;
            if (dot(nearest,nearest)>reach*reach) return WallEvent418::None;
            bedActive=true;
            game.velocity={};
            return WallEvent418::Bed;
        }

        if (!shattered) {
            const Vec2 offset=game.position-WallPrismGate418.center;
            const Vec2 half=WallPrismGate418.halfSize+Vec2{1,1};
            const Vec2 distance{std::max(std::abs(offset.x)-half.x,0.0f),
                                std::max(std::abs(offset.y)-half.y,0.0f)};
            const bool overlaps=dot(distance,distance)<=course.playerRadius*course.playerRadius;
            const bool entered=overlaps && !insideGate;
            insideGate=overlaps;
            if (entered) {
                if (eligible) {
                    shattered=true;
                    shatteredAt=tick;
                    return WallEvent418::Shatter;
                }
                poor=true;
                poorAt=tick;
                return WallEvent418::TooPoor;
            }
        }
        if (!game.won && touchesWallGoal418(game.position,course.playerRadius)) {
            game.won=true;
            game.velocity={};
            return WallEvent418::Goal;
        }
        return WallEvent418::None;
    }

    void advanceEnding(Game& game,bool fastForward) {
        game.position=game.position+(WallBed418-game.position)*0.1f;
        sleepAlpha+=(1-sleepAlpha)*0.002f;
        sleepBlur+=(1-sleepBlur)*0.001f;
        endingTicks=std::min(EndingTicks,endingTicks+(fastForward?4u:1u));
    }

    bool endingFinished() const { return endingTicks==EndingTicks; }
    bool lullabyActive() const { return endingTicks>=LullabyStart && endingTicks<LullabyEnd; }

    float hue(unsigned tick) const {
        return static_cast<float>((180+tick%360)%360)/360;
    }

    float lockScale(unsigned tick) const {
        if (!poor) return 1;
        constexpr float riseTicks=60*0.0625f,fallTicks=60*0.25f;
        const float age=static_cast<float>(tick-poorAt);
        const auto ease=[](float t) {
            t=std::clamp(t,0.0f,1.0f);
            if (t<0.5f) return 4*t*t*t;
            const float tail=1-t;
            return 1-4*tail*tail*tail;
        };
        return 1+0.5f*(age<riseTicks?ease(age/riseTicks):1-ease((age-riseTicks)/fallTicks));
    }
};

struct ElysianRockEdge038 {
    Vec2 point,tangent,normal;
    float length;
    bool convex;
};

template<const auto& Vertices>
inline const auto& elysianRockEdges() {
    static const auto edges=[] {
        constexpr unsigned count=std::size(Vertices);
        std::array<ElysianRockEdge038,count> result{};
        for (unsigned i=0;i<count;++i) {
            const Vec2 point=Vertices[i];
            const Vec2 incoming=point-Vertices[(i+count-1)%count];
            const Vec2 outgoing=Vertices[(i+1)%count]-point;
            const float edgeLength=length(outgoing);
            const Vec2 tangent=outgoing*(1.0f/edgeLength);
            const bool convex=incoming.x*outgoing.y-incoming.y*outgoing.x>0;
            result[i]={point,tangent,{tangent.y,-tangent.x},edgeLength,convex};
        }
        return result;
    }();
    return edges;
}

template<const auto& Vertices,const auto& Centers>
inline WallContact sweepElysianRockGroup(Vec2 position,Vec2 motion,float radius) {
    WallContact contact;
    const float speedSquared=dot(motion,motion);
    if (!speedSquared) return contact;
    const Vec2 end=position+motion;
    const float margin=radius+0.0001f*length(motion);
    const float left=std::min(position.x,end.x)-margin,right=std::max(position.x,end.x)+margin;
    const float top=std::min(position.y,end.y)-margin,bottom=std::max(position.y,end.y)+margin;
    static const auto bounds=[] {
        Vec2 min=Vertices[0],max=min;
        for (const Vec2 point:Vertices) {
            min={std::min(min.x,point.x),std::min(min.y,point.y)};
            max={std::max(max.x,point.x),std::max(max.y,point.y)};
        }
        return std::array<Vec2,2>{min,max};
    }();
    const auto& edges=elysianRockEdges<Vertices>();
    const auto validTime=[&](float time) {
        return time>=-0.0001f && time<=1 && time<contact.time;
    };
    for (const Vec2 center:Centers) {
        if (left>center.x+bounds[1].x || right<center.x+bounds[0].x ||
            top>center.y+bounds[1].y || bottom<center.y+bounds[0].y) continue;
        const Vec2 local=position-center;
        for (unsigned i=0;i<edges.size();++i) {
            const auto& edge=edges[i];
            const Vec2 offset=local-edge.point;
            const float towards=dot(motion,edge.normal);
            if (towards<0) {
                const float time=(radius-dot(offset,edge.normal))/towards;
                if (validTime(time)) {
                    const float along=dot(offset+motion*time,edge.tangent);
                    if (along>=0 && along<=edge.length) contact={std::max(0.0f,time),edge.normal};
                }
            }
            // Only exterior convex corners contribute arcs to the expanded outline.
            if (!edge.convex) continue;
            const float approach=dot(offset,motion);
            if (approach>=0) continue;
            const float discriminant=approach*approach-speedSquared*(dot(offset,offset)-radius*radius);
            if (discriminant<=0) continue;
            const float time=(-approach-std::sqrt(discriminant))/speedSquared;
            if (!validTime(time)) continue;
            const Vec2 radial=(offset+motion*time)*(1.0f/radius);
            const auto& previous=edges[(i+edges.size()-1)%edges.size()];
            if (dot(radial,edge.tangent)<=0 && dot(radial,previous.tangent)>=0)
                contact={std::max(0.0f,time),radial};
        }
    }
    return contact;
}

inline WallContact sweepElysianRocks038(Vec2 position,Vec2 motion,float radius) {
    return sweepElysianRockGroup<ElysianRockVertices038,ElysianRockOffsets038>(position,motion,radius);
}

inline WallEvent418 stepFrontier18501(Game& game,const Course& course,WallRun418* wall,
                                     unsigned tick,const TileField* easyWalls=nullptr) {
    game.step(nullptr,0,{},nullptr,0,nullptr,0,0,nullptr,0,nullptr,0,nullptr,easyWalls,
              nullptr,1,{},false,wall?wall->obstacle():sweepElysianRocks038);
    return wall?wall->step(game,course,tick):WallEvent418::None;
}
