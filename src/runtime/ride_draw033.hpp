#pragma once

#include "coaster_bend033.hpp"

void drawRideTrack033(const CoasterRun033& ride) {
    clips.useRideWorkspace(rideArt);
    constexpr float Left=ViewX+136, Width=272, Top=122.4f, Zoom=0.425f;
    if (const auto* background=rideArt.background(ride.exitPath(),ride.pathProgress)) {
        setTexture(GU_PSM_5650,RideArt033::Width,RideArt033::BackgroundHeight,background);
        patchScaled({0,0,272,122,0,0},Left,0,1.0f,Top/122,White);
    }
    const auto* track=rideArt.track(ride.scrollTiles());
    if (!track) return;
    setTexture(GU_PSM_5650,RideArt033::Width,RideArt033::TrackHeight,track);
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(3*150*2*sizeof(TextureVertex)));
    unsigned count=0;
    for (unsigned row=0;row<150;++row) {
        const float stretch=1.0f+0.25f*CoasterBend033[row];
        float u=std::fmod((512-320*stretch-ride.trackX)*Zoom,Width);
        if (u<0) u+=Width;
        const float y=Top+row*(Height-Top)/150;
        float x=0;
        while (x<Width-0.001f) {
            const float right=std::min(Width,x+(Width-u)/stretch);
            vertices[count++]={u,float(row),White,Left+x,y,0};
            vertices[count++]={std::min(Width,u+(right-x)*stretch),float(row+1),White,
                               Left+right,y+(Height-Top)/150,0};
            u=0;
            x=right;
        }
    }
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                   count,nullptr,vertices);
}

template<class Atlas>
void setRideTexture033(const Atlas& atlas) {
    setTexture(GU_PSM_T8,Atlas::TextureWidth,Atlas::TextureHeight,atlas.texture());
    sceGuClutMode(GU_PSM_8888,0,0xff,0);
    sceGuClutLoad(32,atlas.clut());
}

void drawRideScenery033(const GameplayView& view,const CoasterRun033& ride) {
    if (!rideScenery.texture()) return;
    setRideTexture033(rideScenery);
    const float positions[]={ride.startWallX,ride.startX,ride.safetySignX};
    for (unsigned i=0;i<3;++i) {
        const auto& art=rideScenery.patch(i);
        actor(view,art,{positions[i],0});
    }
    actor(view,rideScenery.patch(4),{ride.startX,0});
}

