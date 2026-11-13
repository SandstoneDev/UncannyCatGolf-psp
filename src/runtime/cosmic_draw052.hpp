#pragma once

constexpr float SantaScale=17.0f/32.0f;
constexpr float SantaCourseTop=-2176;
constexpr float SantaBackdropTop=-1488;
constexpr float SantaChunkWorld=256;
// The backdrops are absent on result and backrooms screens, which reclaim this workspace.
constexpr unsigned SantaBackdropScratchOffset=sizeof fallbackPage-CosmicParallax615VramBytes;
constexpr unsigned SantaScratchBytes=SantaBackdropScratchOffset-ResultScratchOffset;
static_assert(SantaBackdropScratchOffset%64==0 && SantaScratchBytes>=41697);

float santaChunkTop(const Round& round,unsigned index,bool backdrop=false) {
    const float top=backdrop?358.4f-0.4f*round.santa.cameraY+SantaBackdropTop:
                            256-round.santa.cameraY+SantaCourseTop;
    return (top+index*SantaChunkWorld)*SantaScale;
}

template<unsigned N>
void prepareSantaChunks(LiveResultArt& bank,LiveResultArt::Slot (&slots)[N],unsigned frames,
                        const Round& round,uint8_t* memory,bool cached) {
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    for (unsigned i=0;i<frames;++i) {
        const float y=santaChunkTop(round,i);
        if (y>=Height || y+136<=0) continue;
        const unsigned slot=i%N;
        bank.prepare(i,slots[slot],memory+slot*SantaChunkBytes,SantaChunkBytes,scratch,SantaScratchBytes,cached);
    }
    bank.closeStream();
}

bool drawSantaKey052(const Round& round,bool result) {
    return !result && (round.santa.keyActive(round.ticks) || (round.roundKeys.available&2u));
}

void prepareCosmicDraw052(const Round& round,bool result) {
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    if (currentCourse==CosmicFourteenthCourse) {
        cosmicWater614.prepare(round.ticks*25/60%16,cosmicWaterSlot614,vram045(CosmicFieldOffset045),
                               CosmicWater614VramBytes,scratch,ResultScratchBytes);
        cosmicWater614.closeStream();
    } else {
        auto* ram=goalFlags.scratch();
        prepareSantaChunks(santaCourseArt,santaCourseSlots,10,round,ram,true);
        prepareSantaChunks(santaGlassArt,santaGlassSlots,10,round,ram+3*SantaChunkBytes,true);
        if (result || round.backrooms) santaBackdropSlots[2]={};
        else {
            prepareSantaSprite052();
            uint8_t* backdropMemory[]={vram045(SantaBackdropOffset),ram+SantaBackdropRamOffset,
                                       reinterpret_cast<uint8_t*>(fallbackPage)+SantaBackdropScratchOffset};
            for (unsigned i=0;i<CosmicParallax615Count;++i) {
                const float y=santaChunkTop(round,i,true);
                if (y>=Height || y+136<=0) continue;
                const unsigned slot=i%3;
                santaBackdropArt.prepare(i,santaBackdropSlots[slot],backdropMemory[slot],
                                         CosmicParallax615VramBytes,scratch,SantaScratchBytes,slot!=0);
            }
            santaBackdropArt.closeStream();
        }
        santaStuckArt.prepare(0,santaStuckSlot,ram+SantaStuckOffset,CosmicStuck615VramBytes,
                              scratch,SantaScratchBytes,true);
        santaStuckArt.closeStream();
        if (drawSantaKey052(round,result)) {
            const unsigned frame=(round.ticks*80/60)%249;
            santaKeyArt[frame/128].prepare(frame%128,santaKeySlot,vram045(SantaKeyOffset),CosmicKey615VramBytes,
                               scratch,SantaScratchBytes);
            santaKeyArt[frame/128].closeStream();
        }
    }
    fallbackPageIndex=ScreenPages;
}

