#pragma once
#include "world_state.hpp"
#include "world_progress054.hpp"
#include "world_legacy054.hpp"

// Included after Profiles, the legacy award readers, Endless and Club. Main is
// the sole writer; callbacks always begin with the last committed snapshot.
std::unique_ptr<WorldState::Store> worldStore;
bool worldReady=false,worldSaveFailed=false;
bool worldAudibleAwards=false;
bool worldBuilderBlocked=false,worldBuilderCommitAttempted=false;
std::string worldBuilderRoot;
constexpr unsigned LegacyWorldAwards[]={0,32,1,7,2,3,8,14,35,4,9,37,5};
static_assert(std::size(LegacyWorldAwards)==GiftCount);

const WorldState::Data& worldData() {
    static const WorldState::Data empty;
    return worldStore ? worldStore->data() : empty;
}
bool worldBuilderHasTurn() {
    if (!builderPresent.pending() || builderPresent.blocksInput()) return true;
    return WorldState::nextPresent(worldData())==27 && !WorldGift::sequence.active();
}
const WorldState::Data& readWorldGiftData() {
    if ((!builderPresent.pending() || !worldBuilderHasTurn()) && !builderPresent.blocksInput() && !worldBuilderBlocked) return worldData();
    static WorldState::Data filtered;
    filtered=worldData();unsigned count=0;
    for (unsigned i=0;i<filtered.pendingCount;++i)
        if (filtered.pending[i]!=27) filtered.pending[count++]=filtered.pending[i];
    for (unsigned i=count;i<filtered.pendingCount;++i) filtered.pending[i]=0;
    filtered.pendingCount=uint8_t(count);
    return filtered;
}

template<class Mutate>
bool changeWorld(Mutate mutate) {
    if (!worldReady || !worldStore) {worldSaveFailed=true;return false;}
    auto next=worldStore->data();
    const uint64_t known=next.owned | WorldState::pendingMask(next);
    if (!mutate(next)) return true;
    const bool saved=worldStore->commit(next);
    worldSaveFailed=!saved;
    if (saved && worldAudibleAwards) {
        const auto added=WorldState::pendingMask(next)&~known;
        for (unsigned id=0;id<WorldState::VisibleCount;++id)
            if (added&WorldState::bit(id)) audio.play(Sound::GiftAward);
    }
    return saved;
}

bool worldOwns(unsigned id) {return worldReady && WorldState::owns(worldData(),id);}
bool worldLegacyGiftOpened(unsigned id) {
    return id<std::size(LegacyWorldAwards) && worldOwns(LegacyWorldAwards[id]);
}
void applySharedWorldProgress() {
    if (worldReady) WorldProgress054::applyShared(progress,worldData());
}
bool rememberSharedWorldProgress() {
    const bool saved=changeWorld([](WorldState::Data& d) {
        return WorldProgress054::rememberShared(d,progress);
    });
    if (saved) applySharedWorldProgress();
    return saved;
}

bool queueAllWorldAwards() {
    return changeWorld([](WorldState::Data& d) {return WorldState::cheater(d);});
}
bool openWorldGift(unsigned id) {
    if (worldOwns(id)) return true;
    if (WorldState::nextPresent(worldData())!=int(id)) return false;
    return changeWorld([id](WorldState::Data& d) {return WorldState::markOpened(d,id);});
}
bool queueCompletedWorldAwards(unsigned index,bool skipped) {
    return changeWorld([=](WorldState::Data& d) {
        return WorldProgress054::completed(d,progress,index,skipped,!hidesElysianScoring(index));
    });
}
bool rememberWorldResult(unsigned world) {
    if (world>6) return false;
    return changeWorld([=](WorldState::Data& d) {
        if (d.worldProgress>=int(world)) return false;
        d.worldProgress=int8_t(world);return true;
    });
}
int sharedAttractWorld() {
    return worldReady ? std::max(1,int(worldData().worldProgress)) : -1;
}

bool rememberWorldQuest(unsigned phase) {
    if (phase>6) return false;
    const bool saved=changeWorld([=](WorldState::Data& d) {
        bool changed=false;
        if (d.friendPhase<phase) {d.friendPhase=uint8_t(phase);changed=true;}
        if (phase>=5) changed=WorldState::award(d,28) || changed;
        return changed;
    });
    if (saved) applySharedWorldProgress();
    return saved;
}

