#pragma once

void prepareCosmicDraw051(const Round& round);
void drawCosmicField051(const GameplayView& view,const Round& round);

void prepareCosmicDraw046(const Round& round,bool gameplay,bool result) {
    if (!gameplay || currentCourse<CosmicSecondCourse || currentCourse>=CosmicFourteenthCourse) return;
    if (result) {
        if (currentCourse==CosmicTenthCourse) {
            auto& flag=cosmicFlag046[CosmicTenthCourse-CosmicSecondCourse];
            auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
            flag.prepare(round.ticks/5%4,cosmicFlagSlot046,vram045(CosmicTailOffset046),
                         CosmicFlag610VramBytes,scratch,ResultScratchBytes);
            flag.closeStream();
            fallbackPageIndex=ScreenPages;
        }
        return;
    }
    if (currentCourse>=CosmicTwelfthCourse) {
        prepareCosmicDraw051(round);
        return;
    }
    const bool p3=currentCourse==CosmicThirdCourse;
    const unsigned stage=currentCourse-CosmicSecondCourse;
    const unsigned phase=usesPolyrhythm049(currentCourse)?round.polyrhythm.mask:usesMetronome048(currentCourse)?
        (currentCourse==CosmicSeventhCourse?3*round.switches.state:0)+round.rhythm.fieldIndex():
        (p3 || currentCourse==CosmicFourthCourse)?round.switches.state:0;
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    const bool rhythm=usesMetronome048(currentCourse);
    const unsigned glossBank=currentCourse==CosmicSixthCourse?0:1+round.switches.state;
    auto& field=currentCourse>=CosmicTenthCourse?cosmicGloss050[currentCourse==CosmicTenthCourse?0:1+phase/4]:
        currentCourse>=CosmicEighthCourse?cosmicGloss049[currentCourse-CosmicEighthCourse]:
        rhythm?cosmicGloss048[glossBank]:cosmicField046[stage];
    const unsigned frame=rhythm?(usesPolyrhythm049(currentCourse)?phase%4:round.rhythm.fieldIndex())*CosmicGloss048Phases+cosmicGlossPhase048(round.ticks):phase;
    auto& flag=cosmicFlag046[stage];
    if (!field.prepare(frame,cosmicFieldSlot046,vram045(CosmicFieldOffset045),
                       CosmicFieldBytes046,scratch,ResultScratchBytes) && rhythm) {
        cosmicField046[stage].prepare(phase,cosmicFieldSlot046,vram045(CosmicFieldOffset045),
                                     CosmicFieldBytes046,scratch,ResultScratchBytes);
        cosmicField046[stage].closeStream();
    }
    const bool largeFlag=currentCourse==CosmicTenthCourse;
    flag.prepare(round.ticks/5%4,cosmicFlagSlot046,
                 vram045(largeFlag?CosmicTailOffset046:CosmicFlagOffset046),
                 largeFlag?CosmicFlag610VramBytes:CosmicFlagBytes046,scratch,ResultScratchBytes);
    if (currentCourse<=CosmicThirdCourse) cosmicPlants046.prepare(round.ticks*CosmicPlants046Fps/60%CosmicPlants046Count,cosmicPlantSlot046,
                            vram045(p3?CosmicTailOffset046:CosmicPlantsOffset602),
                            CosmicPlantBytes046,scratch,ResultScratchBytes);
    if (!rhythm) field.closeStream();
    flag.closeStream();
    cosmicPlants046.closeStream();
    fallbackPageIndex=ScreenPages;
}

void prepareCosmicResult046(unsigned state) {
    if (!state) {
        cosmicFallback046=nullptr;
        return;
    }
    unsigned generation=0;
    auto* pixels=clips.textureWorkspace<FrameBytes>(generation);
    if (cosmicFallback046 && cosmicFallbackGeneration046==generation) return;
    cosmicFallback046=nullptr;
    const auto file=openAssetFile(cosmicFallbackPath046.c_str());
    if (!file || std::fseek(file.get(),0,SEEK_END)!=0) return;
    const long size=std::ftell(file.get());
    if (size<=0 || size>static_cast<long>(sizeof fallbackPage) ||
        std::fseek(file.get(),0,SEEK_SET)!=0 || std::fread(fallbackPage,size,1,file.get())!=1) return;
    fallbackPageIndex=ScreenPages;
    uLongf output=FrameBytes;
    if (uncompress(pixels,&output,reinterpret_cast<const Bytef*>(fallbackPage),size)!=Z_OK ||
        output!=FrameBytes) return;
    sceKernelDcacheWritebackRange(pixels,FrameBytes);
    cosmicFallback046=reinterpret_cast<const uint16_t*>(pixels);
    cosmicFallbackGeneration046=generation;
}

void drawCosmicResult046() {
    if (!cosmicFallback046) return;
    setTexture(GU_PSM_5650,512,512,cosmicFallback046);
    patch({0,0,272,272,0,0},ViewX+136,0);
}

