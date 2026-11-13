#pragma once

void drawFrontierShade0607(const GameplayView& view) {
    constexpr Vec2 maze[]{{-512,-512},{504,-512},{512,512},{-512,512}};
    constexpr Vec2 production[]{{-384,-384},{384,-384},{384,384},{-384,384}};
    const auto& polygon=currentCourse==FrontierMazeCourse?maze:production;
    auto* vertices=static_cast<ColorVertex*>(sceGuGetMemory(sizeof(ColorVertex)*4));
    for (unsigned i=0;i<4;++i) {
        const auto p=view.position(polygon[i]);
        vertices[i]={GU_RGBA(115,115,115,100),p.x,p.y,0};
    }
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDrawArray(GU_TRIANGLE_FAN,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,vertices);
    sceGuEnable(GU_TEXTURE_2D);
}

constexpr unsigned FrontierDogArtBytes=decltype(frontierDogArt)::TextureWidth*decltype(frontierDogArt)::TextureHeight;
constexpr unsigned FrontierDogTileBytes=decltype(frontierDogTiles)::TextureWidth*decltype(frontierDogTiles)::TextureHeight;
constexpr unsigned FrontierNukeItemOffset=FrontierDogArtBytes+FrontierDogTileBytes;

bool bindFrontierDogArt() {
    unsigned generation=0;
    auto* workspace=clips.compressedTextureWorkspace<FrontierDogArtBytes>(generation);
    return frontierDogArt.useShared(workspace,generation);
}

void drawProductionDog(const GameplayView& view,const Round& round,const DogGolfScene& scene) {
    if (!bindFrontierDogArt()) return;
    if (currentCourse==FrontierDogCourse && !round.dog.winner) {
        unsigned generation=0;
        auto* workspace=clips.compressedTextureWorkspace<FrontierNukeItemOffset>(generation)+FrontierDogArtBytes;
        if (!frontierDogTiles.useShared(workspace,generation)) return;
        setFrontierTexture(frontierDogTiles);
        for (unsigned i=0;i<FrontierDogTileCount035;++i) {
            const auto& art=frontierDogTiles.patch(i);
            patch(art,ViewX+GameplayView::HudWidth+art.x,art.y);
        }
    }
    setFrontierTexture(frontierDogArt);
    sharedActor(view,frontierDogArt.patch(round.ticks/6%11),scene.goal);
    const auto& sprite=frontierDogArt.patch(11);
    const float zoom=view.worldZoom()*round.dog.graphicScale/scene.graphicScale;
    const auto center=view.position(round.dog.body.center);
    patch(sprite,center.x+sprite.x*zoom,center.y+sprite.y*zoom,zoom);
}

struct FrontierNukeView { const Vec2* centers; unsigned count; };
FrontierNukeView frontierNukeItems() {
    if (currentCourse==CosmicSeventeenthCourse) return {CosmicNukes617,std::size(CosmicNukes617)};
    if (currentCourse==CosmicFourteenthCourse) return {CosmicNukes614,std::size(CosmicNukes614)};
    if (currentCourse==CosmicTenthCourse) return {CosmicNukes610,std::size(CosmicNukes610)};
    if (currentCourse==Elysian514Course) return {ElysianNukes514,std::size(ElysianNukes514)};
    if (currentCourse==CrimsonOutpostCourse) return {FrontierNukes416,std::size(FrontierNukes416)};
    if (currentCourse==FrontierDogCourse) return {FrontierNukes407,std::size(FrontierNukes407)};
    if (currentCourse==PlentifulChucklesCourse) return {FrontierNukes408,std::size(FrontierNukes408)};
    if (currentCourse==UncannyValleyCourse) return {FrontierNukes409,std::size(FrontierNukes409)};
    return {};
}

void drawFrontierValleyLight(const GameplayView& view,const Round& round) {
    rectangle(ViewX+GameplayView::HudWidth,0,ViewWidth-GameplayView::HudWidth,Height,GU_RGBA(0,0,0,127));
    if (!bindFrontier409Night()) return;
    sceGuBlendFunc(GU_ADD,GU_DST_COLOR,GU_FIX,0,0xffffffffu);
    actor(view,frontier409Night.patch(ValleyLightPatch035),round.game.position,
          view.worldScale()*ValleyLightScale035);
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
}

void drawFrontierNukeItem(const GameplayView& view,const Round& round) {
    const unsigned frameTick=(currentCourse==Elysian514Course?round.horse.age:round.ticks)%50;
    const unsigned frame=frameTick<30?0:1+(frameTick-30)/2;
    constexpr unsigned size=decltype(frontierNukeArt)::TextureWidth*decltype(frontierNukeArt)::TextureHeight;
    unsigned generation=0;
    auto* workspace=clips.compressedTextureWorkspace<FrontierNukeItemOffset+size>(generation)+FrontierNukeItemOffset;
    if (!frontierNukeArt.useShared(workspace,generation)) return;
    setFrontierTexture(frontierNukeArt);
    const auto items=frontierNukeItems();
    for (unsigned i=0;i<items.count;++i)
        if (!round.nukes.has(i)) sharedActor(view,frontierNukeArt.patch(frame),items.centers[i],currentCourse==CosmicTenthCourse?CosmicShade610:elysianHazardTint040(currentCourse));
}

void drawFrontierNukeExplosions(const Round& round) {
    constexpr Vec2 centers[]{{352,96},{672,96},{512,256},{352,416},{672,416}};
    constexpr unsigned frames[]{0,1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
    constexpr float fit=17.0f/32.0f, scale=136.0f/64.0f;
    bool bound=false;
    for (unsigned item=0;item<frontierNukeItems().count;++item) {
        const unsigned picked=round.nukes.pickedAt[item];
        if (!picked) continue;
        const unsigned age=(currentCourse==Elysian514Course?round.horse.age:round.ticks)-picked;
        for (unsigned i=0;i<std::size(centers);++i) {
            const unsigned delay=3*i;
            if (age<delay) continue;
            const unsigned frame=(age-delay)*25/60;
            if (frame>=std::size(frames)) continue;
            if (!bound) {
                unsigned generation=0;
                // Keep 5_14's Chuckle texture alive through the queued draw calls.
                constexpr unsigned explosionOffset=HorseObjects039Width*HorseObjects039Height+
                    FrontierClouds501Width*FrontierClouds501Height+ChuckleArt::TextureBytes;
                auto* workspace=clips.textureWorkspace<explosionOffset+256*256>(generation)+
                                (currentCourse==Elysian514Course?explosionOffset:ChuckleArt::TextureBytes);
                if (!frontierNukeExplosions.useShared(workspace,generation)) return;
                setFrontierTexture(frontierNukeExplosions);
                bound=true;
            }
            uint32_t bits=(picked+item*257u+i*37u)*0x9e3779b9u;
            bits^=bits>>13;
            const Vec2 jitter{static_cast<float>(bits%193)-96,
                              static_cast<float>((bits>>16)%193)-96};
            const auto center=(centers[i]+jitter)*fit;
            const auto& art=frontierNukeExplosions.patch(frames[frame]);
            patch(art,ViewX+center.x+art.x*scale,center.y+art.y*scale,scale,GU_RGBA(255,255,255,90));
        }
    }
}