void drawSantaChunkGlass(const LiveResultArt::Slot& slot,float slide,unsigned tick,float base) {
    constexpr unsigned Step=24;
    for (unsigned i=0;i<slot.sprite.count;++i) {
        const auto& image=slot.images[i];
        const auto& tile=image.tile;
        const auto extent=[](unsigned size) {
            unsigned power=1;
            while (power<size) power*=2;
            return power;
        };
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(GU_PSM_8888,0,0,GU_TRUE);
        sceGuTexImage(0,extent(tile.width),extent(tile.height),tile.pitch,image.pixels);
        sceGuTexFlush();
        const float x=slot.sprite.x+tile.x,y=slot.sprite.y+tile.y+slide;
        const unsigned cols=(tile.width+Step-1)/Step,rows=(tile.height+Step-1)/Step;
        const unsigned count=2*(cols+1);
        auto* mesh=static_cast<TextureVertex*>(sceGuGetMemory(rows*count*sizeof(TextureVertex)));
        for (unsigned row=0;row<rows;++row) {
            auto* strip=mesh+row*count;
            for (unsigned col=0;col<=cols;++col) {
                const float u=std::min<unsigned>(tile.width,col*Step);
                for (unsigned edge=0;edge<2;++edge) {
                    const float v=std::min<unsigned>(tile.height,(row+edge)*Step);
                    const float wave=tick/30.0f+10*(x+u)/408+4*(y+v)/272;
                    const unsigned alpha=static_cast<unsigned>(255*std::clamp(base+0.2f*std::cos(wave),0.0f,1.0f));
                    strip[2*col+edge]={u,v,GU_RGBA(255,255,255,alpha),ViewX+x+u,y+v,0};
                }
            }
            sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                           count,nullptr,strip);
        }
    }
}

void drawSantaEarthbound(unsigned tick,float cameraY) {
    constexpr unsigned Side=512,Rows=20,Cols=16;
    constexpr unsigned Count=2*(Cols+1);
    const float t=tick/60.0f;
    const float offset=cameraY*0.01f/Side;
    setTexture(GU_PSM_8888,Side,Side,cosmicEarthboundPixels045());
    sceGuTexWrap(GU_REPEAT,GU_REPEAT);
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    for (unsigned layer=0;layer<2;++layer) {
        auto* mesh=static_cast<TextureVertex*>(sceGuGetMemory(Rows*Count*sizeof(TextureVertex)));
        for (unsigned row=0;row<Rows;++row) {
            auto* strip=mesh+row*Count;
            for (unsigned col=0;col<=Cols;++col) {
                const float x=col/static_cast<float>(Cols);
                const float u=x+0.49f;
                for (unsigned edge=0;edge<2;++edge) {
                    const float y=(row+edge)/static_cast<float>(Rows);
                    const float v=y-0.005f+offset;
                    const float wave=3*std::sin(v+0.05f*t)+0.16f*t;
                    const float vertical=v+3*std::sin(u+0.5f*t)+(layer?-0.03f:0.08f)*t;
                    const unsigned tint=layer?GU_RGBA(255,255,255,128):White;
                    strip[2*col+edge]={(u+(layer?-wave:wave))*Side,vertical*Side,tint,
                                       ViewX+136+x*272,y*272,0};
                }
            }
            sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                           Count,nullptr,strip);
        }
    }
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
}

void drawSantaSprite(const GameplayView& view,const Round& round) {
    if (!cosmicSanta615.texture() || round.game.won || round.caught) return;
    const auto& sprite=cosmicSanta615.patch(0);
    const auto center=view.position({123.2f,-54.4f+round.santa.cameraY*0.9f});
    const float zoom=view.worldZoom(),t=round.ticks/60.0f;
    const float top=center.y+sprite.y*zoom;
    if (top>=Height || top+sprite.height*zoom<=0) return;
    setTexture(GU_PSM_8888,CosmicSanta615Width,CosmicSanta615Height,cosmicSanta615.texture());
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    constexpr unsigned Segments=8,Count=2*(Segments+1);
    for (unsigned layer=0;layer<2;++layer) {
        const auto tint=layer?GU_RGBA(255,255,255,128):White;
        auto* mesh=static_cast<TextureVertex*>(sceGuGetMemory(Segments*Count*sizeof(TextureVertex)));
        for (unsigned row=0;row<Segments;++row) {
            auto* strip=mesh+row*Count;
            for (unsigned col=0;col<=Segments;++col) {
                const float u=col/static_cast<float>(Segments);
                const float warp=0.01f*std::sin(2*u+t);
                const float dy=0.01f*std::sin(4*u+4*t);
                for (unsigned edge=0;edge<2;++edge) {
                    const float v=(row+edge)/static_cast<float>(Segments);
                    const float wave=0.01f*std::sin(2*v+4*t);
                    const float x=std::clamp((u+warp+(layer?-wave:wave))*sprite.width,0.5f,sprite.width-0.5f);
                    const float y=std::clamp((v+dy)*sprite.height,0.5f,sprite.height-0.5f);
                    strip[2*col+edge]={sprite.u+x,sprite.v+y,tint,
                                       center.x+(sprite.x+u*sprite.width)*zoom,top+v*sprite.height*zoom,0};
                }
            }
            sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                           Count,nullptr,strip);
        }
    }
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
}

