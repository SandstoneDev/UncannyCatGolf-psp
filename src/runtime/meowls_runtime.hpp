#pragma once

#include "meowls_sim.hpp"
#include "meowls_art.hpp"
#include "meowls_presentation.hpp"
#include "minigame_pause_art.hpp"

namespace Meowls {

using namespace MeowlsSim;
namespace Art=MeowlsArt;
using Art::Graphic;
constexpr float Scale=17.f/32;
constexpr unsigned Backplate=0x1a0000;
static_assert(Backplate+Phone054::PlateBytes<=2*1024*1024);
static_assert(Art::SheetOffsets.back()<=MinigamePauseArt::Address);

MeowlsSim::Game game;
uint64_t clock=0;
Vec2 cursor{660,320};
bool paused=false,quitSelected=false,missing=false,audioStarted=false;
unsigned regionLoaded=UINT32_MAX,pending=0;
unsigned presentationMs=0;
bool showingFilm=false,filmNarration=false,filmCameo=false,filmSidebar=false;
uint32_t audioRandom=1;

void log(const char* path,const char* action,unsigned a=0,unsigned b=0) {
    if (auto* record=std::fopen(path,"a")) {
        std::fprintf(record,"event=meowls_%s a=%u b=%u\n",action,a,b);
        std::fclose(record);
    }
}

constexpr Sound Effects[]={
    Sound::MeowlsFlap,Sound::MeowlsThrow,Sound::MeowlsBreak,Sound::MeowlsHurt,Sound::MeowlsDeath,
    Sound::MeowlsSlurp,Sound::MeowlsMini,Sound::MeowlsWarning,Sound::MeowlsRotate,Sound::MeowlsEggBurst,
    Sound::MeowlsBite,Sound::MeowlsGator,Sound::MeowlsLocation,Sound::MeowlsWin,Sound::ClubGreenheadBoom,
    Sound::ClubGreenheadBoom,Sound::MeowlsBreak,
};
constexpr float Gains[]={0,0,-3,0,10,9,-5,0,0,-10,4,2,6,-1,-10,-10,-2.5f};
constexpr float Jitter[]={1.2f,1.25f,1.4f,1,1,1,1.5f,1,1,1,1,1,1,1,1,1,1.4f};
static_assert(std::size(Effects)==static_cast<unsigned>(Cue::StopRotate) &&
              std::size(Gains)==std::size(Effects) && std::size(Jitter)==std::size(Effects));

float tune(float ratio) {
    audioRandom^=audioRandom<<13;
    audioRandom^=audioRandom>>17;
    audioRandom^=audioRandom<<5;
    return 1.f/ratio+(ratio-1.f/ratio)*(audioRandom%65536)/65535.f;
}

void stopEffects() {
    for (const auto cue:Effects) audio.stop(cue);
}

float tempo() { return std::min(4.f,1.f+game.flightTicks*.000005f); }

void advanceAudio() {
    if (game.signaled(Cue::StopRotate) &&
        std::none_of(game.actors.begin(),game.actors.end(),[](const auto& a) {
            return a.live && a.kind==Kind::Egg && a.age<90;
        })) audio.stop(Sound::MeowlsRotate);
    if (game.signaled(Cue::StopWarning)) audio.stop(Sound::MeowlsWarning);
    for (unsigned i=0;i<std::size(Effects);++i)
        if (game.signaled(static_cast<Cue>(i))) {
            const auto cue=static_cast<Cue>(i);
            const float multiplier=cue==Cue::BabyBoom?1.5f:cue==Cue::Shatter?1.6f:1.f;
            audio.playTuned(Effects[i],Gains[i],tune(Jitter[i])*multiplier);
        }
    if (audioStarted) audio.musicTune(1,tempo());
}

bool loadRegion(const char* diagnostics) {
    if (regionLoaded==game.region()) return true;
    if (Art::world(game.region())) { regionLoaded=game.region(); return true; }
    log(diagnostics,"region_error",game.region());
    return false;
}

bool enter(const char* executable,const char* diagnostics) {
    clock=0;
    pending=0;
    cursor={660,320};
    paused=quitSelected=audioStarted=false;
    audioRandom=sceKernelGetSystemTimeLow()|1u;
    game.reset(audioRandom);
    regionLoaded=UINT32_MAX;
    titleArt=nullptr;
    titleReusedCourseVram=true;
    // Entry reuses Villa/film VRAM only after the previous GE frame has finished.
    sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
    missing=!Art::load(executable) || !MinigamePauseArt::load() || !loadRegion(diagnostics);
    if (missing) {
        Art::unload();
        log(diagnostics,"load_failed");
        return false;
    }
    audio.muteMusic();
    audio.musicTune(1,1);
    log(diagnostics,"open");
    return true;
}

void leave() {
    stopEffects();
    audio.muteMusic();
    audio.musicTune(1,1);
    Art::unload();
}

void resume() {
    paused=false;
    audio.musicTune(1,tempo());
    if (std::any_of(game.actors.begin(),game.actors.end(),[](const auto& a) {
        return a.live && a.kind==Kind::Egg && a.age<90;
    })) audio.play(Sound::MeowlsRotate);
}

void step(Flow& flow,Vec2 stick,unsigned pressed,uint64_t us,const char* diagnostics) {
    if (pressed&PSP_CTRL_START) {
        if (paused) resume();
        else {
            paused=true;
            quitSelected=false;
            audio.musicTune(0,0);
            audio.stop(Sound::MeowlsRotate);
            audio.play(Sound::Pause);
        }
        log(diagnostics,paused?"pause":"resume",game.score);
        return;
    }
    if (paused) {
        if (pressed&(PSP_CTRL_UP|PSP_CTRL_DOWN)) quitSelected=!quitSelected;
        if (pressed&PSP_CTRL_CIRCLE) {
            resume();
            log(diagnostics,"resume",game.score);
        } else if (pressed&PSP_CTRL_CROSS) {
            if (quitSelected) {
                flow.screen=Screen::Villa;
                log(diagnostics,"exit",game.score);
            } else {
                resume();
                log(diagnostics,"resume",game.score);
            }
        }
        return;
    }
    if (pressed&PSP_CTRL_CIRCLE) {
        paused=true;
        quitSelected=true;
        audio.musicTune(0,0);
        audio.stop(Sound::MeowlsRotate);
        audio.play(Sound::Pause);
        log(diagnostics,"pause",game.score);
        return;
    }
    const float move=length(stick);
    if (move>.14f) {
        const float scale=std::min(1.f,move);
        cursor=cursor+stick*((48+340*scale*scale)*std::min<uint64_t>(us,50000)/1000000.f/move);
        cursor.x=std::clamp(cursor.x,256.f,767.f);
        cursor.y=std::clamp(cursor.y,0.f,511.f);
    }
    // Hold edge events until a physics tick consumes them.

    pending|=pressed&(PSP_CTRL_CROSS|PSP_CTRL_SQUARE);
    clock+=std::min<uint64_t>(us,100000)*60;
    for (;clock>=1000000;clock-=1000000) {
        const auto before=game.phase;
        const unsigned oldDifficulty=game.difficulty,oldShots=game.shots;
        game.step(pending&PSP_CTRL_CROSS,pending&PSP_CTRL_SQUARE,cursor);
        pending=0;
        if (before==Phase::Ready && game.phase==Phase::Playing) {
            audioStarted=true;
            audio.musicTune(1,1);
            audio.music(Sound::MeowlsMusic);
            log(diagnostics,"flight",game.score);
        }
        if (game.shots!=oldShots && game.phase!=Phase::Ready)
            log(diagnostics,"shot",game.shots,game.score);
        if (oldDifficulty!=game.difficulty && game.phase!=Phase::Ready) {
            missing=!loadRegion(diagnostics);
            log(diagnostics,"region",game.difficulty,game.score);
        }
        if (game.phase==Phase::Death && before!=Phase::Death) {
            stopEffects();
            audio.muteMusic();
            audioStarted=false;
            log(diagnostics,"death",game.score);
        } else if (game.phase==Phase::Ready && before==Phase::Death) {
            missing=!loadRegion(diagnostics);
            cursor={660,320};
            log(diagnostics,"retry");
        } else {
            advanceAudio();
        }
    }
}

void text(const char* msg,float x,float y,float scale=.78f,uint32_t color=White,unsigned width=242) {
    localeText(msg,x*Scale+1,y*Scale+1,scale,GU_RGBA(13,11,23,255),true,width,true);
    localeText(msg,x*Scale,y*Scale,scale,color,true,width,true);
}

void actorSprite(const Actor& a) {
    if (a.death) {
        if (a.death<=24) {
            const unsigned frame=(a.death-1)/2;
            if (a.kind==Kind::Head)
                Art::spriteScaled(Graphic::Burst,frame,a.pos.x+36,a.pos.y-1,0,1.46479f,.98f);
            else
                Art::sprite(Graphic::Burst,frame,a.pos.x,a.pos.y,0,a.kind==Kind::Baby?1.5f:3.f);
        }
        return;
    }
    switch (a.kind) {
    case Kind::Caholic: Art::sprite(Graphic::Caholic,Art::flyFrame(a.age),a.pos.x,a.pos.y); break;
    case Kind::Officer: Art::sprite(Graphic::Officer,Art::flyFrame(a.age),a.pos.x,a.pos.y); break;
    case Kind::Head:
        if (a.age<=120) Art::sprite(Graphic::ThheadWarning,a.age/6,a.pos.x-32,a.pos.y);
        Art::sprite(Graphic::Thhead,Art::flyFrame(a.age),a.pos.x+40,a.pos.y,-1.57079633f);
        break;
    case Kind::Egg:
        if (a.age<96) Art::sprite(Graphic::Egg,a.age/2,a.pos.x,a.pos.y);
        if (a.age>=90) Art::spriteScaled(Graphic::Burst,std::min(16u,(a.age-90)*5/6),a.pos.x,a.pos.y,0,2.47887f,1.84f);
        break;
    case Kind::Gator: Art::gator(a.pos.x,a.pos.y,std::min(1.f,a.timer/24.f),a.angle); break;
    case Kind::Baby: Art::sprite(Graphic::Meowling,Art::flyFrame(a.age),a.pos.x,a.pos.y); break;
    case Kind::Bottle: Art::sprite(Graphic::Bottle,0,a.pos.x,a.pos.y,a.angle); break;
    case Kind::Pellet: Art::sprite(Graphic::OfficerShot,0,a.pos.x,a.pos.y,a.angle); break;
    case Kind::Life: Art::sprite(Graphic::Blood,a.age/3,a.pos.x,a.pos.y); break;
    }
}

void trails() {
    for (const auto& image:game.trails) {
        if (!image.life) continue;
        const unsigned alpha=(image.kind==Kind::Head?178u:102u)*image.life/image.maxLife;
        if (image.kind==Kind::Head)
            Art::sprite(Graphic::Thhead,image.frame,image.pos.x+40,image.pos.y,-1.57079633f,image.scale,
                        GU_RGBA(255,255,255,alpha));
        else if (image.kind==Kind::Bottle || image.kind==Kind::Pellet)
            Art::sprite(image.kind==Kind::Bottle?(image.frame?Graphic::Burst:Graphic::Bottle):Graphic::OfficerShot,
                        image.frame,image.pos.x,image.pos.y,image.angle,1,GU_RGBA(255,255,255,alpha));
        else {
            Art::sprite(Graphic::Feather,image.frame,image.pos.x,image.pos.y,image.angle,image.scale,
                        GU_RGBA(255,255,255,alpha));
        }
    }
}

unsigned phoneAlpha(unsigned t) {
    constexpr unsigned cuts[]={30,42,72,84,114,126};
    if (t>=486) return 0;
    if (t>=306) return 255*(486-t)/180;
    return (std::upper_bound(std::begin(cuts),std::end(cuts),t)-cuts)%2?0:255;
}

void playerSprite() {
    const auto& p=game.player;
    if (game.phase==Phase::Death) return;
    const bool ready=game.phase==Phase::Ready;
    const unsigned frame=ready?0:Art::flyFrame(game.flightTicks);
    if (!ready && game.phase!=Phase::Exploding) {
        const Vec2 ray=velocity(cursor-p,400);
        Art::aim(p.x,p.y,p.x+ray.x,p.y+ray.y);
        const float x=ViewX+cursor.x*Scale,y=cursor.y*Scale;
        rectangle(x-4,y,9,1,GU_RGBA(255,247,209,166));
        rectangle(x,y-4,1,9,GU_RGBA(255,247,209,166));
    }
    const unsigned red=255,rest=static_cast<unsigned>(255-130*game.hitTint);
    Art::sprite(Graphic::Player,frame,p.x,p.y,game.tilt,1,GU_RGBA(red,rest,rest,255));
    if (game.phase==Phase::Exploding) Art::sprite(Graphic::Burst,std::min(11u,game.explodeTicks/2),p.x+13.3333f,p.y,0,3.f);
}

float regionPosition(unsigned ticks) {
    constexpr float knots[]={512,192,224,-128};
    const unsigned segment=std::min(2u,ticks/36);
    return knots[segment]+(knots[segment+1]-knots[segment])*(ticks%36)/36.f;
}

void draw() {
    beginFrame();
    sceGuScissor(ViewX+136,0,ViewX+408,272);
    const bool death=game.phase==Phase::Death;
    if (death) {
        Art::death(game.deathAlpha);
        text(lang("× Retry","× Повторить"),512,447,.88f,White);
    } else {
        Art::background(game.scroll(),game.flash*255/100);
        if (game.flightTicks<IntroTicks) {
            const float shift=game.introScroll();
            Art::title(shift);
            Art::introTree(shift);
            text(lang("×  Flap   □  Throw","×  Взмах   □  Бросок"),585-shift,134,1.f,White);
            text(lang("Aim with the stick or D-pad","Целься стиком или крестовиной"),585-shift,161,.8f,White);
            text(lang("Press × to fly","Нажми ×, чтобы взлететь"),585-shift,192,.92f,White);
        }
        Art::pole(game.treeX,game.treeY);
        trails();
        for (const auto& a:game.actors) if (a.live) actorSprite(a);
        playerSprite();
        if (game.regionTicks<108) {
            char banner[96];
            if (game.region()==0)
                std::snprintf(banner,sizeof banner,lang("You've circled the world %u times!",
                    "Кругов вокруг света: %u"),game.laps());
            else
                std::snprintf(banner,sizeof banner,"%s",lang("You've entered a new region!","Вы вошли в новый регион!"));
            text(banner,512,regionPosition(game.regionTicks),.86f);
        }
        if (const unsigned alpha=phoneAlpha(game.phoneTicks)) {
            text(lang("New research breakthrough!","Новый научный прорыв!"),512,18,.83f,GU_RGBA(255,255,0,alpha));
            text(lang("Call for a special presentation:","Позвоните, чтобы увидеть презентацию:"),512,42,.73f,
                 GU_RGBA(255,255,0,alpha));
            text("199-EXTINCT",512,72,.89f,GU_RGBA(0,255,255,alpha));
        }
    }
    sceGuScissor(ViewX,0,ViewX+ViewWidth,Height);
    Art::hud(game.ticks,std::clamp(game.health/100,0.f,1.f),!death,russian());
    if (!death) {
        char score[24];
        std::snprintf(score,sizeof score,"%u",game.score);
        text(score,128,260,1.05f,White,120);
        if (russian()) text("Расстояние",128,203,.81f,White,120);
    }
    if (paused) MinigamePauseArt::draw(russian(),quitSelected?1:0);
    if (missing) text(lang("Could not load the region","Не удалось загрузить регион"),512,450,.8f);
    endFrame();
}

bool openFilm(const char* executable,const char* diagnostics) {
    showingFilm=MeowlsPresentation::open(executable);
    if (!showingFilm) { log(diagnostics,"film_load_failed"); return false; }
    auto* target=reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(vram045(Backplate))|0x40000000u);
    filmSidebar=Costumes054::decode(Phone054::basePath[russian()],target,Phone054::PlateBytes);
    presentationMs=0;
    filmNarration=filmCameo=false;
    titleArt=nullptr;
    audio.muteMusic();
    audio.musicTune(1,1);
    log(diagnostics,"film_open");
    return true;
}

