#pragma once

void prepareFinaleDraw045(const Round& round,bool gameplay,bool result) {
    if (!gameplay) return;
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    if (currentCourse==CosmicFirstCourse) {
        if (!result)
            cosmicFieldArt045.prepare(0,cosmicFieldSlot045,vram045(CosmicFieldOffset045),
                                      CosmicField045VramBytes,scratch,ResultScratchBytes);
        const unsigned id=round.ticks*5/60%4;
        cosmicFlagArt045.prepare(id,cosmicFlagSlot045,vram045(CosmicFlagOffset045),
                                 CosmicFlag045VramBytes,scratch,ResultScratchBytes);
        cosmicFieldArt045.closeStream();
        cosmicFlagArt045.closeStream();
        fallbackPageIndex=ScreenPages;
        return;
    }
    if (currentCourse!=Elysian518Course) return;
    if (round.finale518.phase!=FinalePhase518::Hunt) prepareNeutral518();
    const auto& run=round.finale518;
    unsigned slots=0;
    for (unsigned i=0;i<std::size(ElysianMines518) && slots<std::size(mineBlastSlots518);++i) {
        const unsigned tick=run.mineAt[i];
        if (!tick) continue;
        const unsigned age=(round.ticks-tick)+run.defeatTicks+(round.mineLossTick518?round.totalTicks-round.mineLossTick518:0);
        const unsigned frame=age/2;
        if (frame>=ElysianMineBlasts518Count) continue;
        if (mineBlastArt518.prepare(frame,mineBlastSlots518[slots],
                                    vram045(BlastOffset518+slots*ElysianMineBlasts518VramBytes),
                                    ElysianMineBlasts518VramBytes,scratch,ResultScratchBytes))
            mineBlastIndices518[slots++]=i;
    }
    for (;slots<std::size(mineBlastSlots518);++slots) mineBlastSlots518[slots]={};
    mineBlastArt518.closeStream();
    if (run.keyWaiting && run.keyTicks>=2400) {
        const unsigned age=run.keyTicks-2400;
        const float slowing=std::clamp((static_cast<float>(age)-689)/49,0.0f,1.0f);
        const unsigned phase=std::min(age,689u)+static_cast<unsigned>(49*slowing*(1-0.5f*slowing));
        secretArt518.prepare(phase/6%12,secretSlot518,vram045(SecretOffset518),ElysianSecret518VramBytes,
                             scratch,ResultScratchBytes);
        secretArt518.closeStream();
        // All seven mines have detonated when the optional key scene starts.
        const unsigned locale=russian()?2:0;
        if (age>=964)
            secretLabels518.prepare(locale,secretLabelSlots518[0],vram045(MineOffset518),
                                    ElysianSecretLabels518VramBytes,scratch,ResultScratchBytes);
        if (age>=1148)
            secretLabels518.prepare(locale+1,secretLabelSlots518[1],
                                    vram045(MineOffset518+ElysianSecretLabels518VramBytes),
                                    SecretSecondLabelBytes518,scratch,ResultScratchBytes);
        secretLabels518.closeStream();
    } else {
        secretSlot518={};
        for (auto& slot:secretLabelSlots518) slot={};
    }
    fallbackPageIndex=ScreenPages;
}

void drawFinaleSprite045(const LiveResultArt::Slot& slot,Vec2 anchor,float scale,uint32_t tint) {
    const auto extent=[](unsigned size) {
        unsigned power=1;
        while (power<size) power*=2;
        return power;
    };
    for (unsigned i=0;i<slot.sprite.count;++i) {
        const auto& image=slot.images[i];
        const auto& tile=image.tile;
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(tile.format?GU_PSM_5650:GU_PSM_8888,0,0,GU_TRUE);
        sceGuTexImage(0,extent(tile.width),extent(tile.height),tile.pitch,image.pixels);
        sceGuTexFlush();
        patch({0,0,static_cast<int16_t>(tile.width),static_cast<int16_t>(tile.height),0,0},
              anchor.x+(slot.sprite.x+tile.x-272)*scale,
              anchor.y+(slot.sprite.y+tile.y-136)*scale,scale,tint);
    }
}