void drawRideObjects033(const GameplayView& view,const Round& round,bool foreground) {
    const auto& ride=round.ride;
    if (!rideSprites.texture()) return;
    setRideTexture033(rideSprites);
    if (foreground) {
        if (ride.cartClosed()) actor(view,rideSprites.patch(1),{});
        if (ride.stairsFront()) actor(view,rideSprites.patch(28),{ride.endX,0});
        actor(view,rideSprites.patch(2),{});
        const unsigned remaining=ride.remainingCoins();
        if (remaining>=10) actor(view,rideSprites.patch(3+remaining/10),{-127,148});
        actor(view,rideSprites.patch(3+remaining%10),{-115,148});
        if (ride.cloudAlpha>0) {
            setRideTexture033(rideScenery);
            const auto& clouds=rideScenery.patch(3);
            actor(view,clouds,{},1.0f,GU_RGBA(255,255,255,unsigned(ride.cloudAlpha*255)));
        }
        if (ride.skipping) localeText(lang("FAST FORWARD","ПЕРЕМОТКА"),272,12,0.8f,White,true);
        return;
    }
    actor(view,rideSprites.patch(0),{});
    actor(view,rideSprites.patch(27),{ride.endX,0});
    if (!ride.stairsFront()) actor(view,rideSprites.patch(28),{ride.endX,0});
    actor(view,rideSprites.patch(29),{ride.endX,0});
    for (unsigned lane=0;lane<4;++lane) {
        const float y=CoasterRun033::laneY(lane)+8;
        if (ride.coinWarnings[lane]>0)
            actor(view,rideSprites.patch(13+round.ticks/6%2),{-8,y},ride.coinWarnings[lane]);
        if (ride.ghostWarnings[lane]>0)
            actor(view,rideSprites.patch(15+round.ticks/6%2),{24,y},ride.ghostWarnings[lane]);
    }
    for (const auto& object:ride.objects) {
        if (object.kind==CoasterObjectKind033::Coin && !object.collected && object.x<=CoasterRun033::ScreenEdge) {
            actor(view,rideSprites.patch(17+object.ticks/4%10),
                  {object.x,CoasterRun033::laneY(object.lane)},1,object.tint);
        }
    }
    setTexture(GU_PSM_8888,ActorsWidth,ActorsHeight,actors);
    for (const auto& object:ride.objects) {
        if (object.kind==CoasterObjectKind033::Ghost && object.x<=CoasterRun033::ScreenEdge)
            actor(view,visual.ghost,{object.x,CoasterRun033::laneY(object.lane)},1,White,true);
    }
    for (const auto& object:ride.objects) {
        if (!object.popupScore || object.popupAge<=0) continue;
        char value[12];
        std::snprintf(value,sizeof value,"+%u",object.popupScore);
        const auto position=view.position({object.x,CoasterRun033::laneY(object.lane)-60*object.popupAge});
        const auto alpha=uint32_t(255*std::clamp(1.0f-object.popupAge*60/50,0.0f,1.0f));
        if (position.x>=ViewX+136 && position.x<ViewX+408)
            localeText(value,position.x-ViewX,position.y,0.5f,(object.tint&0xffffffu)|(alpha<<24),true);
    }
}

void drawSnow033(unsigned ticks,float snowSeconds=-1.0f,unsigned firstKind=0) {
    if (!bindFrontierCompressed037<decltype(snowArt),248*1024>(snowArt)) return;
    const auto phase=[](unsigned key) {
        return static_cast<float>(coaster033::hashInteger(key)&0xffffu)/65536.0f;
    };
    constexpr float Scale=17.0f/32;
    const float time=1.0f+(snowSeconds>=0?snowSeconds:ticks/60.0f);
    for (unsigned kind=firstKind;kind<2;++kind) {
        const bool mist=kind==1;
        const unsigned particles=mist?200:1000;
        const auto& art=snowArt.patch(kind);
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*particles*sizeof(TextureVertex)));
        unsigned count=0;
        for (unsigned i=0;i<particles;++i) {
            const unsigned key=kind*1000+i;
            const float lifetime=5*(1-0.62f*phase(key*7));
            const float age=std::fmod(time*(mist?0.3f:0.6f)+lifetime*phase(key*7+1),lifetime);
            const float t=age/lifetime;
            const float angle=0.1974f+(2*phase(key*7+2)-1)*0.4095f;
            const float speed=434.34f+282.83f*phase(key*7+3);
            const float x=ViewX+Scale*(992-age*speed*std::cos(angle));
            const float y=Scale*(40+512*phase(key*7+4)-age*speed*std::sin(angle));
            const float size=art.width*Scale*(2-t)*(mist?1+phase(key*7+5):0.1f+0.1f*phase(key*7+5));
            if (x+size/2<ViewX || x-size/2>ViewX+ViewWidth || y+size/2<0 || y-size/2>Height) continue;
            const unsigned alpha=unsigned((mist?25:255)*(1-0.75f*t));
            const uint32_t tint=GU_RGBA(255,255,255,alpha);
            vertices[count++]={float(art.u),float(art.v),tint,x-size/2,y-size/2,0};
            vertices[count++]={float(art.u+art.width),float(art.v+art.height),tint,x+size/2,y+size/2,0};
        }
        if (count) sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                                 count,nullptr,vertices);
    }
}
