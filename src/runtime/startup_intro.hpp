#pragma once
// Included after Main's GU/text primitives. This overlay owns borrowed VRAM only
// until leave; Main reloads localized course resources before returning to Loading.
namespace StartupIntro {
using StartupIntroState::State;
using StartupIntroState::Action;
using StartupIntroState::Error;
inline State state;
inline bool artReady=false;
inline constexpr unsigned ArtOffset=2*FrameBytes;
static_assert(ArtOffset+StartupIntroArt::Bytes<=2*1024*1024);
static_assert(sizeof gameplayPage>=StartupIntroArt::LoadScratchBytes);
inline const char* Marquee="Over two years in the making... proudly presenting, UNCANNY CAT GOLF! The video game that never seems to die, brought to you by some of the weirdest and silliest devs that you'll ever see in the Godot scene. And this time, it's bigger than ever, with all new levels, gamemodes, extras, and devious secrets... Does it get any better than this? What does it matter! Just play the game, have yourself a great time, and find out everything that the Canny Crew has to offer... and maybe stick with us for whatever we do next. We are eager to see you there, too...";
inline bool prepare(const char* path);
// Borrow the path only for this synchronous load. Nothing retains it after return.
inline void begin(const StartupSettings::Values& values,const char* path) {state.begin(values);artReady=false;prepare(path);}
inline void leave() {artReady=false;}
inline bool prepare(const char* path) {
    if (artReady) return true;
    sceGuSync(0,0);
    auto* destination=reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(vram045(ArtOffset))|0x40000000u);
    artReady=StartupIntroArt::load(path,destination,StartupIntroArt::Bytes,gameplayPage,sizeof gameplayPage);
    if (!artReady) state.error=Error::Assets;
    return artReady;
}
inline const char* tr(const char* en,const char* ru) {return state.values.language==Language::Russian?ru:en;}
inline void bind() {sceGuTexFlush();setTexture(GU_PSM_8888,512,128,vram045(ArtOffset));}
inline uint32_t rainbow(float t,unsigned alpha=255) {
    const auto channel=[&](float phase) {return static_cast<unsigned>(164+80*std::sin(t+phase));};
    return GU_RGBA(channel(0),channel(2.0944f),channel(4.1888f),alpha);
}
inline void frame(float x,float y,float w,float h,bool selected) {
    if (!artReady) return;
    bind();const int u=selected?208:192;
    // Source theme four-pixel corners and borders, with a stretched center.
    for (unsigned row=0;row<3;++row) for (unsigned col=0;col<3;++col) {
        const int sx=col==0?0:col==1?4:12,sy=row==0?0:row==1?4:12;
        const int sw=col==1?8:4,sh=row==1?8:4;
        const float dx=x+(col==0?0:col==1?4:w-4),dy=y+(row==0?0:row==1?4:h-4);
        patchScaled({static_cast<int16_t>(u+sx),static_cast<int16_t>(112+sy),static_cast<int16_t>(sw),static_cast<int16_t>(sh),0,0},
            ViewX+dx,dy,(col==1?w-8:4)/sw,(row==1?h-8:4)/sh,White);
    }
}
inline void draw(unsigned elapsedMs) {
    state.elapsedMs=elapsedMs;
    beginFrame();sceGuScissor(ViewX,0,ViewX+ViewWidth,Height);
    const float seconds=elapsedMs/1000.f;
    rectangle(ViewX,0,ViewWidth,Height,GU_RGBA(12+unsigned(7+7*std::sin(seconds*.3f)),5,32,255));
    // Same two counter-scrolling grids as the source's half-resolution viewport.
    for (unsigned half=0;half<2;++half) {
        const float direction=half?-1.f:1.f;
        const int shift=static_cast<int>(seconds*10*direction)%32;
        const auto color=rainbow(seconds*.25f+half,76);
        for (int x=-32;x<ViewWidth+32;x+=32) rectangle(ViewX+x+shift,half*136,1,136,color);
        for (int y=-32;y<168;y+=32) rectangle(ViewX,half*136+y+shift,ViewWidth,1,color);
    }
    if (artReady) {
        bind();
        for (unsigned i=0;i<32;++i) {
            const float age=std::fmod(seconds+i*0.0618034f,2.f),fraction=age*.5f;
            const float angle=i*2.39996323f;
            const float radius=24+50*age*age;
            const float size=(2+float(i%3)*.5f)*std::min(age/1.75f,1.f)*.53125f;
            const float x=ViewWidth*.5f+std::cos(angle)*radius,y=136+std::sin(angle)*radius;
            const unsigned alpha=static_cast<unsigned>(90*std::min(age*4,1.f)*(1-fraction*.5f));
            patch({static_cast<int16_t>((i%8)*32),80,32,32,0,0},ViewX+x-16*size,y-16*size,size,GU_RGBA(114,114,114,alpha));
        }
        bind();patch({0,0,374,70,0,0},ViewX+(ViewWidth-374*.6f)*.5f,3,.6f);
    } else localeText("CANNY CREW",ViewWidth*.5f,18,1,White,true);
    localeText("Presenting:",ViewWidth*.5f,49,.56f,White,true);
    localeText("UNCANNY CAT GOLF",ViewWidth*.5f,64,.82f,rainbow(seconds*.5f),true);
    frame(36,88,336,139,false);
    for (unsigned row=0;row<3;++row) {
        const float y=96+row*23;
        if (state.row==row) frame(42,y-2,324,23,true);
        const char* label=row==0?tr("Master Volume:","Общая громкость:"):
            row==1?tr("Music Volume:","Громкость музыки:"):tr("SFX Volume:","Громкость звуков:");
        localeText(label,48,y,.66f,White,false,151,true);
        const unsigned value=row==0?state.values.master:row==1?state.values.music:state.values.sounds;
        if (artReady) {
            bind();patchScaled({static_cast<int16_t>(state.row==row?96:0),112,96,16,0,0},ViewX+205,y+3,1.05f,.55f,White);
        } else rectangle(ViewX+205,y+6,101,3,GU_RGBA(110,100,130,255));
        // Original cracktro HSlider grabber is a fully transparent gradient.
        char percent[8];std::snprintf(percent,sizeof percent,"%u%%",value*5);
        localeText(percent,319,y,.65f,White);
    }
    if (state.row==3) frame(42,164,324,23,true);
    localeText(tr("Language:","Язык:"),48,166,.66f,White);
    localeText(state.values.language==Language::Russian?"Русский":"English",256,166,.66f,White,true);
    frame(125,193,158,26,state.row==4);
    localeText(tr("Start Game","Начать игру"),204,197,.78f,White,true);
    if (state.error!=Error::None) {
        const char* error=state.error==Error::Save?tr("Could not save. X: retry","Не удалось сохранить. ×: повторить"):
            state.error==Error::Language?tr("Could not load language. X: retry","Не удалось загрузить язык. ×: повторить"):
            tr("Could not load intro. X: retry","Не удалось загрузить вступление. ×: повторить");
        localeText(error,204,222,.55f,GU_RGBA(255,150,150,255),true,400,true);
    }
    // The complete original English source text also serves RU's empty source cell.
    // Draw only on-screen glyphs so long scrolling positions never reach GE coordinates.
    const float scale=.66f,width=localeTextWidth(Marquee)*scale;
    float x=544.f-std::fmod(seconds*106.25f,width+544.f);
    unsigned glyph=0;const char* p=Marquee;
    while (*p) {
        const char* begin=p;const unsigned index=nextGlyph(p);const float advance=glyphAdvance[index]*scale;
        if (x>=-16 && x<ViewWidth) {
            char text[5]{};const unsigned bytes=static_cast<unsigned>(p-begin);
            std::memcpy(text,begin,std::min(bytes,4u));
            localeText(text,x,239+4*std::sin(seconds*-4+glyph*.45f),scale,rainbow(seconds*.5f+glyph*.08f));
        }
        x+=advance;++glyph;
    }
    localeText(tr("D-pad: Select / Adjust   ×: Start","Крестовина: выбор / настройка   ×: начать"),204,258,.51f,White,true,404,true);
    endFrame();
}
}
