#pragma once

#include <psputility.h>

// The game's alternating local slots remain the primary save. Publish a copy
// through the utility from a menu so its card I/O cannot interrupt a victory.
class XmbProfile {
public:
    void queue(unsigned slot,const ProfileEntry& entry,const Progress& progress) {
        if (slot==Profiles::Legacy || slot>=Profiles::Count || !entry.title[0]) return;
        queued.slot=slot;
        queued.title=entry.title;
        queued.player=entry.player;
        queued.chill=entry.chill;
        queued.progress=progress;
        queued.check=checksum(queued);
        pending=std::memcmp(&queued,&completed,sizeof queued)!=0;
    }

    struct Timing {
        unsigned elapsed=0,calls=0,callMax=0;
        int result=0;
    };

    bool takeTiming(Timing& out) {
        if (!timingReady) return false;
        out=timing;
        timingReady=false;
        return true;
    }

    void update(bool allowStart,const char* log,const Game& game) {
        const unsigned started=sceKernelGetSystemTimeLow();
        const bool wasActive=active;
        const bool starting=!active && allowStart && pending && !failed;
        if (starting) {
            timing={};
            timingStart=started;
        }
        updateSavedata(allowStart,log,game);
        if (starting || wasActive) {
            ++timing.calls;
            timing.callMax=std::max(timing.callMax,sceKernelGetSystemTimeLow()-started);
            if (!active) {
                timing.elapsed=sceKernelGetSystemTimeLow()-timingStart;
                timing.result=lastResult;
                timingReady=true;
            }
        }
    }

    bool forget(unsigned slot) {
        if(active)return false;
        if(queued.slot==slot) {queued={};pending=false;failed=false;}
        if(completed.slot==slot)completed={};
        return true; // Native SAVEDATA is deliberately untouched.
    }
    bool busy() const { return active; }
    bool waiting() const { return pending && !failed; }

private:
    void updateSavedata(bool allowStart,const char* log,const Game& game) {
        if (active) {
            switch (sceUtilitySavedataGetStatus()) {
            case PSP_UTILITY_DIALOG_VISIBLE:
                sceUtilitySavedataUpdate(1);
                break;
            case PSP_UTILITY_DIALOG_QUIT:
                if ((lastResult=sceUtilitySavedataShutdownStart())<0) {
                    failed=true;
                    active=false;
                    logEvent(log,"xmb_profile_error",game);
                }
                break;
            case PSP_UTILITY_DIALOG_NONE:
                active=false;
                lastResult=parameter.base.result;
                if (parameter.base.result>=0) completed=transfer;
                logEvent(log,parameter.base.result>=0?"xmb_profile_saved":"xmb_profile_error",game);
                break;
            default:
                break;
            }
            return;
        }
        if (!allowStart || !pending || failed) return;
        pending=false;
        transfer=queued;
        parameter={};
        parameter.base.size=sizeof parameter;
        sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&parameter.base.language);
        sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,&parameter.base.buttonSwap);
        parameter.base.graphicsThread=17;
        parameter.base.accessThread=19;
        parameter.base.fontThread=18;
        parameter.base.soundThread=16;
        parameter.mode=PSP_UTILITY_SAVEDATA_AUTOSAVE;
        parameter.overwrite=1;
#ifdef UCG_DISC_BUILD
        std::strcpy(parameter.gameName,"UCGP00053");
#else
        std::strcpy(parameter.gameName,"UCGP00054");
#endif
        std::snprintf(parameter.saveName,sizeof parameter.saveName,"PROFILE%02u",static_cast<unsigned>(transfer.slot));
        std::strcpy(parameter.fileName,"DATA.BIN");
        constexpr unsigned char saveId[16]={0x55,0x43,0x47,0x50,0x30,0x30,0x30,0x35,
                                           0x33,0x50,0x52,0x4f,0x46,0x49,0x4c,0x45};
        static_assert(sizeof parameter.key==sizeof saveId);
        std::memcpy(parameter.key,saveId,sizeof saveId);
        std::strcpy(parameter.sfoParam.title,"Uncanny Cat Golf");
        std::snprintf(parameter.sfoParam.savedataTitle,sizeof parameter.sfoParam.savedataTitle,"%s",transfer.title.data());
        std::snprintf(parameter.sfoParam.detail,sizeof parameter.sfoParam.detail,"%s",transfer.player.data());
        parameter.sfoParam.parentalLevel=1;
        parameter.dataBuf=&transfer;
        parameter.dataBufSize=sizeof transfer;
        parameter.dataSize=sizeof transfer;
        if (const auto size=profileArt.saveIconSize()) {
            parameter.icon0FileData.buf=const_cast<unsigned char*>(profileArt.saveIcon());
            parameter.icon0FileData.bufSize=size;
            parameter.icon0FileData.size=size;
        }
        lastResult=sceUtilitySavedataInitStart(&parameter);
        active=lastResult>=0;
        if (!active) logEvent(log,"xmb_profile_error",game);
    }

    struct Record {
        uint32_t magic=0x31504355,version=1,slot=0,chill=0;
        ProfileName title{},player{};
        uint8_t reserved[2]{};
        Progress progress{};
        uint32_t check=0;
    };

    static uint32_t checksum(const Record& record) {
        const auto* bytes=reinterpret_cast<const uint8_t*>(&record);
        uint32_t value=2166136261u;
        for (unsigned i=0;i<offsetof(Record,check);++i) value=(value^bytes[i])*16777619u;
        return value;
    }

    SceUtilitySavedataParam parameter{};
    Record queued{},transfer{},completed{};
    bool pending=false,active=false,failed=false;
    bool timingReady=false;
    Timing timing{};
    unsigned timingStart=0;
    int lastResult=0;
} xmbProfile;
