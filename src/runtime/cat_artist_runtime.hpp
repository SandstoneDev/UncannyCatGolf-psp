#pragma once

#include "cat_artist_art.hpp"
#include "cat_artist_dialogue.hpp"

#include <new>
#ifdef __PSP__
#include <pspsysmem.h>
#endif

namespace CatArtist {

using ArtistDialogue::Topic;
using CatArtistArt::Berry;
constexpr int CanvasLeft=136,CanvasSize=272;
constexpr float SceneScale=17.0f/32;
constexpr unsigned InkOffset=0x100000;
static_assert(InkOffset+4*sizeof(ArtistBook::Canvas)==0x110000);
constexpr int BrushX=6,BrushY=78,BrushStep=21;
constexpr int PaletteX=19,PaletteY=109,PaletteStep=20;
constexpr int CostumeY=189,SlotX=8,SlotY=224,SlotStep=42;
constexpr int AskX=5,AskY=74,AskW=124,AskStep=23;

struct Editor {
    ArtistBook::Book saved;
    std::array<ArtistBook::Canvas,ArtistBook::Slots> draft{};
    ArtistBook::BaseSlots bases{};
    ArtistBook::FillQueue fillQueue{};
    Vec2 cursor{272,136};
    Vec2 berry{208,-128},travelFrom{208,-128};
    unsigned slot=0,brush=0,color=1;
    bool erase=false,bucket=false,strokeHeld=false,strokeErase=false,artMissing=false,saveError=false;
    bool asking=false,dialog=false,greeting=true,revealed=false,flipped=false,loadingPose=false;
    bool askPointer=false;
    unsigned ticks=0,danceStart=0,dialogAge=0,poseAge=0,voiceNext=0,returnTick=0;
    unsigned line=0,askFocus=0,lastVoice=4;
    int previousX=0,previousY=0;
    uint32_t random=1;
    Topic topic=Topic::Greet;
    Berry pose=Berry::Idle;
};

#ifdef __PSP__
struct EditorDeleter {
    SceUID block=-1;

    void operator()(Editor* state) const noexcept {
        state->~Editor();
        sceKernelFreePartitionMemory(block);
    }
};
using EditorOwner=std::unique_ptr<Editor,EditorDeleter>;

EditorOwner allocateEditor(CatArtistArt::LoadFailure& failure) {
    const SceUID block=sceKernelAllocPartitionMemory(2,"ucg-cat-artist",PSP_SMEM_Low,sizeof(Editor),nullptr);
    if (block<0) {
        failure.record("editor_partition_alloc",sizeof(Editor),block,nullptr,Z_OK,0);
        return {};
    }
    void* address=sceKernelGetBlockHeadAddr(block);
    const auto bits=reinterpret_cast<intptr_t>(address);
    if (bits<=0 || bits%alignof(Editor)!=0) {
        failure.record("editor_partition_head",alignof(Editor),bits,nullptr,Z_OK,0);
        sceKernelFreePartitionMemory(block);
        return {};
    }
    return EditorOwner(new(address) Editor,EditorDeleter{block});
}
#else
using EditorOwner=std::unique_ptr<Editor>;

EditorOwner allocateEditor(CatArtistArt::LoadFailure& failure) {
    errno=0;
    EditorOwner state(new(std::nothrow) Editor);
    if (!state) failure.record("editor_alloc",sizeof(Editor),0);
    return state;
}
#endif

EditorOwner editor;

void log(const char* path,const char* event,unsigned a=0,unsigned b=0) {
    if (auto* out=std::fopen(path,"a")) {
        std::fprintf(out,"event=cat_artist_%s a=%u b=%u\n",event,a,b);
        std::fclose(out);
    }
}

constexpr Sound voice(unsigned n) {
    return static_cast<Sound>(static_cast<unsigned>(Sound::ArtistTalk1)+n);
}

void stopTalking(Editor& edit) {
    for (unsigned i=0;i<4;++i) audio.stop(voice(i));
    edit.voiceNext=0;
}

uint32_t random(Editor& edit) {
    edit.random^=edit.random<<13;
    edit.random^=edit.random>>17;
    return edit.random^=edit.random<<5;
}

void talk(Editor& edit) {
    unsigned index=random(edit)%(edit.lastVoice<4?3:4);
    if (edit.lastVoice<4 && index>=edit.lastVoice) ++index;
    edit.lastVoice=index;
    const float pitch=(1.0f/1.2f)+(1.2f-1.0f/1.2f)*(random(edit)%65536u)/65535.f;
    audio.playTuned(voice(index),0,pitch);
    const unsigned frames=audio.totalFrames(voice(index));
    edit.voiceNext=edit.ticks+std::max(70u,static_cast<unsigned>(std::ceil(frames/(44.1f*pitch))));
}

void setPose(Editor& edit,Berry pose) {
    edit.pose=pose;
    edit.poseAge=edit.ticks;
}

uint8_t* inkPage(unsigned page) {
    return vram045(InkOffset+page*sizeof(ArtistBook::Canvas));
}

void upload(unsigned page,const ArtistBook::Canvas& canvas) {
    auto* uncached=reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(inkPage(page))|0x40000000u);
    ArtistInk::swizzle(canvas,uncached);
}

