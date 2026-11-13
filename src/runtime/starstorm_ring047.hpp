#pragma once

void drawStarstormRing047(const GameplayView& view,const Round& round) {
    unsigned generation=0;
    auto* workspace=clips.textureWorkspace<ParkArt::Side*ParkArt::Side*4>(generation);
    const auto* pixels=parkArt.ringTexture(workspace,generation);
    if (!pixels) return;
    drawSmileyRing(view,CosmicRings605[0],round.smiley,round.ticks,
                   starstormTint047(CosmicRingTint605),starstormTint047(CosmicRingCoinTint605),pixels);
}
