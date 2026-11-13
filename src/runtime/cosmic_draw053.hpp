#pragma once

uint32_t shadeHurrah053(const Round& round) {
    return round.lastHurrah.revealed?CosmicTrapTint617:White;
}

void prepareCosmicDraw053(const Round& round,bool) {
    const unsigned course=currentCourse-CosmicSixteenthCourse;
    auto& bank=cosmicLayers053[course];
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    auto* memory=goalFlags.scratch()+CosmicLayersRamOffset053;
    const auto load=[&](unsigned id,unsigned bytes,uint8_t* target,bool cached=true) {
        bank.prepare(id,cosmicSlots053[id],target,bytes,scratch,ResultScratchBytes,cached);
    };
    if (course==0) {
        load(0,CosmicLayers616SlotBytes[0],vram045(CosmicLayersVramOffset053),false);
        for (unsigned i=1;i<CosmicLayers616Count;++i) {
            const unsigned bytes=CosmicLayers616SlotBytes[i];
            load(i,bytes,memory);
            memory+=bytes;
        }
    } else if (course==1) {
        load(0,CosmicLayers617SlotBytes[0],vram045(CosmicLayersVramOffset053),false);
        const auto loadRam=[&](unsigned id) {
            const unsigned bytes=CosmicLayers617SlotBytes[id];
            load(id,bytes,memory);
            memory+=bytes;
        };
        loadRam(1);
        if (!round.lastHurrah.revealed) load(2,CosmicLayers617SlotBytes[2],memory);
        memory+=CosmicLayers617SlotBytes[2];
        const unsigned active=3+round.switches.state;
        for (unsigned i=3;i<=6;++i) if (i!=active) cosmicSlots053[i]={};
        load(active,CosmicLayers617SlotBytes[active],memory);
        memory+=63488;
        if (!round.dog.winner) load(7,CosmicLayers617SlotBytes[7],memory);
        memory+=CosmicLayers617SlotBytes[7];
        for (unsigned i=8;i<CosmicLayers617Count;++i) loadRam(i);
    } else load(0,CosmicLayers618SlotBytes[0],vram045(CosmicPropOffset053+128*256*4),false);
    bank.closeStream();
    fallbackPageIndex=ScreenPages;
}

void drawCosmicCloud053(unsigned tick,uint32_t shade) {
    const float speed=currentCourse==CosmicSeventeenthCourse?512.0f/640.0f:1.0f;
    const float scroll=std::fmod(tick*(50.0f/60.0f)*speed,512.0f);
    constexpr unsigned Colors[][3]={{255,0,0},{170,255,0},{0,255,170},{0,0,255},{255,0,0}};
    const unsigned phase=tick%600/150,blend=tick%150;
    const auto channel=[&](unsigned n) {
        return ((Colors[phase][n]*(150-blend)+Colors[phase+1][n]*blend)/150)*((shade>>(8*n))&255)/255;
    };
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_T8,0,0,GU_TRUE);
    sceGuClutMode(GU_PSM_8888,0,255,0);
    sceGuClutLoad(32,cosmicSkyPalette613);
    sceGuTexImage(0,512,512,512,vram045(CosmicEarthboundOffset045));
    sceGuTexFlush();
    sceGuTexWrap(GU_REPEAT,GU_REPEAT);
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    auto* v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
    const auto color=GU_RGBA(channel(0),channel(1),channel(2),255);
    v[0]={0,256-scroll,color,ViewX+136.0f,0,0};
    v[1]={1024,1280-scroll,color,ViewX+408.0f,272,0};
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
}

void drawCosmicGlass053(const LiveResultArt::Slot& slot,float angle,unsigned tick,float base,uint32_t shade) {
    const float c=std::cos(angle),s=std::sin(angle);
    for (unsigned i=0;i<slot.sprite.count;++i) {
        const auto& image=slot.images[i];
        const auto& tile=image.tile;
        unsigned w=1,h=1;
        while (w<tile.width) w*=2;
        while (h<tile.height) h*=2;
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(GU_PSM_8888,0,0,GU_TRUE);
        sceGuTexImage(0,w,h,tile.pitch,image.pixels);
        sceGuTexFlush();
        constexpr unsigned Step=20;
        const unsigned cols=(tile.width+Step-1)/Step,rows=(tile.height+Step-1)/Step,count=2*(cols+1);
        auto* mesh=static_cast<TextureVertex*>(sceGuGetMemory(rows*count*sizeof(TextureVertex)));
        for (unsigned row=0;row<rows;++row) {
            auto* strip=mesh+row*count;
            for (unsigned col=0;col<=cols;++col) for (unsigned edge=0;edge<2;++edge) {
                const float u=std::min<unsigned>(tile.width,col*Step);
                const float v=std::min<unsigned>(tile.height,(row+edge)*Step);
                const auto at=rotateCosmic053({slot.sprite.x+tile.x+u-272,slot.sprite.y+tile.y+v-136},c,s);
                const float wave=tick/30.0f+10*(272+at.x)/408+4*(136+at.y)/272;
                const unsigned alpha=static_cast<unsigned>(255*std::clamp(base+0.2f*std::cos(wave),0.0f,1.0f));
                strip[2*col+edge]={u,v,(shade&0x00ffffffu)|(alpha<<24),ViewX+272+at.x,136+at.y,0};
            }
            sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                           count,nullptr,strip);
        }
    }
}

