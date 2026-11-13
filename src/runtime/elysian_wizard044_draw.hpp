#pragma once

void drawWizard516(const GameplayView& view,const WizardRun516& wizard,uint32_t tint) {
    const auto center=ElysianWizardPosition516;
    const float scale=view.worldScale();
    setTexture(GU_PSM_8888,ElysianWizard516Width,ElysianWizard516Height,wizardArt516.texture());
    constexpr float Pi=3.141592654f;
    Vec2 wand{16,wizard.idleWandY()};
    float angle=Pi/6;
    float glow=0;
    if (wizard.dead()) {
        const float t=std::min(wizard.age,60u)/60.0f;
        wand=Vec2{16,8}+Vec2{96,568}*(t*t);
        angle=Pi*(1+5*t)/6;
        actor(view,wizardArt516.patch(4+std::min(wizard.age/4,3u)),center,scale,
              GU_RGBA(((tint&255u)*166+127)/255,0,0,175));
    } else {
        actor(view,wizardArt516.patch(0),center,scale*(32.768f/33),tint);
        actor(view,wizardArt516.patch(wizard.changedHat()?2:1),center+Vec2{0,-32},scale,tint);
        if (wizard.castAge<WizardRun516::CastTicks) {
            const auto age=wizard.castAge;
            const float turn=age<9?age/9.0f:age<33?1.0f:(42-age)/9.0f;
            angle=Pi/6+Pi/3*turn;
            wand.y=wizard.castWandY;
            glow=age<9?0.0f:(42-age)/33.0f;
        }
    }

    const auto& art=wizardArt516.patch(3);
    const Vec2 pivot=view.position(center+wand);
    const Vec2 right{std::cos(angle),std::sin(angle)},down{-right.y,right.x};
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
    for (unsigned i=0;i<4;++i) {
        const float u=i&1?art.width:0,v=i&2?art.height:0;
        const auto point=pivot+view.delta(right*(art.x+u)+down*(art.y+v));
        vertices[i]={art.u+u,art.v+v,tint,point.x,point.y,0};
    }
    sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                   4,nullptr,vertices);
    if (glow>0 && bindFrontierCompressed037<decltype(elysianMist043),248*1024>(elysianMist043))
        actor(view,elysianMist043.patch(0),center+Vec2{40,8},0.5f*scale,
              (tint&0xffffffu)|(static_cast<unsigned>(255*glow+0.5f)<<24));
}

void drawWizardSpell516(const WizardRun516& wizard) {
    if (wizard.spell==WizardSpell516::None || wizard.dead() || wizard.age>=WizardRun516::SpellIconTicks) return;
    const float progress=wizard.age/static_cast<float>(WizardRun516::SpellIconTicks);
    const float scale=GameplayView::RasterScale*(1+progress);
    const auto& art=spellArt516.patch(static_cast<unsigned>(wizard.spell)-1);
    setTexture(GU_PSM_8888,ElysianSpells516Width,ElysianSpells516Height,spellArt516.texture());
    patch(art,ViewX+272+art.x*scale,136+art.y*scale,scale,GU_RGBA(255,255,255,static_cast<unsigned>(255*(1-progress))));
}

void drawWizardMist516(float elapsed) {
    if (!bindFrontierCompressed037<decltype(elysianMist043),248*1024>(elysianMist043)) return;
    const auto random=[](unsigned key) { return (coaster033::hashInteger(key)&0xffffu)/65536.0f; };
    const auto smooth=[](float t) { return t*t*(3-2*t); };
    const auto& art=elysianMist043.patch(0);
    constexpr float Scale=GameplayView::RasterScale, NominalLife=1.04f, Pi=3.141592654f;
    constexpr unsigned Particles=200;
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(6*Particles*sizeof(TextureVertex)));
    unsigned count=0;
    for (unsigned i=0;i<Particles;++i) {
        const unsigned key=7*i;
        const float lifetime=NominalLife*(1-0.62f*random(key));
        const float age=std::fmod(0.2f+elapsed+NominalLife*random(key+1),NominalLife);
        if (age>=lifetime) continue;
        const float t=age/lifetime;
        const float direction=0.19739556f+(2*random(key+2)-1)*0.40948915f;
        const float distance=age*(767.67f+732.33f*random(key+3));
        const Vec2 center{ViewX+Scale*(896-distance*std::cos(direction)),
                          Scale*(160+512*(random(key+4)-0.5f)+distance*std::sin(direction))};
        const float half=0.5f*Scale*art.width*(0.5f+0.5f*random(key+5))*(2-smooth(t));
        const float reach=1.415f*half;
        if (center.x+reach<ViewX || center.x-reach>ViewX+ViewWidth ||
            center.y+reach<0 || center.y-reach>Height) continue;
        const float angle=Pi/180*(-640+1178.2f*random(key+6));
        const Vec2 right{std::cos(angle)*half,std::sin(angle)*half},down{-right.y,right.x};
        constexpr float Middle=0.450980f,End=0.985294f;
        const float opacity=t<Middle?1.0f-0.404494f*smooth(t/Middle):
            0.595506f-0.348315f*smooth(std::min(1.0f,(t-Middle)/(End-Middle)));
        const auto tint=GU_RGBA(191,211,255,static_cast<unsigned>(35*opacity+0.5f));
        constexpr unsigned corners[]={0,1,2,2,1,3};
        for (const unsigned corner:corners) {
            const auto point=center+right*(corner&1?1.0f:-1.0f)+down*(corner&2?1.0f:-1.0f);
            vertices[count++]={float(art.u+(corner&1?art.width:0)),float(art.v+(corner&2?art.height:0)),
                               tint,point.x,point.y,0};
        }
    }
    if (count) {
        sceGuTexFilter(GU_LINEAR,GU_LINEAR);
        sceGuDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       count,nullptr,vertices);
        sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    }
}
