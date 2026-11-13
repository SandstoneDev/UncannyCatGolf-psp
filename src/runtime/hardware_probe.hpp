struct HardwareProbe {
    static constexpr int NotRun=0x7fffffff;
    int opened=NotRun,written=NotRun,closed=NotRun,stdioError=NotRun,removed=NotRun;
    unsigned spareFiles=0,renderMax[2]{},oskMax[2]{};
    bool visible=false,osk30=false;

    void checkStorage(const char* executable,const char* log=nullptr) {
#ifdef UCG_DISC_BUILD
        const std::string boot="disc0:/PSP_GAME/SYSDIR/EBOOT.BIN";
#else
        const auto boot=companion(executable,"EBOOT.PBP");
#endif
        SceUID files[4]{};
        spareFiles=0;
        for (auto& handle:files) {
            handle=sceIoOpen(boot.c_str(),PSP_O_RDONLY,0);
            if (handle<0) break;
            ++spareFiles;
        }
        for (unsigned i=0;i<spareFiles;++i) sceIoClose(files[i]);

        opened=written=closed=stdioError=removed=NotRun;
        const auto path=statePath(executable,"ucg024-write-probe.tmp");
        SceIoStat existing{};
        if (sceIoGetstat(path.c_str(),&existing)>=0) return;
        const SceUID fd=sceIoOpen(path.c_str(),PSP_O_WRONLY|PSP_O_CREAT|PSP_O_EXCL,0666);
        opened=fd<0?fd:0;
        if (fd<0) return;
        constexpr uint32_t marker=0x31324355;
        written=sceIoWrite(fd,&marker,sizeof marker);
        closed=sceIoClose(fd);
        if (written==static_cast<int>(sizeof marker) && !closed) {
            errno=0;
            if (auto* file=std::fopen(path.c_str(),"ab")) {
                const int write=std::fputc('!',file);
                const int writeError=write==EOF?(errno?errno:EIO):0;
                const int close=std::fclose(file);
                stdioError=writeError?writeError:(close?(errno?errno:EIO):0);
            } else {
                stdioError=errno?errno:EIO;
            }
        }
        removed=sceIoRemove(path.c_str());
        if (log) {
            if (auto* file=std::fopen(log,"a")) {
                std::fprintf(file,"build=024 event=storage_probe spare_fds=%u open=%08X write=%08X close=%08X stdio=%08X\n",
                             spareFiles,unsigned(opened),unsigned(written),unsigned(closed),unsigned(stdioError));
                std::fclose(file);
            }
        }
    }

    void draw() const {
        if (!visible || profileKeyboard.active()) return;
        rectangle(ViewX+5,4,398,140,GU_RGBA(0,0,0,245));
        char line[72];
        std::snprintf(line,sizeof line,"024  SELECT: hide    L: OSK %u Hz    progress.next=%u",
                      osk30?30u:60u,static_cast<unsigned>(progress.next));
        localeText(line,10,8,0.89f,Gold);
        std::snprintf(line,sizeof line,"IO open=%08X write=%08X close=%08X",
                      unsigned(opened),unsigned(written),unsigned(closed));
        localeText(line,10,23,0.89f,White);
        std::snprintf(line,sizeof line,"stdio errno=%08X remove=%08X OSK=%08X",
                      unsigned(stdioError),unsigned(removed),unsigned(profileKeyboard.error()));
        localeText(line,10,38,0.89f,White);
        constexpr const char* labels[]={"Menu ","OSK60","OSK30"};
        localeText("Audio      n    read    mix    gap   late  miss  err",10,53,0.83f,Gold);
        int audioError=0;
        for (unsigned i=0;i<3;++i) {
            const auto& t=audio.timing(static_cast<AudioProbePhase>(i+1));
            std::snprintf(line,sizeof line,"%s %5u %7u %6u %6u %5u %5u %4u",
                          labels[i],t.blocks.load(std::memory_order_relaxed),
                          t.readMax.load(std::memory_order_relaxed),t.mixMax.load(std::memory_order_relaxed),
                          t.gapMax.load(std::memory_order_relaxed),t.late.load(std::memory_order_relaxed),
                          t.underruns.load(std::memory_order_relaxed),t.readErrors.load(std::memory_order_relaxed));
            localeText(line,10,67+14*i,0.83f,White);
            const int error=t.outputError.load(std::memory_order_relaxed);
            if (error) audioError=error;
        }
        std::snprintf(line,sizeof line,"Render/OSK us  60: %u/%u   30: %u/%u",
                      renderMax[0],oskMax[0],renderMax[1],oskMax[1]);
        localeText(line,10,109,0.83f,White);
        std::snprintf(line,sizeof line,"FD headroom %u/4   Audio error=%08X",spareFiles,unsigned(audioError));
        localeText(line,10,123,0.83f,White);
    }
} hardwareProbe;
