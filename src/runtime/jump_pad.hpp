#pragma once

#include "game.hpp"
#include "pad_tracks.hpp"

struct JumpPad {
    Vec2 center;
    float radians,speedGranted;
};

struct JumpStage {
    static constexpr unsigned Capacity=9;
    uint32_t count;
    JumpPad pads[Capacity];
};
static_assert(sizeof(JumpStage)==148);

struct JumpStageView {
    const JumpPad* pads=nullptr;
    unsigned count=0;

    constexpr JumpStageView()=default;
    constexpr JumpStageView(const JumpStage& stage) : pads(stage.pads), count(stage.count) {}
    constexpr JumpStageView(const JumpPad* entries,unsigned size) : pads(entries), count(size) {}
    explicit operator bool() const { return count!=0; }
};

struct JumpRun {
    static constexpr unsigned FlightTicks=26;
    static constexpr float BaseScale=0.064f;
    static constexpr float PadRadius=10.0f;
    static constexpr float OutOfBoundsMargin=24.0f;

    unsigned timer=0,deathBuffer=0;
    float scale=BaseScale;

    bool airborne() const { return timer!=0; }

    bool advance() {
        if (timer) {
            if (++timer>FlightTicks) {
                timer=0;
                scale=BaseScale;
                deathBuffer=5;
            } else {
                animate();
            }
        }
        if (deathBuffer) --deathBuffer;
        return airborne();
    }

    void move(Game& game) const {
        // These levels have no collision shapes on the Jump Walls layer.
        game.position=game.position+game.velocity*(1.0f/60.0f);
    }

    bool tryLaunch(const JumpStage& stage,Game& game,const Course& course,unsigned towerTick=UINT32_MAX) {
        return tryLaunch(stage.pads,stage.count,game,course,towerTick);
    }

    bool tryLaunch(const JumpPad* pads,unsigned count,Game& game,const Course& course,unsigned towerTick=UINT32_MAX) {
        if (airborne()) return false;
        const float radius=course.playerRadius+PadRadius;
        for (unsigned i=0;i<count;++i) {
            const auto& pad=pads[i];
            const Vec2 center=towerTick==UINT32_MAX?pad.center:towerPadCenter(i,towerTick,pad.center);
            const Vec2 offset=center-game.position;
            if (dot(offset,offset)>radius*radius) continue;
            const float speed=pad.speedGranted>course.stopSpeed?pad.speedGranted:
                              std::max(course.stopSpeed,length(game.velocity));
            game.velocity=Vec2{std::cos(pad.radians),std::sin(pad.radians)}*(speed+10.0f);
            game.position=center;
            timer=1;
            animate();
            return true;
        }
        return false;
    }

    bool outside(Vec2 point,const Course& course) const {
        return point.x<course.left-OutOfBoundsMargin || point.x>course.right+OutOfBoundsMargin ||
               point.y<course.top-OutOfBoundsMargin || point.y>course.bottom+OutOfBoundsMargin;
    }

private:
    void animate() {
        scale+=(0.15f-scale)*(timer<=FlightTicks/2?0.13f:-0.13f);
    }
};