void refreshCanvas(Editor& edit,CatArtistArt::LoadFailure* failure=nullptr) {
    const unsigned current=edit.slot;
    edit.artMissing=!CatArtistArt::loadCostume(edit.bases[current],failure);
    upload(0,edit.draft[current]);
}

void preview(Editor& edit,unsigned slot) {
    CatArtistArt::loadPreview(slot,edit.saved.bases[slot]);
    upload(slot+1,edit.saved.art[slot]);
}

void logEntryFailure(const char* diagnostics,const char* phase,const CatArtistArt::LoadFailure& failure,
                     bool editorLive) {
    if (auto* out=std::fopen(diagnostics,"a")) {
        std::fprintf(out,"event=cat_artist_entry_diagnostic phase=%s stage=%s editor_bytes=%u editor_live=%u "
                         "errno=%d ferror=%d expected=%ld actual=%ld zlib=%d "
                         "stage_heap_used=%u stage_heap_free=%u stage_partition_free=%u stage_partition_largest=%u\n",
                     phase,failure.stage,static_cast<unsigned>(sizeof(Editor)),unsigned(editorLive),
                     failure.error,failure.fileError,failure.expected,failure.actual,failure.zlib,
                     failure.heapUsed,failure.heapFree,failure.partitionFree,failure.partitionLargest);
        std::fclose(out);
    }
}

void leave();

bool enter(const char* executable,const char* diagnostics) {
    titleArt=nullptr;
    titleReusedCourseVram=true;
    if (editor) leave();
    CatArtistArt::LoadFailure failure;
    editor=allocateEditor(failure);
    if (!editor || !CatArtistArt::load(executable,failure)) {
        const bool editorLive=bool(editor);
        logMemory(diagnostics,"cat_artist_open_failure_before_reset",audio.effectMemoryBytes());
        CatArtistArt::unload();
        editor.reset();
        logEntryFailure(diagnostics,"open",failure,editorLive);
        log(diagnostics,"open_failed");
        return false;
    }
    auto& edit=*editor;
    if (!ArtistCopycat::loadBook(edit.saved)) {
        log(diagnostics,"book_read_failed");
        CatArtistArt::unload();
        editor.reset();
        return false;
    }
    edit.draft=edit.saved.art;
    edit.bases=edit.saved.bases;
    edit.slot=edit.saved.active;
    edit.random=sceKernelGetSystemTimeLow()|1u;
    for (unsigned i=0;i<ArtistBook::Slots;++i) preview(edit,i);
    refreshCanvas(edit,&failure);
    if (edit.artMissing) {
        logMemory(diagnostics,"cat_artist_costume_failure_before_reset",audio.effectMemoryBytes());
        CatArtistArt::unload();
        editor.reset();
        logEntryFailure(diagnostics,"costume",failure,true);
        log(diagnostics,"open_costume_failed");
        return false;
    }
    audio.music(Sound::ArtistMusic);
    audio.playTuned(Sound::ArtistDingDong,5,1);
    log(diagnostics,"open",edit.slot,edit.bases[edit.slot]);
    return true;
}

