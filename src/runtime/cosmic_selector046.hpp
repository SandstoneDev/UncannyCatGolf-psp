#pragma once

LiveResultArt cosmicWorldTitles046;
LiveResultArt::Slot cosmicWorldSlot046;

void drawCosmicWorldTitle046(bool moving,unsigned world,unsigned from,unsigned frame) {
    constexpr unsigned CosmicWorld=LevelSelectState::WorldCount-1;
    if (progress.next<CosmicSecondCourse) return;
    unsigned id=23;
    if (moving) {
        if (world==CosmicWorld && frame) id=std::min(frame,23u);
        else if (from!=CosmicWorld || frame) return;
    } else if (world!=CosmicWorld) return;
    id+=russian()?24:0;
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    // Gameplay uses the same VRAM, so prepare the patch each time the menu is entered.
    constexpr unsigned titleVram=2*1024*1024-8192;
    static_assert(SelectorWorld046VramBytes<=8192);
    if (cosmicWorldTitles046.prepare(id,cosmicWorldSlot046,vram045(titleVram),
                                    SelectorWorld046VramBytes,scratch,ResultScratchBytes))
        drawLiveResultSprite(cosmicWorldSlot046);
    cosmicWorldTitles046.closeStream();
    fallbackPageIndex=ScreenPages;
}