void drawCosmicRainbow053(unsigned tick) {
    alignas(64) static uint32_t palette[8]{};
    const unsigned phase=(tick+180)%360;
    const auto hsv=[&](unsigned brightness,unsigned saturation) {
        const unsigned rising=phase%60*255/60;
        const unsigned low=brightness*(255-saturation)/255;
        const unsigned up=low+(brightness-low)*rising/255;
        const unsigned down=brightness-(brightness-low)*rising/255;
        switch (phase/60) {
        case 0: return GU_RGBA(brightness,up,low,255);
        case 1: return GU_RGBA(down,brightness,low,255);
        case 2: return GU_RGBA(low,brightness,up,255);
        case 3: return GU_RGBA(low,down,brightness,255);
        case 4: return GU_RGBA(up,low,brightness,255);
        default:return GU_RGBA(brightness,low,down,255);
        }
    };
    palette[1]=hsv(223,255);
    palette[2]=hsv(255,183);
    palette[3]=hsv(255,77);
    sceKernelDcacheWritebackRange(palette,sizeof palette);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_T8,0,0,GU_TRUE);
    sceGuClutMode(GU_PSM_8888,0,255,0);
    sceGuClutLoad(1,palette);
    sceGuTexImage(0,512,512,512,vram045(CosmicRainbowOffset053));
    sceGuTexFlush();
    constexpr unsigned Rows=14,Cols=14,Count=2*(Cols+1);
    auto* mesh=static_cast<TextureVertex*>(sceGuGetMemory(Rows*Count*sizeof(TextureVertex)));
    for (unsigned row=0;row<Rows;++row) {
        auto* strip=mesh+row*Count;
        for (unsigned col=0;col<=Cols;++col) for (unsigned edge=0;edge<2;++edge) {
            const float x=col/float(Cols),y=(row+edge)/float(Rows);
            const unsigned alpha=static_cast<unsigned>(255*(0.61f+0.2f*std::cos(tick/30.0f+10*(136+272*x)/408+4*y)));
            strip[2*col+edge]={512*x,512*y,GU_RGBA(255,255,255,alpha),ViewX+136+272*x,272*y,0};
        }
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       Count,nullptr,strip);
    }
}

void drawCosmicField053(const GameplayView& view,const Round& round,bool result) {
    if (round.backrooms) return;
    sceGuScissor(ViewX+136,0,ViewX+408,Height);
    const auto shade=shadeHurrah053(round);
    const auto layer=[&](unsigned index) {
        const auto& slot=cosmicSlots053[index];
        if (slot.owner) drawLiveResultSprite(slot,0,nullptr,1,shade);
    };
    if (!result) {
        drawCosmicCloud053(round.ticks,shade);
        drawCosmicParticles601(round.ticks,view.worldScale(),shade,-1,290);
    }
    layer(0);
    const auto glass=[&](unsigned id,float angle,float base,uint32_t tint) {
        if (cosmicSlots053[id].owner) drawCosmicGlass053(cosmicSlots053[id],angle,round.ticks,base,tint);
    };
    if (currentCourse==CosmicSixteenthCourse) {
        constexpr uint32_t ambient=GU_RGBA(217,217,217,255);
        glass(1,0,0.61f,ambient);
        for (unsigned i=0;i<2;++i) {
            glass(2+i,CosmicRotationRates616[i]*round.ticks/60,1.0f,ambient);
        }
    } else if (currentCourse==CosmicSeventeenthCourse) {
        glass(1,0,0.61f,shade);
        if (!round.lastHurrah.revealed) glass(2,0,0.61f,shade);
        layer(3+round.switches.state);
        if (!round.dog.winner) layer(7);
        for (unsigned i=0;i<2;++i) {
            const bool active=round.rhythm.phase==Metronome048::Both || round.rhythm.phase==i;
            layer((active?10:8)+i);
        }
    } else if (!result) drawCosmicRainbow053(round.ticks);
    sceGuScissor(0,0,Width,Height);
}