ClubEvents::ClubPersistent readWorldClub(void*) {
    const auto& d=worldData();const auto& t=d.deathLocal;
    return {d.pennyHi,d.appleHasDied,d.deathUnix,{t.year,t.month,t.day,t.weekday,t.hour,t.minute,t.second,t.dst}};
}
bool writeWorldClub(void*,const ClubEvents::ClubPersistent& value) {
    if (value.appleHasDied) {
        const auto& t=value.appleDeathLocal;
        if (t.year<1 || t.year>9999 || t.month<1 || t.month>12 || t.day<1 || t.day>31 ||
            t.weekday<0 || t.weekday>6 || t.hour<0 || t.hour>23 || t.minute<0 || t.minute>59 ||
            t.second<0 || t.second>59) return false;
    }
    return changeWorld([&](WorldState::Data& d) {
        bool changed=false;
        if (value.pennyHi && !d.pennyHi) {d.pennyHi=true;changed=true;}
        if (value.appleHasDied && !d.appleHasDied) {
            d.appleHasDied=true;d.deathUnix=value.appleDeathUnix;
            const auto& t=value.appleDeathLocal;
            d.deathLocal={uint16_t(t.year),uint8_t(t.month),uint8_t(t.day),uint8_t(t.weekday),
                          uint8_t(t.hour),uint8_t(t.minute),uint8_t(t.second),t.dst};
            changed=true;
        }
        return changed;
    });
}

void initWorldBuilder(const char* executable) {
    worldBuilderRoot=statePath(executable,"");
    worldBuilderBlocked=false;worldBuilderCommitAttempted=false;
    if (!worldReady) return;
    const auto marker=worldBuilderRoot+EditorBuilder::FileName;
    const auto value=WorldLegacy054::read<EditorBuilder::Magic,EditorBuilder::validState>(marker);
    if (!WorldLegacy054::readable(value) ||
        (worldOwns(27) && value.value!=1 && !EditorBuilder::File(marker).write(1)) ||
        !builderPresent.init(worldBuilderRoot.c_str())) {
        worldBuilderBlocked=true;worldSaveFailed=true;
    }
}

void rememberOpenedWorldBuilder(bool pressed) {
    if (!worldReady) return;
    if (worldBuilderBlocked && !builderPresent.owned() && !builderPresent.pending() && pressed) {
        const auto root=worldBuilderRoot;
        // statePath requires the executable path, so the recorded root is used
        // directly for this retry of the bridge's checked marker and init.
        const auto marker=root+EditorBuilder::FileName;
        const auto value=WorldLegacy054::read<EditorBuilder::Magic,EditorBuilder::validState>(marker);
        if (WorldLegacy054::readable(value) &&
            (!worldOwns(27) || value.value==1 || EditorBuilder::File(marker).write(1)) &&
            builderPresent.init(root.c_str())) {worldBuilderBlocked=false;worldSaveFailed=false;}
        return;
    }
    if (!builderPresent.owned() || worldOwns(27)) return;
    if (worldBuilderCommitAttempted && !pressed) return;
    worldBuilderCommitAttempted=true;
    // The released bridge has already proved and saved its visible reveal.
    // Import that opening without presenting or awarding Builder a second time.
    worldBuilderBlocked=!changeWorld([](WorldState::Data& d) {
        return WorldState::mergeLegacy(d,WorldState::bit(27),nullptr,0);
    });
}

