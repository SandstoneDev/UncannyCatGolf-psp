#pragma once

bool prepareStarstormDraw047(bool active) {
    if (!active) return true;
    unsigned generation=0;
    auto* storage=clips.compressedTextureWorkspace<CosmicWeather605Width*CosmicWeather605Height*4>(generation);
    if (starstormWeatherGeneration047!=generation) {
        starstormWeather047.unload();
        starstormWeatherGeneration047=generation;
    }
    return starstormWeather047.loadDirect(starstormWeatherPath047,reinterpret_cast<uint32_t*>(storage),false);
}

void drawStarstorm047(unsigned tick) {
    if (!starstormWeather047.texture()) return;
    const auto random=[](uint32_t seed) {
        seed=(seed^(seed>>16))*0x7feb352du;
        seed=(seed^(seed>>15))*0x846ca68bu;
        return static_cast<float>((seed^(seed>>16))>>8)/16777216;
    };
    const auto smooth=[](float t) { return t*t*(3-2*t); };
    const float time=starstormParticleTime047(tick);
    constexpr float Scale=GameplayView::RasterScale, Degrees=3.141592654f/180;
    constexpr unsigned VertexType=GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D;
    setTexture(GU_PSM_8888,CosmicWeather605Width,CosmicWeather605Height,starstormWeather047.texture());
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    sceGuScissor(ViewX,0,ViewX+ViewWidth,Height);
    for (unsigned layer=0;layer<2;++layer) {
        const bool mist=layer==0;
        const unsigned count=mist?200:500;
        const float lifetime=mist?1.04f:2.01f;
        const auto& sprite=starstormWeather047.patch(mist?1:0);
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(count*(mist?6:2)*sizeof(TextureVertex)));
        unsigned n=0;
        for (unsigned i=0;i<count;++i) {
            const float cycle=time/lifetime-static_cast<float>(i)/count;
            if (cycle<0) continue;
            const auto loop=static_cast<unsigned>(cycle);
            const uint32_t seed=12*(i+count*loop+1)+layer;
            const float age=(cycle-loop)*lifetime;
            const float life=lifetime*(1-0.62f*random(seed+1));
            if (age>=life) continue;
            const float progress=age/life;
            const float direction=0.19739556f+(2*random(seed+2)-1)*0.40948915f;
            const float minSpeed=mist?767.67f:818.18f;
            const float speed=minSpeed+(1500-minSpeed)*random(seed+3);
            const float x=ViewX+((mist?896:832)-speed*age*std::cos(direction))*Scale;
            const float y=(512*random(seed+4)+speed*age*std::sin(direction))*Scale;
            const float size=(mist?0.5f+0.5f*random(seed+5):0.5f)*(2-smooth(progress))*Scale;
            constexpr float Middle=0.450980f,End=0.985294f;
            const float opacity=progress<Middle?1-0.404494f*smooth(progress/Middle):
                0.595506f-0.348315f*smooth(std::min(1.0f,(progress-Middle)/(End-Middle)));
            const unsigned alpha=static_cast<unsigned>((mist?45:190)*opacity+0.5f);
            if (mist) {
                const float reach=0.708f*sprite.width*size;
                if (x-reach>=ViewX+ViewWidth || x+reach<=ViewX || y-reach>=Height || y+reach<=0) continue;
                const float angle=Degrees*(-640+1178.2f*random(seed+6));
                const Vec2 right{std::cos(angle)*size,std::sin(angle)*size},down{-right.y,right.x};
                constexpr unsigned Corners[]={0,1,2,2,1,3};
                for (const unsigned corner:Corners) {
                    const float u=corner&1?sprite.width:0,v=corner&2?sprite.height:0;
                    const auto point=Vec2{x,y}+right*(sprite.x+u)+down*(sprite.y+v);
                    vertices[n++]={sprite.u+u,sprite.v+v,GU_RGBA(255,255,255,alpha),point.x,point.y,0};
                }
            } else {
                const float left=x+sprite.x*size,top=y+sprite.y*size;
                const float right=left+sprite.width*size,bottom=top+sprite.height*size;
                if (left>=ViewX+ViewWidth || right<=ViewX || top>=Height || bottom<=0) continue;
                const uint32_t color=cosmicRainbow045(random(seed+6),0.35f,1,alpha);
                vertices[n++]={float(sprite.u),float(sprite.v),color,left,top,0};
                vertices[n++]={float(sprite.u+sprite.width),float(sprite.v+sprite.height),color,right,bottom,0};
            }
        }
        if (n) sceGuDrawArray(mist?GU_TRIANGLES:GU_SPRITES,VertexType,n,nullptr,vertices);
    }
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuScissor(0,0,Width,Height);
}

void drawStarstormBlocks047(const GameplayView& view,const Round& round) {
    const auto& stage=courseHazards(currentCourse);
    setTexture(GU_PSM_8888,CosmicBlocks605Width,CosmicBlocks605Height,cosmicBlocks605.texture());
    for (unsigned i=0;i<stage.blockCount;++i)
        if (!round.brokenBlocks.has(i))
            actor(view,cosmicBlocks605.patch(i?1:0),stage.blocks[i].center,view.worldScale());
    for (unsigned i=0;i<stage.blockCount;++i)
        if (round.brokenBlocks.has(i))
            drawDestruction(view,stage.blocks[i].center,round.ticks-round.blockTicks[i],round.blockValues[i]);
}