template<unsigned Width,unsigned Height,unsigned Count>
void bindProps053(const ParkPatchBank<Width,Height,Count>& bank) {
    setTexture(GU_PSM_8888,Width,Height,bank.texture());
}

void drawLateCosmicDoors053(const GameplayView& view,const Round& round) {
    if (!round.keyStage) return;
    const bool hurrah=currentCourse==CosmicSeventeenthCourse;
    for (unsigned i=0;i<round.keyStage->doorCount;++i) {
        const auto center=round.keyStage->doors[i].center;
        if (round.roundKeys.opened&(1u<<i)) {
            drawDestruction(view,center,round.ticks-round.roundKeys.openedAt[i],KeyDoorRun::DoorBonus);
            continue;
        }
        const unsigned alpha=static_cast<unsigned>(255*round.roundKeys.lockAlpha[i]);
        if (hurrah) {
            bindProps053(cosmicProps617);
            const unsigned id=i<2?CosmicDoor617BlueHorizontal:i<4?CosmicDoor617BlueVertical:CosmicDoor617GreenVertical;
            actor(view,cosmicProps617.patch(id),center,1,shadeHurrah053(round));
            if (alpha) actor(view,cosmicProps617.patch(CosmicLock617),center,1,GU_RGBA(255,255,255,alpha));
        } else {
            bindProps053(cosmicProps616);
            actor(view,cosmicProps616.patch(CosmicDoor616),center);
            if (alpha) actor(view,cosmicProps616.patch(CosmicLock616),center,1,GU_RGBA(255,255,255,alpha));
        }
    }
}

void drawLateCosmicBlocks053(const GameplayView& view,const Round& round) {
    const auto& stage=courseHazards(currentCourse);
    const bool hurrah=currentCourse==CosmicSeventeenthCourse;
    for (unsigned i=0;i<stage.blockCount;++i) {
        const auto& block=stage.blocks[i];
        if (round.brokenBlocks.has(i)) {
            drawDestruction(view,block.center,round.ticks-round.blockTicks[i],round.blockValues[i]);
        } else if (hurrah) {
            bindProps053(cosmicProps617);
            actor(view,cosmicProps617.patch(CosmicBlock617),block.center,1,shadeHurrah053(round));
        } else {
            bindProps053(cosmicProps616);
            const unsigned id=block.halfSize.y==40?1:block.halfSize.x==40?2:0;
            actor(view,cosmicProps616.patch(CosmicBlocks616+id),block.center);
        }
    }
}

