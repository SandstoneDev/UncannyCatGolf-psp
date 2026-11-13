#pragma once

// The board and credits use pre-existing frame and texture workspaces.
bool endingFrameReady045=false;

void drawElysianEnding045(unsigned ms,bool fast) {
    unsigned generation=0;
    auto* scratch=clips.compressedTextureWorkspace<ElysianEnding045::NativeBytes+ElysianEnding045::MaxPayload>(generation);
    auto* key=reinterpret_cast<uint8_t*>(fallbackPage);
    auto* out=levelSelectArt.base;
    fallbackPageIndex=ScreenPages;
    horseMeter039.invalidateShared();
    levelSelectArt.invalidateBase();
    const auto* pixels=endingArt045.frame(russian(),ms/(1000/ElysianEnding045::Fps),out,key,scratch);
    beginFrame();
    endingFrameReady045=pixels!=nullptr;
    if (pixels) {
        setTexture(GU_PSM_5650,512,512,pixels);
        patch({0,0,272,272,0,0},ViewX+136,0);
        if (fast) localeText(lang("Skipping...", "Пропуск..."),369,254,1,White,true);
    }
    endFrame();
}

bool updateElysianEnding045(Flow& flow,EndingClock045& clock,uint64_t delta,bool fast,bool skip) {
    clock.fast=fast;
    if (!skip) clock.micros+=delta*(fast?4:1);
    if (skip || clock.ms()>=EndingClock045::DurationMs) {
        endingArt045.close();
        audio.musicTune(1,1);
        audio.music(Sound::Menu);
        openLevelSelect(flow,CosmicFirstCourse);
        return true;
    }
    if (!clock.music && clock.ms()>=EndingClock045::MusicStartMs) {
        audio.music(Sound::GoodEndingMusic);
        clock.music=true;
    }
    if (clock.music) {
        audio.musicTune(fast?0.5f:1.0f,fast?4.0f:1.0f);
        const float seconds=audio.musicPositionSeconds();
        if (seconds>0) {
            const auto synced=static_cast<unsigned>(seconds*1000)+EndingClock045::MusicStartMs;
            if (synced<EndingClock045::DurationMs-1250 &&
                std::abs(static_cast<int>(synced)-static_cast<int>(clock.ms()))>100)
                clock.micros=static_cast<uint64_t>(synced)*1000;
        }
    }
    if (!clock.scary && clock.ms()>=EndingClock045::ScaryMs) {
        clock.scary=true;
        audio.play(Sound::Loss);
    }
    return false;
}

float finaleEase518(float t) {
    if (t<0.5f) return 4*t*t*t;
    t=1-t;
    return 1-4*t*t*t;
}

float finaleZoom518(const Round& round) {
    const auto& run=round.finale518;
    const float t=std::min(1.0f,run.cameraTicks/60.0f);
    return run.cameraEntered?1+finaleEase518(t):round.horse.zoomPulse;
}

Vec2 finaleCameraShift518(const Round& round) {
    const float t=std::min(1.0f,round.finale518.cameraTicks/60.0f);
    // At zoom one the original camera keeps the HUD offset and centers the arena at y=-144.
    return Vec2{0,144.0f}*finaleEase518(t);
}

uint32_t finaleTint518(const Round& round) {
    const float t=std::min(1.0f,round.finale518.defeatTicks/120.0f);
    const float fade=finaleEase518(t);
    return GU_RGBA(static_cast<unsigned>(191+64*fade),static_cast<unsigned>(207+48*fade),255,255);
}