void drawSeaLights614(const GameplayView& view,unsigned tick) {
    if (!cosmicWaterSlot614.owner) return;
    const DiscoLight051 lights(view,tick);
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_FIX,0,0xffffffffu);
    constexpr unsigned Step=16;
    for (unsigned i=0;i<cosmicWaterSlot614.sprite.count;++i) {
        const auto& image=cosmicWaterSlot614.images[i];
        const auto& tile=image.tile;
        const auto extent=[](unsigned size) {
            unsigned power=1;
            while (power<size) power*=2;
            return power;
        };
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(GU_PSM_8888,0,0,GU_TRUE);
        sceGuTexImage(0,extent(tile.width),extent(tile.height),tile.pitch,image.pixels);
        sceGuTexFlush();
        const float x=ViewX+cosmicWaterSlot614.sprite.x+tile.x;
        const float y=cosmicWaterSlot614.sprite.y+tile.y;
        const unsigned cols=(tile.width+Step-1)/Step,rows=(tile.height+Step-1)/Step;
        const unsigned count=2*(cols+1);
        auto* mesh=static_cast<TextureVertex*>(sceGuGetMemory(rows*count*sizeof(TextureVertex)));
        for (unsigned row=0;row<rows;++row) {
            auto* strip=mesh+row*count;
            for (unsigned col=0;col<=cols;++col) {
                const float u=std::min<unsigned>(tile.width,col*Step);
                for (unsigned edge=0;edge<2;++edge) {
                    const float v=std::min<unsigned>(tile.height,(row+edge)*Step);
                    const Vec2 p{x+u,y+v};
                    strip[2*col+edge]={u,v,lights.tint(p,false),p.x,p.y,0};
                }
            }
            sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                           count,nullptr,strip);
        }
    }
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
}

void drawCosmicField052(const GameplayView& view,const Round& round,bool result) {
    if (round.backrooms) return;
    sceGuScissor(ViewX+136,0,ViewX+408,Height);
    if (currentCourse==CosmicFourteenthCourse) {
        if (cosmicWaterSlot614.owner) drawLiveResultSprite(cosmicWaterSlot614);
        if (!result) drawSeaLights614(view,round.ticks);
    } else {
        if (!result) {
            drawSantaEarthbound(round.ticks,round.santa.cameraY);
            constexpr float DefaultScale=GameplayView::RasterScale;
            drawCosmicParticles601(round.ticks,DefaultScale,White,-1,166,0);
            drawCosmicParticles601(round.ticks,DefaultScale,White,-1,166,193);
            drawCosmicParticles601(round.ticks,DefaultScale,White,-1,166,389);
            for (const auto& slot:santaBackdropSlots)
                if (slot.owner) drawSantaChunkGlass(slot,santaChunkTop(round,slot.id,true),round.ticks,0.2f);
            drawSantaSprite(view,round);
        }
        for (const auto& slot:santaCourseSlots)
            if (slot.owner) drawLiveResultSprite(slot,santaChunkTop(round,slot.id));
        for (const auto& slot:santaGlassSlots)
            if (slot.owner) drawSantaChunkGlass(slot,santaChunkTop(round,slot.id),round.ticks,0.61f);
        if (santaStuckSlot.owner) drawSantaChunkGlass(santaStuckSlot,0,round.ticks,0.61f);
    }
    sceGuScissor(0,0,Width,Height);
}