void drawCosmicGlass602(const LiveResultArt::Slot& slot,unsigned tick) {
    const auto extent=[](unsigned size) {
        unsigned power=1;
        while (power<size) power*=2;
        return power;
    };
    constexpr unsigned MeshStep=16;
    const float time=std::fmod(tick/30.0f,6.283185307f);
    for (unsigned i=0;i<slot.sprite.count;++i) {
        const auto& image=slot.images[i];
        const auto& tile=image.tile;
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(GU_PSM_8888,0,0,GU_TRUE);
        sceGuTexImage(0,extent(tile.width),extent(tile.height),tile.pitch,image.pixels);
        sceGuTexFlush();
        const float x=ViewX+slot.sprite.x+tile.x,y=slot.sprite.y+tile.y;
        const unsigned cols=(tile.width+MeshStep-1)/MeshStep,rows=(tile.height+MeshStep-1)/MeshStep;
        const unsigned verticesPerRow=2*(cols+1);
        auto* mesh=static_cast<TextureVertex*>(sceGuGetMemory(rows*verticesPerRow*sizeof(TextureVertex)));
        for (unsigned row=0;row<rows;++row) {
            auto* strip=mesh+row*verticesPerRow;
            for (unsigned col=0;col<=cols;++col) {
                const float u=std::min<unsigned>(tile.width,col*MeshStep);
                for (unsigned edge=0;edge<2;++edge) {
                    const float v=std::min<unsigned>(tile.height,(row+edge)*MeshStep);
                    const float wave=time+10*(x+u-ViewX)/ViewWidth+4*(y+v)/Height;
                    const unsigned alpha=static_cast<unsigned>((0.61f+0.2f*std::cos(wave))*255);
                    strip[2*col+edge]={u,v,GU_RGBA(255,255,255,alpha),x+u,y+v,0};
                }
            }
            sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                           verticesPerRow,nullptr,strip);
        }
    }
}

void drawCosmicField046(const GameplayView& view,const Round& round) {
    if (currentCourse>=CosmicTwelfthCourse) {
        drawCosmicField051(view,round);
        return;
    }
    const unsigned stage=currentCourse-CosmicSecondCourse;
    const auto* field=currentCourse>=CosmicTenthCourse?
        &cosmicGloss050[currentCourse==CosmicTenthCourse?0:1+round.polyrhythm.mask/4]:
        currentCourse>=CosmicEighthCourse?&cosmicGloss049[currentCourse-CosmicEighthCourse]:usesMetronome048(currentCourse)?
        &cosmicGloss048[currentCourse==CosmicSixthCourse?0:1+round.switches.state]:&cosmicField046[stage];
    if (round.game.won || round.caught || (cosmicFieldSlot046.owner!=field &&
        cosmicFieldSlot046.owner!=&cosmicField046[stage])) return;
    sceGuScissor(ViewX+136,0,ViewX+408,Height);
    if (stage) {
        const auto tint=currentCourse==CosmicFifthCourse?starstormTint047(White):usesMetronome048(currentCourse)?cosmicShade048(currentCourse):White;
        if (currentCourse==CosmicThirdCourse)
            drawCosmicEarthbound045(round.ticks,153.0f/1342,1177.0f/1342,28.0f/1111,1052.0f/1111);
        else if (currentCourse==CosmicSixthCourse)
            drawCosmicEarthbound045(round.ticks,409.0f/1342,921.0f/1342,284.0f/1111,796.0f/1111,tint);
        else if (currentCourse==CosmicSeventhCourse)
            drawCosmicEarthbound045(round.ticks,281.0f/1342,1049.0f/1342,156.0f/1111,924.0f/1111,tint);
        else if (currentCourse==CosmicEighthCourse)
            drawCosmicEarthbound045(round.ticks,48.0f/512,944.0f/512,48.0f/512,944.0f/512,tint);
        else if (currentCourse==CosmicNinthCourse)
            drawCosmicEarthbound045(round.ticks,0.125f,1.125f,0.125f,1.125f,tint,true);
        else if (currentCourse==CosmicTenthCourse || currentCourse==CosmicEleventhCourse) {
            const float end=currentCourse==CosmicTenthCourse?0.875f:1.625f;
            drawCosmicEarthbound045(round.ticks,0.125f,end,0.125f,end,tint,true);
        } else drawCosmicEarthbound045(round.ticks,0,1,0,1,tint);
        if (usesPolyrhythm049(currentCourse)) {
            const bool triple=currentCourse==CosmicEleventhCourse;
            const unsigned count=triple?166:250;
            drawCosmicParticles601(round.ticks,view.worldScale(),tint,round.rhythm.particleSeconds,count,0);
            drawCosmicParticles601(round.ticks,view.worldScale(),tint,round.polyrhythm.particleSeconds,count,193);
            if (triple)
                drawCosmicParticles601(round.ticks,view.worldScale(),tint,round.polyrhythm.orangeSeconds,count,389);
        } else drawCosmicParticles601(round.ticks,view.worldScale(),tint,usesMetronome048(currentCourse)?round.rhythm.particleSeconds:-1.0f);
        drawLiveResultSprite(cosmicFieldSlot046);
    } else {
        setTexture(GU_PSM_5650,512,512,vram045(CosmicEarthboundOffset045));
        patch({0,0,272,272,0,0},ViewX+136,0);
        drawCosmicGlass602(cosmicFieldSlot046,round.ticks);
    }
    sceGuScissor(0,0,Width,Height);
}

void drawCosmicFlag046(const Round& round) {
    const unsigned stage=currentCourse-CosmicSecondCourse;
    if ((currentCourse==CosmicTenthCourse || (!round.game.won && !round.caught)) &&
        cosmicFlagSlot046.owner==&cosmicFlag046[stage])
        drawLiveResultSprite(cosmicFlagSlot046);
    else drawGoalFlag(round,0,0,0);
}
