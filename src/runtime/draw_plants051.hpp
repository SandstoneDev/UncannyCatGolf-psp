#pragma once

struct CosmicPlant {
    Vec2 position;
    float scale;
};

void drawCosmicPlantActors(const GameplayView& view,const LiveResultArt::Slot& slot,
                           const CosmicPlant* plants,unsigned count,float sourceScale) {
    const auto extent=[](unsigned size) {
        unsigned power=1;
        while (power<size) power*=2;
        return power;
    };
    for (unsigned i=0;i<slot.sprite.count;++i) {
        const auto& image=slot.images[i];
        const auto& tile=image.tile;
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(GU_PSM_8888,0,0,GU_TRUE);
        sceGuTexImage(0,extent(tile.width),extent(tile.height),tile.pitch,image.pixels);
        sceGuTexFlush();
        const Patch art{0,0,static_cast<int16_t>(tile.width),static_cast<int16_t>(tile.height),
                        static_cast<int16_t>(slot.sprite.x+tile.x-272),static_cast<int16_t>(slot.sprite.y+tile.y-136)};
        for (unsigned n=0;n<count;++n)
            actor(view,art,plants[n].position,view.worldScale()*plants[n].scale*sourceScale,White,true);
    }
}