void leave() {
    if (editor) stopTalking(*editor);
    CatArtistArt::unload();
    editor.reset();
}

bool chooseSlot(Editor& edit,unsigned slot) {
    if (slot==edit.slot) return false;
    edit.slot=slot;
    refreshCanvas(edit);
    return true;
}

bool cycleBase(Editor& edit,bool forward) {
    constexpr unsigned count=ArtistBook::Blank+1;
    auto& base=edit.bases[edit.slot];
    const unsigned old=base;
    do {
        base=(base+(forward?1:count-1))%count;
    } while (base!=ArtistBook::Blank && (base==ArtistBook::Copycat ||
        !VillaCostumes::available(static_cast<VillaCostumes::Id>(base),Villa054::earned)));
    refreshCanvas(edit);
    return old!=base;
}

bool persist(Editor& edit,bool saveDrawing,const char* diagnostics) {
    if (!saveDrawing && edit.saved.active==edit.slot) return true;
    std::unique_ptr<ArtistBook::Book> next(new(std::nothrow) ArtistBook::Book(edit.saved));
    if (!next) { edit.saveError=true; return false; }
    next->active=edit.slot;
    if (saveDrawing) {
        next->bases[edit.slot]=edit.bases[edit.slot];
        next->art[edit.slot]=edit.draft[edit.slot];
    }
    edit.saveError=!ArtistGlobal::save(ArtistCopycat::prefix,edit.saved,*next);
    if (edit.saveError) { log(diagnostics,"write_failed",edit.slot); return false; }
    edit.saved=*next;
    if (saveDrawing) preview(edit,edit.slot);
    ArtistCopycat::update(edit.saved);
    log(diagnostics,saveDrawing?"save":"slot_confirmed",edit.slot,edit.saved.generation);
    return true;
}

const ArtistDialogue::Line& current(const Editor& edit) {
    return ArtistDialogue::get(edit.topic).lines[edit.line];
}

void startLine(Editor& edit) {
    const auto& page=current(edit);
    edit.dialogAge=edit.ticks;
    edit.revealed=false;
    edit.travelFrom=edit.berry;
    edit.loadingPose=page.flyMs!=0;
    setPose(edit,static_cast<Berry>(edit.loadingPose?page.flyPose:page.pose));
    if (!edit.loadingPose) {
        edit.berry={static_cast<float>(page.x),static_cast<float>(page.y)};
        edit.flipped=page.flip;
        if (edit.topic!=Topic::Exit) talk(edit);
    }
}

void showAsk(Editor& edit) {
    stopTalking(edit);
    edit.greeting=false;
    edit.asking=true;
    edit.askPointer=false;
    edit.askFocus=0;
    edit.topic=Topic::Check;
    edit.line=0;
    startLine(edit);
}

void closeAsk(Editor& edit) {
    stopTalking(edit);
    edit.asking=false;
    edit.danceStart=edit.ticks;
    setPose(edit,Berry::Idle);
}

void showTopic(Editor& edit,Topic topic,const char* diagnostics) {
    closeAsk(edit);
    edit.greeting=false;
    if (topic==Topic::NeverMind) return;
    if (topic==Topic::Save && !persist(edit,true,diagnostics)) return;
    if (topic==Topic::Exit) audio.play(Sound::ArtistGoodbye);
    edit.dialog=true;
    edit.topic=topic;
    edit.line=0;
    startLine(edit);
    log(diagnostics,"dialogue",static_cast<unsigned>(topic),ArtistDialogue::get(topic).count);
}