void drawFinaleField518(const Round& round,const GameplayView& view) {
    constexpr Patch area{GameplayView::HudWidth,0,272,272,0,0};
    const Vec2 center=view.position({});
    const float zoom=view.worldZoom()*2;
    sceGuScissor(ViewX+GameplayView::HudWidth,0,ViewX+ViewWidth,Height);
    setTexture(GU_PSM_5650,512,512,gameplayPage);
    patch(area,center.x-136*zoom,center.y-136*zoom,zoom);
    if (neutralReady518 && round.finale518.defeatTicks) {
        setTexture(GU_PSM_5650,512,512,levelSelectArt.base);
        const float t=std::min(1.0f,round.finale518.defeatTicks/120.0f);
        patch(area,center.x-136*zoom,center.y-136*zoom,zoom,
              GU_RGBA(255,255,255,static_cast<unsigned>(255*finaleEase518(t))));
    }
    if (const auto* water=waterArt518.frame(round.totalTicks*25/60%ElysianWater518Count)) {
        setTexture(GU_PSM_8888,ElysianWater518Width,ElysianWater518Height,water);
        sceGuTexWrap(GU_REPEAT,GU_REPEAT);
        sceGuTexFilter(GU_LINEAR,GU_LINEAR);
        const auto tint=(finaleTint518(round)&0xffffffu)|(124u<<24);
        constexpr unsigned Count=2*std::size(ElysianWater518VisibleRects);
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(Count*sizeof(TextureVertex)));
        unsigned n=0;
        for (const auto& rect:ElysianWater518VisibleRects) {
            for (unsigned i:{0u,2u}) {
                const Vec2 world{float(rect[i]),float(rect[i+1])},p=view.position(world);
                vertices[n++]={world.x,world.y,tint,p.x,p.y,0};
            }
        }
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,Count,nullptr,vertices);
        sceGuTexFilter(GU_NEAREST,GU_NEAREST);
        sceGuTexWrap(GU_CLAMP,GU_CLAMP);
    }
    sceGuScissor(0,0,Width,Height);
}

void drawFinaleHorse518(const GameplayView& view,const Round& round) {
    const auto& horse=round.horse;
    const auto& run=round.finale518;
    if (run.aftermath()) return;
    if (run.phase!=FinalePhase518::Defeat) {
        drawHorseObjects039(view,horse,finaleTint518(round),true,GU_RGBA(230,207,255,255));
        return;
    }
    if (!bindWallAtlas418(horseObjects039)) return;
    for (const auto& boom:horse.explosions) {
        const unsigned frame=(horse.age-boom.tick)*20/60;
        if (boom.tick && frame<17)
            actor(view,horseObjects039.patch(6+frame),boom.position,view.worldScale()*(boom.quiet?1:2),White,true);
    }
    const unsigned tick=run.defeatTicks;
    if (tick>=540) {
        const float seconds=(tick-540)/60.0f;
        float y=0;
        if (seconds<0.25f) y=-48*std::sqrt(seconds*4);
        else if (seconds<0.5f) y=-48+48*std::pow((seconds-0.25f)*4,2);
        else if (seconds<0.75f) y=32*std::sqrt((seconds-0.5f)*4);
        else y=32-4128*std::pow((seconds-0.75f)/2.25f,2);
        setTexture(GU_PSM_8888,ElysianHorseDefeat518Width,ElysianHorseDefeat518Height,horseDefeatArt518.texture());
        const Vec2 position=horse.position+Vec2{0,y};
        actor(view,horseDefeatArt518.patch(2),position,view.worldScale(),White,true);
        actor(view,horseDefeatArt518.patch(3+tick/6%2),position+Vec2{0,-20},1.25f*view.worldScale(),White,true);
    } else if (tick<480) {
        const float flatten=1-std::clamp((tick-std::min(tick,300u))/180.0f,0.0f,1.0f);
        const Vec2 point=view.position(horse.position+Vec2{0,64*(1-flatten)});
        const float x=view.worldScale(),y=x*flatten;
        const bool alternate=tick>=150;
        if (alternate) setTexture(GU_PSM_8888,ElysianHorseDefeat518Width,ElysianHorseDefeat518Height,horseDefeatArt518.texture());
        const auto& art=alternate?horseDefeatArt518.patch(tick<240?0:1):horseObjects039.patch(0);
        patchScaled(art,point.x+art.x*x,point.y+art.y*y,x,y,finaleTint518(round));
    }
}