void stepFilm(Flow& flow,unsigned pressed,unsigned ms,const char* diagnostics) {
    if (!showingFilm || (pressed&PSP_CTRL_CIRCLE)) {
        flow.screen=Screen::Phone;
        log(diagnostics,"film_return",presentationMs);
        return;
    }
    presentationMs+=std::min(ms,100u);
    if (!filmNarration && presentationMs>=MeowlsPresentation::NarrationMs) {
        filmNarration=true;
        audio.musicAt(Sound::MeowlsPresentation,(presentationMs-MeowlsPresentation::NarrationMs)*441ull/10);
        log(diagnostics,"film_narration",presentationMs);
    }
    if (!filmCameo && presentationMs>=MeowlsPresentation::CameoMs) {
        filmCameo=true;
        audio.play(Sound::MeowlsTickle);
        log(diagnostics,"film_cameo",presentationMs);
    }
    if (presentationMs>=MeowlsPresentation::DurationMs) {
        flow.screen=Screen::Phone;
        log(diagnostics,"film_complete",presentationMs);
    }
}

void closeFilm() {
    if (showingFilm) MeowlsPresentation::close();
    showingFilm=false;
    audio.muteMusic();
    audio.stop(Sound::MeowlsTickle);
}

void drawFilm() {
    beginFrame();
    sceGuScissor(ViewX,0,ViewX+ViewWidth,Height);
    if (filmSidebar) {
        setTexture(GU_PSM_5650,512,512,vram045(Backplate));
        patch({0,0,136,272,0,0},ViewX,0);
    }
    if (showingFilm && !MeowlsPresentation::draw(presentationMs))
        text(lang("Could not play the presentation","Не удалось воспроизвести презентацию"),512,248,.85f);
    localeText(lang("○ Back","○ Назад"),143,6,.74f,White);
    endFrame();
}

} // namespace Meowls