void drawCosmicProps053(const GameplayView& view,const Round& round,bool result) {
    if (currentCourse==CosmicSixteenthCourse) {
        bindProps053(cosmicProps616);
        actor(view,cosmicProps616.patch(CosmicGoal616),round.scene.goal);
        actor(view,cosmicProps616.patch(CosmicGoal616Flag+round.ticks/5%4),round.scene.goal);
    } else if (currentCourse==CosmicEighteenthCourse) {
        bindProps053(cosmicProps618);
        actor(view,cosmicProps618.patch(CosmicGoal618),round.scene.goal);
        actor(view,cosmicProps618.patch(CosmicGoal618Flag+round.ticks/5%4),round.scene.goal);
    } else {
        const auto shade=shadeHurrah053(round);
        bindProps053(cosmicProps617);
        actor(view,cosmicProps617.patch(CosmicGoal617True),round.scene.goal,1,shade);
        actor(view,cosmicProps617.patch(CosmicGoal617Fake),CosmicFakeGoal617,1,shade);
        const unsigned at=round.lastHurrah.revealed;
        const unsigned age=at?round.ticks-at:0;
        if (!at) actor(view,cosmicProps617.patch(CosmicGoal617Flag+round.ticks/5%4),CosmicFakeGoal617);
        else if (age<150) {
            actor(view,cosmicProps617.patch(CosmicGoal617DeadFlag),CosmicFakeGoal617,1,
                  GU_RGBA(64,51,51,255*(150-age)/150));
        }
        for (unsigned i=0;i<std::size(CosmicSwitches617);++i)
            actor(view,cosmicProps617.patch(CosmicSwitch617+round.switches.face(i)),CosmicSwitches617[i].center,1,shade);
        actor(view,cosmicProps617.patch(CosmicDoghouse617+round.ticks/6%11),CosmicDog617.goal,1,shade);
        if (!chillMode && !round.game.won) {
            actor(view,cosmicProps617.patch(CosmicFinalGhostPatch617),round.lastHurrah.hunter.position,1,shade);
            Vec2 atScreen;
            if (view.ghostIndicator(round.lastHurrah.hunter.position,atScreen))
                actor(view,cosmicProps617.patch(CosmicGhostPointer617),atScreen);
        }
        if (at && !result) {
            constexpr float indicators[]={-320,-160,304,464};
            const unsigned active=(age/24)%4;
            for (unsigned i=0;i<4;++i)
                actor(view,cosmicProps617.patch(i==active?CosmicExitLit617:CosmicExit617),{indicators[i],544});
            if (age<41) {
                bindProps053(cosmicTrap617);
                const unsigned frame=std::min(16u,age*25/60);
                for (const Vec2 pos: {Vec2{272,576},Vec2{512,576},Vec2{384,512}})
                    actor(view,cosmicTrap617.patch(frame),pos);
            }
        }
        if (bindFrontierDogArt()) {
            setFrontierTexture(frontierDogArt);
            actor(view,frontierDogArt.patch(11),round.dog.body.center,
                  view.worldZoom()*round.dog.graphicScale/CosmicDog617.graphicScale,shade);
        }
        if (!result) {
            auto* memory=vram045(CosmicRingOffset053);
            if (const auto* pixels=parkArt.ringTexture(memory,0))
                drawSmileyRing(view,CosmicRing617,round.smiley,round.ticks,CosmicRingTint617,CosmicRingTint617,pixels);
        }
    }
}


constexpr float HurrahIndicators053[]={-320,-160,304,464};

void drawHurrahLights053(const GameplayView& view,const Round& round) {
    if (!cosmicLights053.texture()) return;
    bindProps053(cosmicLights053);
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_FIX,0,0xffffffffu);
    const float scale=view.worldScale();
    for (unsigned i=0;i<2;++i)
        if (round.rhythm.phase==Metronome048::Both || round.rhythm.phase==i)
            actor(view,cosmicLights053.patch(0),{i?80.0f:-80.0f,544},2*scale,GU_RGBA(177,0,210,255));
    if (round.lastHurrah.revealed) {
        constexpr uint32_t colors[]={0xff0000ffu,0xff00ff80u,
                                     0xffffff00u,0xffff0080u};
        const unsigned age=round.ticks-round.lastHurrah.revealed;
        actor(view,cosmicLights053.patch(0),{640,544},3*scale,colors[age/15%4]);
        actor(view,cosmicLights053.patch(0),round.game.position,2*scale);
        actor(view,cosmicLights053.patch(1),{HurrahIndicators053[age/24%4],544},2*scale,GU_RGBA(0,255,0,153));
    }
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
}

void drawHurrahHud053(const Round& round) {
    if (!cosmicProps617.texture()) return;
    if (round.smiley.everStarted && cosmicUfo617.texture()) {
        const unsigned age=round.ticks-round.smiley.ufoStart;
        const float x=(944.0f-3.0f*age)*GameplayView::RasterScale;
        if (x>-100 && x<498) {
            bindProps053(cosmicUfo617);
            const auto& sprite=cosmicUfo617.patch(round.ticks/6%3);
            sceGuScissor(ViewX,0,ViewX+408,Height);
            patch(sprite,ViewX+x+sprite.x,246.5f+sprite.y);
            sceGuScissor(0,0,Width,Height);
        }
    }
    Vec2 at{108,249};
    if (round.lastHurrah.hunterActive) {
        const auto p=round.lastHurrah.hunter.position;
        at={(256+(p.x+round.scene.right)*256/round.scene.right)*GameplayView::RasterScale,
            ((p.y+round.scene.bottom)*256/round.scene.bottom)*GameplayView::RasterScale};
    }
    if (at.x<-30 || at.x>170 || at.y<-30 || at.y>302) return;
    sceGuScissor(ViewX,0,ViewX+132,Height);
    bindProps053(cosmicProps617);
    const auto& sprite=cosmicProps617.patch(CosmicFakeThinker617);
    patch(sprite,ViewX+at.x+sprite.x,at.y+sprite.y);
    sceGuScissor(0,0,Width,Height);
}