void advance(Flow& flow,Editor& edit,const char* diagnostics) {
    if (edit.loadingPose) return;
    const auto& page=current(edit);
    if (!edit.revealed && edit.ticks-edit.dialogAge<page.flyMs+page.revealMs) {
        edit.revealed=true;
        stopTalking(edit);
        return;
    }
    stopTalking(edit);
    if (edit.line+1<ArtistDialogue::get(edit.topic).count) {
        ++edit.line;
        startLine(edit);
        return;
    }
    if (edit.topic==Topic::Exit && persist(edit,false,diagnostics)) {
        ArtistCopycat::update(edit.saved);
        log(diagnostics,"exit",edit.slot,edit.saved.generation);
        flow.screen=Screen::Villa;
        return;
    }
    edit.dialog=false;
    edit.travelFrom=edit.berry;
    edit.returnTick=edit.ticks;
    edit.danceStart=edit.ticks;
    setPose(edit,Berry::Idle);
}

void animate(Editor& edit) {
    if (edit.greeting && edit.ticks<1000) {
        const float remaining=1.f-edit.ticks/1000.f;
        edit.berry={208,80-208*remaining*remaining*remaining};
        return;
    }
    if (edit.greeting && edit.topic==Topic::Greet && edit.pose==Berry::Idle) {
        edit.berry={208,80};
        startLine(edit);
    }
    if (edit.dialog || edit.asking || edit.greeting) {
        const auto& page=current(edit);
        const unsigned age=edit.ticks-edit.dialogAge;
        if (edit.loadingPose) {
            const float t=std::min(1.f,static_cast<float>(age)/page.flyMs);
            const float cubic=t<.5f?4*t*t*t:1-4*(1-t)*(1-t)*(1-t);
            edit.berry=edit.travelFrom+(Vec2{static_cast<float>(page.x),static_cast<float>(page.y)}-edit.travelFrom)*cubic;
            if (age>=page.flyMs) {
                edit.loadingPose=false;
                edit.flipped=page.flip;
                setPose(edit,static_cast<Berry>(page.pose));
                if (edit.topic!=Topic::Exit) talk(edit);
            }
        }
        if (edit.voiceNext && age>=page.flyMs+page.revealMs) stopTalking(edit);
        else if (edit.voiceNext && edit.ticks>=edit.voiceNext) talk(edit);
        if (edit.greeting && age>=5000) {
            edit.greeting=false;
            setPose(edit,Berry::Idle);
        }
    } else {
        const Vec2 home={208,80};
        const unsigned age=edit.ticks-edit.danceStart;
        const float t=std::min(1.f,(edit.ticks-edit.returnTick)/1000.f);
        const float eased=1.f-(1.f-t)*(1.f-t)*(1.f-t);
        edit.berry=edit.returnTick?edit.travelFrom+(home-edit.travelFrom)*eased:home;
        if (age>=15000 && edit.pose!=Berry::Dance) setPose(edit,Berry::Dance);
    }
}

bool over(const Vec2& p,float x,float y,float width,float height) {
    return p.x>=x && p.x<x+width && p.y>=y && p.y<y+height;
}

bool usesCanvas(const Vec2& p) { return over(p,CanvasLeft,0,CanvasSize,CanvasSize); }

void clickTool(Editor& edit) {
    const auto& p=edit.cursor;
    if (over(p,BrushX,BrushY,6*BrushStep,18)) {
        const int button=(p.x-BrushX)/BrushStep;
        if (button<4) edit.brush=button;
        else if (button==4) edit.erase=!edit.erase;
        else edit.bucket=!edit.bucket;
    } else if (over(p,PaletteX,PaletteY,5*PaletteStep,4*PaletteStep)) {
        edit.color=1+static_cast<int>((p.x-PaletteX)/PaletteStep)+5*static_cast<int>((p.y-PaletteY)/PaletteStep);
    } else if (over(p,26,CostumeY,84,30)) {
        cycleBase(edit,p.x>=68);
    } else if (over(p,SlotX,SlotY,3*SlotStep,37)) {
        chooseSlot(edit,static_cast<unsigned>((p.x-SlotX)/SlotStep));
    } else if (over(p,edit.berry.x*SceneScale-14,edit.berry.y*SceneScale-16,28,34)) {
        showAsk(edit);
    }
}

