#pragma once

#include "game.hpp"
#include "visual.hpp"

class GameplayView {
public:
    static constexpr int Width = 408, Height = 272, HudWidth = 136;
    static constexpr float RasterScale = 17.0f / 32;

    struct Sprite {
        Vec2 topLeft, bottomRight;
        Vec2 uvTopLeft, uvBottomRight;
    };

    GameplayView(const Course& course,int left,bool mirrored=false,float zoomPulse=1,Vec2 cameraShift={})
        : left(left), bounds((course.bottom-course.top)*0.5f),
          zoom(256.0f/bounds*zoomPulse), mirrored(mirrored),shift(cameraShift) {}

    float worldZoom() const { return zoom; }
    float worldScale() const { return RasterScale * zoom; }

    Vec2 position(Vec2 world) const {
        const float rasterX=std::round((512.0f + (world.x+shift.x) * zoom) * RasterScale);
        return {left + (mirrored?Width+HudWidth-rasterX:rasterX),
                std::round((256.0f + (world.y+shift.y) * zoom) * RasterScale)};
    }

    Vec2 delta(Vec2 worldDelta) const { return Vec2{horizontal(worldDelta.x),worldDelta.y} * worldScale(); }

    bool clipSprite(const Patch& patch, Vec2 world, float scale, Sprite& sprite, bool projectedTexture=false) const {
        const Vec2 at = position(world);
        const bool flip=mirrored && !projectedTexture;
        const float offset = flip ? -patch.x-patch.width : patch.x;
        const float x = at.x + offset * scale, y = at.y + patch.y * scale;
        sprite.topLeft = {std::max(x, static_cast<float>(left + HudWidth)), std::max(y, 0.0f)};
        sprite.bottomRight = {std::min(x + patch.width * scale, static_cast<float>(left + Width)),
                              std::min(y + patch.height * scale, static_cast<float>(Height))};
        if (sprite.topLeft.x >= sprite.bottomRight.x || sprite.topLeft.y >= sprite.bottomRight.y) return false;
        const auto u=[&](float screenX) {
            const float offsetX=(screenX-x)/scale;
            return patch.u+(flip?patch.width-offsetX:offsetX);
        };
        sprite.uvTopLeft = {u(sprite.topLeft.x),patch.v + (sprite.topLeft.y - y) / scale};
        sprite.uvBottomRight = {u(sprite.bottomRight.x),patch.v + (sprite.bottomRight.y - y) / scale};
        return true;
    }

    bool ghostIndicator(Vec2 world, Vec2& target) const {
        const float near = bounds + 20, far = bounds + 120;
        const bool outsideX = std::abs(world.x) > near && std::abs(world.x) < far;
        const bool outsideY = std::abs(world.y) > near && std::abs(world.y) < far;
        if (!outsideX && !outsideY) return false;
        const float edge = bounds - 16;
        if (outsideX) {
            target.x = std::copysign(edge, world.x);
            target.y = outsideY ? std::copysign(edge, world.y) : target.x * world.y / world.x;
        } else {
            target.y = std::copysign(edge, world.y);
            target.x = target.y * world.x / world.y;
        }
        return true;
    }

private:
    int left;
    float bounds, zoom;
    bool mirrored;
    Vec2 shift;

    float horizontal(float x) const { return mirrored ? -x : x; }
};
