CostumeScreen056::Bank screenBank056;
std::string screenPath056;
ArleRandomizer056::State arleRandomizer056;
uint32_t arleRandom056=0;
unsigned costumeSoundForPlayback056(unsigned sprite){
    if(sprite!=23)return ucg::editor_audio::costumeSound(sprite);
    if(!arleRandom056)arleRandom056=static_cast<uint32_t>(sceKernelGetSystemTimeWide())|1u;
    return arleRandomizer056.select([](unsigned limit){
        const unsigned threshold=(0u-limit)%limit;
        do{arleRandom056^=arleRandom056<<13;arleRandom056^=arleRandom056>>17;arleRandom056^=arleRandom056<<5;}while(arleRandom056<threshold);
        return arleRandom056%limit;
    });
}

VillaCostumes::Pair screenPair056(){return selected==Id::Copycat?ArtistCopycat::costume():VillaCostumes::actorPair(selected);}
void prepareScreenActors056(){
    CostumeShader056::phase=(sceKernelGetSystemTimeWide()%2000000)/2000000.f;
    if(selected==Id::Canny)return;
    const auto pair=screenPair056();
    if(screenBank056.attempted && screenBank056.pair[0]==pair.player && screenBank056.pair[1]==pair.ghost)return;
    fallbackPageIndex=ScreenPages;horseMeter039.invalidateShared();
    screenBank056.load(screenPath056,pair.player,pair.ghost,fallbackPage,sizeof fallbackPage);
}
bool screenActorReady056(){
    const auto pair=screenPair056();
    return selected!=Id::Canny && screenBank056.ready && screenBank056.pair[0]==pair.player && screenBank056.pair[1]==pair.ghost;
}
bool drawScreenActor056(bool ghost,float x,float y,float scale,uint32_t tint=White,bool flip=false){
    if(!screenActorReady056())return false;
    const unsigned index=ghost?1:0,w=screenBank056.width[index],h=screenBank056.height[index];
    // scale is in the existing17px actor's units; the source canvas is64/512.
    const float raster=scale*17.f/64;
    setTexture(GU_PSM_8888,128,128,screenBank056.pixels[index]);
    if(w && h){
        const float l=x-w*raster*.5f,t=y-h*raster*.5f,r=l+w*raster,b=t+h*raster;
        if(selected==Id::Rainbow){
            TextureVertex quad[4]={{float(flip?w:0),0,tint,l,t,0},{float(flip?0:w),0,tint,r,t,0},{float(flip?0:w),float(h),tint,r,b,0},{float(flip?w:0),float(h),tint,l,b,0}};
            CostumeShader056::drawQuad(quad);
        }else patch({0,0,static_cast<int16_t>(w),static_cast<int16_t>(h),0,0},l,t,raster,tint,flip);
    }
    if(!ghost && selected==Id::Copycat && ArtistCopycat::drawn){
        ArtistInk::bind(ArtistCopycat::pixels.data());
        const float ink=scale*17.f/128;
        patch({0,0,128,128,0,0},x-64*ink,y-64*ink,ink,tint,flip);
    }
    return true;
}
void drawRainbowActor056(const GameplayView& view,const Patch& image,Vec2 world,float scale,uint32_t tint,float angle){
    const auto center=view.position(world);const float cs=std::cos(angle),sn=std::sin(angle);
    const float xs[]={float(image.x),float(image.x+image.width),float(image.x+image.width),float(image.x)};
    const float ys[]={float(image.y),float(image.y),float(image.y+image.height),float(image.y+image.height)};
    const float us[]={float(image.u),float(image.u+image.width),float(image.u+image.width),float(image.u)};
    const float vs[]={float(image.v),float(image.v),float(image.v+image.height),float(image.v+image.height)};
    TextureVertex quad[4];
    for(unsigned i=0;i<4;++i)quad[i]={us[i],vs[i],tint,center.x+(xs[i]*cs-ys[i]*sn)*scale,center.y+(xs[i]*sn+ys[i]*cs)*scale,0};
    CostumeShader056::drawQuad(quad);
}