void step(Flow& flow,Vec2 stick,unsigned held,unsigned pressed,unsigned milliseconds,const char* diagnostics) {
    if (!editor) return;
    auto& edit=*editor;
    const unsigned dt=std::min(40u,milliseconds);
    edit.ticks+=dt;
    animate(edit);
    const float magnitude=std::min(1.f,length(stick));
    if (magnitude>.15f) {
        const float speed=45.f+205.f*(magnitude-.15f)*(magnitude-.15f)/(.85f*.85f);
        edit.cursor=edit.cursor+stick*(speed*dt/(1000*magnitude));
        edit.cursor.x=std::clamp(edit.cursor.x,0.f,407.f);
        edit.cursor.y=std::clamp(edit.cursor.y,0.f,271.f);
    }
    if (edit.dialog) {
        if (pressed&(PSP_CTRL_CROSS|PSP_CTRL_CIRCLE)) advance(flow,edit,diagnostics);
        return;
    }
    if (edit.asking) {
        constexpr unsigned count=std::size(ArtistDialogue::Options);
        if (pressed&(PSP_CTRL_START|PSP_CTRL_CIRCLE)) closeAsk(edit);
        else if (pressed&(PSP_CTRL_UP|PSP_CTRL_DOWN)) {
            const unsigned direction=(pressed&PSP_CTRL_DOWN)?1:count-1;
            edit.askFocus=(edit.askFocus+direction)%count;
            edit.askPointer=false;
        } else if (pressed&PSP_CTRL_CROSS) {
            const auto& p=edit.cursor;
            if (edit.askPointer && over(p,AskX,AskY,AskW,count*AskStep))
                edit.askFocus=static_cast<int>(p.y-AskY)/AskStep;
            showTopic(edit,ArtistDialogue::Options[edit.askFocus].topic,diagnostics);
        } else if (magnitude>.15f) {
            edit.askPointer=true;
        }
        return;
    }
    if (pressed&PSP_CTRL_START) { showAsk(edit); edit.strokeHeld=false; return; }
    if (pressed&PSP_CTRL_CIRCLE) { showTopic(edit,Topic::Exit,diagnostics); edit.strokeHeld=false; return; }
    if (pressed&PSP_CTRL_SELECT) edit.erase=!edit.erase;
    if (pressed&PSP_CTRL_TRIANGLE) edit.bucket=!edit.bucket;
    if (pressed&PSP_CTRL_LTRIGGER) chooseSlot(edit,(edit.slot+ArtistBook::Slots-1)%ArtistBook::Slots);
    if (pressed&PSP_CTRL_RTRIGGER) chooseSlot(edit,(edit.slot+1)%ArtistBook::Slots);

    constexpr unsigned paint=PSP_CTRL_CROSS|PSP_CTRL_SQUARE;
    if (pressed&paint) {
        edit.danceStart=edit.ticks;
        if (edit.pose==Berry::Dance) setPose(edit,Berry::Idle);
    }
    if ((pressed&PSP_CTRL_CROSS) && !usesCanvas(edit.cursor)) clickTool(edit);
    if (!(held&paint) || !usesCanvas(edit.cursor) || edit.asking) { edit.strokeHeld=false; return; }

    const int x=(edit.cursor.x-CanvasLeft)*ArtistBook::Side/CanvasSize;
    const int y=edit.cursor.y*ArtistBook::Side/CanvasSize;
    const bool erase=edit.erase!=(!(held&PSP_CTRL_CROSS) && (held&PSP_CTRL_SQUARE));
    const unsigned color=erase?0:edit.color;
    bool changed=false;
    auto& canvas=edit.draft[edit.slot];
    if (edit.bucket) {
        if (!edit.strokeHeld || edit.strokeErase!=erase || x!=edit.previousX || y!=edit.previousY)
            changed=ArtistBook::flood(canvas,x,y,color,edit.fillQueue);
    } else {
        const int previousX=edit.strokeHeld?edit.previousX:x;
        const int previousY=edit.strokeHeld?edit.previousY:y;
        changed=ArtistBook::stroke(canvas,previousX,previousY,x,y,edit.brush,color);
    }
    edit.previousX=x;
    edit.previousY=y;
    edit.strokeHeld=true;
    edit.strokeErase=erase;
    if (changed) upload(0,canvas);
}