void drawMines518(const GameplayView& view,const Round& round) {
    const auto& run=round.finale518;
    const auto tint=finaleTint518(round);
    setTexture(GU_PSM_8888,ElysianMines518Width,ElysianMines518Height,mineArt518.texture());
    for (unsigned i=0;i<std::size(ElysianMines518);++i) {
        if (run.spent&(1u<<i)) continue;
        const auto center=ElysianMines518[i];
        actor(view,mineArt518.patch(0),center,view.worldScale(),tint,true);
        const float flicker=0.5f+0.5f*std::sin((round.totalTicks+23*i)*(0.035f+0.002f*i));
        actor(view,mineArt518.patch(1),center,0.7f*view.worldScale(),
              GU_RGBA(255,32,24,static_cast<unsigned>(145*flicker)),true);
    }
    for (unsigned i=0;i<3;++i) {
        if (mineBlastSlots518[i].owner!=&mineBlastArt518) continue;
        const Vec2 anchor=view.position(ElysianMines518[mineBlastIndices518[i]]);
        drawFinaleSprite045(mineBlastSlots518[i],anchor,view.worldScale(),tint);
    }
}

void drawGoldenClub518(const GameplayView& view,Vec2 tee,unsigned age) {
    const float t=std::clamp((static_cast<float>(age)-1095)/30,0.0f,1.0f);
    const float rx=2.34223f-3.141591f*t,ry=0.324631f*t;
    const float sy=std::sin(ry),cy=std::cos(ry),sx=std::sin(rx),cx=std::cos(rx);
    const auto& art=teeArt518.patch(1);
    constexpr float focal=96/0.700207538f;
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
    for (unsigned i=0;i<4;++i) {
        const int u=i&1?art.width:0,v=i&2?art.height:0;
        const float x=(art.x+u-100)*0.01f,y=-(art.y+v-128)*0.01f;
        const float cameraZ=2.40043f-(0.136f-sy*x+cy*sx*y);
        const float factor=2*focal/cameraZ;
        const float worldX=(1.638f+cy*x+sy*sx*y)*factor;
        const float worldY=-64-(1.99f+cx*y)*factor;
        const auto point=view.position(tee)+view.delta({worldX,worldY});
        vertices[i]={float(art.u+u),float(art.v+v),White,point.x,point.y,0};
    }
    sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                   4,nullptr,vertices);
}

void drawGoldenTee518(const GameplayView& view,const Round& round) {
    const auto& run=round.finale518;
    if (run.keyWaiting && run.keyTicks>=2400 && secretSlot518.owner==&secretArt518) {
        const float age=run.keyTicks-2400;
        const auto ramp=[age](float start,float end) { return std::clamp((age-start)/(end-start),0.0f,1.0f); };
        const auto tint=[](float a) { return GU_RGBA(255,255,255,static_cast<unsigned>(255*a)); };
        const float alpha=1-ramp(1496,1816);
        const Vec2 position{-688+538*ramp(0,689)+20*std::sqrt(ramp(689,738)),304};
        drawFinaleSprite045(secretSlot518,view.position(position),view.worldScale()*1.6f,tint(alpha));
        const Vec2 origins[]={{-368,72},{-232,144}};
        const float opacity[]={ramp(964,1092),ramp(1148,1308)};
        for (unsigned i=0;i<2;++i)
            if (secretLabelSlots518[i].owner==&secretLabels518)
                drawFinaleSprite045(secretLabelSlots518[i],view.position(origins[i]),view.worldScale(),
                                    tint(alpha*opacity[i]));
    }
    if (!run.usesTee()) return;
    const float y=run.phase==FinalePhase518::Drop?-640*(1-run.dropTicks/30.0f):0;
    setTexture(GU_PSM_8888,ElysianFinale518Width,ElysianFinale518Height,teeArt518.texture());
    const auto at=ElysianTee518.center+Vec2{0,y};
    actor(view,teeArt518.patch(0),at+Vec2{0,8},1.5f*view.worldScale(),White,true);
    actor(view,teeArt518.patch(2),at+Vec2{5,-3+6*std::sin(round.totalTicks/12.0f)},view.worldScale(),White,true);
    if (run.phase!=FinalePhase518::Attract) return;
    const float t=std::min(1.0f,run.attractTicks/720.0f);
    const float blend=std::min(1.0f,run.attractTicks/60.0f);
    const float ratio=blend*blend*(3-2*blend);
    const float r=32*(1-t),angle=100.531f*t*t*(3-2*t);
    const Vec2 orbit=at+Vec2{r*std::sin(angle),-r*std::cos(angle)};
    const Vec2 player=run.attractPosition+(orbit-run.attractPosition)*ratio;
    drawGoldenClub518(view,at,run.attractTicks);
    setTexture(GU_PSM_8888,ActorsWidth,ActorsHeight,actors);
    const float enlarged=std::clamp((run.attractTicks-std::min(run.attractTicks,1116u))/24.0f,0.0f,1.0f);
    actor(view,visual.cat,player,view.worldZoom()*(1-0.25f*t+18*enlarged),White,true);
}

