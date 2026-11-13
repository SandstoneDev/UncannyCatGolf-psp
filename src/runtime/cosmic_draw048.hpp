#pragma once

unsigned cosmicFieldIndex048(const Round& round) {
    if (usesPolyrhythm049(currentCourse)) return round.polyrhythm.mask;
    return (currentCourse==CosmicSeventhCourse?3*round.switches.state:0)+round.rhythm.fieldIndex();
}

void prepareRhythmDraw048(const Round& round,bool result) {
    if (currentCourse==CosmicSeventhCourse && !round.backrooms) {
        unsigned generation=0;
        auto* memory=clips.compressedTextureWorkspace<CosmicDoor607Width*CosmicDoor607Height*4>(generation);
        if (cosmicDoorGeneration048!=generation) {
            cosmicDoor607.unload();
            cosmicDoorGeneration048=generation;
        }
        cosmicDoor607.loadDirect(cosmicDoorPath048,reinterpret_cast<uint32_t*>(memory),false);
    }
    if (result || round.caught) return;
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    const auto prepare=[&](LiveResultArt& art,unsigned frame,LiveResultArt::Slot& slot,unsigned offset,unsigned capacity) {
        art.prepare(frame,slot,vram045(offset),capacity,scratch,ResultScratchBytes);
        art.closeStream();
    };
    if (currentCourse==CosmicSeventhCourse) {
        const unsigned waterFrame=round.ticks*CosmicWater607Fps/60%16;
        prepare(cosmicWater607,cosmicFieldIndex048(round)*16+waterFrame,cosmicWaterSlot048,
                CosmicWaterOffset048,CosmicWater607VramBytes);
        prepare(cosmicPlants607,round.ticks*CosmicPlants607Fps/60%CosmicPlants607Count,cosmicPlantsSlot048,
                CosmicPlantsOffset048,CosmicPlants607VramBytes);
    }
    if (currentCourse==CosmicEleventhCourse) {
        const unsigned mask=cosmicFieldIndex048(round);
        prepare(cosmicWater611,mask*16+round.ticks*CosmicWater611Fps/60%16,
                cosmicWaterSlot048,CosmicWaterOffset048,CosmicWater611VramBytes);
        prepare(cosmicWaterGloss611[mask/4],mask%4*CosmicGloss048Phases+cosmicGlossPhase048(round.ticks),
                cosmicWaterGlossSlot611,CosmicWaterOffset048+CosmicWater611VramBytes,CosmicWaterGloss611VramBytes);
    }
    const auto& rhythm=round.rhythm;
    if (rhythm.beats<15) {
        const unsigned frame=rhythm.beats?
            1+((rhythm.beats-1)%2)*CosmicMetronome048SwingFrames+
                std::min(rhythm.sinceBeat,CosmicMetronome048SwingFrames-1):0;
        prepare(cosmicMetronome048,frame,cosmicMetronomeSlot048,MetronomeOffset048,CosmicMetronome048VramBytes);
    } else {
        const unsigned frame=rhythm.sincePoof*CosmicMetronome048PoofFps/60;
        if (frame<CosmicMetronome048PoofFrames)
            prepare(cosmicMetronome048,1+2*CosmicMetronome048SwingFrames+frame,
                    cosmicMetronomeSlot048,MetronomeOffset048,CosmicMetronome048VramBytes);
        else cosmicMetronomeSlot048.owner=nullptr;
    }
    if (rhythm.labelFadeTicks<10)
        prepare(cosmicMetronome048,CosmicMetronome048Count-1,cosmicBeatLabelSlot048,
                MetronomeOffset048+CosmicMetronome048VramBytes,CosmicMetronome048LabelBytes);
    else cosmicBeatLabelSlot048.owner=nullptr;
    fallbackPageIndex=ScreenPages;
}

void drawRhythmScenery048() {
    sceGuScissor(ViewX+136,0,ViewX+408,Height);
    if (cosmicWaterSlot048.owner) drawLiveResultSprite(cosmicWaterSlot048);
    if (cosmicPlantsSlot048.owner) drawLiveResultSprite(cosmicPlantsSlot048);
    if (cosmicWaterGlossSlot611.owner) drawLiveResultSprite(cosmicWaterGlossSlot611);
    sceGuScissor(0,0,Width,Height);
}

void drawRhythmHud048(const Round& round) {
    if (cosmicMetronomeSlot048.owner) drawLiveResultSprite(cosmicMetronomeSlot048);
    if (cosmicBeatLabelSlot048.owner) {
        const auto& rhythm=round.rhythm;
        const unsigned alpha=(10-rhythm.labelFadeTicks)*255/10;
        const float t=std::min(rhythm.sinceBeat/16.0f,1.0f);
        const float y=rhythm.beats?-5*GameplayView::RasterScale*(1-std::sqrt(t)):0;
        drawLiveResultSprite(cosmicBeatLabelSlot048,y,nullptr,1,GU_RGBA(255,255,255,alpha));
    }
}

void prepareRhythmResult048(const Round& round) {
    const unsigned state=cosmicFieldIndex048(round);
    const auto path=cosmicFallbackPrefix048+std::to_string(state)+".z";
    if (cosmicFallbackPath046!=path) {
        cosmicFallbackPath046=path;
        cosmicFallback046=nullptr;
    }
    const unsigned initial=currentCourse==CosmicEleventhCourse?Polyrhythm049::Triple:
                           usesPolyrhythm049(currentCourse)?Polyrhythm049::All:0;
    prepareCosmicResult046(state!=initial);
}

uint32_t cosmicGhostTint048(const Round& round,unsigned index,uint32_t fallback) {
    if (!usesMetronome048(currentCourse)) return fallback;
    if (usesPolyrhythm049(currentCourse)) return fallback;
    const auto tint=currentCourse==CosmicEighthCourse?CosmicGhostTints608[index]:
        currentCourse==CosmicSixthCourse?CosmicGhostTints606[index]:CosmicGhostTints607[index];
    const unsigned shade=fallback&255;
    const unsigned alpha=round.rhythmGhostActive(index)?255:128;
    return GU_RGBA((tint&255)*shade/255,((tint>>8)&255)*shade/255,((tint>>16)&255)*shade/255,alpha);
}

void drawCosmicDoor607(const GameplayView& view,const Round& round) {
    if (!cosmicDoor607.texture() || !round.keyStage) return;
    const auto& state=round.roundKeys;
    const auto center=round.keyStage->doors[0].center;
    const bool opened=state.opened&1;
    setTexture(GU_PSM_8888,CosmicDoor607Width,CosmicDoor607Height,cosmicDoor607.texture());
    if (!opened) actor(view,cosmicDoor607.patch(0),center,view.worldScale());
    const unsigned alpha=static_cast<unsigned>(255*state.lockAlpha[0]);
    if (alpha) actor(view,cosmicDoor607.patch(1),center,view.worldScale(),GU_RGBA(255,255,255,alpha));
    if (opened) drawDestruction(view,center,round.ticks-state.openedAt[0],CosmicDoorScore607);
}
