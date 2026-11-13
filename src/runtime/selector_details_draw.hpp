// Place after patch() and setTexture(). Keep the texture 64-byte aligned.
void drawSelectorDigits(const SelectorDetails::TextStyle& style,const char* value,float x,float y,bool centered=false) {
    if (centered) {
        for (const char* digit=value;*digit;++digit) {
            const char* at=std::strchr(style.characters,*digit);
            if (at) x-=style.glyphs[at-style.characters].advance*0.5f;
        }
    }
    for (;*value;++value) {
        const char* at=std::strchr(style.characters,*value);
        if (!at) continue;
        const auto& glyph=style.glyphs[at-style.characters];
        const auto& art=glyph.art;
        if (art.width>0) patch(art,ViewX+x+art.x,y+art.y);
        x+=glyph.advance;
    }
}

void drawSelectorDetails(const uint32_t* texture,const Progress& progress,unsigned selected,bool russian) {
    using namespace SelectorDetails;
    const unsigned language=russian?1:0;
    const auto draw=[](const Patch& art) { patch(art,ViewX+art.x,art.y); };
    setTexture(GU_PSM_8888,SelectorDetails::Width,SelectorDetails::Height,texture);
    draw(Header[language]);

    char value[32];
    if (selected<CourseCount) {
        static_assert(CosmicEighteenthCourse+1==CourseCount);
        const auto& best=progress.best[selected];
        const bool park=selected>=TitleCount;
        const bool factory=selected>=MartCourse;
        const bool latePark=selected>=DenseCourse;
        const bool nextPark=selected>=LightsCourse;
        const bool finalPark=selected>=PursuitCourse;
        const bool dogTitle=selected>=BalloonDogCourse;
        const bool extraPark=selected>=DoubleDogCourse;
        const bool nextWorld=selected>=ChuckleCoasterCourse;
        const bool frontier=selected>=FrontierSwitchCourse;
        const bool frontierLate=selected>=WholesomeCourse;
        const bool frontier0607=selected>=FrontierMazeCourse;
        const bool frontier0809=selected>=PlentifulChucklesCourse;
        const bool frontier1011=selected>=FrontierPatrolCourse;
        const bool frontier1213=selected>=WeatherAlertCourse;
        const bool frontier1415=selected>=MovieOnceCourse;
        const bool frontier1617=selected>=CrimsonOutpostCourse;
        const bool frontier18501=selected>=TheWallCourse;
        const bool cosmic053=selected>=CosmicSixteenthCourse;
        const bool cosmic052=selected>=CosmicFourteenthCourse && !cosmic053;
        const bool cosmic051=selected>=CosmicTwelfthCourse && selected<CosmicFourteenthCourse;
        const bool cosmic050=selected>=CosmicTenthCourse && selected<CosmicTwelfthCourse;
        const bool cosmic049=selected>=CosmicEighthCourse && selected<CosmicTenthCourse;
        const bool cosmic048=selected>=CosmicSixthCourse && selected<CosmicEighthCourse;
        const bool cosmic047=selected>=CosmicFourthCourse && selected<CosmicSixthCourse;
        const bool cosmic046=selected>=CosmicSecondCourse && selected<CosmicFourthCourse;
        const bool finale045=selected>=Elysian518Course && selected<CosmicSecondCourse;
        const bool horse1617=selected>=Elysian516Course && selected<Elysian518Course;
        const bool horse1415=selected>=Elysian514Course && selected<Elysian516Course;
        const bool horse1213=selected>=Elysian512Course && selected<Elysian514Course;
        const bool horse1011=selected>=Elysian510Course && selected<Elysian512Course;
        const bool horse0809=selected>=Elysian508Course && selected<Elysian510Course;
        const bool horse0607=selected>=Elysian506Course && selected<Elysian508Course;
        const bool horse0405=selected>=FirstHorseCourse && selected<Elysian506Course;
        const bool elysianTitle=selected>=ElysianRocksCourse && selected<FirstHorseCourse;
        if (cosmic053 || cosmic052 || cosmic051 || cosmic050 || cosmic049 || cosmic048 || cosmic047 || cosmic046) {
            constexpr unsigned offset=2*FrameBytes+8192;
            static_assert(offset+Selector046Width*Selector046Height*4<=2*1024*1024);
            static_assert(offset+Selector047Width*Selector047Height*4<=2*1024*1024);
            auto* pixels=reinterpret_cast<uint32_t*>(vram045(offset));
            static_assert(offset+Selector048Width*Selector048Height*4<=2*1024*1024);
            static_assert(offset+Selector049Width*Selector049Height*4<=2*1024*1024);
            static_assert(offset+Selector050Width*Selector050Height*4<=2*1024*1024);
            static_assert(offset+Selector051Width*Selector051Height*4<=2*1024*1024);
            static_assert(offset+Selector052Width*Selector052Height*4<=2*1024*1024);
            static_assert(offset+Selector053Width*Selector053Height*4<=2*1024*1024);
            if (!cosmic053) cosmicTitles053.unload();
            if (!cosmic052) cosmicTitles052.unload();
            if (!cosmic051) cosmicTitles051.unload();
            if (!cosmic050) cosmicTitles050.unload();
            if (!cosmic049) cosmicTitles049.unload();
            if (!cosmic048) cosmicTitles048.unload();
            if (cosmic053) {
                cosmicTitles046.unload();
                cosmicTitles047.unload();
                if (!cosmicTitles053.loadDirect(cosmicTitlePath053,pixels)) return;
            } else if (cosmic052) {
                cosmicTitles046.unload();
                cosmicTitles047.unload();
                if (!cosmicTitles052.loadDirect(cosmicTitlePath052,pixels)) return;
            } else if (cosmic051) {
                cosmicTitles046.unload();
                cosmicTitles047.unload();
                if (!cosmicTitles051.loadDirect(cosmicTitlePath051,pixels)) return;
            } else if (cosmic050) {
                cosmicTitles046.unload();
                cosmicTitles047.unload();
                if (!cosmicTitles050.loadDirect(cosmicTitlePath050,pixels)) return;
            } else if (cosmic049) {
                cosmicTitles046.unload();
                cosmicTitles047.unload();
                if (!cosmicTitles049.loadDirect(cosmicTitlePath049,pixels)) return;
            } else if (cosmic048) {
                cosmicTitles046.unload();
                cosmicTitles047.unload();
                if (!cosmicTitles048.loadDirect(cosmicTitlePath048,pixels)) return;
            } else if (cosmic047) {
                cosmicTitles046.unload();
                if (!cosmicTitles047.loadDirect(cosmicTitlePath047,pixels)) return;
            } else {
                cosmicTitles047.unload();
                if (!cosmicTitles046.loadDirect(cosmicTitlePath046,pixels)) return;
            }
        }
        const auto& title=cosmic053?cosmicTitles053.patch(3*language+selected-CosmicSixteenthCourse):
                          cosmic052?cosmicTitles052.patch(2*language+selected-CosmicFourteenthCourse):
                          cosmic051?cosmicTitles051.patch(2*language+selected-CosmicTwelfthCourse):
                          cosmic050?cosmicTitles050.patch(2*language+selected-CosmicTenthCourse):
                          cosmic049?cosmicTitles049.patch(2*language+selected-CosmicEighthCourse):
                          cosmic048?cosmicTitles048.patch(2*language+selected-CosmicSixthCourse):
                          cosmic047?cosmicTitles047.patch(2*language+selected-CosmicFourthCourse):
                          cosmic046?cosmicTitles046.patch(2*language+selected-CosmicSecondCourse):
                          finale045?finaleTitles045.patch(2*language+selected-Elysian518Course):
                          horse1617?elysian1617Titles.patch(2*language+selected-Elysian516Course):
                          horse1415?elysian1415Titles.patch(2*language+selected-Elysian514Course):
                          horse1213?elysian1213Titles.patch(2*language+selected-Elysian512Course):
                          horse1011?elysian1011Titles.patch(2*language+selected-Elysian510Course):
                          horse0809?elysian0809Titles.patch(2*language+selected-Elysian508Course):
                          horse0607?elysian0607Titles.patch(2*language+selected-Elysian506Course):
                          horse0405?elysian0405Titles.patch(2*language+selected-FirstHorseCourse):
                          elysianTitle?elysian0203Titles.patch(2*language+selected-ElysianRocksCourse):
                          frontier18501?frontier18501Titles.patch(2*language+selected-TheWallCourse):
                          frontier1617?frontier1617Titles.patch(2*language+selected-CrimsonOutpostCourse):
                          frontier1415?frontier1415Titles.patch(2*language+selected-MovieOnceCourse):
                          frontier1213?frontier1213Titles.patch(2*language+selected-WeatherAlertCourse):
                          frontier1011?frontier1011Titles.patch(2*language+selected-FrontierPatrolCourse):
                          frontier0809?frontier0809Titles.patch(2*language+selected-PlentifulChucklesCourse):
                          frontier0607?frontier0607Titles.patch(2*language+selected-FrontierMazeCourse):
                          frontierLate?frontierLateTitles.patch(2*language+selected-WholesomeCourse):
                          frontier?FrontierTitles[language][selected-FrontierSwitchCourse]:
                          nextWorld?NextWorldTitles[language][selected-ChuckleCoasterCourse]:
                          extraPark?ExtraParkTitles[language][selected-DoubleDogCourse]:
                          dogTitle?DogTitles[language][selected-BalloonDogCourse]:
                          finalPark?FinalParkTitles[language][selected-PursuitCourse]:
                          nextPark?NextTitles[language][selected-LightsCourse]:
                          latePark?LateParkTitles[language][selected-DenseCourse]:
                          factory?FactoryTitles[language][selected-MartCourse]:
                          park?ParkTitles[language][selected-TitleCount]:Titles[language][selected];
        if (frontier18501) {
            constexpr unsigned offset=RideArt033::BackgroundBytes+RideArt033::CompressedBytes;
            if (!elysianTitle) elysian0203Titles.invalidateShared();
            if (!horse0405) elysian0405Titles.invalidateShared();
            if (!horse0607) elysian0607Titles.invalidateShared();
            if (!horse0809) elysian0809Titles.invalidateShared();
            if (!horse1011) elysian1011Titles.invalidateShared();
            if (!horse1213) elysian1213Titles.invalidateShared();
            if (selected>=ElysianRocksCourse) frontier18501Titles.invalidateShared();
            if (cosmic053) {
                setTexture(GU_PSM_8888,Selector053Width,Selector053Height,cosmicTitles053.texture());
            } else if (cosmic052) {
                setTexture(GU_PSM_8888,Selector052Width,Selector052Height,cosmicTitles052.texture());
            } else if (cosmic051) {
                setTexture(GU_PSM_8888,Selector051Width,Selector051Height,cosmicTitles051.texture());
            } else if (cosmic050) {
                setTexture(GU_PSM_8888,Selector050Width,Selector050Height,cosmicTitles050.texture());
            } else if (cosmic049) {
                setTexture(GU_PSM_8888,Selector049Width,Selector049Height,cosmicTitles049.texture());
            } else if (cosmic048) {
                setTexture(GU_PSM_8888,Selector048Width,Selector048Height,cosmicTitles048.texture());
            } else if (cosmic047) {
                setTexture(GU_PSM_8888,Selector047Width,Selector047Height,cosmicTitles047.texture());
            } else if (cosmic046) {
                setTexture(GU_PSM_8888,Selector046Width,Selector046Height,cosmicTitles046.texture());
            } else if (finale045) {
                setTexture(GU_PSM_8888,Selector045Width,Selector045Height,finaleTitles045.texture());
            } else if (horse1617) {
                setTexture(GU_PSM_8888,SelectorHorse044Width,SelectorHorse044Height,elysian1617Titles.texture());
            } else if (horse1415) {
                setTexture(GU_PSM_8888,SelectorHorse043Width,SelectorHorse043Height,elysian1415Titles.texture());
            } else if (horse1213) {
                unsigned generation=0;
                auto* workspace=clips.textureWorkspace<SelectorHorse042Width*SelectorHorse042Height>(generation);
                if (!elysian1213Titles.useShared(workspace,generation)) return;
                setFrontierTexture(elysian1213Titles);
            } else if (horse1011) {
                unsigned generation=0;
                auto* workspace=clips.textureWorkspace<SelectorHorse041Width*SelectorHorse041Height>(generation);
                if (!elysian1011Titles.useShared(workspace,generation)) return;
                setFrontierTexture(elysian1011Titles);
            } else if (horse0809) {
                if (!bindFrontierCompressed037<decltype(elysian0809Titles),offset>(elysian0809Titles)) return;
            } else if (horse0607) {
                unsigned generation=0;
                auto* workspace=clips.textureWorkspace<128*256>(generation);
                if (!elysian0607Titles.useShared(workspace,generation)) return;
                setFrontierTexture(elysian0607Titles);
            } else if (horse0405) {
                if (!bindFrontierCompressed037<decltype(elysian0405Titles),offset>(elysian0405Titles)) return;
            } else if (elysianTitle) {
                if (!bindFrontierCompressed037<decltype(elysian0203Titles),offset>(elysian0203Titles)) return;
            } else if (!bindFrontierCompressed037<decltype(frontier18501Titles),offset>(frontier18501Titles)) return;
        } else if (frontier1617) {
            if (!bindFrontierCompressed037<decltype(frontier1617Titles),FrontierTitlesOffset1617>(frontier1617Titles)) return;
        } else if (frontier1415 || frontier1213) {
            // The new title atlas has its own range within the shared clip buffer.
            constexpr unsigned offset=ChuckleArt::TextureBytes+256*256+2*128*256;
            unsigned generation=0;
            auto* workspace=clips.textureWorkspace<offset+SelectorFrontier1213Width*SelectorFrontier1213Height>(generation)+offset;
            static_assert(SelectorFrontier1415Width==SelectorFrontier1213Width &&
                          SelectorFrontier1415Height==SelectorFrontier1213Height);
            auto& atlas=frontier1415?frontier1415Titles:frontier1213Titles;
            auto& previous=frontier1415?frontier1213Titles:frontier1415Titles;
            previous.invalidateShared();
            if (!atlas.useShared(workspace,generation)) return;
            setFrontierTexture(atlas);
        } else if (frontier1011) {
            constexpr unsigned offset=ChuckleArt::TextureBytes+256*256+128*256;
            unsigned generation=0;
            auto* workspace=clips.textureWorkspace<offset+128*256>(generation)+offset;
            if (!frontier1011Titles.useShared(workspace,generation)) return;
            setFrontierTexture(frontier1011Titles);
        } else if (frontier0809) {
            constexpr unsigned offset=ChuckleArt::TextureBytes+256*256;
            unsigned generation=0;
            auto* workspace=clips.textureWorkspace<offset+128*256>(generation)+offset;
            if (!frontier0809Titles.useShared(workspace,generation)) return;
            setFrontierTexture(frontier0809Titles);
        } else if (frontier0607) setFrontierTexture(frontier0607Titles);
        else if (frontierLate) setFrontierTexture(frontierLateTitles);
        else if (frontier) setTexture(GU_PSM_8888,128,256,selectorFrontierTitles);
        else if (nextWorld) setTexture(GU_PSM_8888,NextWorldTitleWidth,NextWorldTitleHeight,selectorNextWorldTitles);
        else if (extraPark) setTexture(GU_PSM_8888,ExtraParkTitleWidth,ExtraParkTitleHeight,selectorExtraParkTitles);
        else if (dogTitle) setTexture(GU_PSM_8888,DogTitleWidth,DogTitleHeight,selectorDogTitles);
        else if (finalPark) setTexture(GU_PSM_8888,FinalParkTitleWidth,FinalParkTitleHeight,selectorFinalParkTitles);
        else if (nextPark) setTexture(GU_PSM_8888,NextTitleWidth,NextTitleHeight,selectorNextTitles);
        else if (latePark) setTexture(GU_PSM_8888,LateParkTitleSide,LateParkTitleSide,selectorLateParkTitles);
        else if (factory) setTexture(GU_PSM_8888,FactoryTitleWidth,FactoryTitleHeight,selectorFactoryTitles);
        else if (park) setTexture(GU_PSM_8888,ParkTitleSide,ParkTitleSide,selectorParkTitles);
        draw(title);
        if (park) setTexture(GU_PSM_8888,SelectorDetails::Width,SelectorDetails::Height,texture);
        const int shift=std::max(0,title.y+title.height+3-ScoreLabel[language].art.y);
        for (Patch art:{ScoreLabel[language].art,TimeLabel[language].art,
                        Rank[std::min(best.rank,uint32_t{5})]}) {
            art.y+=shift;
            draw(art);
        }
        std::snprintf(value,sizeof value,"%u",static_cast<unsigned>(best.score));
        drawSelectorDigits(Stat,value,ScoreLabel[language].endX,76.5f+shift);
        if (best.time==UINT32_MAX) {
            std::snprintf(value,sizeof value,"--:--.--");
        } else {
            const unsigned time=best.time;
            std::snprintf(value,sizeof value,"%02u:%02u.%02u",time/6000,time/100%60,time%100);
        }
        drawSelectorDigits(Stat,value,TimeLabel[language].endX,89.25f+shift);
    }

    std::snprintf(value,sizeof value," %u !",progress.totalScore());
    drawSelectorDigits(SelectorDetails::Score,value,21.25f,72.25f);
    std::snprintf(value,sizeof value,"%u",progress.totalPrisms());
    drawSelectorDigits(Prisms,value,116.875f,76.5f,true);
    std::snprintf(value,sizeof value,"%u",static_cast<unsigned>(progress.resets));
    drawSelectorDigits(Resets,value,114.75f,204,true);
    const unsigned seconds=progress.ticks/60;
    const unsigned hundredths=progress.ticks%60*100/60;
    std::snprintf(value,sizeof value,"%02u:%02u:%02u.%02u",seconds/3600,seconds/60%60,seconds%60,hundredths);
    drawSelectorDigits(TotalTime,value,0,261.375f);
}