void drawTeeFade518(const Round& round) {
    const unsigned age=round.finale518.attractTicks;
    if (round.finale518.fast && age)
        localeText(lang("Skipping...", "Пропуск..."),369,254,1,White,true);
    if (age<1116) return;
    const unsigned alpha=std::min(255u,(age-1116)*255/24);
    rectangle(ViewX+GameplayView::HudWidth,0,272,272,GU_RGBA(255,255,255,alpha));
}

void drawCosmicParticles601(unsigned tick,float scale=272.0f/768.0f,uint32_t tint=White,float particleSeconds=-1,unsigned amount=500,unsigned seedOffset=0) {
    constexpr float ParticleLife=5,SpeedScale=0.8f;
    const float time=particleSeconds<0?tick/60.0f*SpeedScale:particleSeconds;
    const auto random=[](uint32_t seed) {
        seed=(seed^(seed>>16))*0x7feb352du;
        seed=(seed^(seed>>15))*0x846ca68bu;
        return static_cast<float>((seed^(seed>>16))>>8)/16777216;
    };
    auto* vertices=static_cast<ColorVertex*>(sceGuGetMemory(amount*6*sizeof(ColorVertex)));
    unsigned count=0;
    for (unsigned i=0;i<amount;++i) {
        const float age=(time-i*ParticleLife/amount)/ParticleLife;
        if (age<0) continue;
        const auto cycle=static_cast<uint32_t>(age);
        const float phase=age-cycle,seconds=phase*ParticleLife;
        const uint32_t seed=3*(cycle*amount+i+1)+seedOffset;
        const float angle=6.283185307f*random(seed),velocity=150+50*random(seed+1);
        const float distance=(velocity+0.5f*seconds)*seconds*scale;
        const float x=ViewX+272+std::cos(angle)*distance,y=136+std::sin(angle)*distance;
        if (x<ViewX+134 || x>ViewX+410 || y<-2 || y>274) continue;
        const float size=(0.527472f+0.472528f*phase*phase*(3-2*phase))*
                         (1+3*random(seed+2))*scale*0.5f;
        const float a=std::min(1.0f,phase/0.346667f);
        const auto color=(tint&0xffffffu)|(static_cast<unsigned>(255*a*a*(3-2*a))<<24);
        const float theta=6.283185307f*ParticleLife*phase*phase*phase*(1-0.5f*phase);
        const float dx=size*std::cos(theta),dy=size*std::sin(theta);
        const ColorVertex corners[]={{color,x-dx+dy,y-dy-dx,0},{color,x+dx+dy,y+dy-dx,0},
                                     {color,x-dx-dy,y-dy+dx,0},{color,x+dx-dy,y+dy+dx,0}};
        for (unsigned index:{0u,1u,2u,2u,1u,3u}) vertices[count++]=corners[index];
    }
    sceGuDisable(GU_TEXTURE_2D);
    if (count) sceGuDrawArray(GU_TRIANGLES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,count,nullptr,vertices);
}

