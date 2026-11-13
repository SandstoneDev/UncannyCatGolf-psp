#pragma once

void drawFrontierNight1415(const GameplayView& view,const Round& round) {
    if (!bindFrontierNight1415()) return;
    constexpr FrontierLightColor movieAmbient{0.326667f,0.245f,0.49f};
    constexpr FrontierLightColor noAmbient{};
    const bool movie=currentCourse==MovieOnceCourse;
    drawFrontierLightMap(view,frontierNight1415,FrontierLightPatch037,movie?movieAmbient:noAmbient,
        [&](const auto& light,const auto& lightSprite) {
            if (movie) {
                light({0,-320},512,{0,1,0},0.5f);
            } else {
                lightSprite(FrontierWorkspaceLightPatch037,{-192,0},{384,768},{0.408333f,0.245f,0.49f},1);
                light(frontierSpotlight415(round.ticks),448,FrontierWhiteLight,0.85f);
            }
            light(round.game.position,movie?256:192,FrontierWhiteLight,movie?0.6f:0.5f);
            if (!chillMode) {
                const auto ghostColor=movie?FrontierRedLight:FrontierWhiteLight;
                const float energy=movie?1.0f:0.26f;
                light(round.ghost.position,128,ghostColor,energy);
                for (unsigned i=0;i<round.extraGhostCount;++i)
                    light(round.extraGhosts[i].position,128,ghostColor,energy);
            }
        });
}

void drawFrontierSnow414(unsigned ticks,const GameplayView& view) {
    if (!bindFrontierNight1415()) return;
    const auto phase=[](unsigned seed) {
        return static_cast<float>(coaster033::hashInteger(seed)&0xffffu)/65536.0f;
    };
    const float time=1.4f+frontierSnowSeconds414(ticks);
    for (unsigned kind=0;kind<2;++kind) {
        const bool mist=kind==1;
        const unsigned count=mist?200:1000;
        const auto& sprite=frontierNight1415.patch(mist?FrontierMistPatch037:FrontierSnowflakePatch037);
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*count*sizeof(TextureVertex)));
        unsigned drawn=0;
        for (unsigned i=0;i<count;++i) {
            const unsigned seed=(kind*1000+i)*7;
            const float lifetime=(mist?1.04f:2.01f)*(1-0.62f*phase(seed));
            const float age=std::fmod(time+lifetime*phase(seed+1),lifetime);
            const float angle=-1.3734f+0.4095f*(2*phase(seed+2)-1);
            const float speed=mist?767.67f:818.18f;
            const float distance=age*(speed+(1500.0f-speed)*phase(seed+3));
            const auto center=view.position({192+512*phase(seed+4)+distance*std::cos(angle),
                                             (mist?640.0f:576.0f)+distance*std::sin(angle)});
            const float scale=(mist?0.5f+0.5f*phase(seed+5):0.1f+0.1f*phase(seed+5))*view.worldScale();
            const float half=(mist?FrontierMistSourceWidth037:sprite.width)*scale/2;
            if (center.x+half<ViewX || center.x-half>ViewX+ViewWidth ||
                center.y+half<0 || center.y-half>Height) continue;
            const unsigned alpha=static_cast<unsigned>((mist?30.0f:255.0f)*(1-0.75f*age/lifetime));
            const uint32_t tint=GU_RGBA(191,211,255,alpha);
            vertices[drawn++]={float(sprite.u),float(sprite.v),tint,center.x-half,center.y-half,0};
            vertices[drawn++]={float(sprite.u+sprite.width),float(sprite.v+sprite.height),tint,
                               center.x+half,center.y+half,0};
        }
        if (drawn) sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                                  drawn,nullptr,vertices);
    }
}