void drawSantaDoors(const GameplayView& view,const Round& round) {
    if (!cosmicProps615.texture()) return;
    setTexture(GU_PSM_8888,CosmicProps615Width,CosmicProps615Height,cosmicProps615.texture());
    for (unsigned i=0;i<2;++i) {
        const auto center=CosmicKeys615.doors[i].center;
        if (!(round.roundKeys.opened&(1u<<i)))
            actor(view,cosmicProps615.patch(1+i),center,view.worldZoom());
        const unsigned alpha=static_cast<unsigned>(255*round.roundKeys.lockAlpha[i]);
        if (alpha) actor(view,cosmicProps615.patch(3),center,view.worldZoom(),GU_RGBA(255,255,255,alpha));
        if (round.roundKeys.opened&(1u<<i)) {
            drawDestruction(view,center,round.ticks-round.roundKeys.openedAt[i],KeyDoorRun::DoorBonus);
            setTexture(GU_PSM_8888,CosmicProps615Width,CosmicProps615Height,cosmicProps615.texture());
        }
    }
}

void drawSantaBlocks(const GameplayView& view,const Round& round) {
    const auto& stage=courseHazards(currentCourse);
    if (!cosmicProps615.texture()) return;
    setTexture(GU_PSM_8888,CosmicProps615Width,CosmicProps615Height,cosmicProps615.texture());
    for (unsigned i=0;i<stage.blockCount;++i) {
        const auto& block=stage.blocks[i];
        if (!round.brokenBlocks.has(i))
            actor(view,cosmicProps615.patch(block.halfSize.x==176?4:5),block.center,view.worldZoom());
        else {
            drawDestruction(view,block.center,round.ticks-round.blockTicks[i],round.blockValues[i]);
            setTexture(GU_PSM_8888,CosmicProps615Width,CosmicProps615Height,cosmicProps615.texture());
        }
    }
}

void drawCosmicGoals052(const GameplayView& view,const Round& round) {
    if (currentCourse==CosmicFifteenthCourse) {
        if (!cosmicProps615.texture()) return;
        setTexture(GU_PSM_8888,CosmicProps615Width,CosmicProps615Height,cosmicProps615.texture());
        const Vec2 goal=SantaRun052::goal(round.ticks);
        actor(view,cosmicProps615.patch(10),goal,view.worldZoom());
        actor(view,cosmicProps615.patch(11+round.ticks/5%4),goal,view.worldZoom());
        return;
    }
    if (!cosmicGoals614.texture()) return;
    setTexture(GU_PSM_8888,CosmicGoals614Width,CosmicGoals614Height,cosmicGoals614.texture());
    const auto draw=[&](Vec2 center,unsigned taken) {
        actor(view,cosmicGoals614.patch(0),center,1);
        if (!taken) actor(view,cosmicGoals614.patch(1+round.ticks/5%4),center,1);
        else {
            const unsigned age=round.ticks-taken;
            if (age<150) actor(view,cosmicGoals614.patch(5),center,1,
                               GU_RGBA(255,255,255,255*(150-age)/150));
            if (age<50) drawScorePopup(view,center,age,500);
            setTexture(GU_PSM_8888,CosmicGoals614Width,CosmicGoals614Height,cosmicGoals614.texture());
        }
    };
    draw(round.scene.goal,0);
    for (unsigned i=0;i<std::size(CosmicFakeGoals614);++i)
        draw(CosmicFakeGoals614[i],round.fakeGoals.triggered[i]);
}