void drawCosmicEarthbound045(unsigned tick,float uMin,float uMax,float vMin,float vMax,uint32_t tint=White,bool deform=false,const DiscoLight051* lights=nullptr) {
    const float seconds=tick/60.0f;
    constexpr unsigned Size=CosmicEarthbound045Width;
    setTexture(GU_PSM_8888,Size,Size,cosmicEarthboundPixels045());
    sceGuScissor(ViewX+136,0,ViewX+408,Height);
    sceGuTexWrap(GU_REPEAT,GU_REPEAT);
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    if (deform) {
        constexpr unsigned Rows=28, Columns=12;
        auto* mesh=static_cast<TextureVertex*>(sceGuGetMemory(Rows*(Columns+1)*2*sizeof(TextureVertex)));
        for (unsigned layer=0;layer<2;++layer) {
            // Use a separate command per row to interpolate the authored U-dependent deformation.
            auto* vertices=layer?static_cast<TextureVertex*>(sceGuGetMemory(Rows*(Columns+1)*2*sizeof(TextureVertex))):mesh;
            for (unsigned row=0;row<Rows;++row) {
                auto* strip=vertices+row*(Columns+1)*2;
                for (unsigned col=0;col<=Columns;++col) {
                    const float x=col/static_cast<float>(Columns),u=uMin+(uMax-uMin)*x;
                    const float warp=2*std::sin(u+0.3f*seconds);
                    for (unsigned edge=0;edge<2;++edge) {
                        const float y=(row+edge)/static_cast<float>(Rows),v=vMin+(vMax-vMin)*y;
                        const float wave=std::sin(2*v+0.05f*seconds)+0.16f*seconds;
                        const auto lit=lights?lights->tint({ViewX+136+x*272,y*272},true):tint;
                        const unsigned color=layer?((lit&0xffffffu)|0x80000000u):lit;
                        strip[2*col+edge]={(u+warp+(layer?-wave:wave))*Size,
                            (v+(layer?-0.03f:0.08f)*seconds)*Size,color,ViewX+136+x*272,y*272,0};
                    }
                }
                sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                               (Columns+1)*2,nullptr,strip);
            }
        }
    } else for (unsigned layer=0;layer<2;++layer) {
        constexpr unsigned Strips=68;
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory((Strips+1)*2*sizeof(TextureVertex)));
        for (unsigned row=0;row<=Strips;++row) {
            const float t=row/static_cast<float>(Strips),v=vMin+(vMax-vMin)*t;
            const float wave=std::sin(2*v+0.05f*seconds)+0.16f*seconds;
            const float offset=layer?-wave:wave;
            const float y=t*272;
            const unsigned color=layer?((tint&0xffffffu)|0x80000000u):tint;
            vertices[2*row]={(uMin+offset)*Size,(v+(layer?-0.03f:0.08f)*seconds)*Size,color,ViewX+136.0f,y,0};
            vertices[2*row+1]={(uMax+offset)*Size,vertices[2*row].v,color,ViewX+408.0f,y,0};
        }
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       (Strips+1)*2,nullptr,vertices);
    }
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
}

void drawCosmicField601(const Round& round) {
    if (round.game.won || round.caught || cosmicFieldSlot045.owner!=&cosmicFieldArt045) return;
    drawCosmicEarthbound045(round.ticks,152.0f/1040,920.0f/1040,84.0f/960,852.0f/960);
    drawCosmicParticles601(round.ticks);
    drawLiveResultSprite(cosmicFieldSlot045);
    sceGuScissor(0,0,Width,Height);
}

void drawCosmicGate601(const GameplayView& view,const Round& round) {
    const float phase=std::fmod(0.5f+round.ticks/360.0f,1.0f);
    const auto tint=cosmicRainbow045(phase);
    drawCosmicGateArt045(view,round,tint);
}