void initWorldState(const char* executable,const GiftFile& legacyGifts) {
    (void)legacyGifts;
    worldReady=false;worldSaveFailed=false;worldAudibleAwards=false;
    gifts.useOpened(worldLegacyGiftOpened);
    EditorEntry054::worldOwnsRead=worldOwns;
    Endless054::worldOwnsRead=worldOwns;
    Endless054::onWorldAwards=[](unsigned count,unsigned score,bool lost) {
        changeWorld([=](WorldState::Data& d) {return WorldProgress054::endless(d,count,score,lost);});
    };
    Club054::setWorldCallbacks({nullptr,readWorldClub,writeWorldClub,
        [](void*) {return unsigned(worldData().friendPhase);},
        [](void*,unsigned phase) {return rememberWorldQuest(phase);},nullptr});
    worldStore.reset(new(std::nothrow) WorldState::Store(statePath(executable,"")));
    if (!worldStore) {worldSaveFailed=true;return;}
    const auto status=worldStore->load();
    if (status!=WorldState::Status::Valid && status!=WorldState::Status::Missing) {
        worldSaveFailed=true;return;
    }
    const bool profilesReadable=profiles.refreshChecked();
    if (status==WorldState::Status::Missing && (!profilesReadable || Endless054::recordsReadFailed)) {
        worldSaveFailed=true;return;
    }
    const auto legacy=WorldLegacy054::read<0x31474355,validGifts>(statePath(executable,"gifts.cfg"));
    const auto editor=WorldLegacy054::read<0x454d4355,EditorEntry::validBool>(statePath(executable,"editor-mode.cfg"));
    const auto builder=WorldLegacy054::read<EditorBuilder::Magic,EditorBuilder::validState>(statePath(executable,EditorBuilder::FileName));
    const bool legacyReadable=WorldLegacy054::readable(legacy) && WorldLegacy054::readable(editor) && WorldLegacy054::readable(builder);
    if (!legacyReadable && status==WorldState::Status::Missing) {worldSaveFailed=true;return;}
    if (legacyReadable) {
        auto imported=worldStore->data();
        const uint32_t old=legacy.value;
        // Preserve the late content that accepted R4 already made accessible.
        // The thirteen earlier gifts retain their distinct opened/pending bits.
        uint64_t opened=status==WorldState::Status::Missing ? legacyCampaignUnlockFlags(old>>16) : 0;
        if (editor.value==1) opened|=WorldState::bit(31);
        if (builder.value==1) opened|=WorldState::bit(27);
        uint8_t pending[GiftCount+1]{};unsigned count=0;
        for (unsigned i=0;i<GiftCount;++i) opened&=~WorldState::bit(LegacyWorldAwards[i]);
        for (unsigned i=0;i<GiftCount;++i)
            if ((old>>16)&(1u<<i)) opened|=WorldState::bit(LegacyWorldAwards[i]);
        // The old bitset showed the lowest pending bit first. Restore that
        // observed order when converting to the original last-in-first-out queue.
        for (unsigned i=GiftCount;i>0;--i)
            if ((old&(1u<<(i-1))) && !((old>>16)&(1u<<(i-1)))) pending[count++]=LegacyWorldAwards[i-1];
        if (builder.value==2) pending[count++]=27;
        bool changed=WorldState::mergeLegacy(imported,opened,pending,count);
        for (unsigned i=0;i<Profiles::Count;++i) if (profiles.entry(i).found) {
            const auto& p=profiles.snapshot(i);
            changed=WorldProgress054::rememberShared(imported,p) || changed;
            constexpr unsigned ends[]={5,23,41,59,77};
            for (unsigned w=0;w<std::size(ends);++w)
                if (status==WorldState::Status::Missing && p.next>=ends[w] && imported.worldProgress<int(w)) {
                    imported.worldProgress=int8_t(w);changed=true;
                }
        }
        if ((changed || status==WorldState::Status::Missing) && !worldStore->commit(imported)) {worldSaveFailed=true;return;}
    }
    worldReady=true;
    // Recovery of earned events from valid old profile checkpoints is read-only
    // for those profiles. It also retries an award whose previous global write failed.
    changeWorld([](WorldState::Data& d) {
        bool changed=false;
        for (unsigned slot=0;slot<Profiles::Count;++slot) if (profiles.entry(slot).found) {
            const auto& p=profiles.snapshot(slot);
            changed=WorldProgress054::rememberShared(d,p) || changed;
            for (unsigned i=0;i<CourseCount;++i) {
                const bool won=p.best[i].time!=UINT32_MAX || p.best[i].rank>0;
                if (won || p.next>i)
                    changed=WorldProgress054::completed(d,p,i,!won,!hidesElysianScoring(i)) || changed;
            }
            if (p.quest>=5) changed=WorldState::award(d,28) || changed;
        }
        const auto late=Endless054::unlockFlags();
        for (unsigned id=0;id<WorldState::VisibleCount;++id)
            if (late&WorldState::bit(id)) changed=WorldState::award(d,id) || changed;
        return changed;
    });
    if (!legacyReadable) worldSaveFailed=true;
    applySharedWorldProgress();
    worldAudibleAwards=true;
}