void drawSantaProps(const GameplayView& view,const Round& round,bool result) {
    if (!cosmicProps615.texture()) return;
    setTexture(GU_PSM_8888,CosmicProps615Width,CosmicProps615Height,cosmicProps615.texture());
    const unsigned tick=round.ticks;
    const auto& art=cosmicProps615;
    actor(view,art.patch(9),{-39,-714},view.worldZoom());
    const float angle=tick*0.1f;
    const float c=std::cos(angle),s=std::sin(angle);
    for (unsigned i=0;i<CosmicPortals615.count;++i) {
        const auto& portal=CosmicPortalNodes615[i];
        const auto& sprite=art.patch(0);
        const auto tint=CosmicPortalColors615[i];
        if (portal.target==PortalNode::NoTarget) {
            actor(view,sprite,portal.position,view.worldZoom(),(tint&0x00fefefeu)/2|0xff000000u);
            continue;
        }
        const Vec2 center=view.position(portal.position);
        if (center.y<-24 || center.y>Height+24) continue;
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
        for (unsigned corner=0;corner<4;++corner) {
            const float u=corner&1?sprite.width:0,v=corner&2?sprite.height:0;
            const float x=sprite.x+u,y=sprite.y+v;
            vertices[corner]={sprite.u+u,sprite.v+v,tint,center.x+x*c-y*s,center.y+x*s+y*c,0};
        }
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,vertices);
    }
    if (tick<224) {
        const Vec2 ufo{0,SantaRun052::ufoY(tick)};
        const float alpha=SantaRun052::beamAlpha(tick);
        if (alpha>0) {
            const Vec2 center=view.position(ufo);
            constexpr Vec2 corners[]={{-13,34},{19,34},{-83.8787f,186.0985f},{84.121f,186.0985f}};
            sceGuDisable(GU_TEXTURE_2D);
            constexpr float stops[]={0.0f,0.0188679f,0.556604f,1.0f};
            constexpr unsigned opacities[]={165,165,10,0};
            constexpr unsigned count=2*std::size(stops);
            auto* vertices=static_cast<ColorVertex*>(sceGuGetMemory(count*sizeof(ColorVertex)));
            for (unsigned row=0;row<std::size(stops);++row)
                for (unsigned side=0;side<2;++side) {
                    const Vec2 delta=(corners[side]+(corners[side+2]-corners[side])*stops[row])*view.worldScale();
                    vertices[2*row+side]={GU_RGBA(0,255,0,static_cast<unsigned>(opacities[row]*alpha)),
                                          center.x+delta.x,center.y+delta.y,0};
                }
            sceGuDrawArray(GU_TRIANGLE_STRIP,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,count,nullptr,vertices);
        }
        sceGuEnable(GU_TEXTURE_2D);
        actor(view,art.patch(6+tick/6%3),ufo,view.worldZoom());
    }
    if (drawSantaKey052(round,result) && santaKeySlot.owner) {
        const auto& motion=round.roundKeys.motion[1];
        const Vec2 at=view.position(motion.position);
        const unsigned alpha=motion.pickedAt?255-13*std::min(10u,tick-motion.pickedAt):255;
        const auto tint=(CosmicKeyRingTint615&0x00ffffffu)|(alpha<<24);
        const auto& sprite=santaKeySlot.sprite;
        for (unsigned i=0;i<sprite.count;++i) {
            const auto& image=santaKeySlot.images[i];
            const auto& tile=image.tile;
            const auto extent=[](unsigned size) {
                unsigned power=1;
                while (power<size) power*=2;
                return power;
            };
            sceGuEnable(GU_TEXTURE_2D);
            sceGuTexMode(GU_PSM_8888,0,0,GU_TRUE);
            sceGuTexImage(0,extent(tile.width),extent(tile.height),tile.pitch,image.pixels);
            sceGuTexFlush();
            patch({0,0,static_cast<int16_t>(tile.width),static_cast<int16_t>(tile.height),0,0},
                  at.x+(sprite.x+tile.x-272)*view.worldScale(),
                  at.y+(sprite.y+tile.y-136)*view.worldScale(),view.worldScale(),tint);
        }
    }
    unsigned generation=0;
    auto* workspace=clips.textureWorkspace<ParkArt::Side*ParkArt::Side*4>(generation);
    const auto* pixels=parkArt.ringTexture(workspace,generation);
    if (!pixels) return;
    const auto& ring=round.santa;
    if (!ring.ringFinished || tick-ring.ringFinished<(ring.ringPassed?110u:83u)) {
        setTexture(GU_PSM_8888,ParkArt::Side,ParkArt::Side,pixels);
        const auto tint=ring.keyRingStarted?((CosmicKeyRingTint615&0xffffffu)|0x80000000u):CosmicKeyRingTint615;
        sharedActor(view,parkArt.ring(tick),CosmicKeyRing615.center,tint);
    }
    const auto at=view.position(CosmicKeyRing615.center);
    if (ring.keyActive(tick)) {
        char timer[16];
        std::snprintf(timer,sizeof timer,"%u",(240-(tick-ring.ringTick)+59)/60);
        localeText(timer,at.x-ViewX,at.y-6,0.9f,White,true);
    } else if (ring.ringFinished && tick-ring.ringFinished<(ring.ringPassed?110u:83u)) {
        localeText(ring.ringPassed?lang("Good!","Отлично!"):lang("Fail...","Неудача..."),
                   at.x-ViewX,at.y-6,0.9f,White,true);
    }
}
