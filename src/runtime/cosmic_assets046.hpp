#pragma once

constexpr unsigned CosmicFieldBytes046=std::max({CosmicField602VramBytes,CosmicField603VramBytes,CosmicField604VramBytes,CosmicField605VramBytes,CosmicField606VramBytes,CosmicField607VramBytes,CosmicGloss048VramBytes,CosmicField608VramBytes,CosmicField609VramBytes,CosmicGloss608VramBytes,CosmicGloss609VramBytes,CosmicGloss610VramBytes,CosmicGloss611VramBytes});
constexpr unsigned CosmicFlagBytes046=std::max({CosmicFlag602VramBytes,CosmicFlag603VramBytes,CosmicFlag604VramBytes,CosmicFlag605VramBytes,CosmicFlag606VramBytes,CosmicFlag607VramBytes,CosmicFlag608VramBytes,CosmicFlag609VramBytes});
constexpr unsigned CosmicPlantBytes046=CosmicPlants046VramBytes;
constexpr unsigned CosmicFlagOffset046=CosmicFieldOffset045+CosmicFieldBytes046;
constexpr unsigned CosmicTailOffset046=CosmicFlagOffset046+CosmicFlagBytes046;
constexpr unsigned CosmicSwitchFacesOffset046=CosmicTailOffset046+CosmicPlantBytes046;
constexpr unsigned CosmicPlantsOffset602=CosmicEarthboundOffset045+512*512*2;
static_assert(CosmicSwitchFacesOffset046+ElysianSwitchFaces514Width*ElysianSwitchFaces514Height*4<=2*1024*1024);
static_assert(CosmicTailOffset046+CosmicBlocks602Width*CosmicBlocks602Height*4<=2*1024*1024);
static_assert(CosmicPlantsOffset602+CosmicPlantBytes046<=CosmicFieldOffset045);

static_assert(CosmicTailOffset046+CosmicBlocks605Width*CosmicBlocks605Height*4<=2*1024*1024);
LiveResultArt cosmicField046[12],cosmicFlag046[12],cosmicPlants046;
#include "cosmic_assets047.hpp"
LiveResultArt::Slot cosmicFieldSlot046,cosmicFlagSlot046,cosmicPlantSlot046;
ParkPatchBank<CosmicBlocks602Width,CosmicBlocks602Height,CosmicBlocks602Count> cosmicBlocks602;
ParkPatchBank<ElysianSwitchFaces514Width,ElysianSwitchFaces514Height,ElysianSwitchFaces514Count> cosmicSwitchFaces046;
std::string cosmicFallbackPath046;
const uint16_t* cosmicFallback046=nullptr;
unsigned cosmicFallbackGeneration046=0,cosmicVramRevision046=0;
#include "cosmic_assets048.hpp"
#include "cosmic_assets049.hpp"
#include "cosmic_assets050.hpp"
#include "cosmic_assets051.hpp"
#include "cosmic_assets052.hpp"
#include "cosmic_assets053.hpp"

bool openCosmicAssets046(const char* executable) {
    constexpr unsigned Scratch=sizeof fallbackPage-HorseMeter039Width*HorseMeter039Height;
    const auto path=[executable](const char* name) { return assetPath(executable,name); };
    return cosmicField046[0].open(path("cosmic-field-6_02.ucr"),CosmicField602Count,Scratch) &&
           cosmicField046[1].open(path("cosmic-field-6_03.ucr"),CosmicField603Count,Scratch) &&
           cosmicFlag046[0].open(path("cosmic-flag-6_02.ucr"),CosmicFlag602Count,Scratch) &&
           cosmicFlag046[1].open(path("cosmic-flag-6_03.ucr"),CosmicFlag603Count,Scratch) &&
           cosmicField046[2].open(path("cosmic-field-6_04.ucr"),CosmicField604Count,Scratch) &&
           cosmicField046[3].open(path("cosmic-field-6_05.ucr"),CosmicField605Count,Scratch) &&
           cosmicFlag046[2].open(path("cosmic-flag-6_04.ucr"),CosmicFlag604Count,Scratch) &&
           cosmicFlag046[3].open(path("cosmic-flag-6_05.ucr"),CosmicFlag605Count,Scratch) &&
           cosmicPlants046.open(path("cosmic-plants-046.ucr"),CosmicPlants046Count,Scratch);
}

bool loadCosmicCourse046(const char* executable,unsigned index) {
    ++cosmicVramRevision046;
    cosmicFieldSlot046=cosmicFlagSlot046=cosmicPlantSlot046={};
    cosmicBlocks602.unload();
    cosmicBlocks605.unload();
    starstormWeather047.unload();
    parkArt.invalidateRingTexture();
    resetRhythmArt048();
    resetRhythmArt049();
    resetRhythmArt050();
    resetCosmicArt051();
    resetCosmicArt052();
    resetCosmicArt053();
    cosmicSwitchFaces046.unload();
    if (index<CosmicSecondCourse) return true;
    if (index>=CosmicSixteenthCourse) return loadCosmicArt053(executable,index);
    if (index>=CosmicFourteenthCourse) return loadCosmicArt052(executable,index);
    if (index>=CosmicTwelfthCourse) return loadCosmicArt051(executable,index);
    const auto path=[executable](const char* name) { return assetPath(executable,name); };
    if (index==CosmicSecondCourse)
        return loadVramRaw045(path("cosmic-carpet-6_02.bin"),CosmicEarthboundOffset045,512*512*2) &&
               loadVramAtlas045(cosmicBlocks602,path("cosmic-blocks-6_02.pak"),CosmicTailOffset046);
    cosmicFallback046=nullptr;
    const char* background=usesPolyrhythm049(index)?"cosmic-earthbound-6_09.bin":"cosmic-earthbound-6_01.bin";
    if (!loadVramRaw045(path(background),CosmicEarthboundOffset045,CosmicEarthboundBytes045))
        return false;
    if (index==CosmicFifthCourse) {
        starstormWeatherPath047=path("cosmic-weather-6_05.pak");
        return loadVramAtlas045(cosmicBlocks605,path("cosmic-blocks-6_05.pak"),CosmicTailOffset046);
    }
    if (usesMetronome048(index))
        return loadRhythmArt048(executable,index) && loadRhythmArt049(executable,index) && loadRhythmArt050(executable,index);
    cosmicFallbackPath046=path(index==CosmicFourthCourse?"cosmic-fallback-6_04-state1.z":"cosmic-fallback-6_03-state1.z");
    return loadVramAtlas045(cosmicSwitchFaces046,path("elysian-switch-faces-5_14.pak"),CosmicSwitchFacesOffset046);
}