uint32_t cosmicRainbow045(float hue,float saturation=1,float value=1,unsigned alpha=255) {
    const float sector=hue*6;
    const unsigned part=static_cast<unsigned>(sector)%6;
    const float x=value*(1-saturation),t=sector-static_cast<unsigned>(sector);
    const float up=x+(value-x)*t,down=value-(value-x)*t;
    const float channels[6][3]={{value,up,x},{down,value,x},{x,value,up},
                                {x,down,value},{up,x,value},{value,x,down}};
    const auto channel=[&](unsigned i) { return static_cast<unsigned>(channels[part][i]*255+0.5f); };
    return GU_RGBA(channel(0),channel(1),channel(2),alpha);
}

void drawCosmicFlag601(const GameplayView& view,const Round& round) {
    if (!round.game.won && !round.caught && cosmicFlagSlot045.owner==&cosmicFlagArt045)
        drawLiveResultSprite(cosmicFlagSlot045,0,&view,1);
    else drawGoalFlag(round,0,0,0);
}

void drawCosmicGateArt045(const GameplayView& view,const Round& round,uint32_t tint) {
    const unsigned tick=round.ticks;
    const float phase=std::fmod(0.5f+tick/360.0f,1.0f);
    const auto center=PrismGate601.center;
    const float scale=view.worldScale();
    if (round.cosmicGateBroken) {
        const unsigned age=tick-round.cosmicGateBroken;
        if (age>=50) return;
        setTexture(GU_PSM_8888,CosmicGateProps045Width,CosmicGateProps045Height,cosmicGateArt045.texture());
        const float time=age/60.0f;
        for (unsigned i=0;i<16;++i) {
            const float dx=28.0f*(i%8)-98;
            const float dy=i<8?-16:16;
            const Vec2 offset{dx*(1+2*time),dy+(120*time-90)*time};
            actor(view,cosmicGateArt045.patch(11+i%4),center+offset,scale,
                  (tint&0xffffffu)|((255-age*5)<<24));
        }
        return;
    }
    cosmicPalette045[0]=cosmicRainbow045(phase,1,0.872685f);
    cosmicPalette045[1]=cosmicRainbow045(phase,0.301961f);
    cosmicPalette045[2]=cosmicPalette045[3]=cosmicRainbow045(phase,0.717647f);
    sceKernelDcacheWritebackRange(cosmicPalette045,32);
    setTexture(GU_PSM_T8,CosmicGatePattern045Width,CosmicGatePattern045Height,vram045(CosmicGateOffset045));
    sceGuClutMode(GU_PSM_8888,0,0xff,0);
    sceGuClutLoad(1,cosmicPalette045);
    constexpr unsigned Columns=14,Rows=4;
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(Rows*2*(Columns+1)*sizeof(TextureVertex)));
    for (unsigned row=0;row<Rows;++row) {
        auto* strip=vertices+row*2*(Columns+1);
        for (unsigned column=0;column<=Columns;++column)
            for (unsigned edge=0;edge<2;++edge) {
                const unsigned u=column*16,v=(row+edge)*16;
                const auto point=view.position(center+Vec2{static_cast<float>(u)-112,static_cast<float>(v)-32});
                const float wave=tick/30.0f+10*(point.x-ViewX)/ViewWidth+4*point.y/Height;
                const unsigned alpha=static_cast<unsigned>((0.61f+0.2f*std::cos(wave))*255);
                strip[column*2+edge]={static_cast<float>(u),static_cast<float>(v),
                                     GU_RGBA(255,255,255,alpha),point.x,point.y,0};
            }
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       2*(Columns+1),nullptr,strip);
    }
    setTexture(GU_PSM_8888,CosmicGateProps045Width,CosmicGateProps045Height,cosmicGateArt045.texture());
    const float age=round.cosmicGatePoor?tick-round.cosmicGatePoor:100;
    const float lockScale=1+0.5f*std::max(0.0f,1-std::abs(age-4)/15);
    actor(view,cosmicGateArt045.patch(0),center,scale*3*lockScale,White,true);
    actor(view,cosmicGateArt045.patch(1+tick/4%10),center+Vec2{0,-16},scale,White,true);
    setTexture(GU_PSM_8888,CosmicGateLabel045Width,CosmicGateLabel045Height,cosmicGateLabel045.texture());
    actor(view,cosmicGateLabel045.patch(0),center,scale,White,true);
}
