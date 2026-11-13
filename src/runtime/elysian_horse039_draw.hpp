#pragma once

constexpr unsigned HorseCloudOffset039=HorseObjects039Width*HorseObjects039Height;

void drawHorseZoom039(const uint16_t* texture,float zoom,bool mirrored=false) {
    constexpr int width=ViewWidth-GameplayView::HudWidth;
    constexpr float x=ViewX+GameplayView::HudWidth+width*0.5f,y=Height*0.5f;
    if (zoom<=1.001f && !mirrored) return;
    sceGuScissor(ViewX+GameplayView::HudWidth,0,ViewX+ViewWidth,Height);
    setTexture(GU_PSM_5650,VisualWidth,VisualHeight,texture);
    patch({GameplayView::HudWidth,0,width,Height,0,0},x-width*zoom*0.5f,y-Height*zoom*0.5f,zoom,White,mirrored);
    sceGuScissor(0,0,Width,Height);
}

void drawHomingHorseGoal039(const GameplayView& view,const HorseGoal039& goal,const Patch& art,uint32_t canvasTint) {
    const Vec2 center=view.position(goal.position);
    const float w=art.width*0.5f*view.worldScale()*goal.widthScale/0.376471f;
    const float h=art.height*0.5f*view.worldScale()*goal.heightScale/0.367816f;
    const Vec2 right{std::cos(goal.rotation),std::sin(goal.rotation)};
    const Vec2 down{-right.y,right.x};
    const float facing=view.delta({1,0}).x<0?-1.0f:1.0f;
    const Vec2 x{right.x*w*facing,right.y*w},y{down.x*h*facing,down.y*h};
    const Vec2 corners[]={center-x-y,center+x-y,center+x+y,center-x+y};
    const float u=art.u,v=art.v,r=u+art.width,b=v+art.height;
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
    vertices[0]={u,v,canvasTint,corners[0].x,corners[0].y,0};
    vertices[1]={r,v,canvasTint,corners[1].x,corners[1].y,0};
    vertices[2]={r,b,canvasTint,corners[2].x,corners[2].y,0};
    vertices[3]={u,b,canvasTint,corners[3].x,corners[3].y,0};
    sceGuDrawArray(GU_TRIANGLE_FAN,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                   4,nullptr,vertices);
}

auto horseTint039(uint32_t canvasTint) {
    const auto tint=[canvasTint](float saturation,unsigned alpha) {
        const unsigned channel=255-static_cast<unsigned>(std::clamp(saturation,0.0f,1.0f)*190);
        const unsigned red=canvasTint&255u, green=(canvasTint>>8)&255u, blue=(canvasTint>>16)&255u;
        return static_cast<uint32_t>(GU_RGBA(red,(green*channel+127)/255,(blue*channel+127)/255,alpha));
    };
    return tint;
}

void drawHorseTrails039(const GameplayView& view,const HorseRun039& horse,uint32_t canvasTint) {
    if (!bindWallAtlas418(horseObjects039)) return;
    const auto& body=horseObjects039.patch(0);
    const auto tint=horseTint039(canvasTint);
    for (unsigned i=0;i<horse.trails.size();++i) {
        const auto& trail=horse.trails[(horse.trailIndex+i)%horse.trails.size()];
        const unsigned age=horse.age-trail.tick;
        if (trail.tick && age<20)
            actor(view,body,trail.position,view.worldScale()*4*trail.scale.x,
                  tint(horse.saturation,255-age*13),true);
    }
}

void drawHorseObjects039(const GameplayView& view,const HorseRun039& horse,uint32_t canvasTint=White,
                         bool trails=true,uint32_t horseTint=0) {
    if (!bindWallAtlas418(horseObjects039)) return;
    const auto& body=horseObjects039.patch(0);
    if (!horseTint) horseTint=canvasTint;
    const auto tint=horseTint039(horseTint);
    if (trails) drawHorseTrails039(view,horse,horseTint);
    for (const auto& goal:horse.goals) {
        if (goal.phase==HorseGoalPhase039::Ready) {
            actor(view,horseObjects039.patch(2),goal.position,view.worldScale(),canvasTint,true);
        } else if (goal.phase==HorseGoalPhase039::Homing) {
            const unsigned frame=(horse.age-goal.launchedAt)*25/60%4;
            drawHomingHorseGoal039(view,goal,horseObjects039.patch(2+frame),canvasTint);
        }
    }
    if (horse.phase!=HorsePhase039::Finished) {
        const auto shake=[&](unsigned axis) {
            const unsigned sample=(horse.age*1103515245u+axis*2654435761u)>>24;
            return (sample*(2.0f/255.0f)-1.0f)*horse.shake*horse.scale.x/10;
        };
        const Vec2 point=horse.position+Vec2{shake(1),shake(2)};
        actor(view,horseObjects039.patch(1),horse.position,view.worldScale(),horseTint,true);
        const Vec2 at=view.position(point);
        const float sx=4*horse.scale.x*view.worldScale(),sy=4*horse.scale.y*view.worldScale();
        patchScaled(body,at.x+body.x*sx,at.y+body.y*sy,sx,sy,tint(horse.saturation,255),view.delta({1,0}).x<0);
    }
    for (unsigned i=0;i<horse.explosions.size();++i) {
        const auto& boom=horse.explosions[(horse.explosionIndex+i)%horse.explosions.size()];
        const unsigned frame=(horse.age-boom.tick)*20/60;
        if (boom.tick && frame<17) {
            actor(view,horseObjects039.patch(6+frame),boom.position,
                  view.worldScale()*(boom.quiet?1:2),horseTint,true);
        }
    }
}

void drawHorseMeter039(const HorseRun039& horse,unsigned tick) {
    if (!horse.maximumHealth) return;
    // Hole backdrops use gameplayPage or the clip buffer. Reuse the inactive fallback page.
    static_assert(HorseMeter039Width*HorseMeter039Height<=sizeof fallbackPage);
    fallbackPageIndex=ScreenPages;
    if (!horseMeter039.useShared(reinterpret_cast<uint8_t*>(fallbackPage),0)) return;
    setFrontierTexture(horseMeter039);
    hudPatch(horseMeter039.patch(russian()?1:0));
    const auto& fill=horseMeter039.patch(2);
    Patch live=fill;
    const float progress=std::clamp((horse.age-horse.lastHitAt)/9.0f,0.0f,1.0f);
    const float health=horse.health+(horse.healthBeforeHit-horse.health)*(1-progress)*(1-progress);
    live.width=static_cast<int16_t>(std::round(fill.width*health/horse.maximumHealth));
    if (live.width>0) hudPatch(live);
    hudPatch(horseMeter039.patch(3+tick/4%11));
    const auto& marker=horseMeter039.patch(14);
    for (unsigned i=1;i<horse.maximumHealth;++i) {
        const float x=ViewX+fill.x+std::round(static_cast<float>(fill.width)*i/horse.maximumHealth);
        patch(marker,x,fill.y);
    }
}
