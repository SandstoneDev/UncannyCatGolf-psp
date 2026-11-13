#pragma once

void drawElysianMist043(unsigned ticks) {
    if (!bindFrontierCompressed037<decltype(elysianMist043),248*1024>(elysianMist043)) return;
    const auto random=[](unsigned key) {
        return static_cast<float>(coaster033::hashInteger(key)&0xffffu)/65536.0f;
    };
    const auto smooth=[](float t) { return t*t*(3-2*t); };
    const auto& art=elysianMist043.patch(0);
    constexpr float Scale=17.0f/32.0f;
    constexpr unsigned Particles=200;
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*Particles*sizeof(TextureVertex)));
    const float seconds=1.0f+ticks/60.0f;
    unsigned count=0;
    for (unsigned i=0;i<Particles;++i) {
        const unsigned key=7*i;
        const float lifetime=5.0f*(1-0.62f*random(key));
        const float age=std::fmod(0.3f*seconds+5.0f*random(key+1),5.0f);
        if (age>=lifetime) continue;
        const float t=age/lifetime;
        const float angle=0.19739556f+(2*random(key+2)-1)*0.40948915f;
        const float distance=age*(700+800*random(key+3));
        const float x=ViewX+Scale*(992-distance*std::cos(angle));
        const float y=Scale*(40+512*random(key+4)-distance*std::sin(angle));
        const float radius=0.5f*Scale*art.width*(0.5f+0.5f*random(key+5))*(2-smooth(t));
        if (x+radius<ViewX || x-radius>ViewX+ViewWidth || y+radius<0 || y-radius>Height) continue;
        constexpr float Middle=0.450980f,End=0.985294f;
        const float opacity=t<Middle?1.0f-0.404494f*smooth(t/Middle):
            0.595506f-0.348315f*smooth(std::min(1.0f,(t-Middle)/(End-Middle)));
        const auto tint=GU_RGBA(255,255,255,static_cast<unsigned>(35*opacity+0.5f));
        vertices[count++]={float(art.u),float(art.v),tint,x-radius,y-radius,0};
        vertices[count++]={float(art.u+art.width),float(art.v+art.height),tint,x+radius,y+radius,0};
    }
    if (count) {
        sceGuTexFilter(GU_LINEAR,GU_LINEAR);
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       count,nullptr,vertices);
        sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    }
}
