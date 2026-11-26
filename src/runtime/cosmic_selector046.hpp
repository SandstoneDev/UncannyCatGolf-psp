#pragma once

LiveResultArt cosmicWorldTitles046;
LiveResultArt::Slot cosmicWorldSlot046;
alignas(64) uint8_t cosmicWorldTitlePixels046[SelectorWorld046VramBytes]{};

bool cacheCosmicWorldTitle046(bool russian) {
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    fallbackPageIndex=ScreenPages;
    return cosmicWorldTitles046.prepare(russian?47:23,cosmicWorldSlot046,cosmicWorldTitlePixels046,
                                      sizeof cosmicWorldTitlePixels046,scratch,ResultScratchBytes,true);
}

void drawCosmicWorldTitle046(unsigned world,unsigned from,unsigned alpha) {
    constexpr unsigned CosmicWorld=LevelSelectState::WorldCount-1;
    if (progress.next<CosmicSecondCourse || (world!=CosmicWorld && from!=CosmicWorld) ||
        !cosmicWorldSlot046.owner) return;
    if (world!=CosmicWorld) alpha=255-alpha;
    drawLiveResultSprite(cosmicWorldSlot046,0,nullptr,1,GU_RGBA(255,255,255,alpha));
}
