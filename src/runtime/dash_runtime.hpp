#pragma once

#include "dash_sim.hpp"
#include "dash_art.hpp"
#include "pause_navigation060.hpp"

namespace Dash054 {

DashSim::Run game;
Vec2 pointer{512,256};
bool paused=false,missing=false;
bool pointerHeld=false;
int captured=-1;
unsigned pauseSelected=0;
uint32_t pending=0;
uint64_t accumulator=0,animationUs=0;

void event(const char* log,const char* action,unsigned a=0,unsigned b=0) {
    if (auto* file=std::fopen(log,"a")) {
        std::fprintf(file,"event=dash_%s a=%u b=%u\n",action,a,b);
        std::fclose(file);
    }
}

void stopMovement() {
    audio.stop(Sound::DashJump);
    audio.stop(Sound::DashSkip);
}

void stopEffects() {
    stopMovement();
    audio.stop(Sound::DashTime);
    audio.stop(Sound::DashDeath);
    audio.stop(Sound::DashPause);
}

bool enter(const char* executable,const char* log) {
    titleArt=nullptr;
    titleReusedCourseVram=true;
    // Invalidate stage textures whose borrowed eDRAM is used by this scene.
    slotArt.unload();
    ashArt.unload();
    darkHoleArt.unload();
    dogArt.unload();
    factoryDoors.unload();
    factoryBalloons.unload();
    factorySpawner.unload();
    factoryDecor.unload();
    marcySpillArt.unload();
    marcyHudArt.unload();
    marcyTextArt.unload();
    parkBlockArt.unload();
    wizardArt516.unload();
    spellArt516.unload();
    game.reset(sceKernelGetSystemTimeLow()|1u);
    paused=pointerHeld=false;
    captured=-1;
    pointer={512,256};
    pending=accumulator=animationUs=0;
    missing=!DashArt::load(executable);
    if (missing) {
        DashArt::unload();
        event(log,"load_failed");
        return false;
    }
    audio.pauseDash(false);
    audio.musicTune(1,1);
    audio.music(Sound::DashMusic);
    event(log,"open",game.periodUs/1000,game.remaining);
    return true;
}

void leave() {
    stopEffects();
    audio.pauseDash(false);
    audio.musicTune(1,1);
    audio.muteMusic();
    DashArt::unload();
}

void movePointer(Vec2 motion,unsigned ms) {
    const float magnitude=length(motion);
    if (magnitude<=.18f) return;
    const float ratio=(std::min(1.f,magnitude)-.18f)/.82f;
    const auto movement=motion*(400*ratio*std::min(ms,100u)/(1000*magnitude));
    pointer.x=std::clamp(pointer.x+movement.x,0.f,767.f);
    pointer.y=std::clamp(pointer.y+movement.y,0.f,511.f);
}

void signals(const char* log,unsigned oldScore,unsigned oldBest,unsigned oldRound,bool wasInactive) {
    using DashSim::Cue;
    if (game.signaled(Cue::Jump)) audio.stop(Sound::DashSkip);
    if (game.signaled(Cue::Skip)) audio.stop(Sound::DashJump);
    if (game.signaled(Cue::StopDeath)) audio.stop(Sound::DashDeath);
    if (game.signaled(Cue::Jump)) audio.playTuned(Sound::DashJump,-14,1);
    if (game.signaled(Cue::Skip)) audio.playTuned(Sound::DashSkip,-14,1);
    if (game.signaled(Cue::Time)) audio.playTuned(Sound::DashTime,-14,1);
    if (game.signaled(Cue::Death)) {
        audio.play(Sound::DashDeath);
        event(log,"death",game.best,game.shownScore);
    }
    if (wasInactive && !game.inactive) event(log,"restart",game.best,game.position);
    if (oldRound!=game.rounds) event(log,"clear",game.rounds,game.score);
    else if (oldScore!=game.score && !game.inactive) event(log,"move",game.position,game.score);
    if (oldBest!=game.best) event(log,"record",game.best);
}

void step(Flow& flow,Vec2 motion,unsigned held,unsigned pressed,unsigned released,
          uint64_t delta,const char* log) {
    pointerHeld=held&PSP_CTRL_CROSS;
    if (paused) {
        const auto action=PauseNavigation060::update(pauseSelected,captured,
            pressed&PSP_CTRL_UP,pressed&PSP_CTRL_DOWN,pressed&PSP_CTRL_CROSS,
            released&PSP_CTRL_CROSS,pressed&(PSP_CTRL_START|PSP_CTRL_CIRCLE));
        if(action==PauseNavigation060::Action::Resume){
            paused=false;accumulator=pending=0;
            audio.pauseDash(false);audio.musicTune(1,1);event(log,"resume",game.score);
        }else if(action==PauseNavigation060::Action::Quit){
            flow.screen=Screen::Villa;event(log,"exit",game.score,game.best);
        }
        return;
    }
    movePointer(motion,static_cast<unsigned>(delta/1000));
    if (pressed&PSP_CTRL_START) {
        paused=true;
        pauseSelected=0;
        captured=-1;
        accumulator=pending=0;
        audio.pauseDash(true);
        audio.musicTune(0,0);
        audio.play(Sound::DashPause);
        event(log,"pause",game.score,game.best);
        return;
    }
    animationUs+=std::min<uint64_t>(delta,100000);
    if (pressed&PSP_CTRL_CROSS) pending|=DashSim::Primary;
    if (pressed&PSP_CTRL_SQUARE) pending|=DashSim::Secondary;
    accumulator+=std::min<uint64_t>(delta,100000)*60;
    for (;accumulator>=1000000;accumulator-=1000000) {
        const unsigned oldScore=game.score,oldBest=game.best,oldRound=game.rounds;
        const bool wasInactive=game.inactive;
        game.step(pending,16667);
        pending=0;
        signals(log,oldScore,oldBest,oldRound,wasInactive);
    }
}

constexpr float Scale=17.f/32;
void block(float x,float y,float width,float height,uint32_t color) {
    rectangle(ViewX+x*Scale,y*Scale,width*Scale,height*Scale,color);
}

void drawGame() {
    DashArt::drawBackdrop(static_cast<unsigned>(animationUs*60/1000000));
    DashArt::drawHud();
    DashArt::fieldView();
    constexpr float x=256,y=256,zoom=2;
    block(x,y,64*zoom,zoom,0xff000000u);
    block(x,y,32*zoom,8*zoom,0xff0000ffu);
    block(x,y,(game.remaining-(game.initialBar?1:0))*zoom,8*zoom,0xff00ff00u);
    block(x+60*zoom,y,4*zoom,zoom,0xff404040u);
    // Preserve the authored TileMapLayer, including its fixed ocean tail.
    for (unsigned i=0;i<60;++i) {
        const bool lane=i<DashSim::Size;
        DashArt::tile(lane?game.board[i]:false,lane?i%2:0,x+(i+4)*8*zoom,y,zoom,zoom);
    }
    if (game.scoreVisible) {
        char shown[16],record[16];
        std::snprintf(shown,sizeof shown,"%04u",game.shownScore);
        std::snprintf(record,sizeof record,"%04u",game.best);
        for (unsigned i=0;i<4;++i) {
            // The original numbers remain at these TileMap cell coordinates,
            // beyond the camera's right edge.
            DashArt::digit(shown[i]-'0',x+(60+i)*8*zoom,y,zoom);
            DashArt::digit(record[i]-'0',x+(192+(4+i)*8)*zoom,y,zoom);
        }
    }
    DashArt::cat(x+(game.position+4)*8*zoom,y,zoom,zoom,0xffff722bu);
    DashArt::cat(x,y,4*zoom,zoom,0xff0000ffu);
    DashArt::cat(x,y,game.remaining*zoom,zoom,0xff00ff00u);
    if (game.inactive) DashArt::jumpscare();
}

void draw() {
    beginFrame();
    drawGame();
    if (paused) DashArt::pause(russian(),pauseSelected,pointerHeld&&captured==static_cast<int>(pauseSelected)?captured:-1);
    else DashArt::cursor(pointer.x,pointer.y);
    endFrame();
}

} // namespace Dash054