void drawInk(unsigned page,float x,float y,float size) {
    sceGuTexFlush();
    ArtistInk::bind(inkPage(page));
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    patchScaled(ArtistInk::Full,x,y,size/ArtistBook::Side,size/ArtistBook::Side,White);
    sceGuTexFilter(GU_LINEAR,GU_LINEAR);
}

void border(float x,float y,float w,float h,uint32_t color) {
    rectangle(x,y,w,1,color);
    rectangle(x,y+h-1,w,1,color);
    rectangle(x,y,1,h,color);
    rectangle(x+w-1,y,1,h,color);
}

void drawSidebar(Editor& edit) {
    using namespace CatArtistArt;
    CatArtistArt::drawSidebar(ViewX,0,CanvasLeft,Height,russian());
    for (unsigned i=0;i<4;++i)
        drawBrush(i,i==edit.brush,ViewX+BrushX+i*BrushStep,BrushY,18,18);
    drawMode(edit.erase,ViewX+BrushX+4*BrushStep,BrushY,18,18);
    drawFill(edit.bucket,ViewX+BrushX+5*BrushStep,BrushY,18,18);

    for (unsigned i=0;i<ArtistBook::Colors;++i) {
        const float x=ViewX+PaletteX+(i%5)*PaletteStep,y=PaletteY+(i/5)*PaletteStep;
        rectangle(x,y,17,17,ArtistInk::Palette[i+1]);
        if (i+1==edit.color) border(x-1,y-1,19,19,i==1?GU_RGBA(0,0,0,255):White);
    }
    drawArrow(true,ViewX+29,CostumeY,30,30);
    drawArrow(false,ViewX+75,CostumeY,30,30);
    const auto base=edit.bases[edit.slot];
    const char* name=base==ArtistBook::Blank?lang("Blank","Пустой"):
        VillaCostumes::name(static_cast<VillaCostumes::Id>(base),russian());
    localeText(name,68,CostumeY-10,.58f,White,true,126,true);
    for (unsigned slot=0;slot<ArtistBook::Slots;++slot) {
        const float x=ViewX+SlotX+slot*SlotStep;
        const uint32_t shade=slot==edit.slot?GU_RGBA(255,215,100,255):GU_RGBA(150,123,94,255);
        rectangle(x,SlotY,36,36,GU_RGBA(12,9,9,255));
        drawPreview(slot,x+2,SlotY+2,32,32,edit.ticks*60/1000);
        drawInk(slot+1,x+2,SlotY+2,32);
        border(x,SlotY,36,36,shade);
    }
    localeText(lang("START Help","START Помощь"),68,261,.55f,White,true,124,true);
}

void wrap(const char* source,unsigned visible,float x,float y,unsigned maxWidth,float scale) {
    char row[320]{};
    unsigned chars=0;
    const auto* cursor=source;
    while (*cursor && visible) {
        while (*cursor==' ') ++cursor;
        const char* end=cursor;
        while (*end && *end!=' ') ++end;
        unsigned bytes=end-cursor;
        if (!bytes) break;
        if (chars+bytes+2>=sizeof row) break;
        if (chars) row[chars++]=' ';
        std::memcpy(row+chars,cursor,bytes);
        row[chars+bytes]=0;
        if (chars && localeTextWidth(row)*scale>maxWidth) {
            row[chars-1]=0;
            localeText(row,x,y,scale,GU_RGBA(20,15,13,255),true,maxWidth,true);
            y+=12;
            chars=0;
            continue;
        }
        unsigned copied=0;
        while (copied<bytes && visible) {
            const auto byte=static_cast<unsigned char>(cursor[copied]);
            copied+=byte<0x80?1:byte<0xe0?2:3;
            --visible;
        }
        chars+=copied;
        row[chars]=0;
        cursor=end;
    }
    if (chars) localeText(row,x,y,scale,GU_RGBA(20,15,13,255),true,maxWidth,true);
}

