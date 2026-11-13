#pragma once

void drawTitleHills(const void* pixels,unsigned elapsedMs,unsigned speed,float y,float height) {
    using namespace MenuScroll;
    constexpr float period=640*Scale;
    constexpr float left=HudWidth, right=MenuScroll::ViewWidth;
    setTexture(GU_PSM_4444,512,128,pixels);
    for (float x=left+titleHillPhase(elapsedMs,speed)-period;x<right;x+=period) {
        const float start=std::max(left,x), end=std::min(right,x+period);
        if (start>=end) continue;
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
        vertices[0]={start-x,0,White,ViewX+start,y,0};
        vertices[1]={end-x,height,White,ViewX+end,y+height,0};
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       2,nullptr,vertices);
    }
}

void drawTitleArt(unsigned elapsedMs) {
    using namespace MenuScroll;
    setTexture(GU_PSM_5650,512,512,titleArt);
    hudPatch({0,0,MenuScroll::ViewWidth,MenuScroll::Height,0,0});
    drawTitleHills(titleArt+TitleSkyBytes+TitleLayerBytes,elapsedMs,18,264*Scale,102);
    drawTitleHills(titleArt+TitleSkyBytes+2*TitleLayerBytes,elapsedMs,45,(288-16)*Scale,89);

    const auto* palette=titleArt+TitleFloorOffset-TitlePaletteBytes;
    const unsigned floorFrame=static_cast<uint64_t>(elapsedMs)*30/1000%TitleFloorFrames;
    sceGuClutMode(GU_PSM_8888,0,255,0);
    sceGuClutLoad(32,palette);
    setTexture(GU_PSM_T8,256,64,titleArt+TitleFloorOffset+floorFrame*TitleFloorBytes);
    patchScaled({0,0,250,64,0,0},ViewX+HudWidth,384*Scale,272.0f/250,68.0f/64,White);
    setTexture(GU_PSM_4444,512,128,titleArt+TitleSkyBytes);
    // The lower edge of the capture contains the original tiny version label.
    hudPatch({0,0,272,66,HudWidth,174});
}

// These helpers share the renderer's texture state and are included after hudPatch.
void drawMenuTile(const void* texture,unsigned elapsedMs) {
    using namespace MenuScroll;
    const auto scroll=phase(elapsedMs);
    const float left=ViewX+HudWidth, right=ViewX+MenuScroll::ViewWidth;
    setTexture(GU_PSM_5650,LargeWidth,256,texture);
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(32*sizeof(TextureVertex)));
    unsigned count=0;
    for (float y=-scroll.y;y<MenuScroll::Height;y+=TileSize) {
        const float top=std::max(y,0.0f), bottom=std::min(y+TileSize,static_cast<float>(MenuScroll::Height));
        for (float x=left-scroll.x;x<right;x+=TileSize) {
            const float x0=std::max(x,left), x1=std::min(x+TileSize,right);
            // The packed source has one repeated texel before and after its visible UVs.
            vertices[count++]={1+(x0-x)/Scale,1+(top-y)/Scale,White,x0,top,0};
            vertices[count++]={1+(x1-x)/Scale,1+(bottom-y)/Scale,White,x1,bottom,0};
        }
    }
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                   count,nullptr,vertices);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
}

void drawProfileGradient() {
    using namespace MenuScroll;
    // A single column of 16-byte PSP swizzle blocks needs no byte reordering.
    constexpr int textureWidth=4, textureHeight=128;
    alignas(16) static constexpr auto gradient=[] {
        std::array<uint32_t,textureWidth*textureHeight> pixels{};
        for (int y=0;y<textureHeight;++y) {
            // Match the vertical stops and opacity of the PC profile panel.
            const float fromBottom=1.0f-(y+0.5f)/textureHeight;
            const float t=std::min(fromBottom/0.690141f,1.0f);
            const auto green=static_cast<uint32_t>(204.0f-55.25f*t+0.5f);
            const auto blue=static_cast<uint32_t>(91.8f+163.2f*t+0.5f);
            const uint32_t color=(130u<<24)|(blue<<16)|(green<<8);
            for (int x=0;x<textureWidth;++x) pixels[y*textureWidth+x]=color;
        }
        return pixels;
    }();
    setTexture(GU_PSM_8888,textureWidth,textureHeight,gradient.data());
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    patchScaled({0,0,textureWidth,textureHeight,0,0},ViewX+HudWidth,0,
                static_cast<float>(MenuScroll::ViewWidth-HudWidth)/textureWidth,
                static_cast<float>(MenuScroll::Height)/textureHeight,White);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
}

