#pragma once

#include "phone_art054.hpp"
#include "phone_text054.hpp"

namespace Phone054 {

constexpr unsigned Offset=2*FrameBytes;
constexpr unsigned PlateBytes=512*272*2;
constexpr unsigned OverlayOffset=Offset+PlateBytes;
constexpr unsigned BaseFrames=6199200,CallFrames=2587200;
constexpr float TonePitch[]={0.8f,0.9f,1.0f,1.1f,1.2f,1.32f,1.47f,1.6f,1.8f,2.0f,2.22f,1.0f};

enum class State { Dial, Calling, Talking };
enum class Action { None, Cheater, Cracktro, Pronouns, Quit };
State state=State::Dial;
std::string basePath[2],overlayPath;
char number[11]{},formatted[1024]{};
unsigned length=0,selected=0,elapsed=0,page=0,fragmentsLeft=0;
unsigned callTicks=0,callCarry=0,quitRemainingMs=0;
uint32_t callRandom=0;
unsigned pressedKey=UINT_MAX,pressedUntil=0,tip=0;
Sound response=Sound::Count;
Action answerAction=Action::None;
const char* branch=nullptr;
const ClubText054::Quest::Lines* lines=nullptr;
bool plateReady=false,overlayReady=false,saveFailed=false,researchCall=false;

void setPaths(const char* executable) {
    basePath[0]=assetPath(executable,"phone-plate-en054.bin");
    basePath[1]=assetPath(executable,"phone-plate-ru054.bin");
    overlayPath=assetPath(executable,"phone-overlays054.bin");
}

std::array<bool,8> fragments() {
    return {bool(progress.terryKey),bool(progress.golfKey),bool(progress.glowstickKey),
            bool(progress.parkKey),bool(progress.frontierKey),bool(progress.elysianKey),
            bool(progress.cosmicKey),bool(progress.endlessKey)};
}

void track(Sound song,unsigned frames) {
    audio.musicAt(song,static_cast<unsigned>((elapsed*441ull/10)%frames));
}

void resetCall() {
    state=State::Dial;
    audio.stop(Sound::PhoneStatic);
    if (response==Sound::PhoneFlintstone) audio.stop(response);
    response=Sound::Count;
    answerAction=Action::None;
    branch=nullptr;
    researchCall=false;
    track(Sound::PhoneMusic,BaseFrames);
    length=page=0;
    number[0]=0;
    lines=nullptr;
}

void enter() {
    elapsed=selected=pressedUntil=0;
    quitRemainingMs=0;
    callRandom=static_cast<uint32_t>(sceKernelGetSystemTimeWide());
    pressedKey=UINT_MAX;
    plateReady=overlayReady=saveFailed=false;
    const auto found=fragments();
    fragmentsLeft=static_cast<unsigned>(std::count(found.begin(),found.end(),false));
    resetCall();
}

void suspend() {
    audio.stop(Sound::PhoneStatic);
    if (response==Sound::PhoneFlintstone) audio.stop(response);
    plateReady=overlayReady=false;
}

void leave() {
    suspend();
    quitRemainingMs=0;
}

void resume() {
    if (state==State::Talking && response!=Sound::PhoneRadio)
        audio.playTuned(Sound::PhoneStatic,-6.0206f,1);
}

unsigned ringTicks() {
    // Sample an independent stream so dialing does not advance gameplay randomness.
    constexpr uint32_t count=121,reject=(uint32_t{0}-count)%count;
    do {
        callRandom=1664525u*callRandom+1013904223u;
    } while (callRandom<reject);
    return 180+callRandom%count;
}

struct Call { const char* number; const char* text; Sound response=Sound::Count; Action action=Action::None; };
constexpr Call Calls[]={
    {"67",nullptr,Sound::PhoneUnfunny,Action::Quit},
    {"1312",nullptr,Sound::PhonePronouns,Action::Pronouns},{"9653",nullptr,Sound::PhonePronouns,Action::Pronouns},
    {"3862243287",nullptr,Sound::Count,Action::Cheater},{"7737334263",nullptr,Sound::Count,Action::Cracktro},
    {"1225574663","First"},{"22745453","Zinglo"},{"2787827693",nullptr,Sound::PhoneWaltz},
    {"5535374268",nullptr,Sound::PhoneElephantAsmr},
    {"20240328",nullptr,Sound::PhoneBetaAnniversary},{"28032024",nullptr,Sound::PhoneBetaAnniversary},
    {"03282024",nullptr,Sound::PhoneBetaAnniversary},{"2083",nullptr,Sound::PhoneRevo83},
    {"19800522",nullptr,Sound::PhoneEerieFeline},{"22051980",nullptr,Sound::PhoneEerieFeline},
    {"05221980",nullptr,Sound::PhoneEerieFeline},{"1234567890","New"},
    {"824","UCG"},{"2284653","UCG"},{"8622669","UCG"},{"58259777","Winner"},
    {"1847463757","Tipperson"},{"5512251255",nullptr,Sound::PhoneBeverly},
    {"18009222",nullptr,Sound::PhoneFlintstone},{"09091996",nullptr,Sound::PhoneBandicoot},
    {"9605",nullptr,Sound::PhoneRadio},{"876862371","Congrats"},
};
void dial() {
    if (!length) return;
    state=State::Calling;
    callTicks=ringTicks();
    callCarry=0;
    saveFailed=false;
    track(Sound::PhoneCallMusic,CallFrames);
    lines=nullptr;
    response=Sound::Count;
    answerAction=Action::None;
    branch="N/A";
    researchCall=std::strcmp(number,"1993984628")==0;
    if (researchCall) return;
    if (std::strcmp(number,ClubText054::Quest::FriendNumber)==0) {
        branch=nullptr;
        const auto phase=ClubText054::Quest::friendCall(progress.quest,fragmentsLeft);
        if (!Club054::remember(phase.nextQuestPhase)) saveFailed=true;
        else lines=&ClubText054::Quest::FriendPhases[phase.dialoguePhase];
        return;
    }
    for (const auto& entry:Calls) if (std::strcmp(entry.number,number)==0) {
        branch=entry.text;
        response=entry.response;
        answerAction=entry.action;
        return;
    }
}

void press(unsigned key) {
    pressedKey=key;
    pressedUntil=elapsed+120;
    if (key==11) {
        audio.play(Sound::SlotPhone);
        dial();
        return;
    }
    audio.playTuned(Sound::PhoneButton,0,TonePitch[key]);
    if (key==9) {
        if (length) number[--length]=0;
    } else if (length<10) {
        number[length++]=key==10?'0':static_cast<char>('1'+key);
        number[length]=0;
    }
}

template<class OnAction>
void step(Flow& flow,unsigned pressed,unsigned ms,OnAction&& onAction) {
    ms=std::min(ms,100u);
    elapsed+=ms;
    if (pressed&PSP_CTRL_CIRCLE) {
        flow.screen=Screen::Villa;
        return;
    }
    if (quitRemainingMs) {
        if (ms>=quitRemainingMs) {
            quitRemainingMs=0;
            onAction(Action::Quit);
            return;
        }
        quitRemainingMs-=ms;
    }
    if (state==State::Calling) {
        // Godot checks zero before decrementing. A roll of N answers on tick N+1.
        const unsigned carry=callCarry+ms*60;
        const unsigned ticks=carry/1000;
        callCarry=carry%1000;
        if (ticks<=callTicks) {
            callTicks-=ticks;
            return;
        }
        if (researchCall) {
            flow.screen=Screen::MeowlsFilm;
            return;
        }
        state=State::Talking;
        page=0;
        if (response==Sound::Count || response==Sound::PhoneFlintstone) audio.muteMusic();
        if (response==Sound::PhoneFlintstone) audio.play(response);
        else if (response!=Sound::Count) audio.musicAt(response,0);
        if (response!=Sound::PhoneRadio) audio.playTuned(Sound::PhoneStatic,-6.0206f,1);
        const auto action=answerAction;
        answerAction=Action::None;
        if (action==Action::Quit) {
            if (!quitRemainingMs) quitRemainingMs=5000;
        } else if (action!=Action::None) onAction(action);
        return;
    }
    if (state==State::Talking) {
        if (pressed&PSP_CTRL_CROSS) {
            const bool isTip=branch && std::strcmp(branch,"Tipperson")==0;
            const unsigned pages=lines?lines->count:isTip?2:branch?PhoneText054::count(branch):0;
            if (++page>=pages) resetCall();
            else if (isTip) tip=static_cast<unsigned>(sceKernelGetSystemTimeWide()%PhoneText054::TipCount);
        }
        return;
    }
    const unsigned row=selected/3,col=selected%3;
    if (pressed&PSP_CTRL_LEFT) selected=row*3+(col+2)%3;
    if (pressed&PSP_CTRL_RIGHT) selected=row*3+(col+1)%3;
    if (pressed&PSP_CTRL_UP) selected=(selected+9)%12;
    if (pressed&PSP_CTRL_DOWN) selected=(selected+3)%12;
    if (pressed&PSP_CTRL_CROSS) press(selected);
}

inline void step(Flow& flow,unsigned pressed,unsigned ms) {
    step(flow,pressed,ms,[](Action){});
}

const char* text() {
    if (saveFailed) return lang("Could not save the call.","Не удалось сохранить звонок.");
    if (!lines) {
        if (!branch) return "";
        char key[32];
        if (std::strcmp(branch,"Tipperson")==0 && page==1) return PhoneText054::get(PhoneText054::TipKeys[tip],russian());
        std::snprintf(key,sizeof key,"%s%u",branch,page);
        const char* result=PhoneText054::get(key,russian());
        return result?result:"";
    }
    const char* source=ClubText054::get(lines->keys[page],russian());
    if (!source) return "";
    const char* mark=std::strstr(source,"{number}");
    if (!mark) return source;
    std::snprintf(formatted,sizeof formatted,"%.*s%u%s",static_cast<int>(mark-source),source,fragmentsLeft,mark+8);
    return formatted;
}

void drawText(const char* p,float x,float y) {
    while (*p && y<253) {
        const char* first=p;
        const char* word=nullptr;
        unsigned width=0;
        while (*p && *p!='\n') {
            const char* glyph=p;
            const unsigned index=nextGlyph(p);
            if (width+glyphAdvance[index]>376 && glyph>first) {
                p=word && word>first?word:glyph;
                break;
            }
            if (*glyph==' ') word=glyph;
            width+=glyphAdvance[index];
        }
        char line[255];
        const auto count=std::min<std::size_t>(p-first,sizeof line-1);
        std::memcpy(line,first,count);
        line[count]=0;
        localeText(line,x,y,0.65f,GU_RGBA(28,25,24,255),false,239,true);
        if (*p==' ' || *p=='\n') ++p;
        y+=13;
    }
}

void draw() {
    if (!plateReady) {
        auto* pixels=reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(vram045(Offset))|0x40000000u);
        plateReady=Costumes054::decode(basePath[russian()],pixels,PlateBytes);
    }
    if (!overlayReady) {
        namespace Art=PhoneArt054;
        static_assert(OverlayOffset+Art::OverlayWidth*Art::OverlayHeight*4<=2*1024*1024);
        auto* pixels=reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(vram045(OverlayOffset))|0x40000000u);
        overlayReady=Costumes054::decode(overlayPath,pixels,Art::OverlayWidth*Art::OverlayHeight*4);
    }
    beginFrame();
    Villa054::drawHud(elapsed*60/1000);
    namespace Art=PhoneArt054;
    if (plateReady) {
        sceGuTexFlush();
        setTexture(GU_PSM_5650,512,512,vram045(Offset));
        patch({0,0,272,272,0,0},ViewX+136,0);
    }
    if (overlayReady) {
        sceGuTexFlush();
        setTexture(GU_PSM_8888,Art::OverlayWidth,Art::OverlayHeight,vram045(OverlayOffset));
        const auto found=fragments();
        for (unsigned i=0;i<found.size();++i) if (found[i]) {
            const auto& key=Art::Fragment[i];
            patch(key,ViewX+136+key.x,key.y);
        }
        const auto& scan=Art::Scanline[(elapsed/33)%2];
        patch(scan,ViewX+136+scan.x,scan.y);
        if (state==State::Dial) {
            if (elapsed<pressedUntil && pressedKey<std::size(Art::Pressed)) {
                const auto& hit=Art::Pressed[pressedKey];
                patch(hit,ViewX+136+hit.x,hit.y);
            }
            const auto& key=Art::Keys[selected];
            patch(Art::Focus,ViewX+135+key.x,key.y-1);
        } else if (state==State::Talking) {
            if (lines || branch || saveFailed) patch(Art::Dialogue,ViewX+136+Art::Dialogue.x,Art::Dialogue.y);
            patch(Art::Next,ViewX+136+Art::Next.x,Art::Next.y);
        }
    }
    localeText(state==State::Calling?lang("CALLING…","ВЫЗОВ…"):number,323,36,0.84f,GU_RGBA(0,45,59,255),true,98,true);
    if (state==State::Talking) drawText(text(),136+Art::DialogueText.x,Art::DialogueText.y);
    localeText("○",177,10,0.65f);
    endFrame();
}

} // namespace Phone054