void drawSpeech(Editor& edit) {
    if (!(edit.dialog || edit.asking || (edit.greeting && edit.ticks>=1000)) || edit.loadingPose) return;
    const auto& page=current(edit);
    const char* text=lang(page.en,page.ru);
    char replaced[320];
    if (const auto* marker=std::strstr(text,"{number}")) {
        std::snprintf(replaced,sizeof replaced,"%.*s%u%s",static_cast<int>(marker-text),text,edit.slot+1,marker+8);
        text=replaced;
    }
    const float left=std::clamp(edit.berry.x*SceneScale+25,137.f,271.f);
    const float top=std::clamp(edit.berry.y*SceneScale-36,6.f,SceneScale*512-102);
    CatArtistArt::drawSpeech(ViewX+left-5,top-2,134,98);
    const unsigned age=edit.ticks-edit.dialogAge-page.flyMs;
    unsigned count=0;
    for (const char* letter=text;*letter;++letter)
        count+=(static_cast<unsigned char>(*letter)&0xc0)!=0x80;
    const unsigned visible=edit.revealed?count:count*std::min(age,page.revealMs)/std::max(1u,page.revealMs);
    wrap(text,visible,left+64,top+5,118,.68f);
    if (edit.dialog)
        localeText(lang("× Continue","× Дальше"),left+64,top+77,.55f,GU_RGBA(45,29,20,255),true,114,true);
}

void draw() {
    if (!editor) return;
    auto& edit=*editor;
    beginFrame();
    sceGuScissor(ViewX,0,ViewX+ViewWidth,Height);
    rectangle(ViewX+CanvasLeft,0,CanvasSize,CanvasSize,GU_RGBA(0,0,0,255));
    CatArtistArt::drawCostume(ViewX+CanvasLeft,0,CanvasSize,CanvasSize,edit.ticks*60/1000);
    drawInk(0,ViewX+CanvasLeft,0,CanvasSize);
    drawSidebar(edit);
    CatArtistArt::drawBerry(edit.pose,(edit.ticks-edit.poseAge)*60/1000,
        ViewX+edit.berry.x*SceneScale,edit.berry.y*SceneScale,52,52,edit.flipped);
    if (edit.asking) {
        for (unsigned i=0;i<std::size(ArtistDialogue::Options);++i) {
            const float y=AskY+i*AskStep;
            const bool focus=edit.askPointer?over(edit.cursor,AskX,y,AskW,AskStep):i==edit.askFocus;
            CatArtistArt::drawPlank(ViewX+AskX,y,AskW,20,focus?White:GU_RGBA(198,186,166,255));
            const auto& choice=ArtistDialogue::Options[i];
            localeText(lang(choice.en,choice.ru),AskX+AskW*.5f,y+3,.66f,White,true,AskW-7,true);
        }
    }
    drawSpeech(edit);
    if (edit.artMissing || edit.saveError) {
        rectangle(ViewX+CanvasLeft,250,CanvasSize,22,GU_RGBA(45,15,20,225));
        localeText(edit.saveError?lang("Could not save art","Не удалось сохранить рисунок"):
                   lang("Could not load costume","Не удалось загрузить костюм"),272,254,.7f,White,true,248,true);
    }
    CatArtistArt::drawCursor(ViewX+edit.cursor.x,edit.cursor.y,16,18);
    endFrame();
}

} // namespace CatArtist