void drawMenuForegroundRect(const MenuScroll::Textures& art,int x0,int y0,int x1,int y1) {
    if (x0>=x1 || y0>=y1) return;
    using namespace MenuScroll;
    constexpr int split=HudWidth+LargeWidth;
    if (x0<split) {
        setTexture(GU_PSM_8888,LargeWidth,512,art.foreground);
        hudPatch({static_cast<int16_t>(x0-HudWidth),static_cast<int16_t>(y0),
                  static_cast<int16_t>(std::min(x1,split)-x0),static_cast<int16_t>(y1-y0),
                  static_cast<int16_t>(x0),static_cast<int16_t>(y0)});
    }
    if (x1>split) {
        const int start=std::max(x0,split);
        setTexture(GU_PSM_8888,StripWidth,512,art.strip);
        hudPatch({static_cast<int16_t>(start-split),static_cast<int16_t>(y0),
                  static_cast<int16_t>(x1-start),static_cast<int16_t>(y1-y0),
                  static_cast<int16_t>(start),static_cast<int16_t>(y0)});
    }
}

void drawMenuScroll(const MenuScroll::Textures& art,const Patch* selected,unsigned scrollMs,unsigned menuMs) {
    using namespace MenuScroll;
    setTexture(GU_PSM_5650,LargeWidth,512,art.hud);
    hudPatch({0,0,HudWidth,MenuScroll::Height,0,0});
    drawMenuTile(art.tile,scrollMs);
    // Godot's transparent viewport captures supply premultiplied color.
    sceGuBlendFunc(GU_ADD,GU_FIX,GU_ONE_MINUS_SRC_ALPHA,0x00ffffff,0);
    if (selected) {
        const auto& p=*selected;
        const bool back=decodeBack(menuButtonArt.atlas,menuMs,art.back);
        if (back) {
            sceKernelDcacheWritebackRange(art.back,BackBytes);
            drawMenuForegroundRect(art,HudWidth,0,MenuScroll::ViewWidth,BackY);
            drawMenuForegroundRect(art,HudWidth,BackY,BackX,BackY+BackHeight);
            drawMenuForegroundRect(art,BackX+BackWidth,BackY,MenuScroll::ViewWidth,BackY+BackHeight);
            drawMenuForegroundRect(art,HudWidth,BackY+BackHeight,MenuScroll::ViewWidth,p.y);
            setTexture(GU_PSM_8888,BackStride,32,art.back);
            hudPatch({0,0,BackWidth,BackHeight,BackX,BackY});
        } else {
            drawMenuForegroundRect(art,HudWidth,0,MenuScroll::ViewWidth,p.y);
        }
        drawMenuForegroundRect(art,HudWidth,p.y+p.height,MenuScroll::ViewWidth,MenuScroll::Height);
        drawMenuForegroundRect(art,HudWidth,p.y,p.x,p.y+p.height);
        drawMenuForegroundRect(art,p.x+p.width,p.y,MenuScroll::ViewWidth,p.y+p.height);
        setTexture(GU_PSM_8888,ButtonWidth,ButtonRows,menuButtonArt.atlas);
        hudPatch(p);
    } else {
        drawMenuForegroundRect(art,HudWidth,0,MenuScroll::ViewWidth,MenuScroll::Height);
    }
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
}
