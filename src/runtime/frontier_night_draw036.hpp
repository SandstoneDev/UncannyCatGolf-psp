#pragma once

template<class Atlas,class DrawLights>
void drawFrontierLightMap(const GameplayView& view,const Atlas& atlas,unsigned lightPatch,
                          FrontierLightColor ambient,const DrawLights& drawLights) {
    // Store half the lighting factor so bright overlaps can exceed 1 before compositing.
    constexpr int LightSide=256;
    constexpr uintptr_t LightOffset=FrontierLightVramOffset;
    static_assert(FrontierLightVramBytes==LightSide*LightSide*4);
    constexpr int FieldLeft=ViewX+GameplayView::HudWidth;
    constexpr int FieldWidth=ViewWidth-GameplayView::HudWidth;
    constexpr float LightX=static_cast<float>(LightSide)/FieldWidth;
    constexpr float LightY=static_cast<float>(LightSide)/Height;
    const auto tint=[](FrontierLightColor color,float gain=1.0f) {
        const auto channel=[](float value) { return static_cast<unsigned>(std::clamp(value,0.0f,2.0f)*127.5f+0.5f); };
        return GU_RGBA(channel(gain*color.red),channel(gain*color.green),channel(gain*color.blue),255);
    };

    sceGuDrawBufferList(GU_PSM_8888,reinterpret_cast<void*>(LightOffset),LightSide);
    sceGuScissor(0,0,LightSide,LightSide);
    sceGuDisable(GU_BLEND);
    rectangle(0,0,LightSide,LightSide,tint(ambient));
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD,GU_FIX,GU_FIX,0xffffffffu,0xffffffffu);
    setFrontierTexture(atlas);
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    const auto drawSprite=[&](const GameplayView::Sprite& sprite,uint32_t color) {
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
        vertices[0]={sprite.uvTopLeft.x,sprite.uvTopLeft.y,color,
                     (sprite.topLeft.x-FieldLeft)*LightX,sprite.topLeft.y*LightY,0};
        vertices[1]={sprite.uvBottomRight.x,sprite.uvBottomRight.y,color,
                     (sprite.bottomRight.x-FieldLeft)*LightX,sprite.bottomRight.y*LightY,0};
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,vertices);
    };
    const auto light=[&](Vec2 center,float diameter,FrontierLightColor color,float gain) {
        const auto& patch=atlas.patch(lightPatch);
        GameplayView::Sprite sprite;
        if (view.clipSprite(patch,center,view.worldScale()*diameter/patch.width,sprite))
            drawSprite(sprite,tint(color,gain));
    };
    const auto scaled=[&](unsigned index,Vec2 center,Vec2 size,FrontierLightColor color,float gain) {
        const auto& patch=atlas.patch(index);
        const auto at=view.position(center);
        const Vec2 extent=size*view.worldScale();
        const Vec2 origin=at-extent*0.5f;
        GameplayView::Sprite sprite;
        sprite.topLeft={std::max(origin.x,static_cast<float>(FieldLeft)),std::max(origin.y,0.0f)};
        sprite.bottomRight={std::min(origin.x+extent.x,static_cast<float>(FieldLeft+FieldWidth)),
                            std::min(origin.y+extent.y,static_cast<float>(Height))};
        if (sprite.topLeft.x>=sprite.bottomRight.x || sprite.topLeft.y>=sprite.bottomRight.y) return;
        const auto uv=[&](Vec2 screen) {
            return Vec2{patch.u+patch.width*(screen.x-origin.x)/extent.x,
                        patch.v+patch.height*(screen.y-origin.y)/extent.y};
        };
        sprite.uvTopLeft=uv(sprite.topLeft);
        sprite.uvBottomRight=uv(sprite.bottomRight);
        drawSprite(sprite,tint(color,gain));
    };
    drawLights(light,scaled);

    sceGuDrawBufferList(GU_PSM_5650,screenDrawBuffer,Stride);
    sceGuScissor(FieldLeft,0,ViewX+ViewWidth,Height);
    const auto* lightMap=reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(sceGeEdramGetAddr())+LightOffset);
    sceGuTexMode(GU_PSM_8888,0,0,GU_FALSE);
    sceGuTexImage(0,LightSide,LightSide,LightSide,lightMap);
    sceGuTexFlush();
    // src*dst + dst*src = 2*src*dst, with src being the half-intensity light map.
    sceGuBlendFunc(GU_ADD,GU_DST_COLOR,GU_SRC_COLOR,0,0);
    patchScaled({0,0,LightSide,LightSide,0,0},FieldLeft,0,1/LightX,1/LightY,White);
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
}

void drawFrontierNight1213(const GameplayView& view,const Round& round) {
    if (!bindFrontierNight1213()) return;
    const bool weather=currentCourse==WeatherAlertCourse;
    drawFrontierLightMap(view,frontierNight1213,FrontierLightPatch036,
                         weather?FrontierAmbient412:FrontierAmbient413,
        [&](const auto& light,const auto&) {
            const auto stationary=[&](const auto& lights) {
                for (const auto& lamp:lights) {
                    const float gain=lamp.flashIndex<0?lamp.energy:
                        lamp.energy*round.frontierFlash.brightness(static_cast<unsigned>(lamp.flashIndex));
                    light(lamp.center,lamp.diameter,lamp.color,gain);
                }
            };
            if (weather) stationary(FrontierStaticLights412);
            else stationary(FrontierStaticLights413);
            const auto moving=[&](Vec2 center,FrontierLightColor color,float energy) {
                light(center,FrontierMovingLightDiameter,color,energy);
            };
            moving(round.game.position,FrontierWhiteLight,FrontierPlayerLightEnergy);
            if (!chillMode) {
                moving(round.ghost.position,FrontierRedLight,FrontierGhostLightEnergy);
                for (unsigned i=0;i<round.extraGhostCount;++i)
                    moving(round.extraGhosts[i].position,FrontierRedLight,FrontierGhostLightEnergy);
            }
            if (!weather) {
                for (unsigned i=0;i<round.skullCount;++i) {
                    const auto& body=round.skulls.bodies[i];
                    if (body.radius>0) moving(body.center,FrontierWhiteLight,FrontierSkullLightEnergy);
                }
            }
        });
}
