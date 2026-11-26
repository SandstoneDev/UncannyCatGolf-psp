#pragma once
#include "endless_run.hpp"
#include "endless_key.hpp"
#include "endless_geometry.hpp"
#include "endless_rotating.hpp"
#include "endless_e04_art.hpp"
#include "endless_objects.hpp"
#include "endless_objects_art.hpp"
#include "endless_modifier_art.hpp"
#include "endless_presentation_art.hpp"
#include "endless_particles_art056.hpp"
#include "endless_hud_layout066.hpp"

namespace Endless054 {
unsigned modifierHudEpoch=0;
AvatarIdle059::Speech speech059;
using namespace EndlessRules;
using Item=EndlessGeometry::Item;
using Kind=EndlessGeometry::Kind;

struct Records {
    unsigned magic=0x45574355,version=1,sequence=0,unlocks=0,count=0;
    Rank entries[10]{};
    uint32_t checksum=0;

    uint32_t hash() const {
        const auto* bytes=reinterpret_cast<const unsigned char*>(this);
        uint32_t value=2166136261u;
        for (unsigned i=0;i<sizeof(*this)-sizeof(checksum);++i) value=(value^bytes[i])*16777619u;
        return value;
    }
};
Records records;
std::string recordsPath;
unsigned recordSlot=0;
bool recordsReadFailed=false;
// Optional sidecar: the original v1 score files stay byte-compatible. The two
// slots retain the last good name/history if a write is interrupted.
struct NameMemory {
    unsigned magic=0x4e574355,version=1,sequence=0,count=0,lowest=UINT32_MAX;
    char name[32]{};
    uint32_t checksum=0;
    uint32_t hash() const {
        const auto* bytes=reinterpret_cast<const unsigned char*>(this);
        uint32_t value=2166136261u;
        for (unsigned i=0;i<sizeof(*this)-sizeof(checksum);++i) value=(value^bytes[i])*16777619u;
        return value;
    }
} nameMemory;
std::string namePath;
unsigned nameSlot=0;
ProfileName resultName{};
int resultRecord=-1;
bool resultQualified=false,nameEditing=false,resultMusic=false,recordsWriteFailed=false;
void initName(const char* executable);
void event(const char* log,const char* name);
bool (*worldOwnsRead)(unsigned)=nullptr;
void (*onWorldAwards)(unsigned,unsigned,bool)=nullptr;
Run run;
EndlessPresentation::State presentation;
EndlessParticles056::State demonParticles;
uint64_t resultVisualUs=0;
Course uniqueCourse{};
TileField uniqueWalls;
Game uniqueGame(uniqueCourse,&uniqueWalls);
Controls uniqueControls;
JumpRun jump;
EndlessObjects::State objects;
EndlessModifiers::State modifiers;
unsigned modifierBonus=0;
HoldToSkip skip;
EndlessKey::Trap trap;
std::array<bool,256> collected{};
std::array<SolidBlock,128> blocks{};
std::array<unsigned,128> blockItems{};
std::array<GhostMotion,16> ghosts{};
unsigned blockCount=0,ghostCount=0,level=0,levelTick=0,bonus=0,portalDelay=0,demonCooldown=0;
unsigned switchState=0;
BlockMask broken{};
bool active=false,paused=false,crossReleased=false,loadError=false,trapEnabled=false,scoresShown=false,inputCrossHeld=false;
uint64_t accumulator=0;
Vec2 demon{};
uint16_t* field=nullptr;

bool available() { return worldOwnsRead ? worldOwnsRead(30) : unlocked(progress); }
bool isReplay() { return active && run.active()<First; }

void init(const char* executable) {
    records={};recordSlot=0;recordsReadFailed=false;
    bool present=false,validRecord=false;
    recordsPath=statePath(executable,"endless.cfg");
    for (unsigned i=0;i<2;++i) {
        auto* f=std::fopen((recordsPath+(i?".1":".0")).c_str(),"rb");
        if (!f) {if (errno!=ENOENT) recordsReadFailed=true;continue;}
        present=true;
        Records entry;
        const bool ok=std::fread(&entry,sizeof entry,1,f)==1 && std::fgetc(f)==EOF &&
                      entry.magic==records.magic && entry.version==1 &&
                      entry.sequence>records.sequence && entry.count<=10 && entry.checksum==entry.hash();
        const bool readError=std::ferror(f)!=0;
        if (std::fclose(f)!=0 || readError) {recordsReadFailed=true;continue;}
        if (!ok) continue;
        bool valid=true;
        for (unsigned n=0;n<entry.count;++n) valid=valid && entry.entries[n].name[31]=='\0';
        if (valid) { records=entry; recordSlot=i;validRecord=true; }
    }
    recordsReadFailed=recordsReadFailed || (present && !validRecord);
    initName(executable);
}

VillaUnlocks::Flags unlockFlags() {
    using VillaUnlocks::Id;
    VillaUnlocks::Flags flags=0;
    if (records.unlocks&1) flags|=VillaUnlocks::bit(Id::Sticky);
    if (records.unlocks&2) flags|=VillaUnlocks::bit(Id::Missiles);
    if (records.unlocks&4) flags|=VillaUnlocks::bit(Id::Telekinesis);
    if (records.unlocks&8) flags|=VillaUnlocks::bit(Id::Catfish);
    return flags;
}

bool writeRecords() {
    if (records.sequence==UINT32_MAX) return false;
    ++records.sequence;
    records.checksum=records.hash();
    const unsigned slot=1-recordSlot;
    auto* file=std::fopen((recordsPath+(slot?".1":".0")).c_str(),"wb");
    if (!file) { --records.sequence; return false; }
    const bool written=std::fwrite(&records,sizeof records,1,file)==1;
    const bool closed=std::fclose(file)==0;
    if (written && closed) { recordSlot=slot; return true; }
    --records.sequence;
    return false;
}

const char* trackTitle() { return lang("Endless Shuffle","Бесконечное перемешивание"); }

void enter(const char* executable) {
    if (recordsPath.empty()) init(executable);
    scoresShown=loadError=false;
    presentation.menu(records.count!=0);
    loadError=!EndlessPresentationArt::load(executable,0) || !EndlessParticlesArt056::load(executable);
    audio.music(Sound::EndlessTitle);
}

void initName(const char* executable) {
    nameMemory={};nameSlot=0;resultName={};resultRecord=-1;
    resultQualified=nameEditing=resultMusic=recordsWriteFailed=false;
    // Upgrade only optional metadata; no rewrite of an existing v1 score file.
    nameMemory.count=records.count;
    for (unsigned i=0;i<records.count;++i)
        nameMemory.lowest=std::min(nameMemory.lowest,records.entries[i].score);
    namePath=statePath(executable,"endless-name.cfg");
    for (unsigned i=0;i<2;++i) {
        auto* file=std::fopen((namePath+(i?".1":".0")).c_str(),"rb");
        if (!file) continue;
        NameMemory entry;
        const bool ok=std::fread(&entry,sizeof entry,1,file)==1 && std::fgetc(file)==EOF &&
                      entry.magic==nameMemory.magic && entry.version==1 &&
                      entry.sequence>nameMemory.sequence && entry.name[31]=='\0' && entry.checksum==entry.hash();
        const bool readError=std::ferror(file)!=0;
        const bool closed=std::fclose(file)==0;
        if (ok && !readError && closed) { nameMemory=entry;nameSlot=i; }
    }
}

bool writeName() {
    if (nameMemory.sequence==UINT32_MAX) return false;
    ++nameMemory.sequence;
    nameMemory.checksum=nameMemory.hash();
    const unsigned slot=1-nameSlot;
    auto* file=std::fopen((namePath+(slot?".1":".0")).c_str(),"wb");
    if (!file) { --nameMemory.sequence;return false; }
    const bool written=std::fwrite(&nameMemory,sizeof nameMemory,1,file)==1;
    const bool closed=std::fclose(file)==0;
    if (written && closed) { nameSlot=slot;return true; }
    --nameMemory.sequence;
    return false;
}

bool qualifies(unsigned score) {
    // EndlessLose compares with all historical entries, rather than only the
    // displayed ten. Keeping their count/minimum preserves that decision even
    // after a low score drops out of the displayed personal top ten.
    if (std::max(nameMemory.count,records.count)+std::size(DeveloperRanks)<10) return true;
    if (score>nameMemory.lowest) return true;
    for (unsigned i=0;i<records.count;++i) if (score>records.entries[i].score) return true;
    for (const auto& entry:DeveloperRanks) if (score>entry.score) return true;
    return false;
}

void beginName(const char* log) {
    if (!resultQualified || nameEditing) return;
    nameEditing=profileKeyboard.begin(resultName,lang("Leaderboard name","Имя в таблице"),russian());
    if (!nameEditing) event(log,"endless_name_keyboard_error");
}

void finishName(const char* log) {
    if (!nameEditing || profileKeyboard.active()) return;
    nameEditing=false;
    char normalized[32]{};
    unsigned used=0,count=0;
    const auto* p=reinterpret_cast<const unsigned char*>(resultName.data());
    while (*p && count<12) {
        unsigned ch=*p++;
        if (ch>=0xc2 && ch<0xe0 && *p) ch=((ch&31)<<6)|(*p++&63);
        else if (ch>=0xe0 && ch<0xf0 && p[0] && p[1]) {
            const unsigned b=*p++,c=*p++;
            ch=((ch&15)<<12)|((b&63)<<6)|(c&63);
        }
        if (ch>='a' && ch<='z') ch-=32;
        else if (ch>=0x430 && ch<=0x44f) ch-=32;
        else if (ch==0x451) ch=0x401;
        const unsigned bytes=ch<0x80?1:ch<0x800?2:3;
        if (used+bytes>=sizeof normalized) break;
        if (bytes==1) normalized[used++]=static_cast<char>(ch);
        else if (bytes==2) {
            normalized[used++]=static_cast<char>(0xc0|(ch>>6));
            normalized[used++]=static_cast<char>(0x80|(ch&63));
        } else {
            normalized[used++]=static_cast<char>(0xe0|(ch>>12));
            normalized[used++]=static_cast<char>(0x80|((ch>>6)&63));
            normalized[used++]=static_cast<char>(0x80|(ch&63));
        }
        ++count;
    }
    resultName={};
    std::memcpy(resultName.data(),normalized,sizeof normalized);
    if (std::strcmp(nameMemory.name,normalized)==0) return;
    std::memcpy(nameMemory.name,normalized,sizeof normalized);
    if (resultRecord>=0) std::memcpy(records.entries[resultRecord].name,normalized,sizeof normalized);
    const bool savedRecords=resultRecord<0 || writeRecords();
    const bool savedName=writeName();
    recordsWriteFailed=!savedRecords || !savedName;
    event(log,recordsWriteFailed?"endless_records_error":"endless_name_saved");
}

bool resultReady() { return !nameEditing && (!resultQualified || resultName[0]); }

void event(const char* log,const char* name) {
    if (auto* out=std::fopen(log,"a")) {
        std::fprintf(out,"build=053END event=%s level=%u cleared=%u time=%u skips=%u score=%u quest=%u endless_key=%u\n",
                     name,run.active(),run.cleared(),run.time,run.skips,run.score,static_cast<unsigned>(progress.quest),static_cast<unsigned>(progress.endlessKey));
        std::fclose(out);
    }
}

const char* holeId(unsigned hole,char (&label)[16]) {
    if (hole<First) return CourseIds[hole];
    std::snprintf(label,sizeof label,"E_%02u",hole-First);
    return label;
}

bool loadUnique(const char* executable,unsigned index,bool alternate) {
    char name[64];
    std::snprintf(name,sizeof name,"data/world/endless-%02u%s.bin",index,alternate?"-alt":russian() && (index==0 || index==7)?"-ru":"");
    auto* f=std::fopen(companion(executable,name).c_str(),"rb");
    if (!f) return false;
    field=gameplayPage;
    titleReusedCourseVram=true;
    const bool loaded=readCourse(f,uniqueCourse) &&
        std::fread(field,512*272*sizeof(uint16_t),1,f)==1 && uniqueWalls.read(f);
    std::fclose(f);
    if (!loaded) return false;
    sceKernelDcacheWritebackRange(field,512*272*sizeof(uint16_t));
    return true;
}

Vec2 location(const Item& item) { return {item.x,item.y}; }

void stopModifierAudio() {
    audio.stop(Sound::EndlessAppear);
    audio.stop(Sound::EndlessRadAppear);
    audio.stop(Sound::EndlessMissileDing);
    audio.stop(Sound::EndlessBeeSwarm);
}

void clearReplayModifiers(Round& round) {
    if (!round.endlessReplay) return;
    round.endlessReplay=round.endlessPharaoh=false;
    round.bunkerJumpy={};
    round.ghost.panic=false;
    for (unsigned i=0;i<round.extraGhostCount;++i) round.extraGhosts[i].panic=false;
}

// Keep both input owners clear across Endless UI and level handoffs.
void clearControls(Round& round,bool heldCross) {
    uniqueControls.cancel(heldCross);
    round.controls.cancel(heldCross);
    crossReleased=!heldCross;
}

// Immediate and buffered releases share exactly one Endless shot notification.
void shotEffects(bool replay) {
    if (!replay) objects.coinMultiplier=objects.blockMultiplier=0;
    audio.play(Sound::Shot);
}

bool openHole(const char* executable,Course& course,Round& round,const char* log,bool newHole) {
    if (!EndlessParticlesArt056::load(executable)) return false;
    clearReplayModifiers(round);
    clearControls(round,inputCrossHeld);
    active=true;
    speech059.clearSpeech();
    paused=false;
    crossReleased=false;
    levelTick=bonus=modifierBonus=portalDelay=demonCooldown=0;
    audio.stop(Sound::EndlessMissileDing);
    accumulator=0;
    skip.cancel();
    level=run.active();
    if (newHole) run.updateModifiers(EndlessModifiers::blacklist(level));
    if (run.mods && !EndlessModifierArt::load(executable)) return false;
    if (isReplay()) {
        const Progress saved=progress;
        const bool loaded=enterCourse(executable,level,course,round);
        progress=saved;
        if (!loaded) return false;
        thoughts.begin(round.tvTick());
        demon=course.goal;
        round.endlessReplay=true;
        round.bunkerJumpy.enabled=(run.mods&Jumpy)!=0;
        round.bunkerJumpy.lastGroundPosition={};
        round.endlessPharaoh=(run.mods&Pharaoh)!=0;
        round.ghost.panic=(run.mods&Panic)!=0;
        for (unsigned i=0;i<round.extraGhostCount;++i) round.extraGhosts[i].panic=round.ghost.panic;
    } else {
        clips.resetTextureWorkspace();
        // Costume decoding and unique atlases share fallbackPage.
        Costumes054::prepareGameplay();
        if (!loadUnique(executable,level-First,false) ||
            (level==First+5 && !EndlessRotating::load(executable)) ||
            (level==E04 && !EndlessE04Art::load(executable)) ||
            !EndlessObjectsArt::load(executable,level-First)) return false;
        const auto& stage=EndlessGeometry::levels[level-First];
        if (stage.count>collected.size()) return false;
        collected.fill(false);
        blockCount=ghostCount=switchState=0;
        broken={};
        trap={};
        trapEnabled=level==E04 && progress.quest>=2 && !progress.endlessKey;
        uniqueGame.restart();
        objects.restart(stage,level-First,uniqueCourse.fieldSize*.5f);
        uniqueGame.goalEnabled=true;
        uniqueControls={};
        jump={};
        for (unsigned i=0;i<stage.count;++i) {
            const auto& item=stage.items[i];
            if ((item.kind==Kind::Block || item.kind==Kind::Door || item.kind==Kind::Switch) &&
                blockCount<blocks.size()) {
                blocks[blockCount]={location(item),{item.a,item.b}};
                blockItems[blockCount++]=i;
            }
            if (item.kind==Kind::Ghost && ghostCount<ghosts.size()) ghosts[ghostCount++]={location(item),{},(run.mods&Panic)!=0,ActorPronouns054::session.ready(ActorPronouns054::Kind::Ghost)};
        }
        demon=uniqueCourse.goal;
    }
    modifiers.reset(run.mods,run.random,isReplay()?round.game.position:uniqueGame.position);
    ++modifierHudEpoch;
    demonParticles.reset(run.random ^ (level*0x9e3779b9u),run.radical);
    if (run.demonActive) {
        if (!EndlessPresentationArt::load(executable,run.radical?2:1)) return false;
        if (!presentation.demonSeen) {
            if (!EndlessPresentationArt::prepareCapture()) return false;
            const auto& scene=isReplay()?course:uniqueCourse;
            const float zoom=512/(scene.bottom-scene.top);
            const Vec2 center=isReplay()?Vec2{}:Vec2{scene.fieldLeft+scene.fieldSize*.5f,scene.fieldTop+scene.fieldSize*.5f};
            presentation.random=run.random?run.random:1;
            presentation.startIntro(zoom,center,demon);
            audio.play(run.radical?Sound::EndlessRadAppear:Sound::EndlessAppear);
        }
    } else presentation.demonSeen=false;
    if (run.mods&Bees) audio.play(Sound::EndlessBeeSwarm);
    else audio.stop(Sound::EndlessBeeSwarm);
    if (run.mods&Pharaoh) audio.play(Sound::EndlessCurse);
    audio.stageCue(StageCue::None);
    audio.play(Sound::Join);
    clearControls(round,inputCrossHeld);
    event(log,"endless_hole");
    return true;
}

Sound runMusic() { return !run.demonActive?Sound::EndlessMusic:run.radical?Sound::EndlessRad:Sound::EndlessDemon; }

void chooseFailure(Flow& flow,const char* log) {
    EndlessPresentationArt::releaseCapture();
    audio.stop(Sound::EndlessHurry);
    stopModifierAudio();
    active=false;
    loadError=true;
    flow.screen=Screen::EndlessMenu;
    audio.music(Sound::EndlessTitle);
    event(log,"endless_asset_error");
}

void closeRun(Flow& flow,const char* log) {
    EndlessPresentationArt::releaseCapture();
    stopModifierAudio();
    event(log,"endless_time_up");
    active=false;
    flow.screen=Screen::EndlessResult;
    resultVisualUs=0;
    scoresShown=false;
    resultMusic=false;
    resultRecord=-1;
    nameEditing=false;
    recordsWriteFailed=false;
    audio.stop(Sound::EndlessHurry);
    audio.music(Sound::EndlessLose);
    Rank candidate{run.score,run.cleared(),{}};
    resultQualified=qualifies(run.score);
    resultName={};
    std::memcpy(resultName.data(),nameMemory.name,sizeof nameMemory.name);
    std::memcpy(candidate.name,nameMemory.name,sizeof candidate.name);
    if (onWorldAwards) onWorldAwards(run.progressForUnlock(),run.score,true);
    if (run.score>500000) records.unlocks|=8;
    if (resultQualified && (records.count<10 || candidate.score>records.entries[9].score)) {
        const unsigned last=std::min(records.count,9u);
        unsigned i=last;
        while (i && records.entries[i-1].score<candidate.score) {
            records.entries[i]=records.entries[i-1];
            --i;
        }
        records.entries[i]=candidate;
        resultRecord=static_cast<int>(i);
        records.count=std::min(10u,records.count+1);
    }
    if (resultQualified) {
        if (nameMemory.count<UINT32_MAX) ++nameMemory.count;
        nameMemory.lowest=std::min(nameMemory.lowest,run.score);
    }
    const bool savedRecords=writeRecords();
    const bool savedName=!resultQualified || writeName();
    recordsWriteFailed=!savedRecords || !savedName;
    if (recordsWriteFailed) event(log,"endless_records_error");
}

void advance(Flow& flow,const char* executable,Course& course,Round& round,const char* log,
             Advance how,unsigned score=0,unsigned onTime=0) {
    clearControls(round,inputCrossHeld);
    const auto music=runMusic();
    const unsigned previousSkips=run.skips;
    if (!run.transition(how,score,onTime)) return;
    const unsigned count=run.progressForUnlock(), old=records.unlocks;
    if (onWorldAwards) onWorldAwards(count,run.score,false);
    if (count>=50) records.unlocks|=1;
    if (count>=100) records.unlocks|=2;
    if (count>=150) records.unlocks|=4;
    if (old!=records.unlocks && !writeRecords()) event(log,"endless_records_error");
    if (how==Advance::Skip) audio.play(Sound::Skip);
    if (how==Advance::Win && run.skips>previousSkips) {
        presentation.extraSkip();
        audio.playTuned(Sound::EndlessExtraSkip,4,1);
    }
    if (how!=Advance::Reset) audio.play(Sound::EndlessPass);
    if (run.time>600) audio.stop(Sound::EndlessHurry);
    event(log,how==Advance::Win?"endless_pass":how==Advance::Reset?"endless_restart":"endless_skip");
    if (!openHole(executable,course,round,log,how!=Advance::Reset)) chooseFailure(flow,log);
    else if (music!=runMusic() && !presentation.frozen()) audio.music(runMusic());
}

bool area(Vec2 p,const Item& item,const EndlessGeometry::Level& stage) {
    const unsigned count=static_cast<unsigned>(item.a);
    if (count<3 || item.arg<0 || static_cast<unsigned>(item.arg)+count>stage.pointCount) return false;
    const auto* points=stage.points+item.arg;
    bool inside=false;
    for (unsigned i=0,j=count-1;i<count;j=i++) {
        const auto& a=points[i];
        const auto& b=points[j];
        if ((a.y>p.y)!=(b.y>p.y) && p.x<(b.x-a.x)*(p.y-a.y)/(b.y-a.y)+a.x) inside=!inside;
    }
    return inside;
}

bool demonStep(Vec2 p,const TileField&) {
    if (!run.demonActive) return false;
    if (demonCooldown) --demonCooldown;
    const Vec2 offset=p-demon;
    const float distance=length(offset);
    if (distance>0.01f) {
        // Original RedDemon has collision_mask=0 and pursues through walls.
        demon=demon+offset*((65.0f/60)/distance);
    }
    presentation.trail(demon);
    if (demonCooldown || distance>28) return false;
    demonCooldown=60;
    run.demonHit();
    presentation.caught();
    EndlessPresentationArt::prepareCapture();
    audio.play(run.radical?Sound::EndlessRadCaught:Sound::EndlessCaught);
    return true;
}

bool uniqueStep(const char* executable,SaveFile& save,const char* log) {
    const auto& stage=EndlessGeometry::levels[level-First];
    const Vec2 before=uniqueGame.position;
    const auto bounces=uniqueGame.bounces;
    bool airborne=jump.advance();
    if (!airborne && modifiers.uniqueJumpy.enabled) {
        const auto cue=modifiers.uniqueJumpy.step(uniqueGame,uniqueCourse,uniqueWalls,modifiers.damping(false));
        if (cue==BunkerJumpy::Event::WallStuck || cue==BunkerJumpy::Event::OutOfBounds) return false;
        airborne=cue!=BunkerJumpy::Event::Grounded;
        if (cue==BunkerJumpy::Event::Takeoff) audio.play(Sound::EndlessModJump);
    }
    objects.move(uniqueWalls,uniqueGame.position,blocks.data(),blockCount,broken,levelTick);
    int impact=-1;
    if (airborne) {
        if (jump.airborne()) jump.move(uniqueGame);
    } else {
        const float damping=modifiers.damping(false);
        if (damping!=1) uniqueGame.velocity=uniqueGame.velocity*damping;
        const bool rotating=level==First+5;
        if (rotating) {
            EndlessRotating::tick(levelTick);
            if (!uniqueGame.moving()) uniqueGame.position=uniqueGame.position+
                EndlessRotating::recover(uniqueGame.position,uniqueCourse.playerRadius);
        }
        impact=uniqueGame.step(blocks.data(),blockCount,broken,objects.enemies,objects.enemyCount,nullptr,0,0,
            objects.cars,objects.carCount,nullptr,0,nullptr,nullptr,nullptr,0,{},false,rotating?EndlessRotating::sweep:nullptr);
    }
    if (impact>=0 && static_cast<unsigned>(impact)<blockCount && uniqueGame.bounces!=bounces) {
        const unsigned object=blockItems[impact];
        const Item& item=stage.items[object];
        if (item.kind==Kind::Switch) {
            switchState^=1;
            if (!loadUnique(executable,level-First,switchState)) return false;
            audio.play(Sound::ParkSwitch);
        } else if (item.kind==Kind::Block || objects.open(stage,item,levelTick,bonus)) {
            broken.set(static_cast<unsigned>(impact));
            collected[object]=true;
            if (item.kind==Kind::Block) objects.breakBlock(item,bonus);
            audio.play(item.kind==Kind::Block?Sound::BlockBreak:Sound::DoorOpen);
        } else audio.play(Sound::Bounce);
    } else if (impact>=0 && uniqueGame.bounces!=bounces) {
        switch (objects.defeat(static_cast<unsigned>(impact)-blockCount,levelTick,bonus)) {
        case EndlessObjects::Enemy::Skull: audio.play(Sound::SkullDeath); break;
        case EndlessObjects::Enemy::Chuckle: audio.play(Sound::ChuckleDeath); break;
        case EndlessObjects::Enemy::Car: audio.play(Sound::CarDeath); break;
        case EndlessObjects::Enemy::None: audio.play(Sound::Bounce); break;
        }
    } else if (uniqueGame.bounces!=bounces) audio.play(Sound::Bounce);
    objects.updateKeys(uniqueGame.position,levelTick);
    if (run.mods&Sticky && uniqueGame.fixedTileBounce) uniqueGame.velocity={};
    if (portalDelay) --portalDelay;
    bool teleported=false;
    for (unsigned i=0;i<stage.count;++i) {
        if (airborne || collected[i]) continue;
        const auto& item=stage.items[i];
        const Vec2 p=uniqueGame.position,at=location(item);
        if (item.kind==Kind::Sand || item.kind==Kind::Water) {
            if (!area(p,item,stage)) continue;
            if (item.kind==Kind::Water && !canSwim) return false;
            if (item.kind==Kind::Sand && !(run.mods&Pharaoh))
                uniqueGame.velocity=uniqueGame.velocity*(level==E04?.94f:.85f);
            continue;
        }
        if (item.kind==Kind::Ghost || item.kind==Kind::Hint || item.kind==Kind::Block ||
            item.kind==Kind::Spawner || item.kind==Kind::Door || item.kind==Kind::Switch ||
            item.kind==Kind::Skull || item.kind==Kind::Chuckle) continue;
        const float radius=(item.kind==Kind::JumpPad?10.0f:item.a>0?item.a:12.0f)+uniqueCourse.playerRadius;
        const bool contact=item.kind==Kind::Portal?EndlessKey::Trap::collides(before,p,at,item.a,item.b,uniqueCourse.playerRadius):EndlessKey::touch(before,p,at,radius);
        if (!contact) continue;
        switch (item.kind) {
        case Kind::Portal:
            if (!portalDelay && item.arg>=0 && static_cast<unsigned>(item.arg)<stage.count) {
                uniqueGame.position=location(stage.items[item.arg]);
                portalDelay=3;
                teleported=true;
                audio.play(Sound::Portal);
            }
            break;
        case Kind::JumpPad:
            if (!portalDelay && uniqueGame.moving()) {
                const float speed=(item.b>20?item.b:std::max(length(uniqueGame.velocity),20.0f))+10;
                uniqueGame.position=at;
                uniqueGame.velocity={std::cos(item.a)*speed,std::sin(item.a)*speed};
                jump.timer=1;
                portalDelay=3;
                audio.play(Sound::JumpPad);
            }
            break;
        case Kind::SpeedToken: {
            const float speed=length(uniqueGame.velocity);
            if (speed>0) uniqueGame.velocity=uniqueGame.velocity*(item.b/speed);
            collected[i]=true;
            audio.play(Sound::BigCoin);
            break;
        }
        case Kind::StopToken:
            uniqueGame.velocity={};
            collected[i]=true;
            audio.play(Sound::StopToken);
            break;
        case Kind::Coin:
            objects.coin(item,bonus);
            collected[i]=true;
            audio.play(Sound::Coin);
            break;
        case Kind::Key:
            if (objects.pickUp(stage,i,bonus)) {
                collected[i]=true;
                audio.play(Sound::KeyCollect);
            }
            break;
        default:
            break;
        }
    }
    const Vec2 p=uniqueGame.position;
    if (p.x<uniqueCourse.left-52 || p.y<uniqueCourse.top-52 ||
        p.x>uniqueCourse.right+52 || p.y>uniqueCourse.bottom+52) return false;
    for (unsigned i=0;i<ghostCount;++i)
        if (ghosts[i].chase(p) && !airborne && !jump.airborne()) return false;
    if (!airborne && !jump.airborne()) demonStep(p,uniqueWalls);
    if (trapEnabled) {
        switch (trap.tick(teleported?p:before,p,uniqueCourse.playerRadius,!airborne && !jump.airborne(),uniqueGame.velocity.x)) {
        case EndlessKey::Trap::Event::Started:
            audio.play(Sound::EndlessCrush);
            event(log,"endless_key_trap_started");
            break;
        case EndlessKey::Trap::Event::Fake:
            audio.play(Sound::EndlessFakeKey);
            event(log,"endless_key_fake");
            break;
        case EndlessKey::Trap::Event::Dead:
            audio.play(Sound::EndlessTrapDeath);
            event(log,"endless_key_trap_death");
            return false;
        case EndlessKey::Trap::Event::Award:
            if (activeProfile>=Profiles::Count) return false;
            progress.endlessKey=1;
            if (!save.write(progress)) {
                progress.endlessKey=0;
                event(log,"endless_key_save_error");
                return false;
            }
            xmbProfile.queue(activeProfile,profiles.entry(activeProfile),progress);
            audio.play(Sound::SecretKey);
            event(log,"endless_key_awarded");
            break;
        case EndlessKey::Trap::Event::None: break;
        }
    }
    return true;
}

void step(Flow& flow,const char* executable,Course& course,Round& round,SaveFile& save,
          const char* log,Vec2 stick,unsigned held,unsigned pressed,uint64_t delta,uint64_t now) {
    inputCrossHeld=(held&PSP_CTRL_CROSS)!=0;
    if (flow.screen==Screen::EndlessMenu) {
        presentation.advance(delta);
        if (pressed&PSP_CTRL_CIRCLE) { EndlessPresentationArt::releaseWorkspace();flow.screen=Screen::GameMenu; return; }
        if (pressed&PSP_CTRL_TRIANGLE) {presentation.previousPage=presentation.page=0;presentation.slideUs=250000;}
        else if (pressed&(PSP_CTRL_LEFT|PSP_CTRL_UP)) presentation.turn(-1);
        else if (pressed&(PSP_CTRL_RIGHT|PSP_CTRL_DOWN)) presentation.turn(1);
        if (!(pressed&PSP_CTRL_CROSS)) return;
        presentation={};
        run.begin(static_cast<uint32_t>(now));
        if (!openHole(executable,course,round,log,true)) { chooseFailure(flow,log); return; }
        flow.screen=Screen::EndlessHole;
        audio.music(Sound::EndlessMusic);
        return;
    }
    if (flow.screen==Screen::EndlessResult) {
        presentation.advance(delta);
        resultVisualUs=std::min<uint64_t>(8000000,resultVisualUs+delta);
        finishName(log);
        if (!resultMusic && audio.musicFinished(Sound::EndlessLose)) {
            resultMusic=true;
            audio.music(Sound::EndlessResults);
        }
        if (nameEditing) return;
        if (resultQualified && (pressed&PSP_CTRL_SQUARE)) { beginName(log);return; }
        if (!resultReady()) return;
        if (pressed&PSP_CTRL_CIRCLE) { loadError=!EndlessPresentationArt::load(executable,0); flow.screen=Screen::EndlessMenu; }
        else if (pressed&PSP_CTRL_CROSS) {
            presentation={};
            run.begin(static_cast<uint32_t>(now));
            if (!openHole(executable,course,round,log,true)) { chooseFailure(flow,log);return; }
            flow.screen=Screen::EndlessHole;
            audio.music(Sound::EndlessMusic);
        }
        return;
    }
    if (flow.screen!=Screen::EndlessHole) return;
    if (pressed&PSP_CTRL_START) { paused=!paused;clearControls(round,inputCrossHeld);audio.play(Sound::Pause); }
    if (pressed&PSP_CTRL_CIRCLE) {
        clearControls(round,inputCrossHeld);
        clearReplayModifiers(round);
        stopModifierAudio();
        EndlessPresentationArt::releaseCapture();
        active=false;
        audio.stop(Sound::EndlessHurry);
        loadError=!EndlessPresentationArt::load(executable,0);
        flow.screen=Screen::EndlessMenu;
        return;
    }
    // Original intro sets demon PROCESS_MODE_ALWAYS. Manual pause freezes it;
    // run time remains frozen during intro, particles still receive real delta.
    if (run.demonActive) demonParticles.advance(delta,demon,paused);
    const bool introWasFrozen=presentation.frozen();
    delta=presentation.advance(delta,paused);
    if (presentation.introFinished) {
        audio.stop(Sound::EndlessAppear);audio.stop(Sound::EndlessRadAppear);
        audio.music(runMusic());
        clearControls(round,inputCrossHeld);
    }
    if (!presentation.camera() && !presentation.scareUs) EndlessPresentationArt::releaseCapture();
    if (paused || (introWasFrozen && !delta)) {clearControls(round,inputCrossHeld);return;}
    speech059.tick(static_cast<unsigned>(delta));
    if (pressed&PSP_CTRL_TRIANGLE) { clearControls(round,inputCrossHeld);advance(flow,executable,course,round,log,Advance::Reset); return; }
    if (!(held&PSP_CTRL_CROSS)) crossReleased=true;
    const bool cross=crossReleased && (held&PSP_CTRL_CROSS);
    modifiers.moveCursor(stick,delta);
    if (cross && (pressed&PSP_CTRL_CROSS)) {
        const unsigned caught=modifiers.bash();
        if (caught) {
            modifierBonus+=100*caught;
            audio.play(Sound::EndlessBortOw);
        }
    }
    uniqueControls.setPhysicsFrame(levelTick);
    const auto action=isReplay()?round.updateControls(stick,cross,false):
                                 uniqueControls.update(uniqueGame,uniqueCourse,stick,cross,false,false,nullptr,shotSettings);
    if (action==ControlEvent::Shot) shotEffects(isReplay());
    accumulator+=delta*60;
    while (accumulator>=1000000) {
        accumulator-=1000000;
        if (skip.update(held&PSP_CTRL_SELECT) && run.skips) {
            clearControls(round,inputCrossHeld);
            advance(flow,executable,course,round,log,Advance::Skip);
            return;
        }
        const bool replay=isReplay();
        ++levelTick;
        run.tick(replay || !trapEnabled || !trap.freezesClock());
        if (run.time==600) audio.play(Sound::EndlessHurry);
        if (run.time && run.time<=600) audio.retune(Sound::EndlessHurry,0,EndlessPresentation::State::hurryPitch(run.time));
        if (!run.time) {
            clearControls(round,inputCrossHeld);
            clearReplayModifiers(round);
            closeRun(flow,log);
            return;
        }
        bool alive=true;
        auto idle059=replay?idleRound059(round):AvatarIdle059::Context{};
        if(!replay){idle059.active=true;idle059.spawning=levelTick<61;idle059.moving=uniqueGame.moving();idle059.airborne=jump.airborne()||EndlessModifiers::jumpInFlight(modifiers.uniqueJumpy);idle059.won=uniqueGame.won;}
        AvatarIdle059::step(AvatarIdle059::cosmetic,idle059,speech059);
        if (run.mods&Dizzy && (replay || !jump.airborne())) {
            auto& velocity=replay?round.game.velocity:uniqueGame.velocity;
            constexpr float angle=1.309f/60;
            const float c=std::cos(angle),sin=std::sin(angle);
            velocity={velocity.x*c-velocity.y*sin,velocity.x*sin+velocity.y*c};
        }
        if (replay) {
            const Progress saved=progress;
            const auto bounces=round.game.bounces;
            round.step(held&PSP_CTRL_RTRIGGER,false,false);
            if (run.mods&Sticky && round.game.fixedTileBounce) round.game.velocity={};
            if (bounces!=round.game.bounces) audio.play(Sound::Bounce);
            progress=saved;
            alive=!round.caught && !round.waterDead;
            demonStep(round.game.position,walls);
        } else alive=uniqueStep(executable,save,log);
        const auto& game=replay?round.game:uniqueGame;
        const auto& config=replay?course:uniqueCourse;
        const bool aboveGround=replay?round.jump.airborne() || EndlessModifiers::jumpInFlight(round.bunkerJumpy):
                                    jump.airborne() || EndlessModifiers::jumpInFlight(modifiers.uniqueJumpy);
        const unsigned hazards=modifiers.tick(game.position,config.playerRadius,!aboveGround);
        if (hazards&EndlessModifiers::MissileWarning) audio.play(Sound::EndlessMissileDing);
        if (hazards&EndlessModifiers::MissileBlast) {
            audio.stop(Sound::EndlessMissileDing);
            audio.play(Sound::EndlessMissileBoom);
        }
        if (run.mods&Panic) {
            const unsigned count=replay?1+round.extraGhostCount:ghostCount;
            for (unsigned i=0;i<count;++i) {
                const auto& ghost=replay?(i?round.extraGhosts[i-1]:round.ghost):ghosts[i];
                modifiers.recordGhost(i,ghost.position,ghost.velocity);
            }
        }
        if (!alive || (!game.won && (hazards&EndlessModifiers::MissileHit))) {
            clearControls(round,inputCrossHeld);
            advance(flow,executable,course,round,log,Advance::Reset);
            return;
        }
        if (game.won) {
            clearControls(round,inputCrossHeld);
            if (replay) bonus=round.bonus();
            const unsigned onTime=static_cast<unsigned>((replay?walls:uniqueWalls).onTime*60+.5f);
            advance(flow,executable,course,round,log,Advance::Win,EndlessObjects::score(config.par,game.strokes,onTime,levelTick,bonus+modifierBonus),onTime);
            return;
        }
        // The fixed-step clock, not render calls, defines the strict12-tick buffer.
        // Observe every tick even without pending input to track first-moving-frame.
        if (shotSettings.buffer) {
            if (aboveGround) clearControls(round,inputCrossHeld);
            else {
                const auto buffered=replay?round.consumeBufferedControls():
                    uniqueControls.consumeBuffered(uniqueGame,uniqueCourse,levelTick);
                if (buffered==ControlEvent::Shot) shotEffects(replay);
            }
        }
    }
}

void drawChoice(bool selected) {
    rectangle(ViewX+165,139,215,82,GU_RGBA(24,15,40,244));
    rectangle(ViewX+170,selected?179:146,205,30,GU_RGBA(92,57,119,255));
    localeText(lang("Campaign","Кампания"),ViewX+272,149,.86f,selected?White:Gold,true);
    localeText(trackTitle(),ViewX+272,182,.78f,selected?Gold:White,true,193,true);
}

void drawHud(bool hasBase=false) {
    // Master remains beneath EndlessHUD (CanvasLayer6). Replays already drew
    // their course HUD, including the special moving FakeThinker when present.
    if (!isReplay() && !hasBase) UniqueHud057::drawMaster();
    constexpr float S=GameplayView::RasterScale;
    namespace Hud=EndlessHudLayout066;
    char value[40],label[16];
    if (isReplay() && UniqueHud057::prepared) {
        // Replays just drew the campaign score/strokes/par labels. Restore only
        // the expanded Endless text band from the retained original Master;
        // leave its live TV, Thinker, border and all field pixels in place.
        setTexture(GU_PSM_5650,MenuScroll::LargeWidth,512,UniqueHud057::sidebar);
        hudPatch({0,63,134,107,0,63});
    }
    UniqueHud057::timer(static_cast<unsigned>(presentation.clockUs*60/1000000));
    UniqueHud057::demon(float(DemonMax-run.demon)/DemonMax,run.demonActive);
    const unsigned time=(run.time+59)/60;
    std::snprintf(value,sizeof value,"%u:%02u",time/60,time%60);
    localeText(value,(184+256)*.5f*S,342*S,SmallUiText,time<=10?GU_RGBA(255,95,100,255):White,true,72*S,true);
    localeText(lang("Total Score:","Общий счёт:"),Hud::Left,Hud::ScoreHeaderY,SmallUiText);
    std::snprintf(value,sizeof value,"%u !",run.score);
    localeText(value,Hud::Left,Hud::ScoreY,SmallUiText);
    const char* clearedLabel=lang("Levels Cleared:","Пройдено:");
    std::snprintf(value,sizeof value,"%u",run.cleared());
    const auto rows=Hud::rows(localeTextWidth(clearedLabel),localeTextWidth(value),localeTextWidth(" "),SmallUiText);
    localeText(clearedLabel,Hud::Left,Hud::ClearedY,SmallUiText);
    const float clearedX=rows.inlineCleared?Hud::Left+(localeTextWidth(clearedLabel)+localeTextWidth(" "))*SmallUiText:Hud::Left;
    localeText(value,clearedX,rows.clearedValueY,SmallUiText);
    char next[64];
    std::snprintf(next,sizeof next,"%s %s",lang("Up Next:","Далее:"),holeId(run.next(),label));
    localeText(next,Hud::Left,rows.nextY,SmallUiText);
    std::snprintf(value,sizeof value,"%u",run.skips);
    const float skipScale=SmallUiText*presentation.skipScale();
    localeText(value,Hud::skipCenter(localeTextWidth(value)*skipScale,202*S),Hud::SkipY,skipScale,White,true);
    if (presentation.flashUs) {
        const float age=1-presentation.flashUs/1000000.f;
        localeText("+1",224*S,(392-24*age)*S,.27f,GU_RGBA(89,255,227,static_cast<unsigned>(255*(1-age))));
    }
    // Original modifier grid is presented once by the shared frame HUD.
    if (level>=First) {
        const auto& stage=EndlessGeometry::levels[level-First];
        // Bound overflow to its text viewport; long run counters retain every
        // digit at campaign size instead of squeezing all fields smaller.
        sceGuScissor(ViewX+14,135,ViewX+134,Hud::NameBottom);
        localeParagraph(lang(stage.en,stage.ru),Hud::NameCenter,rows.nameY,SmallUiText,Hud::NameWidth,Hud::NameLeading,true);
        sceGuScissor(0,0,Width,Height);
    }
    if (skip.progress()) rectangle(ViewX+12,264,112.f*skip.progress()/HoldToSkip::RequiredTicks,3,Gold);
    if (paused) Theme067::pause(russian(),512*S,240*S);
}

Vec2 project(Vec2 p) {
    const float scale=272/uniqueCourse.fieldSize;
    return {ViewX+136+(p.x-uniqueCourse.fieldLeft)*scale,(p.y-uniqueCourse.fieldTop)*scale};
}

bool drawE04Item(const Item& item) {
    using namespace EndlessE04Art;
    unsigned icon;
    switch (item.kind) {
    case Kind::Hint:
        if (!trapEnabled || !trap.visible()) return true;
        icon=Hint;
        break;
    case Kind::Portal:
        icon=(item.color==0xffffffffu?PortalBlue:PortalRed)+(item.arg<0);
        break;
    case Kind::JumpPad: icon=Jump; break;
    case Kind::SpeedToken: icon=Speed; break;
    case Kind::Block: icon=Breakable; break;
    default: return false;
    }
    return draw(icon,project(location(item)),272/uniqueCourse.fieldSize,levelTick);
}

void drawPanicTrails(const GameplayView& view) {
    if (!(run.mods&Panic)) return;
    Costumes054::bindActorTexture();
    for (const auto& images:modifiers.trails)
        for (const auto& trail:images)
            if (const unsigned alpha=trail.opacity(modifiers.ticks))
                Costumes054::drawActor(view,true,trail.position,view.worldZoom(),GU_RGBA(255,255,255,alpha));
}

void drawDemon(const GameplayView& view) {
    if (!run.demonActive) return;
    EndlessParticlesArt056::draw(demonParticles,view,demon);
    const float time=presentation.clockUs/1000000.f;
    const Vec2 shake{std::sin(24*time)*4.096f,std::sin(21*time)*4.096f};
    for (const auto& trail:presentation.trails) if (trail.age<20) {
        const unsigned alpha=static_cast<unsigned>((1-trail.age*.05f)*255*presentation.opacity());
        EndlessPresentationArt::demon(view,trail.position+trail.drift*static_cast<float>(trail.age)+shake,
                                     run.radical,trail.scale,GU_RGBA(255,255,255,alpha));
    }
    const unsigned alpha=static_cast<unsigned>(255*presentation.opacity());
    EndlessPresentationArt::demon(view,demon+shake,run.radical,1,GU_RGBA(255,255,255,alpha));
}

void drawReplayEffects(const Course& course) {
    const GameplayView view(course,ViewX);
    EndlessPresentationArt::effects(presentation,view,run.radical);
}

void drawItems() {
    const auto& stage=EndlessGeometry::levels[level-First];
    const Vec2 center{uniqueCourse.fieldLeft+uniqueCourse.fieldSize*.5f,uniqueCourse.fieldTop+uniqueCourse.fieldSize*.5f};
    const GameplayView view(uniqueCourse,ViewX,false,(uniqueCourse.bottom-uniqueCourse.top)/uniqueCourse.fieldSize,center*(-1));
    EndlessObjectsArt::drawItems(view,stage,objects,collected,levelTick,switchState,level==E04);
    if (level==E04)
        for (unsigned i=0;i<stage.count;++i)
            if (!collected[i]) drawE04Item(stage.items[i]);
    ActorLabelDraw054::flush();
    EndlessObjectsArt::drawFlag(view,uniqueCourse.goal,levelTick);
    Costumes054::bindActorTexture();
    drawPanicTrails(view);
    for (unsigned i=0;i<ghostCount;++i) {
        Costumes054::drawActor(view,true,ghosts[i].position,view.worldZoom(),White);
        ActorLabelDraw054::add(ghosts[i].label,view,ghosts[i].position,ActorPronouns054::Kind::Ghost);
    }
    if (ActorLabelDraw054::flush()) Costumes054::bindActorTexture();
    Costumes054::drawActor(view,false,uniqueGame.position,view.worldZoom()*
        (jump.scale/JumpRun::BaseScale)*EndlessModifiers::jumpScale(modifiers.uniqueJumpy),White);
    if (uniqueControls.aiming)
        aimLine(view,uniqueGame.position,uniqueControls.drag,length(uniqueControls.drag)>=uniqueCourse.minDrag);
    drawDemon(view);
    EndlessModifierArt::drawWorld(modifiers,view,uniqueGame.position,modifiers.uniqueJumpy);
    if (trapEnabled && trap.visible()) {
        using namespace EndlessE04Art;
        const float scale=272/uniqueCourse.fieldSize;
        if (trap.stage!=EndlessKey::Trap::Stage::Final)
            draw(Key,project(trap.key),scale,levelTick);
        draw(Crusher,project(trap.crusher1),scale,levelTick);
        draw(WideCrusher,project(trap.crusher2),scale,levelTick);
        if (trap.stage==EndlessKey::Trap::Stage::KeyTwo && trap.sweep<=540)
            draw(AntiCheat,project({EndlessKey::Trap::motion(-488,440,trap.sweep,540),-151}),scale,levelTick);
    }
}

void endlessText(const char* text,float x,float y,float scale=1.0f,uint32_t color=EndlessUi067::Green,bool centered=false,unsigned maxWidth=0,bool keepHeight=false) {
    EndlessUi067::text(0,text,x,y,scale,color,centered,maxWidth,keepHeight);
}
void endlessButtonText(const char* text,float x,float y,float scale=1.0f,uint32_t color=EndlessUi067::Green,bool centered=false,unsigned maxWidth=0,bool keepHeight=false) {
    EndlessUi067::text(1,text,x,y,scale,color,centered,maxWidth,keepHeight);
}
void endlessParagraph(const char* text,float x,float y,float scale,unsigned width,float leading,bool centered=false) {
    EndlessUi067::paragraph(text,x,y,scale,width,leading,centered);
}

void drawRecords(float offset=0) {
    Rank table[16]{};
    std::copy_n(records.entries,records.count,table);
    std::copy(std::begin(DeveloperRanks),std::end(DeveloperRanks),table+records.count);
    const unsigned count=records.count+std::size(DeveloperRanks);
    // Stable insertion sort avoids allocating a temporary array on PSP Fat.
    for (unsigned i=1;i<count;++i) {
        const auto entry=table[i];
        unsigned j=i;
        while (j && table[j-1].score<entry.score) { table[j]=table[j-1]; --j; }
        table[j]=entry;
    }
    for (unsigned i=0;i<std::min(10u,count);++i) {
        char value[65];
        std::snprintf(value,sizeof value,"%2u   %-18s   %8u   %3u",i+1,table[i].name,table[i].score,table[i].cleared);
        endlessText(value,272+offset,65+14*i,.76f,EndlessUi067::Green,true,260,true);
    }
}

void drawMenuPage(unsigned page,float offset) {
    if (!page) {drawRecords(offset);return;}
    EndlessPresentationArt::diagram(page-1,ViewX+272-42.5f+offset,51);
    endlessParagraph(lang(EndlessPresentation::RulesEN[page-1],EndlessPresentation::RulesRU[page-1]),272+offset,148,.65f,230,10,true);
}

void drawGrid(float opacity=1) {
    rectangle(ViewX+136,0,272,272,GU_RGBA(0,0,0,255));
    const float time=presentation.clockUs/1000000.f;
    for (unsigned layer=0;layer<2;++layer) {
        const float speedX=layer?3:-3,speedY=layer?6:-1.5f;
        const auto color=GU_RGBA(0,static_cast<unsigned>((layer?82:31)*opacity),0,255);
        for (unsigned x=ViewX+136;x<ViewX+408;++x)
            if (std::sin((x-ViewX)/GameplayView::RasterScale*3.14159265f/128-time*speedX)<-.999f)
                rectangle(x,0,1,Height,color);
        for (unsigned y=0;y<Height;++y)
            if (std::sin(y/GameplayView::RasterScale*3.14159265f/129-time*speedY)<-.999f)
                rectangle(ViewX+136,y,272,1,color);
    }
}

bool draw(Screen screen,const Round& round) {
    if (screen==Screen::EndlessHole && !isReplay()) {
        beginFrame();
        drawHud();
        sceGuScissor(ViewX+136,0,ViewX+408,272);
        if (field) {
            setTexture(GU_PSM_5650,512,512,field);
            patch({0,0,272,272,0,0},ViewX+136,0);
        }
        if (level==First+5) EndlessRotating::draw(uniqueCourse.fieldLeft,uniqueCourse.fieldTop,uniqueCourse.fieldSize);
        EndlessModifierArt::drawGround(modifiers);
        sceGuScissor(ViewX+136,0,ViewX+408,272);
        drawItems();

        const Vec2 center{uniqueCourse.fieldLeft+uniqueCourse.fieldSize*.5f,uniqueCourse.fieldTop+uniqueCourse.fieldSize*.5f};
        const GameplayView view(uniqueCourse,ViewX,false,(uniqueCourse.bottom-uniqueCourse.top)/uniqueCourse.fieldSize,center*(-1));
        if(speech059.remaining)AvatarSpeech058::draw(view,uniqueGame.position,avatarIdleLayout059);
        EndlessPresentationArt::effects(presentation,view,run.radical);

        sceGuScissor(0,0,Width,Height);
        EndlessModifierArt::drawHud(modifiers);
        endFrame();
        return true;
    }
    if (screen!=Screen::EndlessMenu && screen!=Screen::EndlessResult) return false;
    if(!EndlessUi067::prepare(EndlessPresentationArt::capture,EndlessPresentationArt::CaptureBytes))return false;
    beginFrame();
    const bool finalGhost=screen==Screen::EndlessResult && (level==CataclysmCourse || level==CosmicSeventeenthCourse);
    UniqueHud057::drawMaster(!finalGhost);
    if (finalGhost) {
        if (level==CataclysmCourse) drawFrontierThinker417(round);
        else drawHurrahThinker053(round);
    }
    if (screen==Screen::EndlessResult) drawHud(true);
    drawGrid(screen==Screen::EndlessResult?EndlessParticles056::resultGridAlpha(resultVisualUs):1);
    sceGuScissor(ViewX+136,0,ViewX+408,Height);
    if(screen==Screen::EndlessMenu)endlessText(trackTitle(),272,20,1.25f,EndlessUi067::Green,true,260,true);
    if (screen==Screen::EndlessResult) {
        EndlessUi067::gameOver(272,68);
        char value[48];
        std::snprintf(value,sizeof value,"%u  /  %u",run.score,run.cleared());
        endlessText(value,272,135,1.05f,EndlessUi067::Green,true);
        if (resultQualified) {
            endlessText(lang("Name:","Имя:"),225,174,.8f,EndlessUi067::Green);
            endlessText(resultName[0]?resultName.data():lang("□ Enter name","□ Введите имя"),306,174,.8f,EndlessUi067::Green,true,224,true);
            endlessText(lang("□ Edit name","□ Изменить имя"),272,199,.72f,EndlessUi067::Green,true);
        }
        endlessButtonText(lang("× One more try!    ○ Menu","× Ещё раз!    ○ Меню"),272,223,.82f,resultReady()?EndlessUi067::Green:GU_RGBA(130,130,130,255),true,260,true);
        if (recordsWriteFailed) endlessText(lang("Could not save score","Не удалось сохранить рекорд"),272,249,.68f,EndlessUi067::Green,true);
    } else {
        sceGuScissor(ViewX+136,49,ViewX+408,214);
        if (presentation.slideUs<250000) {
            const float shift=presentation.direction*372*presentation.slide();
            drawMenuPage(presentation.previousPage,-shift);
            drawMenuPage(presentation.page,presentation.direction*372-shift);
        } else drawMenuPage(presentation.page,0);
        sceGuScissor(0,0,Width,Height);
        endlessButtonText("<-",216,220,.86f,EndlessUi067::Green,true);
        endlessButtonText("GO!",272,220,1.05f,EndlessUi067::Green,true);
        endlessButtonText("->",328,220,.86f,EndlessUi067::Green,true);
        endlessText(lang("× Start    ← → Pages    ○ Back","× Начать    ← → Страницы    ○ Назад"),272,248,.68f,EndlessUi067::Green,true,260,true);
        if (loadError) endlessText(lang("Course data could not be loaded","Не удалось загрузить лунку"),272,41,.62f,EndlessUi067::Green,true);
    }
    sceGuScissor(0,0,Width,Height);
    endFrame();
    return true;
}
} // namespace Endless054
