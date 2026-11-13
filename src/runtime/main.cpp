#include "asset_layout.hpp"
#include "live_result_art.hpp"
#include "controls.hpp"
#include "dense_blocks.hpp"
#include "gameplay_view.hpp"
#include "pause_menu.hpp"
#include "goal_flag.hpp"
#include "park_art.hpp"
#include "park_stage_art.hpp"
#include "park_dog_screen.hpp"
#include "park_switch_patches031.hpp"
#include "switch_courses.hpp"
#include "park_video.hpp"
#include "chuckle_run.hpp"
#include "marcy_run.hpp"
#include "park_events031.hpp"
#include "park_late032_data.hpp"
#include "marcy_reveal.hpp"
#include "chuckle_art.hpp"
#include "coaster_art.hpp"
#include "coaster033.hpp"
#include "frontier_courses.hpp"
#include "world4_mechanics034.hpp"
#include "snow_switch_patches034.hpp"
#include "frontier_atlases.hpp"
#include "ride_assets033.hpp"
#include "frontier0607_data.hpp"
#include "frontier1011_data.hpp"
#include "world4_mechanics035.hpp"
#include "frontier1617_mechanics.hpp"
#include "frontier18501_mechanics.hpp"
#include "elysian039.hpp"
#include "frontier18501_titles.hpp"
#include "frontier18501_art.hpp"
#include "elysian_horse040.hpp"
#include "frontier050405_art.hpp"
#include "frontier050809_visual.hpp"
#include "frontier051011_visual.hpp"
#include "frontier051213_visual.hpp"
#include "frontier051415_visual.hpp"
#include "frontier051617_visual.hpp"
#include "frontier05180601_visual.hpp"
#include "elysian_finale045.hpp"
#include "elysian_ending045.hpp"
#include "cosmic_geometry046.hpp"
#include "cosmic060203_visual.hpp"
#include "cosmic060405_visual.hpp"
#include "cosmic_geometry047.hpp"
#include "starstorm047.hpp"
#include "cosmic060607_visual.hpp"
#include "cosmic_geometry048.hpp"
#include "metronome048.hpp"
#include "cosmic0809_visual.hpp"
#include "cosmic_geometry049.hpp"
#include "polyrhythm049.hpp"
#include "cosmic_mechanics050.hpp"
#include "cosmic1011_visual.hpp"
#include "cosmic1213_visual.hpp"
#include "cosmic1415_visual.hpp"
#include "cosmic_geometry052.hpp"
#include "santa052.hpp"
#include "cosmic_geometry053.hpp"
#include "cosmic1618_visual.hpp"
#include "cosmic_late053.hpp"
#include "cosmic_geometry051.hpp"
#include "moving_goal051.hpp"
#include "elysian_wizard044.hpp"
#include "elysian_threats043.hpp"
#include "elysian_traffic042.hpp"
#include "elysian_hazards040.hpp"
#include "frontier1617_art.hpp"
#include "frontier1617_overlays.hpp"
#include "frontier1617_titles.hpp"
#include "world4_mechanics036.hpp"
#include "world4_mechanics037.hpp"
#include "frontier_titles036.hpp"
#include "frontier_night_art036.hpp"
#include "frontier_night_art037.hpp"
#include "frontier_titles037.hpp"
#include "snow_switch_patches035.hpp"
#include "frontier_valley_art035.hpp"
#include "frontier_dog_tiles035.hpp"
#include "nuke.hpp"
#include "coaster_spring_art.hpp"
#include "actor_atlas.hpp"
#include "glowstick.hpp"
#include "pad_tracks.hpp"
#include "baddie_run.hpp"
#include "stop_tokens.hpp"
#include "slot_machine_art.hpp"
#include "billboards.hpp"
#include "level_select_art.hpp"
#include "selector_transition.hpp"
#include "profile_art.hpp"
#include "profile_keyboard.hpp"
#include "selector_details.hpp"
#include "selector_park_titles.hpp"
#include "selector_factory_titles.hpp"
#include "selector_late_park_titles.hpp"
#include "selector_next_titles.hpp"
#include "selector_dog_titles.hpp"
#include "selector_next_world_titles.hpp"
#include "selector_frontier_titles.hpp"
#include "selector-extra-park-titles.hpp"
#include "selector-final-park-titles.hpp"
#include "selector_gifts.hpp"
#include "visual.hpp"
#include "world_transition.hpp"
#include "media.hpp"
#include "course_objects.hpp"
#include "results.hpp"
#include "golf.hpp"
#include "locale.hpp"
#include "par_ball.hpp"
#include "pool.hpp"
#include "key_doors.hpp"
#include "jump_pad.hpp"
#include "police_event.hpp"
#include "secret_runner.hpp"
#include "news.hpp"

#include <pspctrl.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <malloc.h>

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <string>

PSP_MODULE_INFO("UCG_028", PSP_MODULE_USER, 1, 28);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
// libvorbis allocates its transform tables when the title track opens.
PSP_HEAP_SIZE_KB(-1);
PSP_HEAP_THRESHOLD_SIZE_KB(288);

namespace {

constexpr int Width = 480, Height = GameplayView::Height, Stride = 512;
constexpr int FrameBytes = Stride * Height * 2;
constexpr unsigned FrontierLightVramOffset=2*FrameBytes;
constexpr unsigned FrontierLightVramBytes=256*256*4;
constexpr unsigned LiveResultVramOffset=FrontierLightVramOffset+FrontierLightVramBytes;
constexpr unsigned ParkBlockVramOffset=LiveResultVramOffset+ResultVramBytes;
static_assert(ParkBlockVramOffset+128*64*sizeof(uint32_t)<=2*1024*1024);
void* screenDrawBuffer=nullptr;
constexpr int ViewX = 36, ViewWidth = GameplayView::Width;
constexpr uint32_t White = GU_RGBA(255,255,255,255);
constexpr float SmallUiText=0.94f;
constexpr uint32_t Gold = GU_RGBA(255,229,91,255);

static_assert(Height%8==0 && Height<=VisualHeight);
constexpr long StoredPageBytes=VisualWidth*VisualHeight*sizeof(uint16_t);
alignas(64) uint16_t gameplayPage[VisualWidth*Height];
#ifndef UCG_VISUAL_TEST
#include "boot_splash041.inc"
#endif
alignas(64) uint16_t fallbackPage[VisualWidth*Height];
std::string fallbackCoursePath;
unsigned fallbackPageIndex=ScreenPages;
alignas(64) uint16_t hud[VisualWidth*HudHeight];
alignas(64) uint32_t actors[ActorsWidth*ActorsHeight];
ActorAtlas actorAtlas{};
alignas(64) uint32_t resultFont[256*128];
struct IntroPose { int16_t x, y; };
IntroPose stagePositions[87]{};
alignas(64) uint32_t stageTexture[128*64]{};
alignas(64) unsigned displayList[32*1024];
std::atomic<bool> running{true};
VisualHeader visual{};
TileField walls,santaWalls;
LateCosmicWalls053 lateWalls053;
SwitchStage cosmicRhythmFields048;
TileField horseWalls039;
constexpr const char* CourseIds[CourseCount] = {"0_01", "0_02", "0_03", "0_04", "0_05", "1_01", "1_02", "1_03", "1_04", "1_05", "1_06", "1_07", "1_08", "1_09", "1_10", "1_11", "1_12", "1_13", "1_14", "1_15", "1_16", "1_17", "1_18", "2_01", "2_02", "2_03", "2_04", "2_05", "2_06", "2_07", "2_08", "2_09", "2_10", "2_11", "2_12", "2_13", "2_14", "2_15", "2_16", "2_17", "2_18", "3_01", "3_02", "3_03", "3_04", "3_05", "3_06", "3_07", "3_08", "3_09", "3_10", "3_11", "3_12", "3_13", "3_14", "3_15", "3_16", "3_17", "3_18", "4_01", "4_02", "4_03", "4_04", "4_05", "4_06", "4_07", "4_08", "4_09", "4_10", "4_11", "4_12", "4_13", "4_14", "4_15", "4_16", "4_17", "4_18", "5_01", "5_02", "5_03", "5_04", "5_05", "5_06", "5_07", "5_08", "5_09", "5_10", "5_11", "5_12", "5_13", "5_14", "5_15", "5_16", "5_17", "5_18", "6_01", "6_02", "6_03", "6_04", "6_05", "6_06", "6_07", "6_08", "6_09", "6_10", "6_11", "6_12", "6_13", "6_14", "6_15", "6_16", "6_17", "6_18"};
static_assert(GlowstickFirstCourse==TutorialCount+GolfStages && ChuckleParkFirstCourse==GlowstickFirstCourse+GlowstickArt::Courses);
GolfData golf{};
GlowstickArt glowstickArt{};
GoalFlagArt goalFlags{};
ParkArt parkArt;
ParkPatchBank<128,64,MaxBlocks> parkBlockArt;
ParkPatchBank<128,64,4> factoryDoors;
ParkPatchBank<64,32,6> arcadeDoors;
ParkPatchBank<64,64,1> darkHoleArt;
ParkPatchBank<512,256,3> ashArt;
ParkPatchBank<128,64,3> factoryBalloons;
ParkPatchBank<128,32,4> factorySpawner;
ParkPatchBank<256,128,18> factoryDecor;
ParkPatchBank<256,128,16> marcySpillArt;
ParkPatchBank<256,128,4> marcyHudArt;
ParkPatchBank<256,128,2> marcyTextArt;
ParkPatchBank<128,128,18> carouselDecal;
ParkPatchBank<128,64,2> switchFaces;
ParkSwitchOverlay switchTiles;
ParkVideo jumbleVideo;
ParkSwitchTiles jumbleTiles;
ParkPatchBank<128,128,259> pursuitTiles;
ParkPatchBank<128,64,1> dogWideDoorArt;
ParkPatchBank<128,128,256> dualGateArt;
ParkPatchBank<128,32,64> raceGateArt;
ParkPatchBank<128,128,13> dogCameraArt;
ParkPatchBank<128,64,4> raceDoors;
constexpr unsigned DualGatePatches[2][2]={{62,84},{25,25}};
TileField dogGate;
std::unique_ptr<TileField[]> dualGateFields;
const TileField* dualGateLayers[3]{};
const DogCourseStage* multiDogStage(unsigned index) {
    if (index==DoubleDogCourse) return &ParkDogStage316;
    if (index==RacetrackCourse) return &ParkDogStage317;
    return nullptr;
}
const DogGolfScene* dogSceneAt(unsigned index) {
    switch (index) {
    case DogCourse: return &ParkDogScene;
    case BalloonDogCourse: return &DogGolf314;
    case ScreenDogCourse: return &DogGolf315;
    case FrontierDogCourse: return &FrontierDog407;
    case FrontierPatrolCourse: return &FrontierDog410;
    case CosmicSeventeenthCourse: return &CosmicDog617;
    default: return nullptr;
    }
}
ParkPatchBank<256,128,12> dogArt;
ParkPatchBank<128,32,4> elysianSpawnerArt513;
ParkPatchBank<32,64,1> dogDoorArt;
ParkDogScreen dogScreen;
ParkPatchBank<256,128,38> pursuitDecor;
ParkPatchBank<64,64,7> pursuitBalloons;
constexpr unsigned PursuitBalloonCount=7, PursuitMotionFrames=3600;
struct PursuitBalloonPoint {
    int16_t x,y;
    Vec2 position() const { return {x/64.0f,y/64.0f}; }
};
static_assert(sizeof(PursuitBalloonPoint)==4);
std::unique_ptr<PursuitBalloonPoint[]> pursuitMotion;
ParkPatchBank<128,64,5> jumbleFlag;
ChuckleArt chuckleArt;
CoasterArt coasterArt;
CoasterSpringArt coasterSprings;
DenseBlocks denseBlocks;
ParkPatchBank<256,128,DenseBlocks::Capacity> denseBlockArt;
constexpr bool hasCoaster(unsigned index) { return index==CoasterCourse || index==DenseCourse || index==PursuitCourse; }
constexpr Sound coasterSound(unsigned index) {
    if (index==PursuitCourse) return Sound::CoasterFlyby312;
    return index==DenseCourse?Sound::CoasterFlyby308:Sound::CoasterFlyby;
}
constexpr Vec2 MirrorChuckles[]={{-264,264}, {-216,264}, {-240,296}, {216,264}, {264,264}, {240,296}};
SwitchStage parkSwitchStage;
RideAtlas033<FrontierSwitchAtlasWidth,FrontierSwitchAtlasHeight,FrontierSwitchPatchCount> frontierSwitchTiles;
RideAtlas033<128,64,4> frontierSwitchFaces;
RideAtlas033<FrontierPortalAtlasWidth,FrontierPortalAtlasHeight,std::size(FrontierPortals403)> frontierPortalArt;
StopTokenAssets stopTokens{},speedTokens{};
BillboardArt billboards{};
SlotMachineArt slotArt;
constexpr unsigned SlotMachineCourse=CasinoCourse;
constexpr unsigned FirstBaddieCourse=GlowstickFirstCourse+5, TrafficCourse=FirstBaddieCourse+1;
constexpr unsigned DimlitCourse=TrafficCourse+1, MuggingCourse=DimlitCourse+1;
constexpr unsigned JumpCourse=MuggingCourse+1, BankCourse=JumpCourse+1;
constexpr unsigned HighwayCourse=BankCourse+1, CityCourse=HighwayCourse+1;
constexpr unsigned LightboxCourse=CityCourse+1, AlleyCourse=LightboxCourse+1;
constexpr unsigned PoliceBaddieStage=DimlitCourse-FirstBaddieCourse+1;
constexpr unsigned HighwayBaddieStage=PoliceBaddieStage+1, CityBaddieStage=HighwayBaddieStage+1;
constexpr unsigned AlleyBaddieStage=CityBaddieStage+1;
constexpr unsigned SlotBaddieStage=AlleyBaddieStage+1, FinaleBaddieStage=SlotBaddieStage+1;
constexpr unsigned BankKeyDoorStage=KeyDoorGolfStageCount+1, CityKeyDoorStage=BankKeyDoorStage+1;
constexpr unsigned LightboxKeyDoorStage=CityKeyDoorStage+1;
constexpr unsigned CasinoKeyDoorStage=LightboxKeyDoorStage+2;
static_assert(TrafficCourse==GlowstickFirstCourse+GlowstickArt::TrafficIndex &&
              DimlitCourse==GlowstickFirstCourse+GlowstickArt::DimlitIndex &&
              DimlitCourse==TutorialCount+DimlitGolfStage &&
              JumpCourse==GlowstickFirstCourse+GlowstickArt::JumpIndex &&
              BankCourse==GlowstickFirstCourse+GlowstickArt::BankIndex &&
              BankCourse==TutorialCount+BankGolfStage &&
              HighwayCourse==GlowstickFirstCourse+GlowstickArt::HighwayIndex &&
              CityCourse==GlowstickFirstCourse+GlowstickArt::CityIndex &&
              LightboxCourse==GlowstickFirstCourse+GlowstickArt::LightboxIndex &&
              AlleyCourse==GlowstickFirstCourse+GlowstickArt::AlleyIndex &&
              AlleyCourse+1==TowerCourse && ChuckleFirstCourse==ChuckleParkFirstCourse &&
              CosmicEighteenthCourse+1==CourseCount);
constexpr unsigned BaddieArtWidth=256,BaddieArtHeight=32;
constexpr unsigned CarouselBaddieStage=FinaleBaddieStage+1;
BaddieStage baddieStages[CarouselBaddieStage+1]{};
constexpr unsigned JumpArtWidth=128,JumpArtHeight=32,SpotlightSize=128;
constexpr unsigned FactoryJumpStage=GlowstickFinaleCourse-JumpCourse+1;
constexpr unsigned ArcadeJumpStage=FactoryJumpStage+1;
JumpStage jumpStages[ArcadeJumpStage+1]{};
alignas(64) uint32_t jumpPixels[JumpArtWidth*JumpArtHeight]{};
alignas(64) uint32_t spotlightPixels[SpotlightSize*SpotlightSize]{};
alignas(64) uint16_t lightboxLight[SpotlightSize*SpotlightSize]{};
constexpr unsigned RunnerAtlasWidth=128,RunnerAtlasHeight=64;
alignas(64) uint32_t runnerPixels[RunnerAtlasWidth*RunnerAtlasHeight]{};
NewsArt newsArt;
alignas(64) uint32_t baddiePixels[BaddieArtWidth*BaddieArtHeight]{};
alignas(64) uint32_t elysianCarPixels511[32*32]{};
constexpr unsigned FirstStopCourse=GlowstickFirstCourse+1, BillboardCourse=GlowstickFirstCourse+4;
constexpr unsigned FactoryTokenStage=StopTokenStages-1, FactoryKeyDoorStage=CasinoKeyDoorStage+2;
constexpr unsigned ArcadeKeyDoorStage=FactoryKeyDoorStage+1;
static_assert(ArcadeKeyDoorStage+1==KeyDoorStageCount);

const GolfStage& courseHazards(unsigned index) {
    static constexpr GolfStage noHazards{};
    if (index<TutorialCount || index>=CourseCount) return noHazards;
    return golf.stages[index-TutorialCount];
}
constexpr unsigned FirstKeyDoorCourse=TutorialCount+12;
KeyDoorData keyDoorData{};
alignas(64) uint32_t keyDoorSprites[KeyDoorAtlasWidth*KeyDoorAtlasHeight]{};
alignas(64) uint32_t roundKeySprites[256*256]{};
alignas(64) uint32_t bubbleTexture[128*32]{};
alignas(64) uint16_t blockTexture[BlockAtlasWidth*BlockAtlasHeight]{};
alignas(64) uint32_t splashTexture[512*256]{};
alignas(64) uint32_t interactiveTexture[256*256]{};
constexpr unsigned PondCourse=TutorialCount+8, PondFrames=16, BackroomsCourse=TutorialCount+11;
constexpr unsigned PoolCourse=TutorialCount+15;
PoolAssets poolAssets{};
bool canSwim=false;  // Granted by the Golden Tee on 5_18.
constexpr float BackroomsLimit=280.0f;
alignas(64) uint16_t sidebars[128*Height]{};
alignas(64) uint16_t pondAtlas[512*256]{};
constexpr unsigned BackroomsFallbackPage=ScreenPages+1;
std::string backroomsPath;
alignas(64) uint16_t backroomsTv[64*64]{};
uint16_t sidebarReadBuffer[128*Height]{};
std::string decorationsPath;
BackgroundPreference backgroundPreference{},backgroundDraft{};
unsigned backgroundBeforeSettings=0;
bool settingsPreviewError=false;
unsigned activeBackground=UINT32_MAX;
alignas(64) uint32_t menuLabels[256*128]{},settingsWidgets[256*256]{};
alignas(64) uint32_t localeFont[512*128]{};
uint8_t glyphAdvance[LocaleGlyphs]{};
Language language=Language::English;
Language languageDraft=Language::English;
bool settingsError=false;
enum class SettingsRow : unsigned { Language, Background, Random, Save, Count };
SettingsRow settingsRow=SettingsRow::Language;

bool russian() { return language==Language::Russian; }
const char* lang(const char* english,const char* ru) { return russian()?ru:english; }

bool loadLocaleFont(const char* path) {
    std::FILE* file=std::fopen(path,"rb");
    if (!file) return false;
    const bool ok=std::fread(glyphAdvance,sizeof glyphAdvance,1,file)==1 &&
                  std::fread(localeFont,sizeof localeFont,1,file)==1 &&
                  std::fgetc(file)==EOF && !std::ferror(file);
    std::fclose(file);
    if (!ok) return false;
    for (uint8_t advance:glyphAdvance) if (!advance || advance>16) return false;
    sceKernelDcacheWritebackRange(localeFont,sizeof localeFont);
    return true;
}

bool loadUiTexture(const char* path,void* pixels,unsigned bytes) {
    std::FILE* file=std::fopen(path,"rb");
    if (!file) return false;
    const bool ok=std::fread(pixels,bytes,1,file)==1 &&
                  std::fgetc(file)==EOF && !std::ferror(file);
    std::fclose(file);
    if (ok) sceKernelDcacheWritebackRange(pixels,bytes);
    return ok;
}

bool loadGimmicks(const char* path) {
    auto* file=std::fopen(path,"rb");
    if (!file) return false;
    uint32_t magic=0;
    const bool ok=std::fread(&magic,sizeof magic,1,file)==1 && magic==0x314d4355 &&
                  std::fread(jumpStages,sizeof jumpStages,1,file)==1 &&
                  std::fread(jumpPixels,sizeof jumpPixels,1,file)==1 &&
                  std::fread(spotlightPixels,sizeof spotlightPixels,1,file)==1 &&
                  std::fread(runnerPixels,sizeof runnerPixels,1,file)==1 &&
                  std::fread(lightboxLight,sizeof lightboxLight,1,file)==1 &&
                  std::fgetc(file)==EOF && !std::ferror(file);
    std::fclose(file);
    if (!ok || jumpStages[0].count!=9 || jumpStages[1].count!=3 ||
        jumpStages[2].count!=0 || jumpStages[3].count!=6 ||
        jumpStages[TowerCourse-JumpCourse].count!=8 || jumpStages[CasinoCourse-JumpCourse].count!=1 ||
        jumpStages[GlowstickFinaleCourse-JumpCourse].count!=6 || jumpStages[FactoryJumpStage].count!=1 ||
        jumpStages[ArcadeJumpStage].count!=3) return false;
    for (const auto& stage:jumpStages) {
        if (stage.count>JumpStage::Capacity) return false;
        for (unsigned i=0;i<stage.count;++i) {
            const auto& pad=stage.pads[i];
            if (!std::isfinite(pad.center.x) || !std::isfinite(pad.center.y) ||
                !std::isfinite(pad.radians) || !std::isfinite(pad.speedGranted) || pad.speedGranted<0)
                return false;
        }
    }
    sceKernelDcacheWritebackRange(jumpPixels,sizeof jumpPixels);
    sceKernelDcacheWritebackRange(spotlightPixels,sizeof spotlightPixels);
    sceKernelDcacheWritebackRange(runnerPixels,sizeof runnerPixels);
    sceKernelDcacheWritebackRange(lightboxLight,sizeof lightboxLight);
    return true;
}

bool loadBaddies(const char* path) {
    auto* file=std::fopen(path,"rb");
    if (!file) return false;
    uint32_t magic=0;
    const bool ok=std::fread(&magic,sizeof magic,1,file)==1 && magic==0x44424355 &&
                  std::fread(baddieStages,sizeof baddieStages,1,file)==1 &&
                  std::fread(baddiePixels,sizeof baddiePixels,1,file)==1 &&
                  std::fgetc(file)==EOF && !std::ferror(file);
    std::fclose(file);
    if (!ok) return false;
    for (const auto& stage:baddieStages) {
        if (stage.skull>1 || !stage.spawnerCount || stage.spawnerCount>std::size(stage.spawners) ||
            !std::isfinite(stage.despawnDistance) || stage.despawnDistance<=0) return false;
        for (unsigned i=0;i<stage.spawnerCount;++i) {
            const auto& s=stage.spawners[i];
            if (!std::isfinite(s.center.x) || !std::isfinite(s.center.y) ||
                !std::isfinite(s.velocity.x) || !std::isfinite(s.velocity.y) ||
                !s.period || !s.firstTick || !s.limit) return false;
        }
    }
    sceKernelDcacheWritebackRange(baddiePixels,sizeof baddiePixels);
    return true;
}

bool loadGolf(const char* path) {
    std::FILE* file=std::fopen(path,"rb");
    if (!file) return false;
    constexpr long legacyPauseBytes=512*128*sizeof(uint32_t);
    const bool ok=readGolfData(file,golf) && std::fread(bubbleTexture,sizeof bubbleTexture,1,file)==1 &&
                  std::fseek(file,legacyPauseBytes,SEEK_CUR)==0 &&
                  std::fread(blockTexture,sizeof blockTexture,1,file)==1 &&
                  std::fread(splashTexture,sizeof splashTexture,1,file)==1 &&
                  std::fread(interactiveTexture,sizeof interactiveTexture,1,file)==1 &&
                  std::fgetc(file)==EOF && !std::ferror(file);
    std::fclose(file);
    if (ok) {
        sceKernelDcacheWritebackRange(bubbleTexture,sizeof bubbleTexture);
        sceKernelDcacheWritebackRange(blockTexture,sizeof blockTexture);
        sceKernelDcacheWritebackRange(splashTexture,sizeof splashTexture);
        sceKernelDcacheWritebackRange(interactiveTexture,sizeof interactiveTexture);
    }
    return ok;
}

constexpr unsigned BackgroundOffset=8+BackgroundCount*sizeof(uint32_t)+sizeof pondAtlas;

bool selectBackground(unsigned id) {
    const unsigned index=backgroundIndex(id);
    if (index>=BackgroundCount) return false;
    if (id==activeBackground) return true;
    const auto input=openAssetFile(decorationsPath.c_str());
    auto* file=input.get();
    if (!file || std::fseek(file,BackgroundOffset+index*sizeof sidebarReadBuffer,SEEK_SET)!=0 ||
        std::fread(sidebarReadBuffer,sizeof sidebarReadBuffer,1,file)!=1) return false;
    static_assert(sizeof sidebars==sizeof sidebarReadBuffer);
    std::memcpy(sidebars,sidebarReadBuffer,sizeof sidebarReadBuffer);
    sceKernelDcacheWritebackRange(sidebars,sizeof sidebarReadBuffer);
    activeBackground=id;
    return true;
}

bool loadDecorations(const char* path) {
    const auto input=openAssetFile(path);
    auto* file=input.get();
    if (!file) return false;
    uint32_t header[2]{},ids[BackgroundCount]{};
    if (std::fseek(file,0,SEEK_END)!=0 ||
        std::ftell(file)!=static_cast<long>(BackgroundOffset+BackgroundCount*sizeof sidebarReadBuffer) ||
        std::fseek(file,0,SEEK_SET)!=0 ||
        std::fread(header,sizeof header,1,file)!=1 || header[0]!=0x48474355 || header[1]!=BackgroundCount ||
        std::fread(ids,sizeof ids,1,file)!=1 || !std::equal(std::begin(ids),std::end(ids),std::begin(BackgroundIds)) ||
        std::fread(pondAtlas,sizeof pondAtlas,1,file)!=1) return false;
    decorationsPath=path;
    sceKernelDcacheWritebackRange(pondAtlas,sizeof pondAtlas);
    return true;
}

bool loadBackrooms(const char* path) {
    std::FILE* file=std::fopen(path,"rb");
    if (!file) return false;
    uint32_t magic=0;
    const bool ok=std::fread(&magic,sizeof magic,1,file)==1 && magic==0x42424355 &&
                  std::fseek(file,sizeof magic+sizeof fallbackPage,SEEK_SET)==0 &&
                  std::fread(backroomsTv,sizeof backroomsTv,1,file)==1 &&
                  std::fgetc(file)==EOF && !std::ferror(file);
    std::fclose(file);
    if (ok) {
        backroomsPath=path;
        sceKernelDcacheWritebackRange(backroomsTv,sizeof backroomsTv);
    }
    return ok;
}

bool loadKeyDoors(const char* path) {
    std::FILE* file=std::fopen(path,"rb");
    if (!file) return false;
    const bool ok=readKeyDoorData(file,keyDoorData) &&
                  std::fread(keyDoorSprites,sizeof keyDoorSprites,1,file)==1 &&
                  std::fread(roundKeySprites,sizeof roundKeySprites,1,file)==1 &&
                  std::fgetc(file)==EOF && !std::ferror(file);
    std::fclose(file);
    if (!ok) return false;
    for (unsigned index=0;index<KeyDoorStageCount;++index) {
        const auto& stage=keyDoorData.stages[index];
        if ((index==FactoryKeyDoorStage && stage.doorCount!=2) ||
            (index==ArcadeKeyDoorStage && stage.doorCount!=3)) return false;
        for (unsigned i=0;i<stage.keyCount;++i) {
            const auto& key=stage.keys[i];
            if (!std::isfinite(key.position.x) || !std::isfinite(key.position.y) || !(key.color>>24)) return false;
        }
        for (unsigned i=0;i<stage.doorCount;++i) {
            const auto& door=stage.doors[i];
            const auto& art=stage.doorArt[i];
            if (!std::isfinite(door.center.x) || !std::isfinite(door.center.y) ||
                !std::isfinite(door.halfSize.x) || !std::isfinite(door.halfSize.y) ||
                door.halfSize.x<=0 || door.halfSize.y<=0 || !(stage.doorColors[i]>>24)) return false;
            if (index!=FactoryKeyDoorStage && index!=ArcadeKeyDoorStage &&
                (art.u<0 || art.v<0 || art.width<=0 || art.height<=0 ||
                 art.u+art.width>static_cast<int>(KeyDoorAtlasWidth) ||
                 art.v+art.height>static_cast<int>(KeyDoorAtlasHeight))) return false;
        }
    }
    sceKernelDcacheWritebackRange(keyDoorSprites,sizeof keyDoorSprites);
    sceKernelDcacheWritebackRange(roundKeySprites,sizeof roundKeySprites);
    return true;
}

bool loadBackground(const BackgroundFile& file) {
    backgroundPreference=BackgroundPreference::decode(file.read(BackgroundPreference{}.encode()));
    unsigned id=backgroundPreference.chosen;
    if (backgroundPreference.random) {
        const unsigned last=backgroundIndex(backgroundPreference.last);
        unsigned index=1+sceKernelGetSystemTimeLow()%(BackgroundCount-1-(last!=0));
        if (last && index>=last) ++index;
        id=BackgroundIds[index];
    }
    if (!selectBackground(id)) return false;
    if (backgroundPreference.random) {
        backgroundPreference.last=id;
        settingsError=!file.write(backgroundPreference.encode());
    }
    return true;
}

bool changeBackground(const BackgroundFile& file,unsigned id,bool random) {
    const unsigned previous=activeBackground;
    if (!selectBackground(id)) return false;
    const BackgroundPreference next{id,id,random};
    if (!file.write(next.encode())) {
        selectBackground(previous);
        return false;
    }
    backgroundPreference=next;
    return true;
}
ObjectAssets objects;
ThoughtPlayer thoughts;
ResultAssets resultAssets;
RankPlayer ranks;
Progress progress;

bool earnedSwim045() {
    return progress.next>=CosmicFirstCourse || progress.best[Elysian518Course].rank>0;
}
bool chillMode=false;
unsigned menuCourse=0;
WorldResultsState worldResults;
unsigned currentCourse = 0;
RideAtlas033<SelectorHorse041Width,SelectorHorse041Height,SelectorHorse041Count> elysian1011Titles;
RideAtlas033<SelectorHorse042Width,SelectorHorse042Height,SelectorHorse042Count> elysian1213Titles;
ParkPatchBank<SelectorHorse043Width,SelectorHorse043Height,SelectorHorse043Count> elysian1415Titles;
ParkPatchBank<SelectorHorse044Width,SelectorHorse044Height,SelectorHorse044Count> elysian1617Titles;
ParkPatchBank<Selector045Width,Selector045Height,Selector045Count> finaleTitles045;
ParkPatchBank<Selector046Width,Selector046Height,Selector046Count> cosmicTitles046;
std::string cosmicTitlePath046,cosmicTitlePath047,cosmicTitlePath048,cosmicTitlePath049,cosmicTitlePath050,cosmicTitlePath051,cosmicTitlePath052,cosmicTitlePath053;
ParkPatchBank<Selector049Width,Selector049Height,Selector049Count> cosmicTitles049;
ParkPatchBank<Selector050Width,Selector050Height,Selector050Count> cosmicTitles050;
ParkPatchBank<Selector051Width,Selector051Height,Selector051Count> cosmicTitles051;
ParkPatchBank<Selector052Width,Selector052Height,Selector052Count> cosmicTitles052;
ParkPatchBank<Selector053Width,Selector053Height,Selector053Count> cosmicTitles053;
ParkPatchBank<Selector048Width,Selector048Height,Selector048Count> cosmicTitles048;
ParkPatchBank<Selector047Width,Selector047Height,Selector047Count> cosmicTitles047;
unsigned playableCourseCount=CourseCount;
LevelSelectState levelSelect;
LevelSelectArt levelSelectArt;
SelectorTransitionArt selectorTransitionArt;
bool selectorGoArmed=false,selectorGoDown=false;
MenuButtonArt menuButtonArt;
ProfileArt profileArt;
std::string menuScrollPack;
bool menuButtonDown=false;
alignas(64) uint32_t selectorDetails[SelectorDetails::Width*SelectorDetails::Height]{};
alignas(64) uint32_t selectorFrontierTitles[128*256]{};
alignas(64) uint32_t selectorNextWorldTitles[SelectorDetails::NextWorldTitleWidth*SelectorDetails::NextWorldTitleHeight]{};
alignas(64) uint32_t selectorParkTitles[SelectorDetails::ParkTitleSide*SelectorDetails::ParkTitleSide]{};
alignas(64) uint32_t selectorFactoryTitles[SelectorDetails::FactoryTitleWidth*SelectorDetails::FactoryTitleHeight]{};
alignas(64) uint32_t selectorFinalParkTitles[SelectorDetails::FinalParkTitleWidth*SelectorDetails::FinalParkTitleHeight]{};
alignas(64) uint32_t selectorExtraParkTitles[SelectorDetails::ExtraParkTitleWidth*SelectorDetails::ExtraParkTitleHeight]{};
alignas(64) uint32_t selectorDogTitles[SelectorDetails::DogTitleWidth*SelectorDetails::DogTitleHeight]{};
alignas(64) uint32_t selectorNextTitles[SelectorDetails::NextTitleWidth*SelectorDetails::NextTitleHeight]{};
alignas(64) uint32_t selectorLateParkTitles[SelectorDetails::LateParkTitleSide*SelectorDetails::LateParkTitleSide]{};

RideAtlas033<128,256,4> elysian0607Titles;

void openLevelSelect(Flow& flow,unsigned preferred=LevelSelectState::NoCourse) {
    elysian0607Titles.invalidateShared();
    elysian1011Titles.invalidateShared();
    elysian1213Titles.invalidateShared();
    const unsigned available=chillMode?std::min(playableCourseCount,ElysianFieldsCourse):playableCourseCount;
    levelSelect.open(progress,available,preferred);
    selectorGoArmed=selectorGoDown=false;
    menuCourse=levelSelect.course();
    flow.screen=Screen::LevelSelect;
}

unsigned resumeCourse() {
    return std::min(static_cast<unsigned>(progress.next),playableCourseCount-1);
}

void leaveWorldResults(Flow& flow,bool nextWorld) {
    const bool proceed=nextWorld && worldResults.nextAvailable();
    if (proceed) {
        menuCourse=worldResults.courseFor(WorldResultsChoice::NextWorld);
        flow.screen=Screen::Hole;
    } else {
        openLevelSelect(flow,currentCourse);
    }
}
TileField rideStart,rideEnd;
RideArt033 rideArt;
RideAtlas033<256,256,30> rideSprites;
RideAtlas033<512,512,5> rideScenery;
RideAtlas033<128,64,2> snowArt;
RideAtlas033<128,256,4> frontierLateTitles;
RideAtlas033<128,256,4> frontier0607Titles;
RideAtlas033<128,256,4> frontier0809Titles;
RideAtlas033<SelectorFrontier18501Width,SelectorFrontier18501Height,SelectorFrontier18501Count> frontier18501Titles;
RideAtlas033<HorseObjects039Width,HorseObjects039Height,HorseObjects039Count> horseObjects039;
RideAtlas033<HorseMeter039Width,HorseMeter039Height,HorseMeter039Count> horseMeter039;
RideAtlas033<128,128,1> elysianClouds507;
RideAtlas033<ElysianClouds510Width,ElysianClouds510Height,ElysianClouds510Count> elysianClouds510;
RideAtlas033<ElysianClouds508Width,ElysianClouds508Height,ElysianClouds508Count> elysianClouds508;
RideAtlas033<ElysianClouds509Width,ElysianClouds509Height,ElysianClouds509Count> elysianClouds509;
RideAtlas033<ElysianBlock509Width,ElysianBlock509Height,ElysianBlock509Count> elysianBlock509;
RideAtlas033<128,64,4> elysian0203Titles;
RideAtlas033<SelectorHorse039Width,SelectorHorse039Height,SelectorHorse039Count> elysian0405Titles;
RideAtlas033<SelectorHorse040Width,SelectorHorse040Height,SelectorHorse040Count> elysian0809Titles;
RideAtlas033<FrontierPrism418Width,FrontierPrism418Height,FrontierPrism418Count> frontierPrism418;
RideAtlas033<FrontierLight418Width,FrontierLight418Height,FrontierLight418Count> frontierLight418;
RideAtlas033<FrontierBed418Width,FrontierBed418Height,FrontierBed418Count> frontierBed418;
RideAtlas033<FrontierClouds501Width,FrontierClouds501Height,FrontierClouds501Count> frontierClouds501;
RideAtlas033<SelectorFrontier1213Width,SelectorFrontier1213Height,4> frontier1213Titles;
RideAtlas033<SelectorFrontier1415Width,SelectorFrontier1415Height,SelectorFrontier1415Count> frontier1415Titles;
RideAtlas033<FrontierNightWidth037,FrontierNightHeight037,FrontierNightCount037> frontierNight1415;
RideAtlas033<SelectorFrontier1617Width,SelectorFrontier1617Height,SelectorFrontier1617Count> frontier1617Titles;
RideAtlas033<FrontierPropsWidth416,FrontierPropsHeight416,FrontierPropsCount416> frontierProps416;
RideAtlas033<FrontierPropsWidth417,FrontierPropsHeight417,FrontierPropsCount417> frontierProps417;
RideAtlas033<FrontierSwitchWidth417,FrontierSwitchHeight417,FrontierSwitchCount417> frontierSwitchTiles417;
RideAtlas033<FrontierGateWidth417,FrontierGateHeight417,FrontierGateCount417> frontierGateTiles417;
RideAtlas033<FrontierLightsWidth1617,FrontierLightsHeight1617,FrontierLightsCount1617> frontierLights1617;
RideAtlas033<FrontierNightWidth036,FrontierNightHeight036,FrontierNightCount036> frontierNight1213;
RideAtlas033<128,256,4> frontier1011Titles;
RideAtlas033<128,256,2> frontier1011Lights;
RideAtlas033<64,64,1> frontierMineBlock;
static_assert(std::size(FrontierBlocks411)<=DenseBlocks::Capacity);
constexpr unsigned Frontier408OverlayBytes=SnowSwitchAtlasWidth035*SnowSwitchAtlasHeight035;
constexpr unsigned Frontier408FacesOffset=Frontier408OverlayBytes+128*128;
static_assert(Frontier408FacesOffset+128*64<=ChuckleArt::TextureBytes);
constexpr unsigned Frontier408PatchCount=SnowSwitchPatchCounts035[0]+SnowSwitchPatchCounts035[1]+
    SnowSwitchPatchCounts035[2]+SnowSwitchPatchCounts035[3];
RideAtlas033<SnowSwitchAtlasWidth035,SnowSwitchAtlasHeight035,Frontier408PatchCount> frontier408Tiles;
static_assert(Frontier408OverlayBytes+128*128<=ChuckleArt::TextureBytes);
RideAtlas033<128,128,30> frontier408Chuckles;
LiveResultArt elysianSwitchArt512;
LiveResultArt elysianSwitchArt514,elysianTreesArt515;
RideAtlas033<FrontierClouds501Width,FrontierClouds501Height,FrontierClouds501Count> elysianClouds514;
RideAtlas033<ElysianMist043Width,ElysianMist043Height,ElysianMist043Count> elysianMist043;
ParkPatchBank<ElysianSwitchFaces514Width,ElysianSwitchFaces514Height,ElysianSwitchFaces514Count> elysianSwitchFaces514;
ParkPatchBank<ElysianPortals515Width,ElysianPortals515Height,ElysianPortals515Count> elysianPortalArt515;
ParkPatchBank<ElysianWizard516Width,ElysianWizard516Height,ElysianWizard516Count> wizardArt516;
ParkPatchBank<ElysianSpells516Width,ElysianSpells516Height,ElysianSpells516Count> spellArt516;
LiveResultArt::Slot elysianSwitchSlot512;
RideAtlas033<ValleyArtWidth035,ValleyArtHeight035,ValleyArtCount035> frontier409Night;
RideAtlas033<128,64,12> frontierMazePortals;
RideAtlas033<64,64,1> frontierBigCoin;
RideAtlas033<128,64,11> frontierNukeArt;
RideAtlas033<256,256,20> frontierNukeExplosions;
RideAtlas033<256,128,12> frontierDogArt;
RideAtlas033<FrontierDogTileWidth035,FrontierDogTileHeight035,FrontierDogTileCount035> frontierDogTiles;
constexpr PortalStage FrontierMazePortals{FrontierPortals406,std::size(FrontierPortals406)};
static_assert(std::size(FrontierNukes407)<=NukeRun::Capacity);
constexpr unsigned switchPatchTotal034(unsigned level) {
    const auto& counts=SnowSwitchPatchCounts034[level];
    return counts[0]+counts[1]+counts[2]+counts[3];
}
RideAtlas033<SnowSwitchAtlasSize034[0][0],SnowSwitchAtlasSize034[0][1],switchPatchTotal034(0)> wholesomeTiles;
RideAtlas033<SnowSwitchAtlasSize034[1][0],SnowSwitchAtlasSize034[1][1],switchPatchTotal034(1)> cubeTownTiles;
RideAtlas033<128,64,4> frontierLateFaces;
RideAtlas033<128,64,2> wholesomePortalArt;
ClipPlayer clips;
AudioMixer audio;

void stopRideAudio033() {
    audio.stop(Sound::RideCoaster);
    audio.stageCue(StageCue::None);
}

constexpr bool isHorseCourse039(unsigned index) {
    static_assert(FirstHorseCourse==80 && SecondHorseCourse==81);
    static_assert(std::size(ElysianHorseCourses040)==CosmicFirstCourse-FirstHorseCourse);
    return index>=FirstHorseCourse && index<CosmicFirstCourse;
}

constexpr bool hasHorseCourseGhost040(unsigned index) {
    return index==Elysian507Course || index==Elysian508Course || index==Elysian511Course;
}

constexpr bool isElysianHazardCourse040(unsigned index) {
    static_assert(Elysian508Course==84 && Elysian509Course==85);
    return index==Elysian508Course || index==Elysian509Course;
}

constexpr uint32_t elysianHazardTint040(unsigned index) {
    if (index==Elysian507Course || index==Elysian508Course) return GU_RGBA(242,245,255,255);
    if (index==Elysian509Course) return GU_RGBA(230,236,255,255);
    if (index==Elysian510Course) return GU_RGBA(230,236,255,255);
    if (index==Elysian511Course || index==Elysian512Course) return GU_RGBA(217,226,255,255);
    if (index==Elysian513Course || index==Elysian514Course) return GU_RGBA(204,217,255,255);
    if (index>=Elysian515Course && index<CosmicFirstCourse) return GU_RGBA(191,207,255,255);
    return White;
}

Sound courseMusic(unsigned index) {
    if (index==CosmicEighteenthCourse) return Sound::Count;
    if (index>=CosmicSecondCourse) return Sound::CosmicChampionship;
    if (index==CosmicFirstCourse) return Sound::WallMusic;
    if (isHorseCourse039(index)) return Sound::HorseMusic;
    if (index<TutorialCount) return Sound::Course;
    if (index<GlowstickFirstCourse) return Sound::GolfCentral;
    if (index<ChuckleParkFirstCourse) return Sound::GlowstickCity;
    if (index==TheWallCourse) return Sound::WallMusic;
    if (index>=ElysianFieldsCourse) return Sound::ElysianMusic;
    return index<FinalFrontierFirstCourse?Sound::ChucklePark:Sound::FinalFrontier;
}

void startCourseMusic(unsigned index) {
    audio.musicTune(1,1);
    if ((index==TheWallCourse && chillMode) || index==CosmicEighteenthCourse) audio.muteMusic();
    else audio.music(courseMusic(index));
}

void playWorldCues(unsigned cues) {
    constexpr Sound effects[]={Sound::WorldHop,Sound::WorldUnlock,Sound::WorldJump,
                               Sound::WorldJump,Sound::Bounce,Sound::Bounce};
    static_assert(sizeof effects/sizeof *effects==static_cast<unsigned>(WorldCue::Music));
    for (unsigned i=0;i<sizeof effects/sizeof *effects;++i) {
        if (cues&(1u<<i)) audio.play(effects[i]);
    }
    if (cues&cueMask(WorldCue::HopLate)) audio.play(Sound::WorldHop);
    if (cues&cueMask(WorldCue::Wobble)) audio.play(Sound::WorldWobble);
    if (cues&cueMask(WorldCue::Phwing)) audio.play(Sound::WorldPhwing);
    if (cues&cueMask(WorldCue::Drumroll)) audio.play(Sound::WorldDrumroll);
    if (cues&cueMask(WorldCue::DrumrollStop)) audio.stop(Sound::WorldDrumroll);
    if (cues&cueMask(WorldCue::LandThird)) audio.play(Sound::Bounce);
    for (unsigned i=static_cast<unsigned>(WorldCue::JumpThird);
         i<=static_cast<unsigned>(WorldCue::JumpEighth);++i) {
        if (cues&(1u<<i)) audio.play(Sound::WorldJump);
    }
    if (cues&cueMask(WorldCue::FrontierSilence)) audio.muteMusic();
    if (cues&cueMask(WorldCue::Music)) {
        const auto world=worldResults.world();
        audio.music(world==CompletedWorld::ChucklePark?Sound::Onto4:
                    world==CompletedWorld::GlowstickCity?Sound::Onto3:
                    world==CompletedWorld::GolfCentral?Sound::Onto2:Sound::Onto1);
    }
}

struct ColorVertex { uint32_t color; float x, y, z; };
struct TextureVertex { float u, v; uint32_t color; float x, y, z; };

template<std::size_t N>
constexpr StopTokenStage pursuitTokens(const Vec2 (&positions)[N]) {
    static_assert(N<=MaxStopTokens);
    StopTokenStage stage{};
    stage.count=N;
    for (unsigned i=0;i<N;++i) {
        stage.positions[i]=positions[i];
        stage.colors[i]=White;
    }
    return stage;
}
constexpr StopTokenStage WholesomeStop=pursuitTokens(FrontierStopTokens404);
constexpr StopTokenStage WholesomeSpeed=pursuitTokens(FrontierSpeedTokens404);
constexpr StopTokenStage PursuitStop=pursuitTokens(ParkPursuitStopTokens);
constexpr StopTokenStage PursuitSpeed=pursuitTokens(ParkPursuitSpeedTokens);
constexpr StopTokenStage RaceStop=pursuitTokens(ParkStopTokens317);
constexpr StopTokenStage RaceSpeed=pursuitTokens(ParkSpeedTokens317);
constexpr StopTokenStage ProductionStop=pursuitTokens(FrontierStopTokens407);
constexpr StopTokenStage ProductionSpeed=pursuitTokens(FrontierSpeedTokens407);
constexpr StopTokenStage FrontierSpeed=pursuitTokens(FrontierSpeedTokens402);
template<std::size_t N>
constexpr std::array<ChuckleSpawner,N> stageSpawners(const ParkChuckleSpawner (&config)[N]) {
    std::array<ChuckleSpawner,N> out{};
    for (unsigned i=0;i<N;++i) out[i]={config[i].center,config[i].countdown,config[i].rate,config[i].count};
    return out;
}
constexpr auto ChuckleSpawners316=stageSpawners(ParkChuckles316);
constexpr auto ChuckleSpawners317=stageSpawners(ParkChuckles317);
constexpr unsigned doorScore(unsigned index) { return index==RacetrackCourse?150:index==CosmicTenthCourse?CosmicDoorScore610:index==CosmicSecondCourse?200:KeyDoorRun::DoorBonus; }

bool usesParkRingStage(unsigned index) { return index==ArcadeCourse || index==PursuitCourse || multiDogStage(index); }
bool usesRingTimers(unsigned index) { return index==Elysian512Course || index==CosmicFifthCourse || index==CosmicEleventhCourse || index==CosmicFifteenthCourse || index==CosmicSeventeenthCourse || usesParkRingStage(index); }

bool hasUnskippableFrontierHole(unsigned index) {
    return index==TheWallCourse || hidesElysianScoring(index) || index==Elysian518Course || index==CosmicFirstCourse;
}

[[maybe_unused]] bool cannotSkipCourse(unsigned index) {
    return hasUnskippableFrontierHole(index) || index==CosmicEighteenthCourse;
}

WallContact sweepCosmic053(Vec2 position,Vec2 motion,float radius);
void stopLastHurrahSounds053();

struct Round {
    Game game;
    HorseBroadcast039 horseBroadcast;
    Controls controls;
    CoinRun coins;
    PoolRun pool{};
    StopTokenRun stopRun{},speedRun{};
    SlotMachineRun slots{};
    const StopTokenStage* stopStage=nullptr;
    MotionTokenView speedStage;
    TerryRun terry;
    SkullRun skulls;
    BaddieRun baddies;
    JumpRun jump{};
    JumpStageView jumpStage;
    JumpStage elysianPads515{};
    PortalNode elysianPortals515[std::size(ElysianPortalNodes515)]{};
    JumpPad rotatingPads[std::size(FrontierJumpPads406)]{};
    unsigned skullCount=0;
    NukeRun nukes;
    FrontierSkullRun416 skullSpawner416;
    FrontierFinaleRun417 finale417;
    WallRun418 wall418;
    HorseRun039 horse;
    bool liftHorse=false;
    PoliceRun police{};
    SecretRunner secret{};
    NewsRun news{};
    bool fellOut=false;
    KeyDoorRun roundKeys{};
    FrontierFlashRun frontierFlash;
    FrontierWindRun frontierWind;
    WizardRun516 wizard;
    HorseFinale518Run finale518;
    unsigned cosmicGateBroken=0,cosmicGatePoor=0;
    bool cosmicGateEligible=false,cosmicGateInside=false,finaleMineLoss=false;
    unsigned moonMoo=6,mineLossTick518=0;
    unsigned lastWizardMagic=3;
#ifdef UCG_TEST_044
    unsigned focusedHorseHits044=0;
#endif
    const KeyDoorStage* keyStage=nullptr;
    SmileyRingRun smiley,arcadeRings[std::size(ParkRingStages)]{};
    SmileyRingState<ParkPursuitCoinCapacity> pursuitRings[std::size(ParkPursuitRingStages)]{};
    DogGolfRun dog;
    DogGolfRun mineDogs[std::size(FrontierDogs411)];
    CircleCollider mineDogBodies[std::size(FrontierDogs411)]{};
    MultiDogRun dogs;
    ElysianTraffic513 elysianTraffic;
    unsigned elysianRingTick=0;
    SmileyRingRun dogRings[2]{};
    ParkScareRun parkScare{};
    Metronome048 rhythm;
    Polyrhythm049 polyrhythm;
    BaddieRun cosmicSkulls;
    MovingGoal051 movingGoal;
    SantaRun052 santa;
    LastHurrah053 lastHurrah;
    bool rhythmRecovery617=false;
    FakeGoals052 fakeGoals;
    SwitchRun rhythmWalls;
    bool rhythmTestClock=false;
    SwitchRun switches;
    ChuckleRun chuckles;
    MarcyRun marcy{};
    CoasterRun coaster;
    CoasterRun033 ride;
    const SwitchStage* switchStage=nullptr;
    PortalRun portals;
    unsigned questKeyTick=0;
    const Course& scene;
    GhostMotion ghost{},extraGhosts[std::size(FrontierExtraGhosts409)]{};
    unsigned extraGhostCount=0;
    unsigned horseGhostGraze=0;
    bool horseGhostCaught=false;
    bool nextCarHorn=false;
    unsigned ticks = 0, totalTicks = 0;
    bool caught = false, inSand = false, paused = false, cleanPause = false, waterDead = false, backrooms = false;
    BlockMask brokenBlocks;
    unsigned blockMultiplier=0,blockBonus=0,splashTick=0;
    unsigned blockTicks[DenseBlocks::Capacity]{},blockValues[DenseBlocks::Capacity]{};
    PauseMenuState pauseMenu{};
    unsigned tvPausedTicks=0,tvPauseOffset=0;

    explicit Round(const Course& course) : game(course, &walls), scene(course) { restart(); }

    unsigned bonus() const {
        unsigned ringBonus=smiley.bonus;
        for (const auto& ring:arcadeRings) ringBonus+=ring.bonus;
        for (const auto& ring:pursuitRings) ringBonus+=ring.bonus;
        for (const auto& ring:dogRings) ringBonus+=ring.bonus;
        return fakeGoals.bonus+ride.bonus+coins.bonus+blockBonus+skulls.bonus+baddies.bonus+chuckles.bonus+ringBonus+slots.bonus+
               (keyStage?roundKeys.bonus:0)+(currentCourse==CataclysmCourse?finale417.bonus():0);
    }
    unsigned tvTick() const { return totalTicks+tvPauseOffset+tvPausedTicks; }

    void restart() {
        game.restart();
        game.goalEnabled=currentCourse!=CosmicSeventeenthCourse && currentCourse!=CataclysmCourse && currentCourse!=TheWallCourse &&
            currentCourse!=HorseBroadcastCourse && !isHorseCourse039(currentCourse);
        horse={};
        wizard={};
        finale518.restart(currentCourse==Elysian518Course && progress.quest>=2 && !progress.elysianKey);
        finaleMineLoss=false;
        mineLossTick518=0;
        cosmicGateBroken=cosmicGatePoor=0;
        cosmicGateInside=false;
        cosmicGateEligible=currentCourse==CosmicFirstCourse && progress.totalPrisms()>=PrismGateRequired601;
        moonMoo=6;
        if (currentCourse==Elysian518Course) {
            audio.musicTune(1,1);
            for (Sound sound:{Sound::HorseDeathSupreme,Sound::HorseMissileBoom,Sound::HorseSurprise,
                              Sound::HorseKeyHoly,Sound::HorseKeyProverb,Sound::HorseKeyProverbDelay,
                              Sound::GoldenClubThwack}) audio.stop(sound);
            audio.music(Sound::HorseMusic);
        }
        lastWizardMagic=3;
#ifdef UCG_TEST_044
        focusedHorseHits044=0;
#endif
        if (currentCourse==Elysian516Course) {
            audio.rideVolume(1,-80,-10,0.8f);
            audio.restartStageCue(StageCue::RideWind);
            audio.stop(Sound::WizardMagic1);
            audio.stop(Sound::WizardMagic2);
            audio.stop(Sound::WizardMagic3);
            audio.stop(Sound::WizardDeath);
        }
        liftHorse=false;
        if (isHorseCourse039(currentCourse)) horse.restart(ElysianHorseCourses040[currentCourse-FirstHorseCourse]);
        horseBroadcast={};
        const bool isWall=currentCourse==TheWallCourse;
        wall418.restart(isWall?progress.totalPrisms():0,isWall && chillMode);
        skullSpawner416.restart();
        finale417.restart(currentCourse==CataclysmCourse && progress.quest>=2 && !progress.frontierKey);
        if (currentCourse==CataclysmCourse) audio.stageCue(StageCue::None);
        controls = {};
        ride.restart(false,chillMode);
        if (currentCourse==ChuckleCoasterCourse) {
            stopRideAudio033();
            audio.rideVolume(1,-80,-80);
            audio.pauseStageCue(false);
        }
        coins = {};
        nukes={};
        pool = {};
        stopRun={};
        speedRun={};
        slots={};
        const bool glowstick=currentCourse<ChuckleParkFirstCourse;
        stopStage=glowstick && currentCourse>=FirstStopCourse?&stopTokens.stages[currentCourse-FirstStopCourse]:nullptr;
        speedStage=glowstick && currentCourse>=GlowstickFirstCourse+3?
            MotionTokenView(speedTokens.stages[currentCourse-FirstStopCourse]):MotionTokenView{};
        if (currentCourse==CarouselCourse) stopStage=&stopTokens.stages[GlowstickArt::Courses-1];
        if (currentCourse==FactoryCourse) {
            stopStage=&stopTokens.stages[FactoryTokenStage];
            speedStage=speedTokens.stages[FactoryTokenStage];
        }
        if (currentCourse==PursuitCourse) {
            stopStage=&PursuitStop;
            speedStage=PursuitSpeed;
        }
        if (currentCourse==RacetrackCourse) {
            stopStage=&RaceStop;
            speedStage=RaceSpeed;
        }
        if (currentCourse==AbandonedWorkspaceCourse) stopStage=&FrontierStop415;
        if (currentCourse==FrontierSwitchCourse) speedStage=FrontierSpeed;
        if (currentCourse==WholesomeCourse) {
            stopStage=&WholesomeStop;
            speedStage=WholesomeSpeed;
        }
        if (currentCourse==FrontierDogCourse) {
            stopStage=&ProductionStop;
            speedStage=ProductionSpeed;
        }
        if (currentCourse==PlentifulChucklesCourse) stopStage=&FrontierStop408;
        if (currentCourse==UncannyValleyCourse) stopStage=&FrontierStop409;
        if (currentCourse==CataclysmCourse) stopStage=&FrontierStop417;
        if (currentCourse==Elysian510Course) {
            stopStage=&ElysianStop510;
            speedStage=ElysianSpeed510;
        }
        if (currentCourse==CosmicSecondCourse) speedStage=CosmicSpeed602;
        if (currentCourse==CosmicThirdCourse) {
            speedStage=CosmicSpeed603;
            stopStage=&CosmicStop603;
        }
        if (currentCourse==CosmicFifthCourse) {
            speedStage=CosmicSpeed605;
            stopStage=&CosmicStop605;
        }
        if (currentCourse==CosmicEighthCourse) stopStage=&CosmicStop608;
        if (currentCourse==CosmicSixteenthCourse) speedStage=CosmicSpeed616;
        if (currentCourse==CosmicSeventeenthCourse) {
            speedStage=CosmicSpeed617;
            stopStage=&CosmicStop617;
        }
        if (currentCourse==CosmicFourteenthCourse) {
            stopStage=&CosmicStop614;
            speedStage=CosmicSpeed614;
        }
        if (currentCourse==CosmicFifteenthCourse) {
            stopStage=&CosmicStop615;
            speedStage=CosmicSpeed615;
        }
        terry = {};
        questKeyTick=0;
        skulls={};
        jump={};
        jumpStage={};
        if (currentCourse>=JumpCourse && currentCourse<=GlowstickFinaleCourse)
            jumpStage=jumpStages[currentCourse-JumpCourse];
        if (currentCourse==FactoryCourse) jumpStage=jumpStages[FactoryJumpStage];
        if (currentCourse==ArcadeCourse) jumpStage=jumpStages[ArcadeJumpStage];
        if (currentCourse==RacetrackCourse) jumpStage=ParkJump317;
        if (currentCourse==Elysian511Course) jumpStage=ElysianJump511;
        if (currentCourse==Elysian518Course) jumpStage=ElysianJump518;
        if (currentCourse==CosmicSecondCourse) jumpStage=CosmicJump602;
        if (currentCourse==CosmicThirdCourse) jumpStage=CosmicJump603;
        if (currentCourse==CosmicEleventhCourse) jumpStage={CosmicJump611,std::size(CosmicJump611)};
        if (currentCourse==CosmicTwelfthCourse) jumpStage={CosmicJump612,std::size(CosmicJump612)};
        if (currentCourse==CosmicFifteenthCourse) jumpStage={CosmicJump615,std::size(CosmicJump615)};
        if (currentCourse==CosmicSixteenthCourse) jumpStage={CosmicJump616,std::size(CosmicJump616)};
        if (currentCourse==CosmicSeventeenthCourse) jumpStage={CosmicJump617,std::size(CosmicJump617)};
        if (currentCourse==Elysian515Course) {
            elysianPads515=ElysianJump515;
            std::copy(std::begin(ElysianPortalNodes515),std::end(ElysianPortalNodes515),elysianPortals515);
            updateElysianMotion515(0,elysianPads515,elysianPortals515);
            jumpStage=elysianPads515;
        }
        if (currentCourse==FrontierMazeCourse) {
            std::copy(std::begin(FrontierJumpPads406),std::end(FrontierJumpPads406),rotatingPads);
            jumpStage={rotatingPads,std::size(rotatingPads)};
        }
        if (currentCourse==FrontierDogCourse)
            jumpStage={FrontierJumpPads407,std::size(FrontierJumpPads407)};
        if (currentCourse==UncannyValleyCourse)
            jumpStage={FrontierJump409,std::size(FrontierJump409)};
        if (currentCourse==WeatherAlertCourse)
            jumpStage={FrontierJump412,std::size(FrontierJump412)};
        if (usesRingTimers(currentCourse)) {
            audio.stopRingTimers();
            audio.pauseStageCue(false);
        }
        for (auto& ring:arcadeRings) ring={};
        for (auto& ring:pursuitRings) ring={};
        if (const auto* dogScene=dogSceneAt(currentCourse)) dog.restart(*dogScene);
        if (currentCourse==FrontierMineCourse) {
            for (unsigned i=0;i<std::size(mineDogs);++i) {
                mineDogs[i].restart(mineDogScene(i));
                mineDogBodies[i]=mineDogs[i].body;
            }
        }
        if (const auto* stage=multiDogStage(currentCourse)) dogs.restart(*stage);
        for (auto& ring:dogRings) ring={};
        parkScare={};
        if (police.triggeredAt || secret.startedAt || smiley.everStarted || marcy.startedAt)
            audio.stageCue(StageCue::None);
        if (marcy.startedAt && currentCourse==MartCourse) startCourseMusic(currentCourse);
        marcy.restart(currentCourse==MartCourse && progress.quest>=2 && !progress.parkKey);
        if (smiley.everStarted) audio.stop(Sound::SmileyTimer);
        smiley={};
        elysianRingTick=0;
        switchStage=parkSwitchStage?&parkSwitchStage:nullptr;
        switches.restart(switchStage);
        portals.restart(currentCourse==Elysian515Course?
                            PortalStage{elysianPortals515,std::size(elysianPortals515)}:
                        currentCourse==FrontierMazeCourse?FrontierMazePortals:
                        currentCourse==WholesomeCourse?WholesomePortals:
                        currentCourse==CataclysmCourse?PortalStage{FrontierPortals417,std::size(FrontierPortals417)}:
                        currentCourse==CosmicEighthCourse?CosmicPortals608:
                        currentCourse==CosmicEleventhCourse?CosmicPortals611:
                        currentCourse==CosmicFifteenthCourse?CosmicPortals615:frontierPortals(currentCourse));
        chuckles.restart(currentCourse==CoasterCourse?ChuckleCourse::Coaster:
                         currentCourse==MartCourse?ChuckleCourse::Mart:
                         currentCourse==FactoryCourse?ChuckleCourse::Finale:ChuckleCourse::None);
        if (currentCourse==FrontierDogCourse)
            chuckles.restart(FrontierChuckles407,std::size(FrontierChuckles407),scene.right+128.0f);
        if (currentCourse==PlentifulChucklesCourse)
            chuckles.startSpawners(FrontierChuckleSpawners408,std::size(FrontierChuckleSpawners408),FrontierChuckleDespawn408,true);
        if (currentCourse==CrimsonOutpostCourse)
            chuckles.startSpawners(FrontierChuckleSpawners416,std::size(FrontierChuckleSpawners416),FrontierDespawn416);
        if (currentCourse==CosmicSixteenthCourse)
            chuckles.startSpawners(CosmicChuckles616,std::size(CosmicChuckles616),512,true);
        if (currentCourse==CosmicSeventeenthCourse)
            chuckles.startSpawners(CosmicChuckles617,std::size(CosmicChuckles617),768,true);
        if (currentCourse==CosmicFourteenthCourse)
            chuckles.startSpawners(CosmicChuckles614,std::size(CosmicChuckles614),512,true);
        if (currentCourse==CosmicSecondCourse)
            chuckles.restart(CosmicChuckles602,std::size(CosmicChuckles602),scene.right+128.0f);
        if (currentCourse==MirrorCourse) chuckles.restart(MirrorChuckles,std::size(MirrorChuckles),448.0f);
        if (currentCourse==ArcadeCourse) chuckles.restart(ParkChuckles,std::size(ParkChuckles),ParkChuckleDespawn);
        if (currentCourse==DoubleDogCourse) chuckles.startSpawners(ChuckleSpawners316.data(),ChuckleSpawners316.size(),scene.right+128.0f);
        if (currentCourse==RacetrackCourse) chuckles.startSpawners(ChuckleSpawners317.data(),ChuckleSpawners317.size(),scene.right+128.0f);
        if (currentCourse==Elysian514Course)
            chuckles.startSpawners(ElysianChuckles514,std::size(ElysianChuckles514),scene.right+128.0f,true);
        if (currentCourse==Elysian513Course) elysianTraffic.restart(ElysianDogs513,chuckles,ElysianChuckles513,scene.right+128.0f);
        if (hasCoaster(currentCourse)) {
            coaster.restart(sceKernelGetSystemTimeLow());
            audio.stop(coasterSound(currentCourse));
            audio.pauseStageCue(false);
        }
        police={};
        secret={};
        secret.available=currentCourse==CityCourse && progress.quest>=2 && !progress.glowstickKey;
        if (currentCourse>=HighwayCourse && currentCourse<=AlleyCourse) news.reset(0);
        fellOut=false;
        const BaddieStage* baddieStage=nullptr;
        if (currentCourse>=FirstBaddieCourse && currentCourse<=DimlitCourse)
            baddieStage=&baddieStages[currentCourse-FirstBaddieCourse];
        if (currentCourse==HighwayCourse || currentCourse==CityCourse)
            baddieStage=&baddieStages[HighwayBaddieStage+currentCourse-HighwayCourse];
        if (currentCourse==AlleyCourse) baddieStage=&baddieStages[AlleyBaddieStage];
        if (currentCourse==GlowstickFinaleCourse) baddieStage=&baddieStages[FinaleBaddieStage];
        if (currentCourse==CarouselCourse) baddieStage=&baddieStages[CarouselBaddieStage];
        if (currentCourse==UncannyValleyCourse) baddieStage=&FrontierCars409;
        if (currentCourse==CrimsonOutpostCourse) baddieStage=&FrontierCars416;
        if (currentCourse==Elysian509Course) baddieStage=&ElysianBaddie509;
        if (currentCourse==Elysian511Course) baddieStage=&ElysianCars511;
        if (currentCourse==Elysian514Course) baddieStage=&ElysianSkulls514;
        if (currentCourse==CosmicTenthCourse) baddieStage=&CosmicCars610;
        if (currentCourse==CosmicFifteenthCourse) baddieStage=&CosmicCars615;
        if (currentCourse==CosmicSeventeenthCourse) baddieStage=&CosmicCars617;
        cosmicSkulls.restart(currentCourse==CosmicTenthCourse?&CosmicSkulls610:nullptr);
        if (currentCourse==AbandonedWorkspaceCourse) restartFrontierCar415(baddies);
        else baddies.restart(baddieStage,0,currentCourse==CosmicFifteenthCourse?BaddieRun::Capacity:currentCourse==UncannyValleyCourse?BaddieRun::LegacyCapacity:BaddieRun::DefaultCapacity);
        extraGhostCount=0;
        skullCount=0;
        keyStage=nullptr;
        if (currentCourse>=FirstKeyDoorCourse && currentCourse<FirstKeyDoorCourse+KeyDoorGolfStageCount)
            keyStage=&keyDoorData.stages[currentCourse-FirstKeyDoorCourse];
        if (currentCourse==BillboardCourse) keyStage=&keyDoorData.stages[KeyDoorGolfStageCount];
        if (currentCourse==BankCourse) keyStage=&keyDoorData.stages[BankKeyDoorStage];
        if (currentCourse==CityCourse) keyStage=&keyDoorData.stages[CityKeyDoorStage];
        if (currentCourse>=LightboxCourse && currentCourse<=AlleyCourse)
            keyStage=&keyDoorData.stages[LightboxKeyDoorStage+currentCourse-LightboxCourse];
        if (currentCourse==CasinoCourse) keyStage=&keyDoorData.stages[CasinoKeyDoorStage];
        if (currentCourse==CoasterCourse) keyStage=&keyDoorData.stages[CasinoKeyDoorStage+1];
        if (currentCourse==FactoryCourse) keyStage=&keyDoorData.stages[FactoryKeyDoorStage];
        if (currentCourse==ArcadeCourse) keyStage=&keyDoorData.stages[ArcadeKeyDoorStage];
        if (currentCourse==RacetrackCourse) keyStage=&ParkKeys317;
        if (currentCourse==UncannyValleyCourse) keyStage=&FrontierKeys409;
        if (currentCourse==WeatherAlertCourse) keyStage=&FrontierKeys412;
        if (currentCourse==MovieOnceCourse) keyStage=&FrontierKeys414;
        if (currentCourse==AbandonedWorkspaceCourse) keyStage=&FrontierKeys415;
        if (currentCourse==CosmicSecondCourse) keyStage=&CosmicKeys602;
        if (currentCourse==CosmicSeventhCourse) keyStage=&CosmicKeys607;
        if (currentCourse==CosmicTenthCourse) keyStage=&CosmicKeys610;
        if (currentCourse==CosmicFifteenthCourse) keyStage=&CosmicKeys615;
        if (currentCourse==CosmicSixteenthCourse) keyStage=&CosmicKeys616;
        if (currentCourse==CosmicSeventeenthCourse) keyStage=&CosmicKeys617;
        frontierFlash.restart(currentCourse);
        frontierWind={};
        if (currentCourse==WeatherAlertCourse || currentCourse==DownfallCourse)
            frontierWind.restart(currentCourse==DownfallCourse);
        if (currentCourse==MovieOnceCourse) frontierWind.restartMovieOnce();
        if (currentCourse==CosmicFifthCourse) {
            frontierWind.restartStarstorm();
            audio.rideVolume(1,-80,-10,0.8f);
            audio.restartStageCue(StageCue::StarstormWind);
        }
        if (keyStage) roundKeys.restart(*keyStage);
        else roundKeys={};
        if (stopStage) {
            for (unsigned i=0;i<stopStage->ghostCount;++i) extraGhosts[extraGhostCount++]={stopStage->ghosts[i],{}};
        }
        if (currentCourse>=TutorialCount) {
            const auto& stage=courseHazards(currentCourse);
            skullCount=stage.skullCount;
            skulls.restart(stage);
            if (currentCourse==Elysian508Course) {
                skullCount=std::size(ElysianSkulls508);
                skulls.restart(ElysianSkulls508,skullCount);
            }
            if (currentCourse==FrontierDogCourse) {
                skullCount=std::size(FrontierSkulls407);
                skulls.restart(FrontierSkulls407,skullCount);
            } else if (currentCourse==FrontierMineCourse) {
                skullCount=std::size(FrontierSkulls411);
                skulls.restart(FrontierSkulls411,skullCount);
            }
            // 2_09 stores the remaining ghosts in GolfStage, after its StopTokenStage entries.
            if (!stopStage || currentCourse==MuggingCourse || currentCourse==RacetrackCourse ||
                currentCourse==AbandonedWorkspaceCourse || currentCourse==CataclysmCourse ||
                currentCourse==CosmicFifteenthCourse) {
                for (unsigned i=0;i<stage.extraGhostCount;++i)
                    extraGhosts[extraGhostCount++]={stage.extraGhostSpawns[i],{}};
            }
        }
        tvPausedTicks=tvPauseOffset=0;
        thoughts.begin(tvTick());
        ghost={walls.ghostSpawn,{}};
        if (currentCourse==CosmicEighthCourse) {
            extraGhostCount=std::size(CosmicGhosts608)-1;
            for (unsigned i=0;i<extraGhostCount;++i) extraGhosts[i]={CosmicGhosts608[i+1],{}};
        }
        movingGoal={};
        santa={};
        lastHurrah.restart(CosmicFinalGhost617);
        rhythmRecovery617=false;
        if (currentCourse>=CosmicSixteenthCourse) lateWalls053.restart();
        if (currentCourse==CosmicSeventeenthCourse) stopLastHurrahSounds053();
        fakeGoals={};
        if (currentCourse==CosmicFifteenthCourse) santaWalls.setOffset({});
        polyrhythm={};
        if (currentCourse==CosmicEleventhCourse) polyrhythm.mask=Polyrhythm049::Triple;
        rhythm.restart(rhythmTestClock?0:Metronome048::musicBeat(audio.musicPositionFrames(Sound::CosmicChampionship)));
        rhythmWalls.restart(usesMetronome048(currentCourse)?&cosmicRhythmFields048:nullptr);
        if (usesMetronome048(currentCourse)) rhythmWalls.setState(currentCourse==CosmicEleventhCourse?6:Metronome048::Both);
        horseGhostGraze=0;
        horseGhostCaught=false;
        nextCarHorn=(sceKernelGetSystemTimeLow()&1)!=0;
        if (currentCourse==UncannyValleyCourse) {
            extraGhostCount=0;
            for (const auto& position:FrontierExtraGhosts409) extraGhosts[extraGhostCount++]={position,{}};
        }
        ticks = 0;
        caught = false;
        waterDead = false;
        backrooms = false;
        brokenBlocks={};
        blockMultiplier=blockBonus=splashTick=0;
        inSand = false;
        paused = false;
        cleanPause = false;
        pauseMenu={};
    }

    void stepChuckles(const GolfStage& hazards,const TileField* gate=nullptr) {
        if (!chuckles.count && !chuckles.spawnerVisible()) return;
        if (currentCourse==Elysian513Course) {
            stepElysianTraffic513(ticks);
            return;
        }
        const auto* group=multiDogStage(currentCourse);
        chuckles.step(walls,game.position,hazards.blocks,hazards.blockCount,brokenBlocks,
                      keyStage?keyStage->doors:nullptr,keyStage?keyStage->doorCount:0,roundKeys.opened,
                      switchStage?&switches:nullptr,gate,
                      currentCourse==DoubleDogCourse?skulls.bodies:nullptr,
                      currentCourse==DoubleDogCourse?hazards.skullCount:0,
                      group?dogs.bodies:currentCourse==FrontierDogCourse || currentCourse==CosmicSeventeenthCourse?&dog.body:nullptr,
                      group?group->dogCount:currentCourse==FrontierDogCourse || currentCourse==CosmicSeventeenthCourse?1:0,
                      currentCourse==CosmicSixteenthCourse || currentCourse==CosmicSeventeenthCourse?sweepCosmic053:nullptr);
    }

    void stepCosmicEnemies602(const GolfStage& hazards) {
        const auto* door=roundKeys.opened?nullptr:CosmicKeys602.doors;
        skulls.step(walls,skullCount,chuckles.bodies,chuckles.count,scene.right+128.0f,
                    door,nullptr,hazards.blocks,hazards.blockCount,brokenBlocks);
        stepChuckles(hazards);
    }

    void stepElysianTraffic513(unsigned tick) {
        const auto events=elysianTraffic.step(ElysianDogs513,chuckles,walls,game.position,tick);
        if (events.bounced) audio.play(Sound::Bounce);
        if (events.defeated) {
            audio.play(Sound::ChuckleDeath);
            thoughts.play(Thought::Chuckle,tvTick());
        }
    }

    ControlEvent updateControls(Vec2 aim,bool cross,bool retry) {
        if (wall418.bedActive || horse.flying() ||
            (currentCourse==Elysian518Course && !retry && finale518.phase!=FinalePhase518::Hunt &&
             finale518.phase!=FinalePhase518::TeeReady)) return ControlEvent::None;
        const auto event=controls.update(game,scene,wizard.input(aim),cross,retry,
                                         frontierWind.drifting() || wizard.drifting());
        if (currentCourse==Elysian516Course && event==ControlEvent::Shot) wizard.beginMove();
        if (event==ControlEvent::Shot) frontierWind.beginMove();
        return event;
    }

    void togglePause() {
        if (paused) tvPauseOffset+=tvPausedTicks;
        tvPausedTicks=0;
        paused=!paused;
        if (police.triggeredAt || secret.startedAt || smiley.everStarted || marcy.startedAt ||
            hasCoaster(currentCourse) || usesRingTimers(currentCourse) || currentCourse==ChuckleCoasterCourse ||
            frontierWind.active || finale417.revealedAt || currentCourse==Elysian516Course ||
            currentCourse==CosmicThirteenthCourse)
            audio.pauseStageCue(paused);
        if (usesMetronome048(currentCourse) || currentCourse==CosmicSeventeenthCourse) audio.musicTune(1,paused?0.0f:1.0f);
        cleanPause=false;
        pauseMenu={};
        controls={};
    }

    bool rhythmGhostActive(unsigned index) const {
        if (currentCourse==CosmicEighthCourse) return CosmicGhostMasks608[rhythm.phase]&(1u<<index);
        return usesPolyrhythm049(currentCourse) || rhythm.ghostActive(index,1+extraGhostCount);
    }

    bool rhythmCoinActive(unsigned index) const {
        if (currentCourse!=CosmicNinthCourse) return true;
        const unsigned group=(CosmicCoinMasks609[0]&(1u<<index))?Polyrhythm049::Pink:Polyrhythm049::Cyan;
        return polyrhythm.mask&group;
    }

    void chaseGhosts(bool vulnerable) {
        if (chillMode || currentCourse==CosmicEighteenthCourse) return;
        bool caughtByGhost=false;
        if (usesMetronome048(currentCourse) || currentCourse>=CosmicTwelfthCourse) {
            const unsigned count=1+extraGhostCount;
            unsigned mask=0;
            for (unsigned i=0;i<count;++i) if (rhythmGhostActive(i)) mask|=1u<<i;
            caughtByGhost=stepFrontierGhosts409(ghost,extraGhosts,extraGhostCount,game.position,nullptr,mask);
        } else if (currentCourse==CataclysmCourse) {
            caughtByGhost=stepFrontierGhosts417(ghost,extraGhosts,extraGhostCount,finale417,game.position,scene.playerRadius);
        } else if (currentCourse==UncannyValleyCourse || currentCourse==FrontierPatrolCourse ||
            currentCourse==DownfallCourse || currentCourse==MovieOnceCourse ||
            currentCourse==AbandonedWorkspaceCourse) {
            caughtByGhost=stepFrontierGhosts409(ghost,extraGhosts,extraGhostCount,game.position);
        } else {
            caughtByGhost=ghost.chase(game.position);
            for (unsigned i=0;i<extraGhostCount;++i) caughtByGhost|=extraGhosts[i].chase(game.position);
        }
        if (currentCourse==CosmicSeventeenthCourse)
            caughtByGhost|=lastHurrah.chase(game.position,scene.playerRadius);
        if (vulnerable && caughtByGhost) {
            caught=true;
            game.velocity={};
            controls={};
        }
    }

    void chaseHorseCourseGhosts040(bool vulnerable=true) {
        chaseGhosts(vulnerable);
        if (caught) {
            horseGhostCaught=true;
            horse.lose();
            return;
        }
        if (chillMode) return;
        const float radius=24+scene.playerRadius;
        const auto near=[&](Vec2 position) {
            const Vec2 difference=game.position-position;
            return dot(difference,difference)<=radius*radius;
        };
        unsigned mask=vulnerable && near(ghost.position)?1u:0u;
        for (unsigned i=0;i<extraGhostCount;++i)
            if (vulnerable && near(extraGhosts[i].position)) mask|=2u<<i;
        if (horseGhostGraze&~mask) audio.play(Sound::UncannyGraze);
        horseGhostGraze=mask;
    }

    static DogGolfScene mineDogScene(unsigned index) {
        const auto& spawn=FrontierDogs411[index];
        return {spawn.center,spawn.velocity,{},spawn.radius,0,spawn.graphicScale,0,0,spawn.bounceOffPlayer,false};
    }

    void breakBlock(unsigned index) {
        if (brokenBlocks.has(index)) return;
        brokenBlocks.set(index);
        blockTicks[index]=ticks;
        blockValues[index]=10*++blockMultiplier;
        blockBonus+=blockValues[index];
        audio.play(currentCourse>=ChuckleParkFirstCourse?Sound::ParkBlockBreak:Sound::BlockBreak);
    }

    void stepMineDogs() {
        for (unsigned i=0;i<std::size(mineDogs);++i) {
            const auto event=mineDogs[i].step(mineDogScene(i),walls,nullptr,nullptr,
                skulls.bodies,skullCount,nullptr,0,
                {FrontierBlocks411,std::size(FrontierBlocks411),brokenBlocks,
                 mineDogBodies,std::size(mineDogBodies),i});
            mineDogBodies[i]=mineDogs[i].body;
            if (event.bounced) audio.play(Sound::Bounce);
            if (event.block>=0) breakBlock(static_cast<unsigned>(event.block));
            if (event.baddie>=0) {
                skulls.defeat(static_cast<unsigned>(event.baddie),ticks);
                audio.play(Sound::SkullDeath);
                thoughts.play(Thought::Skull,tvTick());
            }
            if (event.dog>=0) {
                const unsigned target=static_cast<unsigned>(event.dog);
                mineDogs[target].struckByPlayer(mineDogScene(target),mineDogs[i].body.center);
            }
        }
    }

    void stepDog(const DogGolfScene& dogScene) {
        const bool production=currentCourse==FrontierDogCourse || currentCourse==CosmicSeventeenthCourse;
        const auto event=dog.step(dogScene,walls,dog.winner || currentCourse==CosmicSeventeenthCourse?nullptr:&dogGate,&switches,
                                  skulls.bodies,production?skullCount:0,
                                  chuckles.bodies,production?chuckles.count:0,
                                  currentCourse==CosmicSeventeenthCourse?
                                  DogGolfPeers{courseHazards(currentCourse).blocks,courseHazards(currentCourse).blockCount,brokenBlocks}:DogGolfPeers{},
                                  currentCourse==CosmicSeventeenthCourse?sweepCosmic053:nullptr);
        if (currentCourse==CosmicSeventeenthCourse) {
            lateWalls053.dogOpen=dog.winner;
            if (event.block>=0) breakBlock(static_cast<unsigned>(event.block));
        }
        if (event.bounced) audio.play(Sound::Bounce);
        if (event.switched) audio.play(Sound::ParkSwitch);
        if (event.won) {
            audio.play(Sound::ParkLightsOut);
            audio.play(Sound::ParkDog);
            thoughts.play(Thought::Dog,tvTick());
        }
        if (event.baddie<0) return;
        const unsigned enemy=static_cast<unsigned>(event.baddie);
        if (enemy<skullCount) {
            skulls.defeat(enemy,ticks);
            audio.play(Sound::SkullDeath);
            thoughts.play(Thought::Skull,tvTick());
        } else if (chuckles.defeat(enemy-skullCount,ticks)) {
            audio.play(Sound::ChuckleDeath);
            thoughts.play(Thought::Chuckle,tvTick());
        }
    }

    void stepProductionSkulls() {
        CircleCollider peers[ChuckleRun::Capacity+1];
        std::copy_n(chuckles.bodies,chuckles.count,peers);
        peers[chuckles.count]=dog.body;
        skulls.step(walls,skullCount,peers,chuckles.count+1,
                    scene.right+128.0f,nullptr,dog.winner?nullptr:&dogGate);
    }

    void stepSlotSkulls(const GolfStage& stage) {
        const SolidBlock* door=keyStage && keyStage->doorCount && !(roundKeys.opened&1)?keyStage->doors:nullptr;
        if (stage.skullCount) skulls.step(walls,stage.skullCount,baddies.skulls,baddies.count,512.0f,door);
        baddies.step(walls,ticks,skulls.bodies,stage.skullCount,door);
    }

    void settleSlots(uint32_t newlyCollected) {
        if (currentCourse!=SlotMachineCourse) return;
        for (unsigned reel=0;reel<SlotMachineRun::ReelCount;++reel) {
            if (!(newlyCollected&(1u<<reel)) || !slots.stop(reel,sceKernelGetSystemTimeLow())) continue;
            constexpr Sound sounds[]={Sound::SlotJackpot,Sound::SlotHappy,Sound::SlotOk,Sound::SlotQuestion,
                Sound::SlotPhone,Sound::SlotCherry,Sound::SlotEye,Sound::SlotBlue,Sound::SlotSad,Sound::SlotSkull,
                Sound::SlotSad,Sound::SlotGood,Sound::SlotSad,Sound::SlotLose};
            static_assert(std::size(sounds)+1==static_cast<unsigned>(SlotOutcome::Count));
            audio.play(sounds[static_cast<unsigned>(slots.outcome)-1]);
            if (slots.outcome==SlotOutcome::Skull)
                baddies.restart(&baddieStages[SlotBaddieStage],slots.ticks,BaddieRun::Capacity);
        }
    }

    template<class Stage,class Ring,std::size_t N,std::size_t R>
    void stepRings(const Stage (&stages)[N],Ring (&rings)[R],const bool (&bells)[N],bool canTouch) {
        static_assert(N<=R);
        for (unsigned i=0;i<N;++i) {
            const auto& stage=stages[i];
            const auto event=rings[i].step(stage,game.position,scene.playerRadius,ticks,canTouch);
            if (event.started) {
                if (bells[i]) audio.play(Sound::SmileyRing);
                audio.playRingTimer(i,stage.timeTicks);
            }
            if (event.newCoins) audio.play(Sound::SmileyCoin);
            if (event.passed || event.failed) {
                audio.playRingTimer(i,0);
                audio.play(event.passed?Sound::SmileyPass:Sound::SmileyFail);
            }
        }
    }

    void stepElysianRing512(bool canTouch) {
        const auto& stage=ElysianRings512[0];
        const auto event=smiley.step(stage,game.position,scene.playerRadius,elysianRingTick,canTouch);
        if (event.started) {
            if (ElysianRingBell512) audio.play(Sound::SmileyRing);
            audio.playRingTimer(0,stage.timeTicks);
        }
        if (event.newCoins) audio.play(Sound::SmileyCoin);
        if (event.passed || event.failed) {
            audio.playRingTimer(0,0);
            audio.play(event.passed?Sound::SmileyPass:Sound::SmileyFail);
        }
    }

    void stepCosmicRing(bool canTouch=true) {
        const auto& stage=currentCourse==CosmicSeventeenthCourse?CosmicRing617:
                          currentCourse==CosmicEleventhCourse?CosmicRing611:CosmicRings605[0];
        const auto event=smiley.step(stage,game.position,scene.playerRadius,ticks,canTouch && !game.won);
        if (event.started) {
            if (currentCourse==CosmicEleventhCourse || currentCourse==CosmicSeventeenthCourse || CosmicRingBell605)
                audio.play(Sound::SmileyRing);
            if (currentCourse==CosmicSeventeenthCourse) audio.play(Sound::HurrahUfo);
            audio.playRingTimer(0,stage.timeTicks);
        }
        if (event.newCoins) audio.play(Sound::SmileyCoin);
        if (event.passed || event.failed) {
            audio.playRingTimer(0,0);
            audio.play(event.passed?Sound::SmileyPass:Sound::SmileyFail);
        }
    }

    void stepWizard516() {
        if (wizard.advance()) {
            unsigned next=sceKernelGetSystemTimeLow()%(lastWizardMagic<3?2:3);
            if (next>=lastWizardMagic) ++next;
            if (lastWizardMagic<3)
                audio.stop(static_cast<Sound>(static_cast<unsigned>(Sound::WizardMagic1)+lastWizardMagic));
            lastWizardMagic=next;
            audio.play(static_cast<Sound>(static_cast<unsigned>(Sound::WizardMagic1)+next));
        }
        audio.rideVolume(1,-80,wizard.volumeDb(),wizard.pitch());
    }

    void stepHorse039(bool lift,bool playerGrounded=true) {
        unsigned health=horse.health;
        const auto event=horse.step(game,scene.playerRadius,horseWalls039,lift,playerGrounded);
#ifdef UCG_TEST_044
        focusedHorseHits044|=event.hit;
#endif
        if (currentCourse==Elysian516Course) {
            wizard.prepare(event.launched);
            while (health>horse.health) {
                switch (wizard.impact(--health)) {
                case WizardSpell516::Mirror:
                    controls={};
                    break;
                case WizardSpell516::Blind:
                    controls={};
                    audio.play(Sound::ParkLightsOut);
                    thoughts.play(Thought::Lights,tvTick());
                    break;
                case WizardSpell516::Death:
                    audio.play(Sound::WizardDeath);
                    break;
                default: break;
                }
            }
        }
        if (event.chargeUp) audio.play(Sound::HorseChargeUp);
        if (event.charge) audio.play(Sound::HorseCharge);
        if (event.bounce) audio.play(Sound::HorseBounce);
        if (event.burst) audio.play(Sound::HorseFlightBurst);
        if (event.launched) audio.play(Sound::HorseGoalLaunch);
        if (event.hit) {
            constexpr Sound hurts[]={Sound::HorseHurt1,Sound::HorseHurt2,Sound::HorseHurt3};
            audio.play(hurts[sceKernelGetSystemTimeLow()%std::size(hurts)]);
            audio.play(Sound::HorseImpact);
            thoughts.play(Thought::Horse,tvTick());
            if (horse.flying()) controls={};
            if (currentCourse==Elysian516Course && wizard.blind() && !wizard.dead())
                thoughts.play(Thought::Lights,tvTick());
        }
        if (event.caught) {
            caught=true;
            controls={};
        }
    }

#include "elysian_finale045_runtime.hpp"

    void step(bool skipRide=false,bool parryRide=false,bool horseLift=false) {
        if (paused) return;
        ++totalTicks;
        if (currentCourse==Elysian518Course) {
            stepFinale518(skipRide);
            return;
        }
        if (currentCourse==CosmicFirstCourse) {
            if (!caught && !game.won) stepCosmic601();
            return;
        }
        const bool slotCourse=currentCourse==SlotMachineCourse;
        if (slotCourse) slots.advance();
        if (wall418.bedActive) {
            const bool wasLullaby=wall418.lullabyActive();
            wall418.advanceEnding(game,skipRide);
            if (wall418.lullabyActive()!=wasLullaby) {
                if (wasLullaby) audio.muteMusic();
                else audio.music(Sound::EasyLullaby);
            }
            return;
        }
        if (horseBroadcast.active()) {
            switch (horseBroadcast.advance()) {
            case HorseBroadcast039::Cue::MuteMusic: audio.muteMusic(); break;
            case HorseBroadcast039::Cue::Alarm: audio.play(Sound::HorseAlarm); break;
            default: break;
            }
            return;
        }
        if (currentCourse==Elysian512Course) {
            ++elysianRingTick;
            if (caught || game.won || horse.flying()) stepElysianRing512(false);
        }
        if (caught || game.won) return;
        if (horse.flying()) {
            if (currentCourse==Elysian516Course) stepWizard516();
            if (isElysianHazardCourse040(currentCourse)) {
                stepElysianSkulls040(walls,courseHazards(currentCourse),brokenBlocks,
                                    skulls,skullCount,baddies,horse.bounds,horse.age+1);
            }
            if (currentCourse==Elysian511Course) baddies.step(walls,horse.age+1);
            if (currentCourse==Elysian513Course) stepElysianTraffic513(horse.age+1);
            if (currentCourse==Elysian514Course)
                stepElysianThreats514(baddies,chuckles,walls,switches,game.position,horse.age+1);
            if (currentCourse==Elysian515Course)
                updateElysianMotion515(horse.age+1,elysianPads515,elysianPortals515);
            stepHorse039(horseLift);
            return;
        }
        ++ticks;
        const bool santaCourse=currentCourse==CosmicFifteenthCourse;
        const bool singularity=currentCourse==CosmicSixteenthCourse;
        const bool hurrah=currentCourse==CosmicSeventeenthCourse;
        const bool farewell=currentCourse==CosmicEighteenthCourse;
        if (singularity) lateWalls053.setRotation(ticks);
        if (hurrah) {
            const unsigned beat=rhythmTestClock?Metronome048::testBeat(ticks):
                Metronome048::musicBeat(audio.musicPositionFrames(Sound::CosmicChampionship));
            if (rhythm.beats<=6) thoughts.play(Thought::Metronome,tvTick());
            if (rhythm.step(beat)) {
                if (rhythm.beats<=15) audio.playTuned(Sound::MetronomeBeat,11,rhythm.beats%4==1?1.2f:1.0f);
                if (rhythm.phase!=lateWalls053.rhythm) {
                    lateWalls053.rhythm=rhythm.phase;
                    rhythmRecovery617=true;
                    audio.playTuned(Sound::RhythmBlock,-10,1);
                }
            }
        }
        if (santaCourse) {
            santa.track(game.position);
            santaWalls.setOffset({0,santa.cameraY});
            game.setGoal(SantaRun052::goal(ticks));
            if (ticks==80) audio.playTuned(Sound::SantaBegin,-6,1.2f);
            if (santa.finishRing(false,ticks)) {
                audio.playRingTimer(0,0);
                audio.play(Sound::SmileyFail);
            }
        }
        if (currentCourse==CosmicThirteenthCourse) {
            if (ticks==1) audio.restartStageCue(StageCue::MovingGoalRun);
            if (movingGoal.advance(ticks)) audio.stageCue(StageCue::MovingGoalLaugh);
            game.setGoal(movingGoal.position());
        }
        if (usesMetronome048(currentCourse)) {
            const unsigned beat=rhythmTestClock?Metronome048::testBeat(ticks):
                Metronome048::musicBeat(audio.musicPositionFrames(Sound::CosmicChampionship));
            if (rhythm.beats<=6) thoughts.play(Thought::Metronome,tvTick());
            const bool beatChanged=rhythm.step(beat);
            if (usesPolyrhythm049(currentCourse)) {
                const bool triple=currentCourse==CosmicEleventhCourse;
                const unsigned previousMask=polyrhythm.mask;
                const unsigned events=polyrhythm.step(rhythm.beats,beatChanged,triple);
                if (events&Polyrhythm049::Cyan) audio.playTuned(Sound::PolyrhythmBlock,-6,1.5f);
                if (events&Polyrhythm049::Orange) audio.playTuned(Sound::PolyrhythmBlockOrange,-6,0.7f);
                rhythmWalls.setState(polyrhythm.collisionState(triple),(polyrhythm.mask&~previousMask)!=0);
            }
            if (beatChanged) {
                if (rhythm.beats<=15) audio.playTuned(Sound::MetronomeBeat,11,rhythm.beats%4==1?1.2f:1.0f);
                if (rhythm.beats%CosmicRhythmTriggerBeats048==0) {
                    if (!usesPolyrhythm049(currentCourse)) rhythmWalls.setState(rhythm.phase);
                    audio.playTuned(Sound::RhythmBlock,usesPolyrhythm049(currentCourse)?-8:-10,1);
                }
            }
        }
        if (currentCourse==Elysian515Course)
            updateElysianMotion515(ticks,elysianPads515,elysianPortals515);
        if (hasUnskippableFrontierHole(currentCourse)) {
            if (jump.outside(game.position,scene)) {
                backrooms=caught=true;
                game.velocity={};
                controls={};
                return;
            }
            if (currentCourse==ElysianRocksCourse || currentCourse==HorseBroadcastCourse) {
                stepElysian039(game,scene,horseBroadcast,currentCourse==HorseBroadcastCourse);
                return;
            }
            auto* wall=currentCourse==TheWallCourse?&wall418:nullptr;
            switch (stepFrontier18501(game,scene,wall,ticks,wall418.easyMode?&dogGate:nullptr)) {
            case WallEvent418::TooPoor: audio.play(Sound::SlotSad); break;
            case WallEvent418::Shatter: audio.play(Sound::PrismGateShatter); break;
            case WallEvent418::Bed: controls={}; break;
            default: break;
            }
            return;
        }
        if (frontierWind.starstorm) {
            const float gust=starstormGust047(ticks-1);
            audio.rideVolume(1,-80,-10+10*gust,0.8f+0.4f*gust);
        } else if (frontierWind.active) {
            if (frontierWind.movieOnce) audio.rideVolume(1.0f,0.0f,frontierWindDb414(ticks-1));
            else {
                frontierFlash.step();
                audio.rideVolume(1.0f,0.0f,frontierWindDb(ticks-1));
            }
            if (ticks==1) audio.stageCue(StageCue::RideWind);
        }
        if (currentCourse==FrontierMazeCourse) {
            for (const auto& track:FrontierJumpTracks406) {
                const float phase=static_cast<float>(ticks%track.period)/track.period;
                rotatingPads[track.index].radians=track.from+(track.to-track.from)*phase;
            }
        }
        if (currentCourse==ChuckleCoasterCourse) {
            const auto event=ride.step(game,scene,walls,rideStart,rideEnd,skipRide,parryRide);
            audio.rideVolume(ride.musicVolume,ride.cartVolumeDb,ride.windVolumeDb);
            if (event.elapsedTicks>1) {
                ticks+=event.elapsedTicks-1;
                totalTicks+=event.elapsedTicks-1;
            }
            if (event.started) audio.play(Sound::RideCoaster);
            if (event.windStarted) audio.stageCue(StageCue::RideWind);
            if (event.coinWarning>=0) audio.play(static_cast<Sound>(
                static_cast<unsigned>(Sound::RideCoinWarning)+static_cast<unsigned>(event.coinWarning)));
            if (event.ghostWarning) audio.play(Sound::RideGhostWarning);
            if (event.coins) audio.play(Sound::SmileyCoin);
            if (event.coinThought) thoughts.play(Thought::Money,tvTick());
            if (event.pushed) audio.play(Sound::Push);
            if (event.parried) controls={};
            if (event.launched) {
                controls={};
                audio.play(Sound::JumpPad);
                thoughts.play(Thought::Jump,tvTick());
            }
            if (event.caught || game.won || (event.phaseChanged && ride.phase==CoasterPhase033::Station)) {
                stopRideAudio033();
            }
            if (event.caught) {
                caught=true;
                controls={};
                fellOut=ride.death==CoasterDeath033::OutOfBounds;
                backrooms=ride.death==CoasterDeath033::Backrooms;
            }
            return;
        }
        const auto* group=multiDogStage(currentCourse);
        const TileField* activeGate=santaCourse?&santaWalls:rhythmWalls.activeTiles();
        if (group) {
            const auto& hazards=courseHazards(currentCourse);
            const auto event=dogs.step(*group,walls,dualGateLayers,switchStage?&switches:nullptr,
                {keyStage?keyStage->doors:nullptr,keyStage?keyStage->doorCount:0,roundKeys.opened,
                 skulls.bodies,hazards.skullCount,chuckles.bodies,chuckles.count});
            activeGate=dogs.activeGate(*group,dualGateLayers);
            if (event.bounced) audio.play(Sound::Bounce);
            if (event.switched) audio.play(Sound::ParkSwitch);
            if (event.won) {
                audio.play(Sound::ParkLightsOut);
                audio.play(Sound::ParkDog);
                thoughts.play(Thought::Dog,tvTick());
            }
            for (unsigned i=0;i<hazards.skullCount;++i) {
                if (!(event.skullHits&(1u<<i))) continue;
                skulls.defeat(i,ticks);
                audio.play(Sound::SkullDeath);
                thoughts.play(Thought::Skull,tvTick());
            }
            for (unsigned i=0;i<chuckles.count;++i) {
                if (!(event.chuckleHits&(1u<<i)) || !chuckles.defeat(i,ticks)) continue;
                audio.play(Sound::ChuckleDeath);
                thoughts.play(Thought::Chuckle,tvTick());
            }
        }
        const unsigned coasterTick=currentCourse==PursuitCourse?356:CoasterRun::startSoundTick(currentCourse==DenseCourse);
        if (hasCoaster(currentCourse) && ticks==coasterTick) audio.play(coasterSound(currentCourse));
        if (currentCourse>=HighwayCourse && currentCourse<=AlleyCourse &&
            news.x(ticks)+newsArt.width(russian(),news.headline)<NewsArt::ClipLeft)
            news.reset(ticks);
        if ((!santaCourse || santa.boundsArmed) &&
            (jumpStage || currentCourse==CosmicFourteenthCourse || currentCourse==CosmicThirteenthCourse || currentCourse==CosmicTenthCourse || currentCourse==Elysian514Course || currentCourse>=Elysian516Course || currentCourse==DoubleDogCourse || frontierWind.active || currentCourse==CataclysmCourse) && jump.outside(game.position,scene)) {
            fellOut=hurrah || currentCourse==CosmicNinthCourse || currentCourse==CosmicTenthCourse || (currentCourse>=CosmicThirdCourse && currentCourse!=CosmicThirteenthCourse && currentCourse!=CosmicFourteenthCourse && !singularity && !farewell && !usesMetronome048(currentCourse)) || jump.airborne() || jump.deathBuffer;
            backrooms=!fellOut;
            if (isHorseCourse039(currentCourse)) horse.lose();
            caught=true;
            game.velocity={};
            controls={};
            return;
        }
        if (jumpStage && jump.advance()) {
            if (currentCourse==CosmicSecondCourse) stepCosmicEnemies602(courseHazards(currentCourse));
            if (slotCourse) stepSlotSkulls(courseHazards(currentCourse));
            else if (currentCourse==UncannyValleyCourse) stepFrontierCars409(baddies,walls,ticks);
            else baddies.step(walls,ticks,nullptr,0,nullptr,nullptr,0,{},nullptr,santaCourse?0xCu:0);
            if (group) stepChuckles(courseHazards(currentCourse),activeGate);
            if (currentCourse==FrontierDogCourse) {
                stepProductionSkulls();
                stepChuckles(courseHazards(currentCourse),dog.winner?nullptr:&dogGate);
                stepDog(FrontierDog407);
            }
            jump.move(game);
            if (currentCourse==Elysian515Course || currentCourse==CosmicEleventhCourse || santaCourse) portals.step(game,scene.playerRadius,false);
            if (currentCourse==CosmicEleventhCourse || hurrah) stepCosmicRing(false);
            if (hurrah) {
                stepDog(CosmicDog617);
                stepChuckles(courseHazards(currentCourse));
            } else if (singularity) stepChuckles(courseHazards(currentCourse));
            if (currentCourse==FactoryCourse || currentCourse==ArcadeCourse)
                stepChuckles(courseHazards(currentCourse));
            if (currentCourse==ArcadeCourse) stepRings(ParkRingStages,arcadeRings,ParkRingBells,false);
            if (currentCourse==RacetrackCourse) stepRings(ParkRings317,dogRings,ParkRingBells317,false);
            if (keyStage) roundKeys.step(*keyStage,game.position,ticks);
            if (currentCourse==Elysian511Course || currentCourse==Elysian515Course) {
                stepHorse039(false,false);
                if (currentCourse==Elysian511Course && !horse.flying()) chaseHorseCourseGhosts040(false);
            } else chaseGhosts(false);
            return;
        }
        if (currentCourse>=TutorialCount) {
            const auto& golfStage=courseHazards(currentCourse);
            const bool pursuit=currentCourse==PursuitCourse;
            const bool overlaps=pursuit?onParkPursuitSand(game.position,scene.playerRadius):
                golfStage.onSand(game.position,scene.playerRadius);
            if (overlaps) {
                if (!inSand) thoughts.play(Thought::Sand,tvTick());
                game.velocity=game.velocity*(pursuit?ParkPursuitSandDamping:golf.damping);
            }
            inSand=overlaps;
            if (currentCourse==Elysian514Course)
                stepElysianThreats514(baddies,chuckles,walls,switches,game.position,ticks);
            else if (currentCourse==CosmicSecondCourse) stepCosmicEnemies602(golfStage);
            else if (slotCourse) stepSlotSkulls(golfStage);
            else if (isElysianHazardCourse040(currentCourse)) {
                stepElysianSkulls040(walls,golfStage,brokenBlocks,skulls,skullCount,baddies,horse.bounds,ticks);
            } else {
                if (currentCourse==FrontierMineCourse) {
                    stepMineDogs();
                    skulls.step(walls,skullCount,mineDogBodies,std::size(mineDogBodies),scene.right+128.0f,
                                nullptr,nullptr,FrontierBlocks411,std::size(FrontierBlocks411),brokenBlocks);
                } else if (currentCourse==FrontierDogCourse) {
                    stepProductionSkulls();
                } else if (currentCourse==AbandonedWorkspaceCourse) {
                    stepFrontierSkulls415(skulls,walls,skullCount,baddies,roundKeys.opened);
                } else if (currentCourse==DownfallCourse) {
                    skulls.step(walls,golfStage.skullCount,nullptr,0,scene.right+128.0f,nullptr,nullptr,
                                golfStage.blocks,golfStage.blockCount,brokenBlocks);
                } else if (golfStage.skullCount) {
                    if (group) {
                        CircleCollider peers[DogCourseStage::MaxDogs+ChuckleRun::Capacity]{};
                        std::copy_n(dogs.bodies,group->dogCount,peers);
                        std::copy_n(chuckles.bodies,chuckles.count,peers+group->dogCount);
                        skulls.step(walls,golfStage.skullCount,peers,group->dogCount+chuckles.count,
                                    scene.right+128.0f,nullptr,activeGate);
                    } else {
                        if (hurrah) {
                            const CircleCollider peers[]={dog.body};
                            skulls.step(walls,golfStage.skullCount,peers,1,scene.right+128.0f,
                                        nullptr,nullptr,golfStage.blocks,golfStage.blockCount,brokenBlocks,sweepCosmic053);
                        } else skulls.step(walls,golfStage.skullCount);
                    }
                }
                if (currentCourse==UncannyValleyCourse) stepFrontierCars409(baddies,walls,ticks);
                else baddies.step(walls,ticks,nullptr,0,nullptr,nullptr,0,{},nullptr,santaCourse?0xCu:0);
            }
            const bool claustrophobia=currentCourse==CosmicTenthCourse;
            if (claustrophobia)
                cosmicSkulls.step(walls,ticks,nullptr,0,roundKeys.opened?nullptr:CosmicKeys610.doors,
                                  nullptr,0,{},&rhythmWalls);
            const unsigned doorCount=keyStage?keyStage->doorCount:0;
            const bool spawningSkulls=baddies.stage && baddies.stage->skull;
            const bool replaceSkulls=spawningSkulls && !slotCourse && currentCourse!=Elysian509Course;
            const bool outpost=currentCourse==CrimsonOutpostCourse;
            const unsigned extraSkulls=spawningSkulls && (slotCourse || currentCourse==Elysian509Course)?baddies.count:0;
            const unsigned carCount=baddies.stage && !spawningSkulls?baddies.count:0;
            const TileField* enemyGate=currentCourse==FrontierDogCourse && !dog.winner?&dogGate:activeGate;
            if (outpost)
                skullSpawner416.step(walls,ticks,golfStage.blocks,golfStage.blockCount,brokenBlocks,
                                     baddies.cars,carCount,chuckles.bodies,chuckles.count);
            if (currentCourse!=Elysian514Course && currentCourse!=CosmicSecondCourse) stepChuckles(golfStage,enemyGate);
            const auto* enemies=claustrophobia?cosmicSkulls.skulls:outpost?skullSpawner416.bodies:replaceSkulls?baddies.skulls:skulls.bodies;
            const unsigned skullCount=claustrophobia?cosmicSkulls.count:outpost?skullSpawner416.count:replaceSkulls?baddies.count:this->skullCount;
            const auto* extraEnemies=chuckles.count?chuckles.bodies:baddies.skulls;
            const unsigned extraEnemyCount=chuckles.count?chuckles.count:extraSkulls;
            const auto* blocks=currentCourse==FrontierMineCourse?FrontierBlocks411:
                               currentCourse==DenseCourse?denseBlocks.data():golfStage.blocks;
            const unsigned blockCount=currentCourse==FrontierMineCourse?std::size(FrontierBlocks411):
                                      currentCourse==DenseCourse?denseBlocks.size():golfStage.blockCount;
            const auto* dogScene=dogSceneAt(currentCourse);
            if (dogScene) stepDog(*dogScene);
            if (hurrah && rhythmRecovery617) {
                const auto* field=lateWalls053.phase(rhythm.phase);
                const auto delta=field?field->recoverCircle(game.position,scene.playerRadius+0.08f):Vec2{};
                game.position=game.position+delta;
                rhythmRecovery617=dot(delta,delta)>1.0e-8f;
            }
            const unsigned beforeCollision=game.bounces;
            if (usesMetronome048(currentCourse))
                rhythmWalls.recover(game.position,scene.playerRadius,&walls,switches.activeTiles());
            const int hit=game.step(blocks,blockCount,brokenBlocks,
                                    enemies,skullCount,
                                    keyStage?keyStage->doors:nullptr,doorCount,roundKeys.opened,
                                    baddies.cars,carCount,extraEnemies,extraEnemyCount,
                                    switchStage?&switches:nullptr,
                                    currentCourse==CataclysmCourse?finale417.gate(dogGate):
                                        dogScene && !dog.winner && !hurrah?&dogGate:activeGate,
                                    currentCourse==Elysian513Course?elysianTraffic.bodies:
                                    currentCourse==FrontierMineCourse?mineDogBodies:
                                        group?dogs.bodies:dogScene?&dog.body:nullptr,
                                    currentCourse==Elysian513Course?ElysianTraffic513::DogCount:
                                        currentCourse==FrontierMineCourse?std::size(mineDogBodies):group?group->dogCount:1,
                                    currentCourse==Elysian516Course?wizard.acceleration():frontierWind.acceleration(ticks-1),
                                    currentCourse==Elysian516Course?wizard.drifting():frontierWind.drifting(),
                                    currentCourse==Elysian515Course?sweepElysianRocks515:
                                    singularity || hurrah?sweepCosmic053:nullptr);
            if (currentCourse>=CosmicSecondCourse && currentCourse!=CosmicFourthCourse && game.bounces!=beforeCollision && game.fixedTileBounce)
                audio.playTuned(Sound::GlassClink,0,0.8f+(sceKernelGetSystemTimeLow()%301)/1000.0f);
            else if (currentCourse>=CosmicSecondCourse && game.bounces!=beforeCollision &&
                     hit<static_cast<int>(blockCount+doorCount))
                audio.play(Sound::Bounce);
            frontierWind.afterStep(game);
            if (currentCourse==Elysian516Course) {
                wizard.afterStep(game);
                stepWizard516();
            }
            if (group && game.passiveBounceIndex>=0)
                dogs.struckByPlayer(*group,static_cast<unsigned>(game.passiveBounceIndex),game.position);
            if (dogScene && game.passiveBounce) dog.struckByPlayer(*dogScene,game.position);
            if (currentCourse==Elysian513Course && game.passiveBounceIndex>=0)
                elysianTraffic.struckByPlayer(ElysianDogs513,static_cast<unsigned>(game.passiveBounceIndex),game.position);
            if (currentCourse==FrontierMineCourse && game.passiveBounceIndex>=0) {
                const unsigned target=static_cast<unsigned>(game.passiveBounceIndex);
                mineDogs[target].struckByPlayer(mineDogScene(target),game.position);
            }
            if (switches.hitBlock>=0) audio.play(Sound::ParkSwitch);
            if (game.pushed) audio.play(Sound::Push);
            if (hit>=static_cast<int>(blockCount+doorCount)) {
                audio.play(Sound::Bounce);
                const unsigned index=hit-blockCount-doorCount;
                if (chuckles.count && index>=skullCount && index<skullCount+chuckles.count) {
                    if (chuckles.defeat(index-skullCount,ticks)) {
                        audio.play(Sound::ChuckleDeath);
                        if (group || currentCourse==FrontierDogCourse || currentCourse==PlentifulChucklesCourse ||
                            currentCourse==Elysian513Course || currentCourse==Elysian514Course || currentCourse==CosmicSecondCourse || outpost) thoughts.play(Thought::Chuckle,tvTick());
                    }
                } else if (index>=skullCount+extraEnemyCount) {
                    baddies.defeat(index-skullCount-extraEnemyCount,ticks);
                    audio.play(Sound::CarDeath);
                    thoughts.play(Thought::Car,tvTick());
                    if (currentCourse==Elysian511Course) {
                        audio.play(nextCarHorn?Sound::CarHorn1:Sound::CarHorn2);
                        nextCarHorn=!nextCarHorn;
                    } else audio.play(sceKernelGetSystemTimeLow()&1?Sound::CarHorn1:Sound::CarHorn2);
                } else {
                    if (claustrophobia) cosmicSkulls.defeat(index,ticks);
                    else if (outpost) skullSpawner416.defeat(index,ticks);
                    else if (replaceSkulls) baddies.defeat(index,ticks);
                    else if (index>=skullCount) baddies.defeat(index-skullCount,ticks);
                    else skulls.defeat(index,ticks);
                    audio.play(Sound::SkullDeath);
                    thoughts.play(Thought::Skull,tvTick());
                }
            } else if (hit>=static_cast<int>(blockCount)) {
                if (keyStage && roundKeys.open(*keyStage,hit-blockCount,ticks,doorScore(currentCourse))) audio.play(Sound::DoorOpen);
            } else if (hit>=0) {
                breakBlock(static_cast<unsigned>(hit));
            }
            if (golfStage.hasKey && progress.quest>=2 && !progress.golfKey &&
                touchesKey(golfStage,game.position,scene.playerRadius)) {
                progress.golfKey=1;
                questKeyTick=totalTicks;
                audio.play(Sound::SecretKey);
            }
            if (currentCourse==CosmicThirteenthCourse) {
                const bool key=progress.quest>=2 && !progress.cosmicKey &&
                               touchesCosmicKey613(game.position,scene.playerRadius);
                if (key) {
                    progress.cosmicKey=1;
                    questKeyTick=std::max(totalTicks,1u);
                    audio.play(Sound::SecretKey);
                }
                if ((key || game.won) && movingGoal.cry(ticks)) {
                    audio.stageCue(StageCue::None);
                    audio.play(Sound::GoalGasp);
                }
            }
            const bool water=!game.won && golfStage.inWater(game.position,scene.playerRadius);
            if (water && !pool.swimming) {
                constexpr Sound splashes[]={Sound::WaterSplash,Sound::WaterSplash2,Sound::WaterSplash3};
                audio.play(splashes[sceKernelGetSystemTimeLow()%std::size(splashes)]);
                if (!canSwim) {
                    waterDead=caught=true;
                    splashTick=totalTicks;
                    game.velocity={};
                    controls={};
                    audio.play(Sound::WaterDeath);
                    return;
                }
                thoughts.play(Thought::Swim,tvTick());
            }
            if (currentCourse==PoolCourse && pool.touch(game.position,scene.playerRadius,water,
                                                       canSwim,ticks,length(game.velocity)) && !progress.quest) {
                progress.quest=1;
            } else if (currentCourse!=PoolCourse) {
                pool.swimming=water && canSwim;
            }
        } else {
            game.step();
        }
        if (portals.step(game,scene.playerRadius)) {
            audio.play(Sound::Portal);
            thoughts.play(Thought::Portal,tvTick());
        }
        if (currentCourse==CataclysmCourse) {
            const unsigned events=finale417.step(game,scene,ticks,chillMode);
            if (events&FrontierFakeGoal417) {
                controls={};
                audio.play(Sound::FakeGoalReveal);
                audio.stageCue(StageCue::FrontierAlarm);
                if (finale417.finalGhostActive) audio.play(Sound::FinalGhostWake);
            }
            if (events&FrontierUfoLaunched417) audio.play(Sound::FrontierUfo);
            if (events&FrontierKeyGranted417) {
                progress.frontierKey=1;
                questKeyTick=totalTicks;
                audio.play(Sound::SecretKey);
            }
        }
        if (hurrah && !lastHurrah.revealed &&
            fakeGoals.step(&CosmicFakeGoal617,1,game,scene.playerRadius,ticks)) {
            lastHurrah.revealed=ticks;
            lastHurrah.hunterActive=!chillMode;
            lateWalls053.trapOpen=true;
            controls={};
            audio.play(Sound::FakeGoalReveal);
            audio.play(Sound::HurrahExplosion);
            audio.play(Sound::HurrahSiren);
            if (lastHurrah.hunterActive) audio.play(Sound::FinalGhostWake);
        }
        if (hurrah && touchesCosmicExit053(game.position,scene.goal,scene.playerRadius)) {
            game.won=true;
            game.position=scene.goal;
            game.velocity={};
        }
        if (currentCourse==CosmicFourteenthCourse && !game.won &&
            fakeGoals.step(CosmicFakeGoals614,std::size(CosmicFakeGoals614),game,scene.playerRadius,ticks))
            audio.play(Sound::FakeGoalReveal);
        const auto& stage=objects.data.courses[currentCourse];
        if (santaCourse && santa.startRing(game.position,scene.playerRadius,ticks)) {
            audio.play(Sound::SmileyRing);
            audio.playRingTimer(0,240);
        }
        if (keyStage) {
            const uint32_t mask=santaCourse && !santa.keyActive(ticks)?1u:UINT32_MAX;
            if (roundKeys.collect(*keyStage,game.position,scene.playerRadius,ticks,mask)) {
                audio.play(Sound::KeyCollect);
                if (santaCourse && (roundKeys.collected&2u) && santa.finishRing(true,ticks)) {
                    audio.playRingTimer(0,0);
                    audio.play(Sound::SmileyPass);
                }
            }
            roundKeys.step(*keyStage,game.position,ticks);
        }
        if (currentCourse==FrontierMazeCourse && !coins.has(stage.count)) {
            const Vec2 offset=game.position-FrontierBigCoin406;
            const float range=scene.playerRadius+FrontierBigCoinRadius406;
            if (dot(offset,offset)<=range*range) {
                coins.collect(stage.count,ticks,FrontierBigCoinScore406,FrontierBigCoinStep406);
                audio.play(Sound::BigCoin);
                if (coins.multiplier>=6) thoughts.play(Thought::Money,tvTick());
            }
        }
        const float coinRange=scene.playerRadius+objects.data.coinRadius;
        for (unsigned i=0;i<stage.count;++i) {
            if (coins.has(i) || !rhythmCoinActive(i)) continue;
            const Vec2 offset=game.position-stage.coins[i];
            if (dot(offset,offset)>coinRange*coinRange) continue;
            coins.collect(i,ticks,objects.data.coinValue,objects.data.coinStep);
            audio.play(Sound::Coin);
            if (coins.multiplier>=6) thoughts.play(Thought::Money,tvTick());
        }
        if (currentCourse==Elysian514Course &&
            nukes.step(ElysianNukes514,std::size(ElysianNukes514),game.position,scene.playerRadius,ticks)) {
            clearElysianThreats514(baddies,chuckles);
            audio.play(Sound::Nuke);
        }
        if (currentCourse==FrontierDogCourse &&
            nukes.step(FrontierNukes407,std::size(FrontierNukes407),game.position,scene.playerRadius,ticks)) {
            for (unsigned i=0;i<skullCount;++i) skulls.bodies[i].radius=0;
            for (unsigned i=0;i<chuckles.count;++i) chuckles.bodies[i].radius=0;
            audio.play(Sound::Nuke);
        }
        if (currentCourse==PlentifulChucklesCourse &&
            nukes.step(FrontierNukes408,std::size(FrontierNukes408),game.position,scene.playerRadius,ticks)) {
            chuckles.clearSpawned();
            audio.play(Sound::Nuke);
        }
        if (currentCourse==UncannyValleyCourse &&
            nukes.step(FrontierNukes409,std::size(FrontierNukes409),game.position,scene.playerRadius,ticks)) {
            clearFrontierCars409(baddies);
            audio.play(Sound::Nuke);
        }
        if (hurrah && nukes.step(CosmicNukes617,std::size(CosmicNukes617),game.position,scene.playerRadius,ticks)) {
            clearFrontierCars409(baddies);
            chuckles.clearSpawned();
            for (unsigned i=0;i<skullCount;++i) skulls.bodies[i].radius=0;
            audio.play(Sound::Nuke);
        }
        if (currentCourse==CosmicFourteenthCourse &&
            nukes.step(CosmicNukes614,std::size(CosmicNukes614),game.position,scene.playerRadius,ticks)) {
            chuckles.clearSpawned();
            audio.play(Sound::Nuke);
        }
        if (currentCourse==CosmicTenthCourse &&
            nukes.step(CosmicNukes610,std::size(CosmicNukes610),game.position,scene.playerRadius,ticks)) {
            clearFrontierCars409(baddies);
            clearFrontierCars409(cosmicSkulls);
            audio.play(Sound::Nuke);
        }
        if (currentCourse==CrimsonOutpostCourse &&
            nukes.step(FrontierNukes416,std::size(FrontierNukes416),game.position,scene.playerRadius,ticks)) {
            clearFrontierCars409(baddies);
            skullSpawner416.clearSpawned();
            chuckles.clearSpawned();
            audio.play(Sound::Nuke);
        }
        if (currentCourse>=ChuckleParkFirstCourse && parkArt.data.smiley.coinCount) {
            const auto event=smiley.step(parkArt.data.smiley,game.position,scene.playerRadius,ticks);
            if (event.started) {
                audio.play(Sound::SmileyRing);
                if (parkArt.data.ufoSpeed>0) audio.stageCue(StageCue::Park);
                else if (parkArt.data.smiley.timeTicks==300) audio.stageCue(StageCue::ParkShort);
                else audio.play(Sound::SmileyTimer);
            }
            if (event.newCoins) audio.play(Sound::SmileyCoin);
            if (event.passed || event.failed || game.won)
                audio.stop(parkArt.data.smiley.timeTicks==300?Sound::SmileyTimerShort:Sound::SmileyTimer);
            if (event.passed) audio.play(Sound::SmileyPass);
            if (event.failed) audio.play(Sound::SmileyFail);
        }
        if (currentCourse==Elysian512Course) stepElysianRing512(true);
        if (currentCourse==CosmicFifthCourse || currentCourse==CosmicEleventhCourse || hurrah) stepCosmicRing();
        if (currentCourse==ArcadeCourse) stepRings(ParkRingStages,arcadeRings,ParkRingBells,true);
        if (currentCourse==PursuitCourse) stepRings(ParkPursuitRingStages,pursuitRings,ParkPursuitRingBells,true);
        if (currentCourse==DoubleDogCourse) stepRings(ParkRings316,dogRings,ParkRingBells316,true);
        if (currentCourse==RacetrackCourse) stepRings(ParkRings317,dogRings,ParkRingBells317,true);
        if (currentCourse==LightsCourse) {
            const auto event=parkScare.step(game.position,scene.playerRadius,!chillMode);
            if (event.lightsOut) {
                audio.play(Sound::ParkLightsOut);
                thoughts.play(Thought::Lights,tvTick());
            }
            if (event.ashBaby) audio.play(Sound::ParkAshBaby);
        }
        if (game.won) return;
        if (currentCourse>=FirstBaddieCourse && currentCourse<=DimlitCourse &&
            baddies.collect(game.position,scene.playerRadius,ticks)) audio.play(Sound::Coin);
        const bool pursuit=currentCourse==PursuitCourse || currentCourse==RacetrackCourse ||
                           currentCourse==WholesomeCourse || currentCourse==FrontierDogCourse;
        const bool frontierSpeed=currentCourse==FrontierSwitchCourse || pursuit;
        const float speedRadius=(currentCourse==WholesomeCourse || currentCourse==Elysian510Course)?FrontierSpeedRadius:frontierSpeed?ParkPursuitSpeedRadius:speedTokens.radius;
        const float stopRadius=currentCourse==CosmicEighthCourse?CosmicStopRadius608:currentCourse==AbandonedWorkspaceCourse?FrontierStopRadius415:
            (currentCourse==WholesomeCourse || currentCourse>=PlentifulChucklesCourse)?FrontierStopRadius:
            pursuit?ParkPursuitStopRadius:stopTokens.radius;
        if (speedStage && speedRun.step(speedStage,game,scene.playerRadius+speedRadius,ticks,875.0f))
            audio.play(Sound::StopToken);
        const uint32_t previousStops=stopRun.collected;
        if (stopStage && stopRun.step(*stopStage,game,scene.playerRadius+stopRadius,ticks)) {
            audio.play(Sound::StopToken);
            settleSlots(stopRun.collected&~previousStops);
        }

        if (currentCourse==BackroomsCourse &&
            std::max(std::abs(game.position.x),std::abs(game.position.y))>BackroomsLimit) {
            backrooms=caught=true;
            game.velocity={};
            controls={};
            return;
        }
        if (currentCourse==BankCourse && police.trigger(game.position,scene.playerRadius,ticks)) {
            baddies.restart(&baddieStages[PoliceBaddieStage]);
            audio.stageCue(StageCue::Police);
            thoughts.play(Thought::Police,tvTick());
            thoughts.setFallback(Thought::Police);
        }
        if (currentCourse==CityCourse) {
            const auto event=secret.step(game.position,scene.playerRadius,ticks);
            if (event==SecretEvent::Start) {
                audio.stageCue(StageCue::SecretRun);
            } else if (event==SecretEvent::Key) {
                progress.glowstickKey=1;
                questKeyTick=totalTicks;
                audio.play(Sound::SecretKey);
            }
        }
        if (currentCourse==MartCourse) {
            const auto event=marcy.step(game.position,scene.playerRadius,ticks);
            if (event.started) {
                audio.muteMusic();
                audio.stageCue(StageCue::Marcy);
            }
            if (event.intercom) audio.play(Sound::MarcyIntercom);
            if (event.cleaned) audio.play(Sound::MarcySlurp);
            if (event.key) {
                progress.parkKey=1;
                questKeyTick=totalTicks;
                audio.play(Sound::SecretKey);
            }
        }
        if (jumpStage && jump.tryLaunch(jumpStage.pads,jumpStage.count,game,scene,
                                       currentCourse==TowerCourse?ticks:UINT32_MAX)) {
            frontierWind.beginMove();
            controls={};
            audio.play(Sound::JumpPad);
            thoughts.play(Thought::Jump,tvTick());
        }
        if (isHorseCourse039(currentCourse)) {
            stepHorse039(false,!jump.airborne());
            if (hasHorseCourseGhost040(currentCourse) && !caught && !horse.flying())
                chaseHorseCourseGhosts040(!jump.airborne());
        } else chaseGhosts(!jump.airborne());
    }
};

int exitCallback(int, int, void*) {
    running.store(false,std::memory_order_relaxed);
    return 0;
}

int callbackThread(SceSize, void*) {
    const int cb = sceKernelCreateCallback("ucg-exit",exitCallback,nullptr);
    if (cb >= 0) {
        sceKernelRegisterExitCallback(cb);
        sceKernelSleepThreadCB();
    }
    return 0;
}

[[maybe_unused]] std::string companion(const char* executable,const char* filename) {
    const std::string path = executable ? executable : "";
    const auto slash = path.find_last_of("/\\");
    return slash == std::string::npos ? filename : path.substr(0,slash+1)+filename;
}

bool organizedAssets=false;

#ifdef UCG_DISC_BUILD
#include "iso_assets052.inc"
#endif

std::string statePath(const char* executable,const char* filename) {
#ifdef UCG_DISC_BUILD
    (void)executable;
    return std::string("ms0:/PSP/UCG053-ISO/")+filename;
#else
    return companion(executable,filename);
#endif
}

std::string assetPath(const char* executable,const char* filename) {
#ifdef UCG_DISC_BUILD
    (void)executable;
    return isoAssetPath(AssetLayout::relative(filename));
#else
    return organizedAssets?companion(executable,AssetLayout::relative(filename).c_str()):
                           companion(executable,filename);
#endif
}

bool validPatch(const Patch& p,int w,int h) {
    return p.u>=0 && p.v>=0 && p.width>0 && p.height>0 &&
           p.u+p.width<=w && p.v+p.height<=h;
}

bool loadAssets(const char* path,Course& course,bool extraCourse) {
    std::FILE* f=std::fopen(path,"rb");
    if (!f) return false;
    walls = {};
    constexpr long legacySize=CourseBytes+VisualBytes+sizeof resultFont+sizeof stagePositions+sizeof stageTexture;
    const unsigned page=extraCourse ? static_cast<unsigned>(Screen::Hole) : 0;
    constexpr long pagePadding=StoredPageBytes-sizeof gameplayPage;
    bool ok=(extraCourse || (std::fseek(f,0,SEEK_END)==0 && std::ftell(f)==legacySize)) &&
            readCourse(f,course) && std::fread(&visual,sizeof visual,1,f)==1 &&
            visual.magic==0x32564355 && visual.spinnerCount>=1 && visual.spinnerCount<=MaxSpinner &&
            validPatch(visual.cat,ActorsWidth,ActorsHeight) && validPatch(visual.ghost,ActorsWidth,ActorsHeight);
    const long skippedPages=StoredPageBytes*(ScreenPages-page-1);
    ok=ok && std::fseek(f,skippedPages,SEEK_CUR)==0 &&
            std::fread(gameplayPage,sizeof gameplayPage,1,f)==1 && std::fseek(f,pagePadding,SEEK_CUR)==0 &&
            std::fread(hud,sizeof hud,1,f)==1 &&
            std::fseek(f,sizeof actors,SEEK_CUR)==0 &&
            (extraCourse || std::fread(resultFont,sizeof resultFont,1,f)==1) &&
            std::fread(stagePositions,sizeof stagePositions,1,f)==1 &&
            std::fread(stageTexture,sizeof stageTexture,1,f)==1 &&
            (!extraCourse || walls.read(f));
    std::fclose(f);
    if (!ok) return false;
    for (unsigned i=0;i<visual.spinnerCount;++i) {
        ok=ok && validPatch(visual.spinner[i],VisualWidth,HudHeight);
    }
    for (const Patch& p:visual.strokes) ok=ok && validPatch(p,VisualWidth,HudHeight);
    for (const auto& cells:visual.clock) {
        for (const Patch& p:cells) ok=ok && validPatch(p,VisualWidth,HudHeight);
    }
    if (ok) {
        sceKernelDcacheWritebackRange(gameplayPage,sizeof gameplayPage);
        sceKernelDcacheWritebackRange(hud,sizeof hud);
        sceKernelDcacheWritebackRange(resultFont,sizeof resultFont);
        sceKernelDcacheWritebackRange(stageTexture,sizeof stageTexture);
        if (!extraCourse) {
            fallbackCoursePath=path;
            fallbackPageIndex=ScreenPages;
        }
    }
    return ok;
}

const uint16_t* fallbackBackdrop(unsigned page) {
    if (page==static_cast<unsigned>(Screen::Hole)) return gameplayPage;
    if (page>=ScreenPages) return nullptr;
    if (page==fallbackPageIndex) return fallbackPage;
    const auto input=openAssetFile(fallbackCoursePath.c_str());
    auto* file=input.get();
    if (!file || std::fseek(file,CourseBytes+sizeof(VisualHeader)+page*StoredPageBytes,SEEK_SET)!=0)
        return nullptr;
    fallbackPageIndex=ScreenPages;
    horseMeter039.invalidateShared();
    if (std::fread(fallbackPage,sizeof fallbackPage,1,file)!=1) return nullptr;
    sceKernelDcacheWritebackRange(fallbackPage,sizeof fallbackPage);
    fallbackPageIndex=page;
    return fallbackPage;
}

const uint16_t* backroomsBackdrop() {
    if (fallbackPageIndex==BackroomsFallbackPage) return fallbackPage;
    const auto input=openAssetFile(backroomsPath.c_str());
    auto* file=input.get();
    if (!file || std::fseek(file,4,SEEK_SET)!=0) return nullptr;
    fallbackPageIndex=ScreenPages;
    horseMeter039.invalidateShared();
    if (std::fread(fallbackPage,sizeof fallbackPage,1,file)!=1) return nullptr;
    sceKernelDcacheWritebackRange(fallbackPage,sizeof fallbackPage);
    fallbackPageIndex=BackroomsFallbackPage;
    return fallbackPage;
}

void logEvent(const char* path,const char* event,const Game& game,const char* mode="a") {
    if (std::FILE* f=std::fopen(path,mode)) {
        std::fprintf(f,"build=053 event=%s course=%s strokes=%u bounces=%u x=%.2f y=%.2f\n",
                     event,CourseIds[currentCourse],game.strokes,game.bounces,game.position.x,game.position.y);
        std::fclose(f);
    }
}

void logMemory(const char* path,const char* event,unsigned effectBytes,const char* mode="a") {
    const auto heap=mallinfo();
    const unsigned total=sceKernelTotalFreeMemSize();
    const unsigned largest=sceKernelMaxFreeMemSize();
    if (auto* out=std::fopen(path,mode)) {
        std::fprintf(out,"build=053 event=%s partition_free=%u partition_largest=%u heap_used=%u heap_free=%u effect_bytes=%u\n",
                     event,total,largest,static_cast<unsigned>(heap.uordblks),
                     static_cast<unsigned>(heap.fordblks),effectBytes);
        std::fclose(out);
    }
}

void logBackground(const char* path,const char* event) {
    if (std::FILE* file=std::fopen(path,"a")) {
        char active[16],chosen[16];
        const auto format=[](char* code,unsigned id) {
            if (id>=CatalogBackgroundBase) std::snprintf(code,16,"%03u",id-CatalogBackgroundBase);
            else std::snprintf(code,16,"G%02u",id);
        };
        format(active,activeBackground);
        format(chosen,backgroundPreference.chosen);
        std::fprintf(file,"build=053 event=%s id=%s random=%u chosen=%s\n",event,active,
                     static_cast<unsigned>(backgroundPreference.random),chosen);
        std::fclose(file);
    }
}

std::string localizedPath(const char* executable,Language selected,const char* filename) {
    const std::string path=selected==Language::Russian ? std::string("ru/")+filename : filename;
    return assetPath(executable,path.c_str());
}

unsigned flagSlot(unsigned index) { return index+(russian()?CourseCount:0); }

#include "elysian_finale045_assets.hpp"
#include "cosmic_assets046.hpp"

const uint8_t* titleArt=nullptr;
bool titleReusedCourseVram=false;

bool loadCourseAt(const char* executable,unsigned index,Course& course) {
    titleArt=nullptr;
    titleReusedCourseVram=false;
    cosmicTitles046.unload();
    cosmicTitles047.unload();
    cosmicTitles048.unload();
    cosmicTitles049.unload();
    cosmicTitles050.unload();
    cosmicTitles051.unload();
    cosmicTitles052.unload();
    cosmicTitles053.unload();
    if (index!=PlentifulChucklesCourse) {
        frontier408Tiles.unload();
        frontier408Chuckles.unload();
    }
    elysianSwitchArt512.closeStream();
    elysianSwitchArt514.closeStream();
    elysianTreesArt515.closeStream();
    elysianSwitchSlot512={};
    if (index!=UncannyValleyCourse) frontier409Night.unload();
    if (index<WeatherAlertCourse || index>DownfallCourse) frontierNight1213.unload();
    if (index<MovieOnceCourse || index>AbandonedWorkspaceCourse) frontierNight1415.unload();
    if (index!=CrimsonOutpostCourse) frontierProps416.unload();
    if (index!=CataclysmCourse) {
        frontierProps417.unload();
        frontierSwitchTiles417.unload();
        frontierGateTiles417.unload();
    }
    if (index<CrimsonOutpostCourse) frontierLights1617.unload();
    if (index!=WholesomeCourse) {
        wholesomeTiles.unload();
        wholesomePortalArt.unload();
    }
    if (index!=CubeTownCourse) cubeTownTiles.unload();

    if (index>=CourseCount) return false;
    const std::string filename=std::string(CourseIds[index])+".bin";
    const auto failed=[&](const char* part) {
        const auto file=statePath(executable,"session.log");
        if (auto* out=std::fopen(file.c_str(),"ab")) {
            const auto memory=mallinfo();
            std::fprintf(out,"build=053 event=course_asset_error course=%s part=%s heap_used=%u heap_free=%u\n",
                         CourseIds[index],part,static_cast<unsigned>(memory.uordblks),
                         static_cast<unsigned>(memory.fordblks));
            std::fclose(out);
        }
        return false;
    };
    if (!loadAssets(localizedPath(executable,language,filename.c_str()).c_str(),course,index!=0)) return failed("field");
    if (index==CosmicSixthCourse) walls.setOffset(CosmicGridOffset606);
    cosmicRhythmFields048.load(nullptr,0,nullptr,0);
    if (usesMetronome048(index)) {
        const auto layer=[&](unsigned phase) {
            const auto name=std::string(CourseIds[index])+"-rhythm-"+std::to_string(phase)+".bin";
            return assetPath(executable,name.c_str());
        };
        std::array<std::string,7> names;
        const char* paths[7];
        const unsigned count=index==CosmicEleventhCourse?7:3;
        for (unsigned i=0;i<count;++i) {
            names[i]=layer(i);
            paths[i]=names[i].c_str();
        }
        if (!cosmicRhythmFields048.load(paths,count,nullptr,0)) return failed("cosmic_rhythm_collision");
        if (index==CosmicSixthCourse) cosmicRhythmFields048.setGridOffset(CosmicGridOffset606);
    }
    if (index==CosmicFifteenthCourse) {
        const auto path=assetPath(executable,"6_15-stuck.bin");
        const auto file=openAssetFile(path.c_str());
        if (!file || !santaWalls.read(file.get())) return failed("santa_stuck");
    }
    if (!actorAtlas.load(index,visual,actors)) return failed("actors");
    fallbackPageIndex=ScreenPages;
    if (!goalFlags.load(flagSlot(index),reinterpret_cast<uint8_t*>(fallbackPage),sizeof fallbackPage)) return failed("flag");
    if (index>=CosmicSixteenthCourse && !loadCosmicCollisions053(executable,index))
        return failed("cosmic_053_collision");

    if (isHorseCourse039(index)) {
        const auto wallsName=std::string(CourseIds[index])+"-horse.bin";
        const auto horseFile=openAssetFile(assetPath(executable,wallsName.c_str()).c_str());
        if (!horseFile || !horseWalls039.read(horseFile.get())) return failed("horse_collision");
        if (!horseObjects039.openShared(assetPath(executable,"frontier-horse-050405.pak")) ||
            !horseMeter039.openShared(assetPath(executable,"frontier-horse-meter-050405.pak")))
            return failed("horse_art");
    } else {
        horseObjects039.unload();
        horseMeter039.unload();
    }
    if (index!=SlotMachineCourse) slotArt.unload();
    if (index<ChuckleParkFirstCourse || index==JumbleCourse ||
        (index>=DenseCourse && !usesRingTimers(index) && index!=BalloonDogCourse)) parkArt.unload();
    if (index!=JumbleCourse) { jumbleVideo.close(); jumbleTiles.close(); jumbleFlag.unload(); }
    if (index<PursuitCourse || index>=FinalFrontierFirstCourse) pursuitTiles.unload();
    if (index!=FrontierSwitchCourse) frontierSwitchTiles.unload();
    if (index!=FrontierSwitchCourse && index!=WholesomeCourse && index!=CubeTownCourse) frontierSwitchFaces.unload();
    if (index!=FrontierPortalCourse) frontierPortalArt.unload();
    if (index!=Elysian514Course) elysianSwitchFaces514.unload();
    if (index!=Elysian515Course) elysianPortalArt515.unload();
    if (index!=Elysian516Course) {
        wizardArt516.unload();
        spellArt516.unload();
    }
    if (index!=FrontierMazeCourse) {
        frontierMazePortals.unload();
        frontierBigCoin.unload();
    }
    if (index!=FrontierDogCourse) frontierDogTiles.unload();
    if (index!=FrontierDogCourse && (index<FrontierPatrolCourse || index>FrontierMineCourse))
        frontierDogArt.unload();
    if (index<FrontierPatrolCourse || index>FrontierMineCourse) frontier1011Lights.unload();
    if (index!=FrontierMineCourse) frontierMineBlock.unload();
    if (index!=currentCourse) frontierNukeArt.unload();
    if (index!=FrontierDogCourse && index!=CrimsonOutpostCourse && index!=Elysian514Course && (index<PlentifulChucklesCourse || index>UncannyValleyCourse)) {
        frontierNukeArt.unload();
        frontierNukeExplosions.unload();
    }
    if (!dogSceneAt(index) && !multiDogStage(index) && index!=Elysian513Course) dogArt.unload();
    if (index!=currentCourse || !multiDogStage(index)) {
        dualGateFields.reset();
        std::fill(std::begin(dualGateLayers),std::end(dualGateLayers),nullptr);
    }
    if (index!=DogCourse) dogDoorArt.unload();
    if (index!=BalloonDogCourse && index!=ScreenDogCourse) dogWideDoorArt.unload();
    if (index!=DoubleDogCourse) dualGateArt.unload();
    if (index!=RacetrackCourse) raceGateArt.unload();
    if (index!=DoubleDogCourse) dogCameraArt.unload();
    if (index!=RacetrackCourse) raceDoors.unload();
    if (index!=DogCourse && index!=ScreenDogCourse) dogScreen.unload();
    if (index!=PursuitCourse) {
        pursuitDecor.unload();
        pursuitBalloons.unload();
        pursuitMotion.reset();
    }
    if (!hasCoaster(index)) {
        if (coasterArt.pixels()) {
            audio.stop(Sound::CoasterFlyby);
            audio.stop(Sound::CoasterFlyby308);
            audio.stop(Sound::CoasterFlyby312);
            audio.pauseStageCue(false);
        }
        coasterArt.unload();
    }
    const bool needsChuckles=index==CoasterCourse || index==MartCourse ||
        index==FactoryCourse || index==MirrorCourse || index==ArcadeCourse || index==FrontierDogCourse || index==Elysian513Course || index==Elysian514Course || index==CosmicSecondCourse || multiDogStage(index);
    if (!needsChuckles || (index!=currentCourse &&
        (index==FrontierDogCourse || currentCourse==FrontierDogCourse))) chuckleArt.unload();
    if (index!=ArcadeCourse) arcadeDoors.unload();
    if (index!=LightsCourse) { darkHoleArt.unload(); ashArt.unload(); }
    if (index!=MartCourse) {
        marcySpillArt.unload();
        marcyHudArt.unload();
        marcyTextArt.unload();
    }
    if (index!=FactoryCourse) {
        factoryDoors.unload();
        factoryBalloons.unload();
        if (!multiDogStage(index)) factorySpawner.unload();
        factoryDecor.unload();
    }
    if (index==SlotMachineCourse && !slotArt.load()) return false;
    if (index==ChuckleParkFirstCourse && !parkArt.load()) return false;
    if ((index>ChuckleParkFirstCourse && index!=JumbleCourse && index<DenseCourse) ||
        usesParkRingStage(index) || index==BalloonDogCourse) {
        const std::string stage=multiDogStage(index)?"park-stage-3_11.pak":std::string("park-stage-")+CourseIds[index]+".pak";
        if (!parkArt.load(assetPath(executable,stage.c_str()).c_str(),usesParkRingStage(index))) return failed("park_stage");
    }
    const auto path=[executable](const char* name) { return assetPath(executable,name); };
    if (index==ChuckleCoasterCourse) {
        if (!rideArt.load(path("coaster-track-3_18.bin"),path("coaster-bg-3_18.pak")) ||
            !rideSprites.load(path("coaster-ride-3_18.pak")) ||
            !rideScenery.load(path("coaster-scenery-3_18.pak")))
            return failed("ride_art");
        const auto start=openAssetFile(path("3_18-start.bin").c_str());
        const auto end=openAssetFile(path("3_18-end.bin").c_str());
        if (!start || !end || !rideStart.read(start.get()) || !rideEnd.read(end.get()))
            return failed("ride_collisions");
    } else {
        if (currentCourse==ChuckleCoasterCourse) {
            stopRideAudio033();
            audio.rideVolume(1,-80,-80);
        }
        rideArt.unload();
        rideSprites.unload();
        rideScenery.unload();
    }
    if (index>=FinalFrontierFirstCourse && index<=TheWallCourse) {
        if (!snowArt.openShared(path("snow-4_01.pak"))) return failed("snow_art");
    } else {
        snowArt.unload();
    }
    const bool frontierSwitch=index==FrontierSwitchCourse;
    if (frontierSwitch) {
        parkSwitchStage.load(nullptr,0,nullptr,0);
        if (!loadFrontierSwitchLayers(parkSwitchStage,CourseIds[index],FrontierSwitchBlocks402,
                                      std::size(FrontierSwitchBlocks402),path))
            return failed("frontier_switch_collision");
        if (!frontierSwitchTiles.load(path("snow-switch-4_02.pak")) ||
            !frontierSwitchFaces.load(path("snow-switch-faces.pak")))
            return failed("frontier_switch_art");
    } else if (index==PlentifulChucklesCourse) {
        parkSwitchStage.load(nullptr,0,nullptr,0);
        if (!loadFrontierSwitchLayers(parkSwitchStage,CourseIds[index],FrontierSwitchBlocks408,
                                      std::size(FrontierSwitchBlocks408),path))
            return failed("frontier_408_switch_collision");
        if (!frontier408Tiles.openShared(path("snow-switch-4_08.pak")))
            return failed("frontier_408_tiles");
        if (!frontierSwitchFaces.openShared(path("snow-switch-faces.pak")))
            return failed("frontier_408_faces");
        if (!frontier408Chuckles.openShared(path("frontier-chuckle-4_08.pak")))
            return failed("frontier_408_chuckles");
    } else if (index==CataclysmCourse) {
        parkSwitchStage.load(nullptr,0,nullptr,0);
        if (!loadFrontierSwitchLayers(parkSwitchStage,CourseIds[index],FrontierSwitchBlocks417,
                                      std::size(FrontierSwitchBlocks417),path))
            return failed("frontier_417_switch_collision");
        if (!frontierSwitchTiles417.openShared(path("snow-switch-4_17.pak")) ||
            !frontierGateTiles417.openShared(path("snow-gate-4_17.pak")) ||
            !frontierProps417.openShared(path("frontier-props-4_17.pak")))
            return failed("frontier_417_art");
    } else if (index==WholesomeCourse || index==CubeTownCourse) {
        // Release the previous stage's four TileFields before loading another set.
        parkSwitchStage.load(nullptr,0,nullptr,0);
        const bool wholesome=index==WholesomeCourse;
        const auto* blocks=wholesome?FrontierSwitchBlocks404:FrontierSwitchBlocks405;
        const auto count=wholesome?std::size(FrontierSwitchBlocks404):std::size(FrontierSwitchBlocks405);
        if (!loadFrontierSwitchLayers(parkSwitchStage,CourseIds[index],blocks,count,path))
            return failed("late_switch_collision");
        const bool overlayLoaded=wholesome?
            wholesomeTiles.openShared(path("snow-switch-4_04.pak")):
            cubeTownTiles.openShared(path("snow-switch-4_05.pak"));
        if (!overlayLoaded || !frontierSwitchFaces.load(path("snow-switch-faces.pak")))
            return failed("late_switch_art");
        if (wholesome && !wholesomePortalArt.load(path("snow-portals-4_04.pak")))
            return failed("late_portal_art");
    } else if (index==Elysian512Course) {
        const auto first=path("5_12-switch-0.bin"),second=path("5_12-switch-1.bin");
        const char* layers[]={first.c_str(),second.c_str()};
        parkSwitchStage.load(nullptr,0,nullptr,0);
        if (!parkSwitchStage.load(layers,std::size(layers),ElysianSwitchBlocks512,std::size(ElysianSwitchBlocks512)) ||
            !parkArt.load(path("park-stage-5_12.pak").c_str(),true))
            return failed("elysian_switch_512");
    } else if (index==Elysian514Course) {
        if (!loadFrontierSwitchLayers(parkSwitchStage,CourseIds[index],ElysianSwitchBlocks514,
                                      std::size(ElysianSwitchBlocks514),path) ||
            !elysianSwitchFaces514.load(path("elysian-switch-faces-5_14.pak")) ||
            elysianSwitchFaces514.size()!=ElysianSwitchFaces514Count)
            return failed("elysian_switch_514");
    } else if (index==CosmicThirdCourse || index==CosmicFourthCourse) {
        const bool fourth=index==CosmicFourthCourse;
        const auto first=path(fourth?"6_04-switch-0.bin":"6_03-switch-0.bin");
        const auto second=path(fourth?"6_04-switch-1.bin":"6_03-switch-1.bin");
        const char* layers[]={first.c_str(),second.c_str()};
        parkSwitchStage.load(nullptr,0,nullptr,0);
        const auto* blocks=fourth?CosmicSwitches604:CosmicSwitches603;
        const unsigned count=fourth?std::size(CosmicSwitches604):std::size(CosmicSwitches603);
        if (!parkSwitchStage.load(layers,std::size(layers),blocks,count))
            return failed("cosmic_switch_collision");
    } else if (index==CosmicSeventhCourse) {
        const auto first=path("6_07-switch-0.bin"),second=path("6_07-switch-1.bin");
        const char* layers[]={first.c_str(),second.c_str()};
        parkSwitchStage.load(nullptr,0,nullptr,0);
        if (!parkSwitchStage.load(layers,std::size(layers),CosmicSwitches607,std::size(CosmicSwitches607)))
            return failed("cosmic_switch_607_collision");
    } else if (index==CosmicSeventeenthCourse) {
        const auto a=path("6_17-s0.bin"),b=path("6_17-s1.bin"),c=path("6_17-s2.bin"),d=path("6_17-s3.bin");
        const char* layers[]={a.c_str(),b.c_str(),c.c_str(),d.c_str()};
        if (!parkSwitchStage.load(layers,std::size(layers),CosmicSwitches617,std::size(CosmicSwitches617),
                                  0,reinterpret_cast<TileField*>(goalFlags.scratch())+4))
            return failed("cosmic_switch_617_collision");
    } else if (index==CosmicTwelfthCourse) {
        const auto first=path("6_12-switch-0.bin"),second=path("6_12-switch-1.bin");
        const auto third=path("6_12-switch-2.bin"),fourth=path("6_12-switch-3.bin");
        const char* layers[]={first.c_str(),second.c_str(),third.c_str(),fourth.c_str()};
        parkSwitchStage.load(nullptr,0,nullptr,0);
        if (!parkSwitchStage.load(layers,std::size(layers),CosmicSwitches612,std::size(CosmicSwitches612)))
            return failed("cosmic_switch_612_collision");
    } else if (!loadSwitchCourse(parkSwitchStage,CourseIds[index],path)) {
        return failed("switch_collision");
    }
    if (index>=PlentifulChucklesCourse && index<=UncannyValleyCourse &&
        (!frontierNukeArt.openShared(path(index==PlentifulChucklesCourse?"frontier-nuke-4_08.pak":"frontier-nuke-4_07.pak")) ||
         !frontierNukeExplosions.openShared(path("frontier-nuke-explosions.pak"))))
        return failed("frontier_nuke_art");
    if ((index==CrimsonOutpostCourse || index==CataclysmCourse) && !frontierLights1617.openShared(path("frontier-lights-1617.pak")))
        return failed("frontier_1617_lights");
    if (index==CrimsonOutpostCourse &&
        (!frontierProps416.openShared(path("frontier-props-4_16.pak")) ||
         !frontierNukeArt.openShared(path("frontier-nuke-4_07.pak")) ||
         !frontierNukeExplosions.openShared(path("frontier-nuke-explosions.pak"))))
        return failed("frontier_416_art");
    if ((index==Elysian514Course || index==CosmicTenthCourse || index==CosmicFourteenthCourse || index==CosmicSeventeenthCourse) &&
        (!frontierNukeArt.openShared(path("frontier-nuke-4_07.pak")) ||
         !frontierNukeExplosions.openShared(path("frontier-nuke-explosions.pak"))))
        return failed("nuke_art");
    if (index==Elysian515Course &&
        (!elysianPortalArt515.load(path("elysian-portals-5_15.pak")) ||
         elysianPortalArt515.size()!=ElysianPortals515Count))
        return failed("elysian_portals_515");
    if (index>=MovieOnceCourse && index<=AbandonedWorkspaceCourse &&
        !frontierNight1415.openShared(path("frontier-night-1415.pak")))
        return failed("frontier_1415_art");
    if (index>=WeatherAlertCourse && index<=DownfallCourse && !frontierNight1213.openShared(path("frontier-night-1213.pak")))
        return failed("frontier_1213_art");
    if (index==UncannyValleyCourse && !frontier409Night.openShared(path("frontier-night-4_09.pak")))
        return failed("frontier_409_art");
    if (index==FrontierPortalCourse && !frontierPortalArt.load(path("snow-portals-4_03.pak")))
        return failed("frontier_portal_art");
    if (index==FrontierMazeCourse &&
        (!frontierMazePortals.load(path("snow-portals-4_06.pak")) ||
         !frontierBigCoin.load(path("frontier-big-coin-4_06.pak"))))
        return failed("frontier_maze_art");
    if (parkSwitchStage && index>=PursuitCourse && index<FinalFrontierFirstCourse) {
        const auto overlay=std::string("park-switch-")+CourseIds[index]+".pak";
        const auto& counts=ParkSwitchPatchCounts[index-PursuitCourse];
        if (!pursuitTiles.load(path(overlay.c_str()))) return failed("switch_art");
        if (pursuitTiles.size()!=counts[0]+counts[1]) return failed("switch_art_count");
        if (!switchFaces.load(path("park-switch-faces.pak")) || switchFaces.size()!=2)
            return failed("switch_faces");
    }
    if (index==JumbleCourse && (!jumbleVideo.open(path("park-video-3_04.pak").c_str()) ||
        !jumbleTiles.open(path("park-tiles-3_04.pak").c_str()) ||
        !jumbleFlag.load(path("park-flag-3_04.pak")) || jumbleFlag.size()!=5)) return false;
    if (multiDogStage(index) && !dualGateFields) {
        std::unique_ptr<TileField[]> fields(new(std::nothrow) TileField[3]);
        if (!fields) return failed("dual_gate_memory");
        constexpr const char* suffixes[]{"-dog-0.bin","-dog-1.bin","-dog-both.bin"};
        for (unsigned i=0;i<std::size(suffixes);++i) {
            const auto name=std::string(CourseIds[index])+suffixes[i];
            const auto file=openAssetFile(path(name.c_str()).c_str());
            if (!file || !fields[i].read(file.get())) return failed("dual_gate");
        }
        dualGateFields=std::move(fields);
        for (unsigned i=0;i<std::size(dualGateLayers);++i) dualGateLayers[i]=&dualGateFields[i];
    }
    if (multiDogStage(index)) {
        const auto door=std::string("park-dog-tiles-")+CourseIds[index]+".pak";
        const auto& counts=DualGatePatches[index-DoubleDogCourse];
        const bool gate=index==RacetrackCourse?
            raceGateArt.load(path(door.c_str())) && raceGateArt.size()==counts[0]+counts[1]:
            dualGateArt.load(path(door.c_str())) && dualGateArt.size()==counts[0]+counts[1];
        if (!gate || !dogArt.load(path("park-dogs-3_16-17.pak")) || dogArt.size()!=12 ||
            !factorySpawner.load(path("park-spawner-3_07.pak")) || factorySpawner.size()!=4)
            return failed("multi_dog_art");
        if (index==DoubleDogCourse && (!dogCameraArt.load(path("park-camera-3_16.pak")) || dogCameraArt.size()!=13))
            return failed("dog_camera");
        if (index==RacetrackCourse && (!raceDoors.load(path("park-key-doors-3_17.pak")) || raceDoors.size()!=4))
            return failed("race_doors");
    }
    if (index==FrontierPatrolCourse || index==FrontierMineCourse) {
        if (!frontierDogArt.openShared(path("frontier-dog-4_07.pak")) ||
            !frontier1011Lights.openShared(path("frontier-light-1011.pak")))
            return failed("frontier_patrol_art");
        if (index==FrontierMineCourse && !frontierMineBlock.load(path("frontier-block-4_11.pak")))
            return failed("frontier_mine_blocks");
        dogGate={};
    } else if (dogSceneAt(index) && index!=CosmicSeventeenthCourse) {
        const std::string name=std::string(CourseIds[index])+"-dog.bin";
        const auto gate=openAssetFile(path(name.c_str()).c_str());
        const std::string actor=std::string("park-dog-")+CourseIds[index]+".pak";
        const std::string door=std::string("park-dog-tiles-")+CourseIds[index]+".pak";
        if (!gate || !dogGate.read(gate.get())) return failed("dog_collision");
        if (index==FrontierDogCourse) {
            if (!frontierDogArt.openShared(path("frontier-dog-4_07.pak")) ||
                !frontierDogTiles.openShared(path("snow-dog-tiles-4_07.pak")) ||
                !frontierNukeArt.openShared(path("frontier-nuke-4_07.pak")) ||
                !frontierNukeExplosions.openShared(path("frontier-nuke-explosions.pak")))
                return failed("frontier_dog_art");
        } else if (!dogArt.load(path(actor.c_str())) || dogArt.size()!=12) {
            return failed("dog_assets");
        }
        if (index==FrontierDogCourse) {
            dogArt.unload();
        } else if (index==DogCourse) {
            if (!dogDoorArt.load(path(door.c_str())) || dogDoorArt.size()!=1) return failed("dog_door");
        } else if (!dogWideDoorArt.load(path(door.c_str())) || dogWideDoorArt.size()!=1) {
            return failed("dog_door");
        }
    }
    if (index==CataclysmCourse) {
        const auto gate=openAssetFile(path("4_17-fake-goal-gate.bin").c_str());
        if (!gate || !dogGate.read(gate.get())) return failed("frontier_417_gate_collision");
    } else if (currentCourse==CataclysmCourse) {
        audio.stageCue(StageCue::None);
    }
    if (index!=TheWallCourse || chillMode) frontierPrism418.unload();
    if (index!=TheWallCourse || !chillMode) frontierBed418.unload();
    if (index!=TheWallCourse) frontierLight418.unload();
    if (index!=Elysian508Course) elysianClouds508.unload();
    else if (!elysianClouds508.openOnDemand(path("frontier-clouds-5_08.pak"))) return failed("elysian_clouds_508");
    if (index!=Elysian509Course) elysianClouds509.unload();
    else if (!elysianClouds509.openOnDemand(path("frontier-clouds-5_09.pak"))) return failed("elysian_clouds_509");
    if (index==Elysian511Course) {
        const auto car=openAssetFile(path("elysian-car-5_11.bin").c_str());
        if (!car || std::fread(elysianCarPixels511,sizeof elysianCarPixels511,1,car.get())!=1 ||
            std::fgetc(car.get())!=EOF || std::ferror(car.get())) return failed("elysian_car_511");
        sceKernelDcacheWritebackRange(elysianCarPixels511,sizeof elysianCarPixels511);
    }
    if (index!=Elysian510Course) elysianClouds510.unload();
    else if (!elysianClouds510.openOnDemand(path("frontier-clouds-5_10.pak"))) return failed("elysian_clouds_510");
    if (index!=Elysian509Course && index!=Elysian510Course) elysianBlock509.unload();
    else if (!elysianBlock509.openOnDemand(path("frontier-dblock-5_09.pak"))) return failed("elysian_dblock");
    if (index!=Elysian507Course) elysianClouds507.unload();
    else if (!elysianClouds507.openOnDemand(path("frontier-clouds-5_07.pak")))
        return failed("elysian_clouds_507");
    if (index!=Elysian514Course && index!=Elysian517Course) elysianClouds514.unload();
    else if (!elysianClouds514.openOnDemand(path("frontier-clouds-5_14.pak"))) return failed("elysian_clouds_514");
    if (index<Elysian514Course || index>=Elysian517Course) elysianMist043.unload();
    else if (!elysianMist043.openShared(path("elysian-mist-051415.pak"))) return failed("elysian_mist_043");
    const bool hasElysianClouds=(hidesElysianScoring(index) || isHorseCourse039(index)) &&
                               !isElysianHazardCourse040(index) && index!=Elysian507Course && index!=Elysian510Course && index!=Elysian514Course && index!=Elysian517Course;
    if (!hasElysianClouds || (index==ElysianRocksCourse)!=(currentCourse==ElysianRocksCourse))
        frontierClouds501.unload();
    if (hasElysianClouds && !frontierClouds501.openShared(path(index==ElysianRocksCourse?
             "frontier-clouds-5_02.pak":"frontier-clouds-5_01.pak"))) return failed("elysian_clouds");
    if (index==TheWallCourse) {
        if (!frontierLight418.openShared(path("frontier-light-4_18.pak")) ||
            (chillMode && !frontierBed418.openShared(path("frontier-bed-4_18.pak"))) ||
            (!chillMode && !frontierPrism418.openShared(path("frontier-prism-4_18.pak"))))
            return failed("frontier_418_art");
        const auto easy=openAssetFile(path("4_18-easy-tiles.bin").c_str());
        if (!easy || !dogGate.read(easy.get())) return failed("frontier_418_easy_collision");
    }
    if ((index==DogCourse || index==ScreenDogCourse) &&
        !dogScreen.load(path("park-screen-3_13.pak"))) return failed("dog_screen");
    if (index==PursuitCourse) {
        if (!pursuitDecor.load(path("park-decoration-3_12.pak")) || pursuitDecor.size()!=38 ||
            !pursuitBalloons.load(path("park-balloons-3_12.pak")) ||
            pursuitBalloons.size()!=PursuitBalloonCount) return failed("pursuit_art");
        if (!pursuitMotion) {
            const auto source=openAssetFile(path("park-balloon-motion-3_12.bin").c_str());
            constexpr unsigned samples=PursuitBalloonCount*PursuitMotionFrames;
            std::unique_ptr<PursuitBalloonPoint[]> motion(new(std::nothrow) PursuitBalloonPoint[samples]);
            if (!source || !motion || std::fread(motion.get(),sizeof(PursuitBalloonPoint),samples,source.get())!=samples ||
                std::fgetc(source.get())!=EOF || std::ferror(source.get())) return failed("pursuit_motion");
            pursuitMotion=std::move(motion);
        }
    }
    if (hasCoaster(index) && !coasterArt.pixels() && !coasterArt.load(path("coaster.pak").c_str())) return failed("coaster");
    if (index==FrontierDogCourse || index==Elysian513Course || index==Elysian514Course || index==CosmicSecondCourse || (index>=CosmicFourteenthCourse && index<=CosmicSeventeenthCourse)) {
        if (!chuckleArt.openShared(path("chuckle.pak").c_str())) return failed("frontier_chuckle");
    } else if (needsChuckles && !chuckleArt.pixels() && !chuckleArt.load(path("chuckle.pak").c_str())) {
        return failed("chuckle");
    }
    if (index==ArcadeCourse &&
        (!arcadeDoors.load(path("park-doors-3_11.pak")) || arcadeDoors.size()!=6)) return false;
    if (index==LightsCourse &&
        (!darkHoleArt.load(path("park-lights-3_10.pak")) || darkHoleArt.size()!=1 ||
         !ashArt.load(path("park-ash-3_10.pak")) || ashArt.size()!=3)) return false;
    if (index==MartCourse &&
        (!marcySpillArt.load(path("park-marcy-spill-3_06.pak")) || marcySpillArt.size()!=16 ||
         !marcyHudArt.load(path("park-marcy-hud-3_06.pak")) || marcyHudArt.size()!=4 ||
         !marcyTextArt.load(localizedPath(executable,language,"park-marcy-text-3_06.pak")) ||
         marcyTextArt.size()!=2)) return false;
    if (index==FactoryCourse &&
        (!factoryDoors.load(path("park-doors-3_07.pak")) || factoryDoors.size()!=4 ||
         !factoryBalloons.load(path("park-balloons-3_07.pak")) || factoryBalloons.size()!=3 ||
         !factorySpawner.load(path("park-spawner-3_07.pak")) || factorySpawner.size()!=4 ||
         !factoryDecor.load(path("park-decor-3_07.pak")) || factoryDecor.size()!=18)) return false;
    if (parkSwitchStage && (index<FinalFrontierFirstCourse || index==Elysian512Course)) {
        const std::string overlay=std::string("park-switch-")+CourseIds[index]+".pak";
        if (!switchFaces.load(path("park-switch-faces.pak")) || switchFaces.size()!=2) return false;
        if (index<PursuitCourse && index!=JumbleCourse && !switchTiles.load(path(overlay.c_str()))) return false;
        if (index==JumbleCourse || index>=PursuitCourse) switchTiles.unload();
    } else {
        switchTiles.unload();
        switchFaces.unload();
    }
    if (index!=Elysian513Course && index!=Elysian514Course) elysianSpawnerArt513.unload();
    else if (!elysianSpawnerArt513.load(path("elysian-spawner-5_13.pak")) || elysianSpawnerArt513.size()!=4)
        return failed("elysian_spawner_art");
    if (index==Elysian513Course && (!dogArt.load(path("park-dogs-3_16-17.pak")) || dogArt.size()!=12))
        return failed("elysian_traffic_art");
    if (index==DenseCourse) {
        if (!coasterSprings.load(path("coaster-springs.pak").c_str()) ||
            !denseBlocks.load(path("dense-blocks-3_08.pak").c_str()) || denseBlocks.size()!=60 ||
            !denseBlockArt.load(path("park-blocks-3_08.pak")) || denseBlockArt.size()!=denseBlocks.size()) return false;
    } else {
        coasterSprings.unload();
        denseBlocks.unload();
        denseBlockArt.unload();
    }
    const unsigned blocks=index>=ChuckleParkFirstCourse && index!=DownfallCourse &&
                          index!=CrimsonOutpostCourse && index!=Elysian509Course && index!=Elysian510Course && index!=CosmicSecondCourse && index!=CosmicFifthCourse && index<CosmicFifteenthCourse?
                          courseHazards(index).blockCount:0;
    if (blocks) {
        const std::string name=std::string("park-blocks-")+CourseIds[index]+".pak";
        const char* failure="park_block_art";
        uint32_t* vram=nullptr;
        uint32_t* scratch=nullptr;
        if (index==Elysian506Course) {
            constexpr unsigned scratchOffset=HorseMeter039Width*HorseMeter039Height;
            static_assert(scratchOffset+128*64*sizeof(uint32_t)<=sizeof fallbackPage);
            if (static_cast<unsigned>(sceGeEdramGetSize())<ParkBlockVramOffset+128*64*sizeof(uint32_t))
                return failed("park_block_vram");
            vram=reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(sceGeEdramGetAddr())+ParkBlockVramOffset);
            scratch=reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(fallbackPage)+scratchOffset);
            fallbackPageIndex=ScreenPages;
        }
        if (!parkBlockArt.load(path(name.c_str()),&failure,vram,scratch)) return failed(failure);
        if (parkBlockArt.size()!=blocks) return failed("park_block_count");
    } else {
        parkBlockArt.unload();
    }
    if (index==Elysian516Course) {
        constexpr unsigned worldBytes=ElysianWizard516Width*ElysianWizard516Height*4;
        constexpr unsigned spellBytes=ElysianSpells516Width*ElysianSpells516Height*4;
        constexpr unsigned spellOffset=LiveResultVramOffset+ResultLargeBytes+ResultClearBytes;
        static_assert(worldBytes<=FrontierLightVramBytes && spellBytes<=ResultConfettiBytes);
        if (static_cast<unsigned>(sceGeEdramGetSize())<spellOffset+spellBytes) return failed("wizard_vram");
        auto* edram=static_cast<uint8_t*>(sceGeEdramGetAddr());
        unsigned generation=0;
        auto* scratch=reinterpret_cast<uint32_t*>(clips.textureWorkspace<std::max(worldBytes,spellBytes)>(generation));
        if (!wizardArt516.load(path("elysian-wizard-5_16.pak"),nullptr,
                              reinterpret_cast<uint32_t*>(edram+FrontierLightVramOffset),scratch) ||
            wizardArt516.size()!=ElysianWizard516Count ||
            !spellArt516.load(path("elysian-spells-5_16.pak"),nullptr,
                              reinterpret_cast<uint32_t*>(edram+spellOffset),scratch) ||
            spellArt516.size()!=ElysianSpells516Count) return failed("wizard_art");
    }
    if (!loadFinaleCourse045(executable,index)) return failed("finale_art");
    if (!loadCosmicCourse046(executable,index)) return failed("cosmic_art");
    if (index==CarouselCourse) {
        if (!carouselDecal.load(path("park-carousel-3_02.pak")) || carouselDecal.size()!=18) return false;
    } else {
        carouselDecal.unload();
    }
    return true;
}

bool loadLocalizedAssets(const char* executable,Course& course,Language selected,const char** error=nullptr) {
    language=selected;
    const auto fail=[error](const char* asset) {
        if (error) *error=asset;
        return false;
    };
    if (!loadCourseAt(executable,currentCourse,course)) return fail("course.bin");
    if (!clips.open(localizedPath(executable,selected,"anim.pak").c_str())) return fail("anim.pak");
    if (!objects.load(localizedPath(executable,selected,"objects.pak").c_str())) return fail("objects.pak");
    if (!loadGolf(localizedPath(executable,selected,"golf.pak").c_str())) return fail("golf.pak");
    if (!resultAssets.load(localizedPath(executable,selected,"results.pak").c_str())) return fail("results.pak");
    if (!ranks.open(localizedPath(executable,selected,"ranks.pak").c_str())) return fail("ranks.pak");
    return true;
}

bool changeLanguage(const char* executable,Course& course,const LanguageFile& config,Language selected) {
    const Language previous=language;
    if (selected==previous) return true;
    if (loadLocalizedAssets(executable,course,selected) && config.write(selected)) return true;
    loadLocalizedAssets(executable,course,previous);
    return false;
}

void preloadTutorialVictory();

bool enterCourse(const char* executable,unsigned index,Course& course,Round& round) {
    clips.resetTextureWorkspace();
    const bool modeChanged=(index==TheWallCourse && round.wall418.easyMode!=chillMode) || index>=Elysian518Course;
    if (index>=CourseCount ||
        ((index!=currentCourse || modeChanged || titleReusedCourseVram) && !loadCourseAt(executable,index,course))) return false;
    if (currentCourse==Elysian516Course && index!=currentCourse) {
        audio.stageCue(StageCue::None);
        audio.rideVolume(1,-80,-80);
        audio.stop(Sound::WizardMagic1);
        audio.stop(Sound::WizardMagic2);
        audio.stop(Sound::WizardMagic3);
        audio.stop(Sound::WizardDeath);
    }
    if (currentCourse==CosmicFifthCourse && index!=currentCourse) {
        audio.stageCue(StageCue::None);
        audio.stopRingTimers();
        audio.rideVolume(1,-80,-80);
    }
    if (currentCourse==Elysian518Course && index!=currentCourse) audio.musicTune(1,1);
    if (currentCourse==CosmicThirteenthCourse) audio.stageCue(StageCue::None);
    if (currentCourse==CosmicSeventeenthCourse) stopLastHurrahSounds053();
    currentCourse=index;
    preloadTutorialVictory();
    round.restart();
    return true;
}

constexpr unsigned RankAnimationEndMs = 4500;

bool canAdvance(unsigned resultMs) {
    return currentCourse!=HorseBroadcastCourse &&
        resultMs>=(hidesElysianScoring(currentCourse)?2000:RankAnimationEndMs);
}

Clip resultClip(bool won,bool lightsOut=false) {
    if (currentCourse==Elysian518Course) return won?Clip::Win518:Clip::Loss518;
    if (currentCourse>=CosmicFirstCourse)
        return static_cast<Clip>(static_cast<unsigned>(Clip::Win601)+2*(currentCourse-CosmicFirstCourse)+!won);
    if (currentCourse==TheWallCourse) return won?Clip::Win418:Clip::Loss418;
    if (currentCourse>=ElysianRocksCourse)
        return static_cast<Clip>(static_cast<unsigned>(Clip::Win502)+2*(currentCourse-ElysianRocksCourse)+!won);
    if (currentCourse==ElysianFieldsCourse) return won?Clip::Win501:Clip::Loss501;
    if (currentCourse==CrimsonOutpostCourse) return won?Clip::Win416:Clip::Loss416;
    if (currentCourse==CataclysmCourse) return won?Clip::Win417:Clip::Loss417;
    if (currentCourse==MovieOnceCourse) return won?Clip::Win414:Clip::Loss414;
    if (currentCourse==AbandonedWorkspaceCourse) return won?Clip::Win415:Clip::Loss415;
    if (currentCourse==WeatherAlertCourse) return won?Clip::Win412:Clip::Loss412;
    if (currentCourse==DownfallCourse) return won?Clip::Win413:Clip::Loss413;
    if (currentCourse==FrontierPatrolCourse) return won?Clip::Win410:Clip::Loss410;
    if (currentCourse==FrontierMineCourse) return won?Clip::Win411:Clip::Loss411;
    if (currentCourse>=PlentifulChucklesCourse)
        return static_cast<Clip>(static_cast<unsigned>(Clip::Win408)+2*(currentCourse-PlentifulChucklesCourse)+!won);
    if (currentCourse==FrontierMazeCourse) return won?Clip::Win406:Clip::Loss406;
    if (currentCourse==FrontierDogCourse) return won?Clip::Win407:Clip::Loss407;
    if (currentCourse>=WholesomeCourse)
        return static_cast<Clip>(static_cast<unsigned>(Clip::Win404)+2*(currentCourse-WholesomeCourse)+!won);
    if (currentCourse==FrontierSwitchCourse) return won?Clip::Win402:Clip::Loss402;
    if (currentCourse==FrontierPortalCourse) return won?Clip::Win403:Clip::Loss403;
    if (currentCourse==LightsCourse && !won && lightsOut) return Clip::Loss310Dark;
    if (currentCourse>=PursuitCourse)
        return static_cast<Clip>(static_cast<unsigned>(Clip::Win312)+2*(currentCourse-PursuitCourse)+!won);
    if (currentCourse==ChuckleParkFirstCourse) return won?Clip::Win301:Clip::Loss301;
    if (currentCourse>=CarouselCourse)
        return static_cast<Clip>(static_cast<unsigned>(Clip::Win302)+2*(currentCourse-CarouselCourse)+!won);
    if (currentCourse>=GlowstickFirstCourse+5)
        return static_cast<Clip>(static_cast<unsigned>(Clip::Win206)+
                                 2*(currentCourse-GlowstickFirstCourse-5)+!won);
    if (currentCourse>=GlowstickFirstCourse)
        return static_cast<Clip>(static_cast<unsigned>(Clip::Win201)+2*(currentCourse-GlowstickFirstCourse)+!won);
    const auto first=static_cast<unsigned>(Clip::Win);
    return static_cast<Clip>(first+2*currentCourse+(currentCourse>=TutorialCount)+!won);
}

void initializeGraphics() {
    sceGuInit();
    sceGuStart(GU_DIRECT,displayList);
    sceGuDrawBuffer(GU_PSM_5650,nullptr,Stride);
    sceGuDispBuffer(Width,Height,reinterpret_cast<void*>(FrameBytes),Stride);
    sceGuOffset(2048-Width/2,2048-Height/2);
    sceGuViewport(2048,2048,Width,Height);
    sceGuScissor(0,0,Width,Height);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_DITHER);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
    sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

#ifndef UCG_VISUAL_TEST
bool showBootSplash() {
    static_assert(BootSplashWidth<=Width && BootSplashHeight==Height &&
                  BootSplashWidth*BootSplashHeight*sizeof(uint16_t)<=sizeof gameplayPage);
    if (static_cast<unsigned>(sceGeEdramGetSize())<2*FrameBytes) return false;
    auto bytes=static_cast<uLongf>(BootSplashWidth*BootSplashHeight*sizeof(uint16_t));
    const auto expected=bytes;
    if (uncompress(reinterpret_cast<Bytef*>(gameplayPage),&bytes,BootSplashDeflate,sizeof BootSplashDeflate)!=Z_OK ||
        bytes!=expected) return false;
    auto* display=static_cast<uint16_t*>(sceGeEdramGetAddr())+FrameBytes/sizeof(uint16_t);
    auto* uncached=reinterpret_cast<uint16_t*>(reinterpret_cast<uintptr_t>(display)|0x40000000u);
    std::fill_n(uncached,Stride*Height,UINT16_MAX);
    for (unsigned row=0;row<Height;++row) {
        std::memcpy(uncached+row*Stride+(Width-BootSplashWidth)/2,
                    gameplayPage+row*BootSplashWidth,BootSplashWidth*sizeof(uint16_t));
    }
    return sceDisplaySetMode(PSP_DISPLAY_MODE_LCD,Width,Height)>=0 &&
        sceDisplaySetFrameBuf(display,Stride,PSP_DISPLAY_PIXEL_FORMAT_565,PSP_DISPLAY_SETBUF_NEXTFRAME)>=0;
}
#endif

void rectangle(float x,float y,float w,float h,uint32_t color) {
    sceGuDisable(GU_TEXTURE_2D);
    auto* vertices=static_cast<ColorVertex*>(sceGuGetMemory(2*sizeof(ColorVertex)));
    vertices[0]={color,x,y,0};
    vertices[1]={color,x+w,y+h,0};
    sceGuDrawArray(GU_SPRITES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,vertices);
}

void setTexture(int format,int width,int height,const void* pixels) {
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(format,0,0,GU_TRUE);
    sceGuTexImage(0,width,height,width,pixels);
}

void patchScaled(const Patch& p,float x,float y,float scaleX,float scaleY,uint32_t tint,bool flip=false) {
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
    vertices[0]={static_cast<float>(p.u+(flip?p.width:0)),static_cast<float>(p.v),tint,x,y,0};
    vertices[1]={static_cast<float>(p.u+(flip?0:p.width)),static_cast<float>(p.v+p.height),
                 tint,x+p.width*scaleX,y+p.height*scaleY,0};
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,vertices);
}

void patch(const Patch& p,float x,float y,float scale=1.0f,uint32_t tint=White,bool flip=false) {
    patchScaled(p,x,y,scale,scale,tint,flip);
}

void hudPatch(const Patch& p) { patch(p,ViewX+p.x,p.y); }

#include "menu_scroll_draw.hpp"

// Use rasterScale=1 for patches extracted from this course's PC capture.
void actor(const GameplayView& view,const Patch& p,Vec2 world,float rasterScale=1.0f,uint32_t tint=White,bool projectedTexture=false) {
    GameplayView::Sprite sprite;
    if (!view.clipSprite(p,world,rasterScale,sprite,projectedTexture)) return;
    // Clip before issuing the GE sprite so offscreen positions cannot wrap.
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
    vertices[0]={sprite.uvTopLeft.x,sprite.uvTopLeft.y,tint,sprite.topLeft.x,sprite.topLeft.y,0};
    vertices[1]={sprite.uvBottomRight.x,sprite.uvBottomRight.y,tint,sprite.bottomRight.x,sprite.bottomRight.y,0};
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,vertices);
}

void sharedActor(const GameplayView& view,const Patch& p,Vec2 world,uint32_t tint=White) {
    actor(view,p,world,view.worldZoom(),tint);
}

void aimLine(const GameplayView& view,Vec2 world,Vec2 drag,bool valid) {
    const float magnitude=length(drag);
    if (magnitude<1) return;
    const Vec2 start=view.position(world),end=start+view.delta(drag);
    const Vec2 normal=Vec2{-drag.y,drag.x}*(view.worldZoom()/magnitude);
    const uint32_t color=valid ? GU_RGBA(237,118,83,182) : GU_RGBA(128,0,0,255);
    sceGuDisable(GU_TEXTURE_2D);
    auto* v=static_cast<ColorVertex*>(sceGuGetMemory(4*sizeof(ColorVertex)));
    v[0]={color,start.x-normal.x,start.y-normal.y,0};
    v[1]={color,start.x+normal.x,start.y+normal.y,0};
    v[2]={color,end.x-normal.x,end.y-normal.y,0};
    v[3]={color,end.x+normal.x,end.y+normal.y,0};
    sceGuDrawArray(GU_TRIANGLE_STRIP,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,v);
}

void drawClock(unsigned centis,int first,bool hours) {
    const unsigned seconds=centis/100%60, minutes=centis/6000%60, h=centis/360000%100;
    if (hours) {
        hudPatch(visual.clock[first++][h/10]);
        hudPatch(visual.clock[first++][h%10]);
    }
    const unsigned values[]={minutes/10,minutes%10,seconds/10,seconds%10,centis/10%10,centis%10};
    for (unsigned digit:values) hudPatch(visual.clock[first++][digit]);
}

void drawGhostIndicator(const GameplayView& view,Vec2 world,uint32_t tint=White) {
    Vec2 indicator{};
    if (!view.ghostIndicator(world,indicator)) return;
    const float scale=std::max(0.25f,-0.4f+1.15f*dot(indicator,indicator)/dot(world,world));
    const Vec2 point=view.position(indicator);
    if (point.x<ViewX+136-10 || point.x>ViewX+ViewWidth+10) return;
    actor(view,visual.ghost,indicator,scale,tint,true);

    const Vec2 direction=world-indicator;
    const Vec2 forward=view.delta(direction)*(1.0f/length(direction));
    const Vec2 side{-forward.y,forward.x};
    constexpr Vec2 polygon[]={{14,0},{9,-3},{10,0},{9,3}};
    auto* vertices=static_cast<ColorVertex*>(sceGuGetMemory(4*sizeof(ColorVertex)));
    for (unsigned i=0;i<std::size(polygon);++i) {
        const Vec2 corner=point+forward*polygon[i].x+side*polygon[i].y;
        vertices[i]={GU_RGBA(255u,0u,0u,255u),corner.x,corner.y,0};
    }
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDrawArray(GU_TRIANGLE_FAN,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,vertices);
    sceGuEnable(GU_TEXTURE_2D);
}

void drawStageIntro(unsigned ticks) {
    const IntroPose& pose=stagePositions[std::min(ticks,86u)];
    const int left=std::max(0,-pose.x),right=std::min(128,ViewWidth-pose.x);
    if (left>=right) return;
    setTexture(GU_PSM_8888,128,64,stageTexture);
    patch({static_cast<int16_t>(left),0,static_cast<int16_t>(right-left),51,0,0},
          ViewX+pose.x+left,pose.y-2);
}

void drawStrokes(unsigned strokes) {
    char digits[11];
    std::snprintf(digits,sizeof digits,"%u",strokes);
    const int lift=russian()?2:0;
    const Patch& stroke=visual.strokes[digits[0]-'0'];
    patch(stroke,ViewX+stroke.x,stroke.y-lift);
    for (int i=1;digits[i] && i<6;++i) {
        const Patch& digit=visual.clock[6][digits[i]-'0'];
        patch(digit,ViewX+68+(i-1)*8,126-lift);
    }
}

unsigned localeTextWidth(const char* text) {
    unsigned width=0;
    while (*text) width+=glyphAdvance[nextGlyph(text)];
    return width;
}

void localeText(const char* text,float x,float y,float scale=1.0f,uint32_t color=White,
                bool centered=false,unsigned maxWidth=0,bool keepHeight=false) {
    const float requestedScale=scale;
    if (centered || maxWidth) {
        const unsigned width=localeTextWidth(text);
        if (maxWidth && width) scale=std::min(scale,static_cast<float>(maxWidth)/width);
        if (centered) x-=width*scale*0.5f;
    }
    setTexture(GU_PSM_8888,512,128,localeFont);
    while (*text) {
        const unsigned glyph=nextGlyph(text);
        patchScaled({static_cast<int16_t>(glyph%32*16),static_cast<int16_t>(glyph/32*20),16,20,0,0},
                    ViewX+x,y,scale,keepHeight?requestedScale:scale,color);
        x+=glyphAdvance[glyph]*scale;
    }
}

// Stretch a clean column of the current HUD backdrop across the baked text.
void patchHudGradient(float x,float y,float width,float height) {
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
    vertices[0]={126,y,White,ViewX+x,y,0};
    vertices[1]={127,y+height,White,ViewX+x+width,y+height,0};
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                   2,nullptr,vertices);
}

void drawParBadge(const uint16_t* backdrop,unsigned par,unsigned elapsedMs) {
    constexpr int centerX=102, centerY=140;
    setTexture(GU_PSM_5650,VisualWidth,VisualHeight,backdrop);
    patchHudGradient(85,124,34,32);

    const auto u=static_cast<int16_t>(32*(elapsedMs/100%4));
    setTexture(GU_PSM_4444,128,64,ParBallTexture);
    patch({u,0,30,30,0,0},ViewX+centerX-15,centerY-15);
    char digits[11];
    const int count=std::snprintf(digits,sizeof digits,"%u",par);
    const int width=12+10*(count-1);
    const float scale=std::min(1.0f,22.0f/width);
    float x=ViewX+centerX-width*scale*0.5f;
    for (const char* digit=digits;*digit;++digit) {
        const auto digitU=static_cast<int16_t>(12*(*digit-'0'));
        patch({digitU,32,12,18,0,0},x,centerY-9*scale,scale);
        x+=10*scale;
    }
}

void resultText(const char* text,float x,float y,uint32_t tint=White,float scale=1.0f) {
    if (russian()) {
        localeText(text,x,y,0.82f*scale,tint);
        return;
    }
    setTexture(GU_PSM_8888,256,128,resultFont);
    for (;*text;++text,x+=6*scale) {
        const unsigned index=static_cast<unsigned char>(*text)-32u;
        if (index>=96) continue;
        const Patch glyph{static_cast<int16_t>(index%16*16),static_cast<int16_t>(index/16*16),16,16,0,0};
        patch(glyph,ViewX+x,y,scale,tint);
    }
}

Score scoreFor(const Course& course,const Round& round) {
    return calculateScore(course.par,round.game.strokes,round.ticks*5/3,
                          static_cast<unsigned>(walls.onTime*100),round.bonus(),
                          objects.data.courses[currentCourse].ranks);
}

void resultLabel(const char* text,int x,int y,bool small=false,uint32_t color=White,bool centered=false) {
    if (russian() || small) {
        localeText(text,x,y,SmallUiText,color,centered);
        return;
    }
    constexpr unsigned bank=0;
    float pen=static_cast<float>(x);
    if (centered) {
        unsigned width=0;
        for (const char* letter=text;*letter;++letter) {
            const unsigned i=static_cast<unsigned char>(*letter)-32u;
            if (i<96) width+=resultAssets.advance[bank][i];
        }
        pen-=width/16.0f;
    }
    setTexture(GU_PSM_8888,256,256,resultAssets.letters);
    for (;*text;++text) {
        const unsigned i=static_cast<unsigned char>(*text)-32u;
        if (i>=96) continue;
        const Patch glyph{static_cast<int16_t>((i%16)*16),static_cast<int16_t>(bank*128+(i/16)*16),16,16,0,0};
        patch(glyph,ViewX+pen,y,1,color);
        pen+=resultAssets.advance[bank][i]/8.0f;
    }
}

void showResultStats(const Course& course,const Round& round,unsigned clock,unsigned animationMs,
                     bool paintBoard=true,float slide=0) {
    const Score result=scoreFor(course,round);
    const auto* thresholds=objects.data.courses[currentCourse].ranks;
    const unsigned peak=thresholds[4];
    const unsigned onTime=static_cast<unsigned>(walls.onTime*100);
    constexpr uint32_t green=GU_RGBA(161u,255u,116u,255u), red=GU_RGBA(255u,154u,123u,255u);
    char line[160];
    if (paintBoard) {
        setTexture(GU_PSM_5650,256,256,resultAssets.board);
        patch({0,0,228,163,0,0},ViewX+162,83);
    }
    if (animationMs>=2010) {
        if (round.game.strokes<=1) {
            resultLabel(round.game.strokes?lang("HOLE IN ONE!","ДЫРКА В ОДНОМ!"):lang("HOLE IN NONE!","ДЫРКА В НУЛИ?"),170,89);
        } else {
            std::snprintf(line,sizeof line,lang("Strokes: %u","Удары: %u"),round.game.strokes);
            resultLabel(line,170,89);
        }
        const int difference=static_cast<int>(round.game.strokes)-static_cast<int>(course.par);
        const bool above=difference>0;
        const char* parText=nullptr;
        if (isHorseCourse039(currentCourse)) {
            const char* format=above?lang("%u Over Par...","%u Выше пар..."):
                                     lang("%u Under Par!","%u Ниже пар!");
            if (difference) {
                std::snprintf(line,sizeof line,format,static_cast<unsigned>(std::abs(difference)));
                parText=line;
            } else parText=lang("Even With Par!","Равен с Паром!");
        } else if (difference>=-3 && difference<=3) {
            const char* parNames[]={
                lang("Albatross!","Альбатросс!"),lang("Eagle!","Игл!"),lang("Birdie!","Бёрди!"),
                lang("Even with Par!","Равен с Паром!"),
                lang("Bogey.","Богги."),lang("Double Bogey.","Двойной Богги."),lang("Triple Bogey.","Тройной Богги.")
            };
            parText=parNames[difference+3];
        } else {
            const char* format=above?lang("%u Above Par...","%u Выше пар..."):lang("%u Below Par!","%u Ниже пар!");
            std::snprintf(line,sizeof line,format,static_cast<unsigned>(std::abs(difference)));
            parText=line;
        }
        resultLabel(parText,170,104,true,above?red:green);
        std::snprintf(line,sizeof line,lang("Bonus: %u","Бонус: %u"),result.strokeBonus);
        resultLabel(line,170,118,true,above?red:green);
    }
    if (animationMs>=2355) {
        const char* timeFormat=isHorseCourse039(currentCourse)?lang("Time: %02u:%02u","Время: %02u:%02u"):
                                                              lang("Time: %02u:%02u.%02u","Время: %02u:%02u.%02u");
        std::snprintf(line,sizeof line,timeFormat,clock/6000,clock/100%60,clock%100);
        resultLabel(line,170,137);
        uint32_t timeColor=green;
        const char* timeText=lang("On Time!","Во Время!");
        if (clock>=onTime) {
            const unsigned late=(clock-onTime)/100;
            timeColor=late || isHorseCourse039(currentCourse)?red:Gold;
            if (!late) timeText=lang("Just Barely Late...","Капельку поздно..");
            else {
                std::snprintf(line,sizeof line,lang("%u Seconds Late...","%u Секунд поздно..."),late);
                timeText=line;
            }
        }
        resultLabel(timeText,170,151,true,timeColor);
        std::snprintf(line,sizeof line,lang("Bonus: %u","Бонус: %u"),result.timeBonus);
        resultLabel(line,170,165,true,timeColor);
    }
    if (animationMs>=2705) {
        std::snprintf(line,sizeof line,lang("Level Bonus: %u","Бонус уровня: %u"),round.bonus());
        resultLabel(line,170,184);
    }

    unsigned displayed=0;
    if (animationMs>=3890) displayed=result.total;
    else if (animationMs>=3050) {
        const float t=(animationMs-3050)/840.0f;
        displayed=static_cast<unsigned>(result.total*(1.0f-std::exp2(-8.0f*t))/(1.0f-1.0f/256));
    }
    setTexture(GU_PSM_8888,256,64,resultAssets.bar);
    patch({0,0,204,9,0,0},ViewX+170,212+slide);
    const int16_t width=static_cast<int16_t>(204*std::min(displayed,peak)/peak);
    if (width) patch({0,16,width,9,0,0},ViewX+170,212+slide);
    for (unsigned i=0;i<5;++i) {
        const int x=170+static_cast<int>(204*thresholds[i]/peak);
        const auto tint=displayed>=thresholds[i]?White:static_cast<uint32_t>(GU_RGBA(90u,90u,90u,255u));
        patch({0,32,17,17,0,0},ViewX+x-8,207+slide,1,tint);
    }
    std::snprintf(line,sizeof line,lang("FINAL SCORE: %u","Счёт: %u"),displayed);
    resultLabel(line,272,222+static_cast<int>(std::lround(slide)),false,White,true);
    if (animationMs>=4400) {
        if (russian()) {
            setTexture(GU_PSM_8888,256,64,resultAssets.bar);
            patch({32,32,54,28,0,0},ViewX+300,81,0.75f);
        }
        RankPlayer::Frame frame{};
        if (const auto* pixels=ranks.frame(result.rank,animationMs-4400,frame)) {
            setTexture(GU_PSM_8888,128,128,pixels);
            patch({0,0,frame.width,frame.height,0,0},ViewX+323-frame.width/2,145-frame.height/2);
        }
        if (result.rank<5) {
            std::snprintf(line,sizeof line,lang("%u to Next Rank","%u до след. ранга"),thresholds[result.rank]-result.total);
            resultLabel(line,272,236,true,White,true);
        }
    }
}

void drawTerry(const GameplayView& view,const Round& round) {
    if (currentCourse!=1) return;
    const auto& data=objects.data;
    Vec2 position=data.terryPosition;
    if (round.terry.running) {
        const unsigned age=round.totalTicks-round.terry.keyTick;
        if (age>=60) return;
        position.x+=(352-position.x)*age/60;
    }
    const Vec2 center=view.position(position);
    const float scale=GameplayView::RasterScale*(data.terryScale+0.02f*round.terry.clicks);
    const unsigned frame=round.terry.clicks;
    const int16_t width=static_cast<int16_t>(std::round(89*scale));
    const int16_t height=static_cast<int16_t>(std::round(129*scale));
    const Patch sprite{static_cast<int16_t>(32*(frame%4)),static_cast<int16_t>(32*(frame/4)),width,height,0,0};
    setTexture(GU_PSM_8888,128,256,objects.terry);
    patch(sprite,center.x-width/2*view.worldZoom(),center.y-height/2*view.worldZoom(),
          view.worldZoom(),White,round.terry.running);
}

void drawBubbles(const GameplayView& view,unsigned ticks) {
    if (currentCourse<TutorialCount) return;
    const auto& stage=courseHazards(currentCourse);
    setTexture(GU_PSM_8888,128,32,bubbleTexture);
    for (unsigned i=0;i<stage.bubbleCount;++i) {
        const auto& bubble=stage.bubbles[i];
        Vec2 position=bubble.position;
        position.y+=bubble.scale*golf.bubblePoses[bubble.phase][ticks%BubbleTicks];
        const Vec2 center=view.position(position);
        const int16_t size=static_cast<int16_t>(std::round(200*GameplayView::RasterScale*bubble.scale));
        const float halfSize=size*view.worldZoom()/2.0f;
        patch({static_cast<int16_t>(32*bubble.sprite),0,size,size,0,0},
              center.x-halfSize,center.y-halfSize,view.worldZoom());
    }
}

void drawScorePopup(const GameplayView& view,Vec2 world,unsigned age,unsigned value) {
    const Vec2 center=view.position(world);
    const float zoom=view.worldZoom();
    char number[8];
    std::snprintf(number,sizeof number,"%u",value);
    const auto ink=GU_RGBA(255u,255u,255u,255-255*age/50);
    if (zoom<1) localeText(number,center.x-ViewX,center.y-(14+age/2)*zoom,1,ink,true);
    else resultText(number,center.x-ViewX-9,center.y-(14+age/2),ink);
}

template<class Atlas>
void setFrontierTexture(const Atlas& atlas);

uint32_t cosmicCoinTint049(const Round& round,unsigned index);

void drawCoins(const GameplayView& view,const Round& round) {
    const auto& course=objects.data.courses[currentCourse];
    const bool shared=course.firstCoin==SharedCoin;
    if (shared) setTexture(GU_PSM_8888,16,16,objects.sharedCoin);
    else setTexture(GU_PSM_5650,CoinAtlasWidth,CoinAtlasHeight,objects.coin);
    for (unsigned i=0;i<course.count;++i) {
        if (round.coins.has(i)) continue;
        if (shared) {
            sharedActor(view,{1,1,13,13,-6,-6},course.coins[i],
                        currentCourse==CosmicNinthCourse?cosmicCoinTint049(round,i):
                        usesMetronome048(currentCourse)?cosmicShade048(currentCourse):
                        currentCourse==Elysian510Course?elysianHazardTint040(currentCourse):White);
            continue;
        }
        const Vec2 center=view.position(course.coins[i]);
        const unsigned sprite=course.firstCoin+i;
        const Patch tile{static_cast<int16_t>((sprite%8)*32),static_cast<int16_t>((sprite/8)*32),24,24,0,0};
        patch(tile,center.x-12,center.y-12);
    }
    if (currentCourse==FrontierMazeCourse && !round.coins.has(course.count)) {
        setFrontierTexture(frontierBigCoin);
        sharedActor(view,frontierBigCoin.patch(0),FrontierBigCoin406);
    } else if (currentCourse==FrontierMazeCourse && round.ticks-round.coins.ticks[course.count]<50) {
        drawScorePopup(view,FrontierBigCoin406,round.ticks-round.coins.ticks[course.count],round.coins.values[course.count]);
    }
    for (unsigned i=0;i<course.count;++i) {
        if (!round.coins.has(i)) continue;
        const unsigned age=round.ticks-round.coins.ticks[i];
        if (age>=50) continue;
        drawScorePopup(view,course.coins[i],age,round.coins.values[i]);
    }
}

void drawDestruction(const GameplayView& view,Vec2 world,unsigned age,unsigned value) {
    if (age>=50) return;
    if (age<40) {
        setTexture(GU_PSM_8888,512,256,splashTexture);
        sharedActor(view,{static_cast<int16_t>(36*(age/4)),144,32,20,-16,-14},world);
    }
    if (value) drawScorePopup(view,world,age,value);
}

void drawCarouselCar(const GameplayView& view,const BaddieRun& run,unsigned index) {
    constexpr Patch sprite{96,0,17,13,-8,-6};
    constexpr uint32_t tints[]={
        GU_RGBA(219,237,201,255u),GU_RGBA(166,255,255,255u),GU_RGBA(255,166,255,255u),
        GU_RGBA(166,166,255,255u),GU_RGBA(255,166,166,255u),GU_RGBA(255,255,166,255u),
        GU_RGBA(255,210,255,255u)
    };
    const uint32_t color=tints[(run.bornAt[index]+3*index+run.owners[index])%std::size(tints)];
    const Vec2 center=view.position(run.center(index));
    const auto velocity=run.velocities[index];
    const Vec2 right=velocity*(view.worldZoom()/length(velocity));
    const Vec2 down{-right.y,right.x};
    const Vec2 origin=center+right*sprite.x+down*sprite.y;
    const Vec2 horizontal=right*sprite.width, vertical=down*sprite.height;
    const Vec2 points[]={origin,origin+horizontal,origin+vertical,origin+horizontal+vertical};
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
    for (unsigned i=0;i<4;++i)
        vertices[i]={static_cast<float>(sprite.u+(i%2)*sprite.width),
                     static_cast<float>(sprite.v+(i/2)*sprite.height),color,points[i].x,points[i].y,0};
    sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                   4,nullptr,vertices);
}

void drawElysianCar511(const GameplayView& view,Vec2 position,bool east,uint32_t tint) {
    const Vec2 center=view.position(position);
    const float scale=view.worldScale();
    const float x=16*scale,y=12*scale;
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
    vertices[0]={east?0.0f:32.0f,east?0.0f:24.0f,tint,center.x-x,center.y-y,0};
    vertices[1]={east?32.0f:0.0f,east?0.0f:24.0f,tint,center.x+x,center.y-y,0};
    vertices[2]={east?0.0f:32.0f,east?24.0f:0.0f,tint,center.x-x,center.y+y,0};
    vertices[3]={east?32.0f:0.0f,east?24.0f:0.0f,tint,center.x+x,center.y+y,0};
    sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                   4,nullptr,vertices);
}

void drawBaddieRun(const GameplayView& view,const Round& round,const BaddieRun& run) {
    const unsigned drawTick=(isElysianHazardCourse040(currentCourse) || currentCourse==Elysian511Course ||
                             currentCourse==Elysian514Course)?round.horse.age:round.ticks;
    if (!run.stage) return;
    const uint32_t shade=currentCourse==UncannyValleyCourse?White:run.stage->shade;
    setTexture(GU_PSM_8888,BaddieArtWidth,BaddieArtHeight,baddiePixels);
    for (unsigned i=0;i<run.stage->spawnerCount;++i) {
        if (run.doorRemovedAt[i] || currentCourse==Elysian511Course || currentCourse==Elysian514Course) continue;
        const auto frame=run.doorFrame(i,drawTick);
        sharedActor(view,{static_cast<int16_t>(24*frame),0,17,17,-8,-8},
                    run.stage->spawners[i].center,currentCourse==AlleyCourse?White:shade);
    }
    if (run.stage->skull) setTexture(GU_PSM_8888,256,256,interactiveTexture);
    else if (currentCourse==Elysian511Course) setTexture(GU_PSM_8888,32,32,elysianCarPixels511);
    for (unsigned i=0;i<run.count;++i) {
        if (!run.alive(i)) continue;
        if (run.stage->skull) {
            const unsigned frame=drawTick*24/60%24;
            const Patch sprite{static_cast<int16_t>((frame%8)*32),static_cast<int16_t>((frame/8)*32),26,26,-13,-13};
            sharedActor(view,sprite,run.center(i),shade);
        } else if (currentCourse==CosmicSeventeenthCourse ||
                   (currentCourse==CosmicFifteenthCourse && run.owners[i]==3)) {
            const Vec2 center=view.position(run.center(i));
            if (center.y<-10 || center.y>Height+10) continue;
            const float zoom=view.worldZoom(),x=8*zoom,y=6*zoom;
            const float direction=run.velocities[i].y<0?1:-1;
            auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
            vertices[0]={96,0,shade,center.x-y*direction,center.y+x*direction,0};
            vertices[1]={113,0,shade,center.x-y*direction,center.y-x*direction,0};
            vertices[2]={96,13,shade,center.x+y*direction,center.y+x*direction,0};
            vertices[3]={113,13,shade,center.x+y*direction,center.y-x*direction,0};
            sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,vertices);
        } else if (currentCourse==CarouselCourse) {
            drawCarouselCar(view,run,i);
        } else if (currentCourse==Elysian511Course) {
            const bool east=run.velocities[i].x>0;
            drawElysianCar511(view,run.center(i),east,shade);
        } else {
            const Patch sprite=currentCourse==DimlitCourse ? Patch{144,0,13,17,-6,-8} :
                              Patch{static_cast<int16_t>((currentCourse==CityCourse ||
                                  currentCourse==AbandonedWorkspaceCourse || run.velocities[i].x>0)?96:120),0,17,13,-8,-6};
            sharedActor(view,sprite,run.center(i),shade);
        }
    }
    for (unsigned i=0;i<run.count;++i) {
        if (run.defeatedAt[i]) drawDestruction(view,run.center(i),drawTick-run.defeatedAt[i],0);
    }
    for (unsigned i=0;i<run.stage->spawnerCount;++i) {
        if (run.doorRemovedAt[i])
            drawDestruction(view,run.stage->spawners[i].center,drawTick-run.doorRemovedAt[i],BaddieRun::DoorBonus);
    }
}

void drawSkulls(const GameplayView& view,const Round& round) {
    const unsigned drawTick=isElysianHazardCourse040(currentCourse)?round.horse.age:round.ticks;
    if (currentCourse<TutorialCount) return;
    if (!round.skullCount) return;
    setTexture(GU_PSM_8888,256,256,interactiveTexture);
    const unsigned frame=drawTick*24/60%24;
    const Patch sprite{static_cast<int16_t>((frame%8)*32),static_cast<int16_t>((frame/8)*32),26,26,-13,-13};
    for (unsigned i=0;i<round.skullCount;++i) {
        if (round.skulls.bodies[i].radius>0)
            sharedActor(view,sprite,round.skulls.bodies[i].center,
                        currentCourse==CasinoCourse?GU_RGBA(191,191,191,255):elysianHazardTint040(currentCourse));
    }
    for (unsigned i=0;i<round.skullCount;++i) {
        if (round.skulls.defeatedAt[i]) {
            drawDestruction(view,round.skulls.bodies[i].center,
                            drawTick-round.skulls.defeatedAt[i],SkullRun::Bonus);
        }
    }
}

void drawGolfKey(const GameplayView& view,const Round& round) {
    if (currentCourse<TutorialCount) return;
    const auto& stage=courseHazards(currentCourse);
    if (!stage.hasKey || progress.quest<2 || progress.golfKey) return;
    const unsigned frame=std::min(round.ticks/4,11u);
    setTexture(GU_PSM_8888,256,256,interactiveTexture);
    sharedActor(view,{static_cast<int16_t>((frame%8)*32),static_cast<int16_t>(96+(frame/8)*32),17,17,-8,-8},
                stage.keyHint,GU_RGBA(255,255,255,160));
}

bool bindFrontier409Night() {
    unsigned generation=0;
    auto* workspace=clips.textureWorkspace<ValleyArtWidth035*ValleyArtHeight035>(generation);
    if (!frontier409Night.useShared(workspace,generation)) return false;
    setFrontierTexture(frontier409Night);
    return true;
}

bool bindFrontierNight1213() {
    unsigned generation=0;
    auto* workspace=clips.textureWorkspace<FrontierNightWidth036*FrontierNightHeight036>(generation);
    if (!frontierNight1213.useShared(workspace,generation)) return false;
    setFrontierTexture(frontierNight1213);
    return true;
}

bool bindFrontierNight1415() {
    unsigned generation=0;
    auto* workspace=clips.textureWorkspace<FrontierNightWidth037*FrontierNightHeight037>(generation);
    if (!frontierNight1415.useShared(workspace,generation)) return false;
    setFrontierTexture(frontierNight1415);
    return true;
}

void drawCosmicDoor602(const GameplayView& view,const Round& round);
void drawCosmicDoor607(const GameplayView& view,const Round& round);
void drawCosmicDoor610(const GameplayView& view,const Round& round);
void drawLateCosmicDoors053(const GameplayView& view,const Round& round);
void drawLateCosmicBlocks053(const GameplayView& view,const Round& round);
void drawSantaDoors(const GameplayView& view,const Round& round);
void drawSantaBlocks(const GameplayView& view,const Round& round);

void drawKeyDoors(const GameplayView& view,const Round& round) {
    if (currentCourse==CosmicSixteenthCourse || currentCourse==CosmicSeventeenthCourse) {
        drawLateCosmicDoors053(view,round);
        return;
    }
    if (currentCourse==CosmicFifteenthCourse) {
        drawSantaDoors(view,round);
        return;
    }
    if (currentCourse==CosmicTenthCourse) {
        drawCosmicDoor610(view,round);
        return;
    }
    if (currentCourse==CosmicSeventhCourse) {
        drawCosmicDoor607(view,round);
        return;
    }
    if (currentCourse==CosmicSecondCourse) {
        drawCosmicDoor602(view,round);
        return;
    }
    if (!round.keyStage) return;
    const auto& stage=*round.keyStage;
    const auto& state=round.roundKeys;
    const bool factory=currentCourse==FactoryCourse, arcade=currentCourse==ArcadeCourse;
    const bool race=currentCourse==RacetrackCourse;
    const bool separate=factory || arcade || race;
    const auto artFor=[&](unsigned i) -> const Patch& {
        return race?raceDoors.patch(i):arcade?arcadeDoors.patch(i):factoryDoors.patch(i);
    };
    if (currentCourse==MovieOnceCourse || currentCourse==AbandonedWorkspaceCourse) {
        if (!bindFrontierNight1415()) return;
        const float scale=view.worldScale();
        for (unsigned i=0;i<stage.doorCount;++i) {
            const auto& door=stage.doors[i];
            const bool opened=(state.opened&(1u<<i))!=0;
            const unsigned art=currentCourse==AbandonedWorkspaceCourse?FrontierWorkspaceDoorFirstPatch037+i:
                               door.halfSize.y==32?FrontierCyanTallDoorPatch037:FrontierCyanShortDoorPatch037;
            if (!opened) actor(view,frontierNight1415.patch(art),door.center,scale);
            const unsigned alpha=static_cast<unsigned>(255*state.lockAlpha[i]);
            if (alpha) actor(view,frontierNight1415.patch(FrontierLockPatch037),door.center,scale,
                             GU_RGBA(255,255,255,alpha));
            if (opened) drawDestruction(view,door.center,round.ticks-state.openedAt[i],doorScore(currentCourse));
        }
        return;
    }
    if (currentCourse==WeatherAlertCourse) {
        if (!bindFrontierNight1213()) return;
        const Vec2 center=stage.doors[0].center;
        const float scale=view.worldScale();
        if (!(state.opened&1u)) actor(view,frontierNight1213.patch(FrontierDoorPatch036),center,scale);
        actor(view,frontierNight1213.patch(FrontierLockPatch036),center,scale,
              GU_RGBA(255,255,255,static_cast<unsigned>(255*state.lockAlpha[0])));
        if (state.opened&1u)
            drawDestruction(view,center,round.ticks-state.openedAt[0],doorScore(currentCourse));
        return;
    }
    if (currentCourse==UncannyValleyCourse) {
        if (!bindFrontier409Night()) return;
        const Vec2 center=stage.doors[0].center;
        if (!(state.opened&1u)) sharedActor(view,frontier409Night.patch(ValleyDoorPatch035),center);
        sharedActor(view,frontier409Night.patch(ValleyLockPatch035),center,
                    GU_RGBA(255,255,255,static_cast<unsigned>(255*state.lockAlpha[0])));
        if (state.opened&1u)
            drawDestruction(view,center,round.ticks-state.openedAt[0],doorScore(currentCourse));
        return;
    }
    if (race) setTexture(GU_PSM_8888,128,64,raceDoors.texture());
    else if (arcade) setTexture(GU_PSM_8888,64,32,arcadeDoors.texture());
    else if (factory) setTexture(GU_PSM_8888,128,64,factoryDoors.texture());
    else setTexture(GU_PSM_8888,KeyDoorAtlasWidth,KeyDoorAtlasHeight,keyDoorSprites);
    for (unsigned i=0;i<stage.doorCount;++i) {
        if (state.opened&(1u<<i)) continue;
        const auto& art=separate?artFor(i):stage.doorArt[i];
        patch(art,ViewX+(separate?136:0)+art.x,art.y);
    }
    if (!separate) setTexture(GU_PSM_8888,256,256,roundKeySprites);
    for (unsigned i=0;i<stage.doorCount;++i) {
        if ((state.opened&(1u<<i)) && round.ticks-state.openedAt[i]>=50) continue;
        const auto alpha=static_cast<unsigned>(255*state.lockAlpha[i]);
        const uint32_t tint=GU_RGBA(255u,255u,255u,alpha);
        if (separate) {
            const auto& art=artFor(i+stage.doorCount);
            patch(art,ViewX+136+art.x,art.y,1.0f,tint);
        } else {
            sharedActor(view,{0,216,9,9,-4,-4},stage.doors[i].center,tint);
        }
    }
    for (unsigned i=0;i<stage.doorCount;++i) {
        if (!(state.opened&(1u<<i))) continue;
        drawDestruction(view,stage.doors[i].center,round.ticks-state.openedAt[i],doorScore(currentCourse));
    }
}

void drawRoundKeys(const GameplayView& view,const Round& round) {
    if (!round.keyStage) return;
    const auto& stage=*round.keyStage;
    const auto& state=round.roundKeys;
    const unsigned frame=round.ticks/3%KeyFrames;
    const Patch sprite{static_cast<int16_t>(frame%10*24),static_cast<int16_t>(frame/10*24),17,17,-8,-8};
    setTexture(GU_PSM_8888,256,256,roundKeySprites);
    for (unsigned i=0;i<stage.keyCount;++i) {
        const uint32_t bit=1u<<i;
        if (currentCourse==CosmicFifteenthCourse && i==1) continue;
        if ((state.collected&bit) && !(state.available&bit)) continue;
        const auto& key=state.motion[i];
        const unsigned alpha=(state.available&bit)?255-std::min(10u,round.ticks-key.pickedAt)*13:255;
        const uint32_t base=currentCourse==CosmicSeventhCourse?0x00b200b2u:stage.keys[i].color;
        const uint32_t color=(base&0x00ffffffu)|(alpha<<24);
        sharedActor(view,sprite,key.position,color);
    }
}

void drawJumpPads(const GameplayView& view,const Round& round) {
    if (!round.jumpStage) return;
    const uint32_t shade=currentCourse==CosmicTwelfthCourse?CosmicShade611:
                         currentCourse==CityCourse?GU_RGBA(255,140,140,255):
                         currentCourse==BankCourse?GU_RGBA(255,184,199,255):
                         (currentCourse==AlleyCourse || currentCourse==CasinoCourse)?GU_RGBA(191,191,191,255):
                         currentCourse==CosmicEleventhCourse?CosmicShade611:
                         currentCourse==GlowstickFinaleCourse?GU_RGBA(166,166,166,255):elysianHazardTint040(currentCourse);
    constexpr float halfPi=1.570796327f;
    setTexture(GU_PSM_8888,JumpArtWidth,JumpArtHeight,jumpPixels);
    const float scale=view.worldScale();
    for (unsigned i=0;i<round.jumpStage.count;++i) {
        const auto& pad=round.jumpStage.pads[i];
        const Vec2 padCenter=currentCourse==TowerCourse?towerPadCenter(i,round.ticks,pad.center):pad.center;
        actor(view,{0,0,32,32,-16,-16},padCenter,scale,shade);
        const Vec2 center=view.position(padCenter);
        const float angle=pad.radians+halfPi;
        const Vec2 right{std::cos(angle)*12*scale,std::sin(angle)*12*scale};
        const Vec2 down{-right.y,right.x};
        const Vec2 topLeft=center-right-down, topRight=center+right-down;
        const Vec2 bottomLeft=center-right+down, bottomRight=center+right+down;
        const float u=pad.speedGranted>round.scene.stopSpeed?64.0f:32.0f;
        auto* v=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
        const auto arrowTint=(currentCourse==Elysian511Course || currentCourse==Elysian515Course)?shade:White;
        v[0]={u,0,arrowTint,topLeft.x,topLeft.y,0};
        v[1]={u+24,0,arrowTint,topRight.x,topRight.y,0};
        v[2]={u,24,arrowTint,bottomLeft.x,bottomLeft.y,0};
        v[3]={u+24,24,arrowTint,bottomRight.x,bottomRight.y,0};
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                       4,nullptr,v);
    }
}

void drawStageLights(const GameplayView& view,const Round& round) {
    const bool traffic=currentCourse==HighwayCourse || currentCourse==CityCourse;
    const bool lightbox=currentCourse==LightboxCourse;
    if (!traffic && !lightbox && !round.police.triggeredAt) return;
    constexpr Patch light{0,0,128,128,-64,-64};
    const float scale=view.worldScale();
    if (lightbox) {
        setTexture(GU_PSM_5650,SpotlightSize,SpotlightSize,lightboxLight);
        sceGuBlendFunc(GU_ADD,GU_DST_COLOR,GU_FIX,0,0xffffffff);
        actor(view,light,round.game.position,2.0f*scale);
        sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
        return;
    }
    setTexture(GU_PSM_8888,SpotlightSize,SpotlightSize,spotlightPixels);
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_FIX,0,0xffffffff);
    if (traffic) {
        actor(view,light,round.game.position,2.0f*scale);
        if (!chillMode && !round.game.won) {
            constexpr uint32_t red=GU_RGBA(255,0,0,255);
            actor(view,light,round.ghost.position,1.5f*scale,red);
            for (unsigned i=0;i<round.extraGhostCount;++i)
                actor(view,light,round.extraGhosts[i].position,1.5f*scale,red);
        }
    } else {
        actor(view,light,round.game.position,4.0f*scale,GU_RGBA(210,255,255,255));
    }
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
}

void drawSecretRunner(const GameplayView& view,const Round& round) {
    if (!round.secret.available) return;
    setTexture(GU_PSM_8888,256,256,interactiveTexture);
    sharedActor(view,{96,128,17,17,-8,-8},SecretRunner::Hint,GU_RGBA(255,255,255,160));
    if (!round.secret.startedAt) return;
    setTexture(GU_PSM_8888,RunnerAtlasWidth,RunnerAtlasHeight,runnerPixels);
    const unsigned frame=(round.ticks/3)%12;
    const Patch pose{static_cast<int16_t>((frame%6)*20),static_cast<int16_t>((frame/6)*24),17,22,-8,-11};
    actor(view,pose,round.secret.position(round.ticks));
}

void drawNews(const Round& round) {
    if (currentCourse<HighwayCourse || currentCourse>AlleyCourse) return;
    setTexture(GU_PSM_5650,NewsArt::Width,NewsArt::BandHeight,newsArt.bandPixels);
    hudPatch(newsArt.band);
    const unsigned line=round.news.headline;
    const unsigned width=newsArt.width(russian(),line);
    const float start=round.news.x(round.ticks);
    for (unsigned left=0;left<width;left+=NewsArt::Width) {
        const float x=start+left;
        const unsigned pixels=std::min(NewsArt::Width,width-left);
        if (x>=NewsArt::ClipRight || x+pixels<=NewsArt::ClipLeft) continue;
        if (const auto* page=newsArt.page(russian(),line,left/NewsArt::Width)) {
            setTexture(GU_PSM_8888,NewsArt::Width,NewsArt::LineHeight,page);
            patch({0,0,static_cast<int16_t>(pixels),NewsArt::TextHeight,0,0},ViewX+x,NewsArt::TextY);
        }
    }
}

void drawFrontierBlocks416(const GameplayView& view,const Round& round);
void drawElysianBlocks509(const GameplayView& view,const Round& round);

void drawCosmicBlocks602(const GameplayView& view,const Round& round);
void drawStarstormBlocks047(const GameplayView& view,const Round& round);

void drawBlocks(const GameplayView& view,const Round& round) {
    if (currentCourse==CosmicSixteenthCourse || currentCourse==CosmicSeventeenthCourse) {
        drawLateCosmicBlocks053(view,round);
        return;
    }
    if (currentCourse==CosmicFifteenthCourse) {
        drawSantaBlocks(view,round);
        return;
    }
    if (currentCourse==CosmicFifthCourse) {
        drawStarstormBlocks047(view,round);
        return;
    }
    if (currentCourse==CosmicSecondCourse) {
        drawCosmicBlocks602(view,round);
        return;
    }
    if (currentCourse==Elysian509Course || currentCourse==Elysian510Course) {
        drawElysianBlocks509(view,round);
        return;
    }
    if (currentCourse==CrimsonOutpostCourse) {
        drawFrontierBlocks416(view,round);
        return;
    }
    if (currentCourse==DownfallCourse) {
        if (!bindFrontierNight1213()) return;
        const auto& stage=courseHazards(currentCourse);
        for (unsigned i=0;i<stage.blockCount;++i) {
            if (round.brokenBlocks.has(i)) continue;
            const auto& block=stage.blocks[i];
            const unsigned sizeIndex=static_cast<unsigned>((block.halfSize.x-32)/16);
            actor(view,frontierNight1213.patch(FrontierBlock64Patch036+sizeIndex),block.center,view.worldScale());
        }
        for (unsigned i=0;i<stage.blockCount;++i) {
            if (round.brokenBlocks.has(i))
                drawDestruction(view,stage.blocks[i].center,round.ticks-round.blockTicks[i],round.blockValues[i]);
        }
        return;
    }
    if (currentCourse==FrontierMineCourse) {
        setFrontierTexture(frontierMineBlock);
        for (unsigned i=0;i<std::size(FrontierBlocks411);++i)
            if (!round.brokenBlocks.has(i))
                sharedActor(view,frontierMineBlock.patch(0),FrontierBlocks411[i].center);
        for (unsigned i=0;i<std::size(FrontierBlocks411);++i)
            if (round.brokenBlocks.has(i))
                drawDestruction(view,FrontierBlocks411[i].center,round.ticks-round.blockTicks[i],round.blockValues[i]);
        return;
    }
    if (currentCourse<TutorialCount) return;
    if (currentCourse==DenseCourse) {
        setTexture(GU_PSM_8888,256,128,denseBlockArt.texture());
        for (unsigned i=0;i<denseBlocks.size();++i) {
            if (round.brokenBlocks.has(i)) continue;
            const auto& art=denseBlockArt.patch(i);
            patch(art,ViewX+GameplayView::HudWidth+art.x,art.y);
        }
        for (unsigned i=0;i<denseBlocks.size();++i) {
            if (!round.brokenBlocks.has(i)) continue;
            drawDestruction(view,denseBlocks.data()[i].center,round.ticks-round.blockTicks[i],round.blockValues[i]);
        }
        return;
    }
    const auto& stage=courseHazards(currentCourse);
    if (!stage.blockCount) return;
    const bool park=currentCourse>=ChuckleParkFirstCourse;
    if (park && parkBlockArt.texture()) setTexture(GU_PSM_8888,128,64,parkBlockArt.texture());
    else setTexture(GU_PSM_5650,BlockAtlasWidth,BlockAtlasHeight,blockTexture);
    for (unsigned i=0;i<stage.blockCount;++i) {
        if (round.brokenBlocks.has(i)) continue;
        if (park && i<parkBlockArt.size()) {
            const auto& art=parkBlockArt.patch(i);
            patch(art,ViewX+GameplayView::HudWidth+art.x,art.y);
            continue;
        }
        const auto& block=stage.blocks[i];
        const Vec2 corner=view.position(block.center-block.halfSize);
        const bool rushHour=currentCourse==TutorialCount+RushHourGolfStage;
        const bool bank=currentCourse==BankCourse;
        const bool lightbox=currentCourse==LightboxCourse;
        if (rushHour || currentCourse==DimlitCourse || bank || lightbox) {
            const Vec2 end=view.position(block.center+block.halfSize);
            const auto atlasX=static_cast<int16_t>(bank?BankBlockAtlasX:
                              i*(rushHour?RushHourBlockAtlasStride:
                                 lightbox?LightboxBlockAtlasStride:DimlitBlockAtlasStride));
            const auto atlasY=static_cast<int16_t>(bank?BankBlockAtlasY:
                              rushHour?RushHourBlockAtlasY:lightbox?LightboxBlockAtlasY:DimlitBlockAtlasY);
            patch({atlasX,atlasY,
                   static_cast<int16_t>(end.x-corner.x),static_cast<int16_t>(end.y-corner.y),0,0},
                  corner.x,corner.y);
        } else {
            const unsigned sprite=stage.firstBlock+i;
            patch({static_cast<int16_t>((sprite%8)*24),static_cast<int16_t>((sprite/8)*24),17,17,0,0},
                  corner.x,corner.y);
        }
    }
    for (unsigned i=0;i<stage.blockCount;++i) {
        if (!round.brokenBlocks.has(i)) continue;
        drawDestruction(view,stage.blocks[i].center,round.ticks-round.blockTicks[i],round.blockValues[i]);
    }
}

void drawSplash(const GameplayView& view,const Round& round) {
    const unsigned frame=(round.totalTicks-round.splashTick)/6;
    if (!frame || frame>8) return;
    const unsigned sprite=frame-1;
    const Vec2 center=view.position(round.game.position);
    const float zoom=view.worldZoom();
    setTexture(GU_PSM_8888,512,256,splashTexture);
    patch({static_cast<int16_t>((sprite%4)*72),static_cast<int16_t>((sprite/4)*72),68,67,0,0},
          center.x-34*zoom,center.y-33*zoom-48*view.worldScale(),zoom);
}

void drawGoalFlag(const Round& round,unsigned signal,unsigned phase,unsigned weight) {
    if (currentCourse==CataclysmCourse) {
        if (round.finale417.revealedAt) return;
        phase=0;
    } else if (currentCourse>=FinalFrontierFirstCourse) phase=round.switches.state;
    const auto draw=[&](unsigned fieldPhase,unsigned alpha) {
        Patch area{};
        if (const auto* pixels=goalFlags.frame(flagSlot(currentCourse),signal,fieldPhase,round.ticks,area)) {
            // These flags overlap independently drawn layers.
            const int format=(currentCourse==DenseCourse || (currentCourse>=PursuitCourse && currentCourse<=DogCourse) || multiDogStage(currentCourse))?GU_PSM_5551:GU_PSM_5650;
            setTexture(format,GoalFlagArt::Size,GoalFlagArt::Size,pixels);
            patch(area,ViewX+area.x,area.y,1,GU_RGBA(255u,255u,255u,alpha));
        }
    };
    draw(phase,255);
    if (weight) draw(phase+1,weight);
}

void drawGlowstick(const Round& round) {
    const bool dimlit=currentCourse==DimlitCourse, jumpCourse=currentCourse==JumpCourse;
    const bool police=round.police.triggeredAt!=0, staticBank=currentCourse==BankCourse && !police;
    const bool highway=currentCourse==HighwayCourse, city=currentCourse==CityCourse;
    const unsigned step=police?GlowstickArt::PoliceTickStep:city?GlowstickArt::CityTickStep:
                        dimlit?GlowstickArt::DimlitTickStep:GlowstickArt::TickStep;
    const unsigned startPhase=(police || dimlit || jumpCourse || highway || city)?0:currentCourse==CasinoCourse?3:6;
    const unsigned fieldTick=police?round.ticks-round.police.triggeredAt:round.ticks;
    const bool staticField=staticBank || currentCourse==GlowstickFinaleCourse;
    const unsigned phase=staticField?0:(fieldTick/step+startPhase)%GlowstickArt::Frames;
    const unsigned weight=staticField?0:255*(fieldTick%step)/step;
    const unsigned level=currentCourse-GlowstickFirstCourse;
    const bool stoplight=currentCourse==TrafficCourse || dimlit || jumpCourse || highway || city ||
                         currentCourse==LightboxCourse;
    const unsigned signal=police?1:stoplight?(round.ticks/31)%3:0;
    const auto* base=glowstickArt.frame(level,phase,signal);
    const auto* next=weight?glowstickArt.frame(level,phase+1,signal):nullptr;
    const Patch field{0,0,272,272,0,0};
    if (base) {
        setTexture(GU_PSM_5650,512,512,base);
        patch(field,ViewX+136,0);
        if (next) {
            setTexture(GU_PSM_5650,512,512,next);
            patch(field,ViewX+136,0,1,GU_RGBA(255u,255u,255u,weight));
        }
    }
    drawGoalFlag(round,signal,phase,next?weight:0);
    if (currentCourse==BillboardCourse || jumpCourse || city || currentCourse==AlleyCourse ||
        currentCourse==TowerCourse) {
        const unsigned first=currentCourse==TowerCourse?7:currentCourse==AlleyCourse?5:city?3:jumpCourse?2:0;
        const unsigned count=jumpCourse?1:2;
        for (unsigned index=first;index<first+count;++index) {
            Patch board{};
            if (const auto* pixels=billboards.frame(index,jumpCourse?0u:round.ticks,board)) {
                setTexture(GU_PSM_8888,BillboardArt::Size,BillboardArt::Size,pixels);
                hudPatch(board);
            }
        }
    }
    if (!level) {
        const unsigned locale=russian()?1:0;
        setTexture(GU_PSM_8888,GlowstickArt::SignWidth,GlowstickArt::SignHeight,glowstickArt.signPixels[locale]);
        hudPatch(glowstickArt.signs[locale][(round.ticks/30)%2]);
    }
}

void drawJumbleHouse(const Round& round) {
    const unsigned state=round.switches.state;
    if (const auto* pixels=jumbleVideo.frame(static_cast<uint64_t>(round.ticks)*1000000/60)) {
        setTexture(GU_PSM_5650,ParkVideo::Side,ParkVideo::Side,pixels);
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
        const uint32_t tint=ParkVideo::tint(state);
        vertices[0]={0,0,tint,ViewX+136,0,0};
        vertices[1]={256,256,tint,ViewX+408,ParkVideo::NativeHeight,0};
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,vertices);
    }
    if (jumbleFlag.texture()) {
        const auto& hole=jumbleFlag.patch(4);
        setTexture(GU_PSM_8888,128,64,jumbleFlag.texture());
        patch(hole,ViewX+136+hole.x,hole.y);
    }
    if (const auto* pixels=jumbleTiles.pixels(state)) {
        setTexture(GU_PSM_8888,ParkSwitchTiles::Side,ParkSwitchTiles::Side,pixels);
        patch({0,0,272,272,0,0},ViewX+136,0);
    }
    if (jumbleFlag.texture()) {
        const auto& flag=jumbleFlag.patch((round.ticks/4)%4);
        setTexture(GU_PSM_8888,128,64,jumbleFlag.texture());
        patch(flag,ViewX+136+flag.x,flag.y);
    }
}

void drawPursuitDecor(unsigned tick) {
    setTexture(GU_PSM_8888,256,128,pursuitDecor.texture());
    constexpr unsigned cameraDurations[]{80,1,1,1,1,1,12,1,1,1,1,1,1};
    unsigned cameraTime=tick/5%103, cameraFrame=0;
    while (cameraTime>=cameraDurations[cameraFrame]) cameraTime-=cameraDurations[cameraFrame++];
    for (unsigned index:{tick/3%18,18+tick/4%7,25+cameraFrame}) {
        const auto& art=pursuitDecor.patch(index);
        patch(art,ViewX+GameplayView::HudWidth+art.x,art.y);
    }
}

void drawBalloon(const GameplayView& view,const ParkBalloon& balloon,Vec2 motion,Vec2 previous,
                 const Patch& sprite,float rasterScale,uint32_t tint) {
    const Vec2 anchor=view.position(balloon.anchor);
    const Vec2 tip=view.position(balloon.anchor+Vec2{motion.x,motion.y-balloon.length});
    const Vec2 line=tip-anchor;
    const float size=length(line);
    if (size>0) {
        const Vec2 normal=Vec2{-line.y,line.x}*(balloon.width*view.worldScale()/(2*size));
        const Vec2 points[]={anchor-normal,anchor+normal,tip-normal,tip+normal};
        sceGuDisable(GU_TEXTURE_2D);
        auto* vertices=static_cast<ColorVertex*>(sceGuGetMemory(4*sizeof(ColorVertex)));
        for (unsigned i=0;i<4;++i) vertices[i]={balloon.stringColor,points[i].x,points[i].y,0};
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,vertices);
        sceGuEnable(GU_TEXTURE_2D);
    }
    const float angle=previous.x-motion.x;
    const Vec2 right{std::cos(angle)*rasterScale,std::sin(angle)*rasterScale};
    const Vec2 down{-right.y,right.x};
    const Vec2 top=tip+right*sprite.x+down*sprite.y;
    const Vec2 across=right*sprite.width, vertical=down*sprite.height;
    const Vec2 points[]={top,top+across,top+vertical,top+across+vertical};
    auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
    for (unsigned i=0;i<4;++i) {
        vertices[i]={static_cast<float>(sprite.u+(i%2)*sprite.width),
                     static_cast<float>(sprite.v+(i/2)*sprite.height),tint,points[i].x,points[i].y,0};
    }
    sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,
                   4,nullptr,vertices);
}

void drawPursuitBalloons(const GameplayView& view,unsigned tick) {
    if (!pursuitMotion) return;
    constexpr ParkBalloon balloons[]{
    {{80.0f,-176.0f},48.0f,2.0f,White,White},
    {{-352.0f,176.0f},48.0f,2.0f,White,White},
    {{344.0f,160.0f},48.0f,2.0f,White,White},
    {{224.0f,208.0f},48.0f,2.0f,White,White},
    {{192.0f,-80.0f},48.0f,2.0f,White,White},
    {{-128.0f,-208.0f},48.0f,2.0f,White,White},
    {{-312.0f,-144.0f},48.0f,2.0f,White,White},
    };
    static_assert(std::size(balloons)==PursuitBalloonCount);
    setTexture(GU_PSM_8888,64,64,pursuitBalloons.texture());
    constexpr unsigned overlap=60,period=PursuitMotionFrames-overlap;
    const auto motion=[](const PursuitBalloonPoint* samples,unsigned at) {
        const unsigned phase=at%period;
        const Vec2 position=samples[phase].position();
        if (at<period || phase>=overlap) return position;
        const float fraction=static_cast<float>(phase)/overlap;
        const float blend=fraction*fraction*(3-2*fraction);
        return samples[period+phase].position()*(1-blend)+position*blend;
    };
    for (unsigned i=0;i<PursuitBalloonCount;++i) {
        const auto* samples=pursuitMotion.get()+i*PursuitMotionFrames;
        drawBalloon(view,balloons[i],motion(samples,tick),motion(samples,tick?tick-1:0),
                    pursuitBalloons.patch(i),1.0f,balloons[i].color);
    }
}

#include "frontier_draw0607.hpp"
#include "frontier_night_draw036.hpp"
#include "frontier_night_draw037.hpp"
#include "frontier1617_draw.hpp"
#include "frontier18501_draw.hpp"
#include "elysian_horse039_draw.hpp"
#include "elysian_hazards040_draw.hpp"
#include "frontier_lighting.hpp"

void drawDog(const GameplayView& view,const Round& round,const DogGolfScene& scene) {
    if (currentCourse==FrontierDogCourse || currentCourse==FrontierPatrolCourse) {
        drawProductionDog(view,round,scene);
        return;
    }
    const bool legacy=currentCourse==DogCourse;
    if (!round.dog.winner) {
        if (legacy) setTexture(GU_PSM_8888,32,64,dogDoorArt.texture());
        else setTexture(GU_PSM_8888,128,64,dogWideDoorArt.texture());
        const auto& gate=legacy?dogDoorArt.patch(0):dogWideDoorArt.patch(0);
        patch(gate,ViewX+GameplayView::HudWidth+gate.x,gate.y);
    }
    setTexture(GU_PSM_8888,256,128,dogArt.texture());
    const auto& house=dogArt.patch((round.ticks+3)/6%11);
    if (legacy) patch(house,ViewX+GameplayView::HudWidth+house.x,house.y);
    else actor(view,house,scene.goal);
    actor(view,dogArt.patch(11),round.dog.body.center,round.dog.graphicScale/scene.graphicScale);
}

void drawMultiDogs(const GameplayView& view,const Round& round,const DogCourseStage& stage) {
    const auto& counts=DualGatePatches[currentCourse-DoubleDogCourse];
    const bool race=currentCourse==RacetrackCourse;
    setTexture(GU_PSM_8888,128,race?32:128,race?raceGateArt.texture():dualGateArt.texture());
    unsigned first=0;
    for (unsigned gate=0;gate<stage.goalCount;++gate) {
        if (!(round.dogs.openMask&(1u<<gate))) {
            for (unsigned i=0;i<counts[gate];++i) {
                const auto& art=race?raceGateArt.patch(first+i):dualGateArt.patch(first+i);
                patch(art,ViewX+GameplayView::HudWidth+art.x,art.y);
            }
        }
        first+=counts[gate];
    }
    setTexture(GU_PSM_8888,256,128,dogArt.texture());
    const auto& house=dogArt.patch((round.ticks+3)/6%11);
    for (unsigned i=0;i<stage.goalCount;++i) sharedActor(view,house,stage.goals[i].center);
    for (unsigned i=0;i<stage.dogCount;++i)
        actor(view,dogArt.patch(11),round.dogs.bodies[i].center,
              view.worldZoom()*round.dogs.graphicScales[i]/stage.dogs[i].graphicScale);
}

void drawLateParkDecor(const GameplayView& view,const Round& round) {
    if (currentCourse==DoubleDogCourse) {
        setTexture(GU_PSM_8888,128,128,dogCameraArt.texture());
        constexpr unsigned durations[]{80,1,1,1,1,1,12,1,1,1,1,1,1};
        unsigned time=round.ticks/5%103, frame=0;
        while (time>=durations[frame]) time-=durations[frame++];
        const auto& camera=dogCameraArt.patch(frame);
        patch(camera,ViewX+GameplayView::HudWidth+camera.x,camera.y);
    }
    setTexture(GU_PSM_8888,128,32,factorySpawner.texture());
    for (unsigned i=0;i<round.chuckles.spawners();++i) {
        auto art=factorySpawner.patch(round.chuckles.spawnerFrame(round.ticks,i));
        art.x=-art.width/2;
        art.y=-art.height/2;
        sharedActor(view,art,round.chuckles.spawner(i).center);
    }
}

void drawDogScreen(unsigned tick) {
    setTexture(GU_PSM_T4,ParkDogScreen::Width,ParkDogScreen::Height,dogScreen.pixels());
    sceGuClutMode(GU_PSM_8888,0,15,0);
    sceGuClutLoad(2,dogScreen.colors());
    const auto& screen=dogScreen.patch(tick/2%2);
    patch(screen,ViewX+GameplayView::HudWidth+screen.x,screen.y);
}

void drawParkCoaster(const GameplayView& view,const Round& round) {
    if (!coasterArt.pixels()) return;
    setTexture(GU_PSM_8888,CoasterArt::Width,CoasterArt::Height,coasterArt.pixels());
    const Vec2 slide=currentCourse==PursuitCourse?
        Vec2{-128.0f+(std::clamp(round.ticks,420u,540u)-420)*1136.0f/120.0f,160.0f}:
        Vec2{CoasterRun::offset(round.ticks),0}+CoasterRun::courseShift(currentCourse==DenseCourse);
    actor(view,CoasterArt::Train,CoasterRun::TrainOrigin+slide,view.worldScale());
    for (unsigned i=0;i<std::size(CoasterRun::RiderCenters);++i)
        actor(view,CoasterArt::rider(round.coaster.riders[i]),CoasterRun::RiderCenters[i]+slide,view.worldScale());
}

bool bindFrontier408Chuckles() {
    unsigned generation=0;
    auto* workspace=clips.textureWorkspace<Frontier408OverlayBytes+128*128>(generation)+Frontier408OverlayBytes;
    if (!frontier408Chuckles.useShared(workspace,generation)) return false;
    setFrontierTexture(frontier408Chuckles);
    return true;
}

void drawChuckles(const GameplayView& view,const Round& round) {
    if (currentCourse==Elysian513Course || currentCourse==Elysian514Course) {
        const auto& run=round.chuckles;
        const unsigned tick=round.horse.age;
        const auto tint=elysianHazardTint040(currentCourse);
        setTexture(GU_PSM_8888,128,32,elysianSpawnerArt513.texture());
        if (currentCourse==Elysian514Course) {
            const auto& skulls=round.baddies;
            for (unsigned i=0;i<skulls.stage->spawnerCount;++i)
                actor(view,elysianSpawnerArt513.patch(skulls.doorFrame(i,tick)),skulls.stage->spawners[i].center,
                      view.worldScale(),tint,true);
        }
        for (unsigned i=0;i<run.spawners();++i)
            actor(view,elysianSpawnerArt513.patch(run.spawnerFrame(tick,i)),run.spawner(i).center,
                  view.worldScale(),tint,true);
        unsigned generation=0;
        constexpr unsigned offset=HorseObjects039Width*HorseObjects039Height+
                                  FrontierClouds501Width*FrontierClouds501Height;
        auto* workspace=clips.textureWorkspace<offset+ChuckleArt::TextureBytes>(generation)+offset;
        if (chuckleArt.useShared(workspace,generation)) {
            setTexture(GU_PSM_8888,ChuckleArt::Width,ChuckleArt::Height,chuckleArt.pixels());
            for (unsigned i=0;i<run.count;++i)
                if (run.bodies[i].radius>0)
                    actor(view,ChuckleArt::frame(run.actorTick(i,tick)),run.bodies[i].center,
                          view.worldScale(),tint,true);
        }
        for (unsigned i=0;i<run.count;++i)
            if (run.defeatedAt[i]) drawDestruction(view,run.defeatedPosition[i],tick-run.defeatedAt[i],0);
        return;
    }
    if (currentCourse==PlentifulChucklesCourse) {
        if (!bindFrontier408Chuckles()) return;
        for (unsigned i=0;i<std::size(FrontierChuckleSpawners408);++i) {
            const unsigned frame=round.chuckles.spawnerFrame(round.ticks,i);
            sharedActor(view,frontier408Chuckles.patch(25+frame),FrontierChuckleSpawners408[i].center);
        }
        for (unsigned i=0;i<round.chuckles.count;++i) {
            const auto& body=round.chuckles.bodies[i];
            const auto frame=round.chuckles.actorTick(i,round.ticks)/2%25;
            if (body.radius>0) sharedActor(view,frontier408Chuckles.patch(frame),body.center);
        }
        for (unsigned i=0;i<round.chuckles.count;++i) {
            const unsigned at=round.chuckles.defeatedAt[i];
            if (at && round.ticks-at<50)
                drawDestruction(view,round.chuckles.defeatedPosition[i],round.ticks-at,0);
        }
        return;
    }
    if ((currentCourse==FrontierDogCourse || currentCourse==CosmicSecondCourse || (currentCourse>=CosmicFourteenthCourse && currentCourse<=CosmicSeventeenthCourse)) &&
        std::any_of(round.chuckles.bodies,round.chuckles.bodies+round.chuckles.count,
                    [](const CircleCollider& body){return body.radius>0;})) {
        unsigned generation=0;
        auto* workspace=clips.textureWorkspace<ChuckleArt::TextureBytes+256*256>(generation);
        if (!chuckleArt.useShared(workspace,generation)) return;
    }
    if (currentCourse==CosmicFourteenthCourse || currentCourse==CosmicSixteenthCourse || currentCourse==CosmicSeventeenthCourse) {
        setTexture(GU_PSM_8888,BaddieArtWidth,BaddieArtHeight,baddiePixels);
        const auto frame=round.chuckles.spawnerFrame(round.ticks,0);
        sharedActor(view,{static_cast<int16_t>(24*frame),0,17,17,-8,-8},round.chuckles.spawner(0).center);
    }
    if (!chuckleArt.pixels()) return;
    setTexture(GU_PSM_8888,ChuckleArt::Width,ChuckleArt::Height,chuckleArt.pixels());
    for (unsigned i=0;i<round.chuckles.count;++i) {
        const auto& body=round.chuckles.bodies[i];
        if (body.radius>0) actor(view,ChuckleArt::frame(round.chuckles.actorTick(i,round.ticks)),
                                 body.center,view.worldScale());
    }
    for (unsigned i=0;i<round.chuckles.count;++i) {
        const unsigned at=round.chuckles.defeatedAt[i];
        if (!at || round.ticks-at>=50) continue;
        if (currentCourse==CoasterCourse)
            drawScorePopup(view,round.chuckles.defeatedPosition[i],round.ticks-at,round.chuckles.points());
        else
            drawDestruction(view,round.chuckles.defeatedPosition[i],round.ticks-at,round.chuckles.points());
    }
}

void drawFactoryDecor(const Round& round) {
    setTexture(GU_PSM_8888,256,128,factoryDecor.texture());
    constexpr unsigned durations[]{80,1,1,1,1,1,12,1,1,1,1,1,1};
    unsigned cameraTime=round.ticks/5%103, cameraFrame=0;
    while (cameraTime>=durations[cameraFrame]) cameraTime-=durations[cameraFrame++];
    for (unsigned frame:{round.ticks/4%3,3+round.ticks/6%2,5+cameraFrame}) {
        const auto& art=factoryDecor.patch(frame);
        patch(art,ViewX+136+art.x,art.y);
    }
    setTexture(GU_PSM_8888,128,32,factorySpawner.texture());
    const auto& spawner=factorySpawner.patch(round.chuckles.spawnerFrame(round.ticks));
    patch(spawner,ViewX+136+spawner.x,spawner.y);
}

void drawMarcySpill(const GameplayView& view,const Round& round) {
    if (!round.marcy.hintVisible()) return;
    setTexture(GU_PSM_8888,256,128,marcySpillArt.texture());
    const unsigned phase=std::min(17u,round.ticks/4);
    sharedActor(view,marcySpillArt.patch(4+(phase<7?0:phase-6)),MarcyRun::Hint);
    if (round.marcy.spillVisible())
        actor(view,marcySpillArt.patch(round.marcy.spillFrame(round.ticks)),round.marcy.spillCenter,
              round.marcy.spillScale());
}

void drawMarcyHud(const Round& round) {
    if (!round.marcy.startedAt) return;
    const float offset=round.marcy.hudOffsetY(round.ticks)*17.0f/32.0f;
    if (229.5f+offset>=Height) return;
    rectangle(ViewX+136,229.5f+offset,272,42.5f,GU_RGBA(0,0,0,135));
    setTexture(GU_PSM_8888,256,128,marcyHudArt.texture());
    const auto& portrait=marcyHudArt.patch(round.marcy.hudFrame(round.ticks));
    patch(portrait,ViewX+136+portrait.x,portrait.y+offset);
    const unsigned label=round.marcy.label(round.ticks);
    if (!label) return;
    setTexture(GU_PSM_8888,256,128,marcyTextArt.texture());
    const unsigned step=std::min(90u,static_cast<unsigned>(std::lround(90*round.marcy.textRatio(round.ticks))));
    for (unsigned line=0;line<3;++line) {
        auto text=marcyTextArt.patch(label-1);
        const int row=MarcyReveal::Rows[line];
        text.v+=row;
        text.y+=row;
        text.height=MarcyReveal::Rows[line+1]-row;
        text.width=MarcyReveal::Widths[label-1][step][line];
        if (text.width) patch(text,ViewX+136+text.x,text.y+offset);
    }
}

#include "frontier_draw.hpp"

template<class Atlas>
void drawSharedFrontierTiles034(Atlas& atlas,const unsigned (&counts)[4],unsigned state) {
    unsigned generation=0;
    auto* workspace=clips.textureWorkspace<Atlas::TextureWidth*Atlas::TextureHeight>(generation);
    if (atlas.useShared(workspace,generation)) drawFrontierTiles(atlas,counts,state);
}

void drawSwitchParkTiles(const Round& round) {
    if (currentCourse==Elysian512Course || currentCourse==Elysian514Course || currentCourse==CosmicThirdCourse || currentCourse==CosmicFourthCourse || currentCourse==CosmicSeventhCourse) return;
    if (currentCourse==CataclysmCourse) {
        drawFrontierOverlays417(round);
        return;
    }
    if (currentCourse==PlentifulChucklesCourse) {
        drawSharedFrontierTiles034(frontier408Tiles,SnowSwitchPatchCounts035,round.switches.state);
        return;
    }
    if (currentCourse==WholesomeCourse) {
        drawSharedFrontierTiles034(wholesomeTiles,SnowSwitchPatchCounts034[0],round.switches.state);
        return;
    }
    if (currentCourse==CubeTownCourse) {
        drawSharedFrontierTiles034(cubeTownTiles,SnowSwitchPatchCounts034[1],round.switches.state);
        return;
    }
    if (currentCourse==FrontierSwitchCourse) {
        drawFrontierTiles(frontierSwitchTiles,FrontierSwitchPatchCounts,round.switches.state);
        return;
    }
    if (!round.switchStage) return;
    if (currentCourse>=PursuitCourse) {
        if (const auto* pixels=pursuitTiles.texture()) {
            setTexture(GU_PSM_8888,128,128,pixels);
            const auto& counts=ParkSwitchPatchCounts[currentCourse-PursuitCourse];
            const unsigned first=round.switches.state?counts[0]:0;
            for (unsigned i=first;i<first+counts[round.switches.state];++i) {
                const auto& art=pursuitTiles.patch(i);
                patch(art,ViewX+GameplayView::HudWidth+art.x,art.y);
            }
        }
        return;
    }
    Patch art{};
    if (const auto* pixels=switchTiles.frame(round.switches.state,art)) {
        setTexture(GU_PSM_5650,ParkSwitchOverlay::Side,ParkSwitchOverlay::Side,pixels);
        patch(art,ViewX+GameplayView::HudWidth+art.x,art.y);
    }
}

void drawSwitchParkBlocks(const GameplayView& view,const Round& round) {
    if (currentCourse==PlentifulChucklesCourse) {
        unsigned generation=0;
        auto* workspace=clips.textureWorkspace<Frontier408FacesOffset+128*64>(generation)+Frontier408FacesOffset;
        if (frontierSwitchFaces.useShared(workspace,generation)) drawFrontierBlocks(view,round,frontierSwitchFaces);
        return;
    }
    if (currentCourse==WholesomeCourse || currentCourse==CubeTownCourse) {
        drawFrontierBlocks(view,round,frontierSwitchFaces);
        return;
    }
    if (currentCourse==FrontierSwitchCourse) {
        drawFrontierBlocks(view,round,frontierSwitchFaces);
        return;
    }
    if (currentCourse==CosmicThirdCourse || currentCourse==CosmicFourthCourse || currentCourse==CosmicSeventhCourse) {
        if (!round.switchStage || !cosmicSwitchFaces046.texture()) return;
        setTexture(GU_PSM_8888,ElysianSwitchFaces514Width,ElysianSwitchFaces514Height,cosmicSwitchFaces046.texture());
        for (unsigned i=0;i<round.switchStage->blockCount;++i)
            actor(view,cosmicSwitchFaces046.patch(round.switches.face(i)),round.switchStage->blocks[i].center,view.worldScale());
        return;
    }
    if (currentCourse==Elysian514Course) {
        if (!round.switchStage || !elysianSwitchFaces514.texture()) return;
        setTexture(GU_PSM_8888,ElysianSwitchFaces514Width,ElysianSwitchFaces514Height,elysianSwitchFaces514.texture());
        for (unsigned i=0;i<round.switchStage->blockCount;++i)
            actor(view,elysianSwitchFaces514.patch(round.switches.face(i)),round.switchStage->blocks[i].center,
                  view.worldScale(),elysianHazardTint040(currentCourse));
        return;
    }
    if (!round.switchStage || !switchFaces.texture()) return;
    setTexture(GU_PSM_8888,128,64,switchFaces.texture());
    for (unsigned i=0;i<round.switchStage->blockCount;++i) {
        const unsigned face=round.switches.face(i);
        if (face<switchFaces.size())
            sharedActor(view,switchFaces.patch(face),round.switchStage->blocks[i].center,elysianHazardTint040(currentCourse));
    }
}

void drawCarouselDecal(const GameplayView& view,unsigned tick) {
    if (!carouselDecal.texture()) return;
    setTexture(GU_PSM_8888,128,128,carouselDecal.texture());
    sharedActor(view,carouselDecal.patch(tick/3%carouselDecal.size()),{});
}

void drawCarouselLight(const GameplayView& view,unsigned tick) {
    constexpr Vec2 quadrants[4][4]={
        {{0,0},{0,-384},{-384,0},{-384,-384}},
        {{0,0},{0,-384},{384,0},{384,-384}},
        {{0,0},{0,384},{-384,0},{-384,384}},
        {{0,0},{0,384},{384,0},{384,384}}
    };
    constexpr uint32_t colors[]={
        GU_RGBA(255,0,0,20),GU_RGBA(255,208,0,20),
        GU_RGBA(0,255,8,20),GU_RGBA(0,140,255,20)
    };
    const float angle=-6.28319f*(tick%480)/480;
    const Vec2 right{std::cos(angle),std::sin(angle)};
    const Vec2 down{-right.y,right.x};
    sceGuDisable(GU_TEXTURE_2D);
    for (unsigned sector=0;sector<4;++sector) {
        auto* vertices=static_cast<ColorVertex*>(sceGuGetMemory(4*sizeof(ColorVertex)));
        for (unsigned i=0;i<4;++i) {
            const auto p=quadrants[sector][i];
            const Vec2 pixel=view.position(right*p.x+down*p.y);
            vertices[i]={colors[sector],pixel.x,pixel.y,0};
        }
        sceGuDrawArray(GU_TRIANGLE_STRIP,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,vertices);
    }
    sceGuEnable(GU_TEXTURE_2D);
}

void drawParkBalloons(const GameplayView& view,const Round& round) {
    const auto* pixels=parkArt.pixels();
    if (!pixels || !parkArt.data.balloonCount) return;
    const bool factory=currentCourse==FactoryCourse;
    if (factory) setTexture(GU_PSM_8888,128,64,factoryBalloons.texture());
    else setTexture(GU_PSM_8888,ParkArt::Side,ParkArt::Side,pixels);
    const unsigned tick=round.ticks;
    for (unsigned index=0;index<parkArt.data.balloonCount;++index) {
        const auto& balloon=parkArt.data.balloons[index];
        const auto& sprite=factory?factoryBalloons.patch(index):parkArt.balloon();
        drawBalloon(view,balloon,parkArt.balloonMotion(index,tick),parkArt.balloonMotion(index,tick?tick-1:0),
                    sprite,view.worldZoom(),factory?White:balloon.color);
    }
}

template<class Stage,class Ring>
void drawSmileyRing(const GameplayView& view,const Stage& stage,const Ring& ring,
                    unsigned tick,uint32_t ringTint,uint32_t coinTint,const uint32_t* texture=nullptr) {
    setTexture(GU_PSM_8888,ParkArt::Side,ParkArt::Side,texture?texture:parkArt.pixels());
    if (ring.phase!=SmileyPhase::Hidden) {
        const auto tint=ring.phase==SmileyPhase::Ready?ringTint:((ringTint&0xffffffu)|0x80000000u);
        sharedActor(view,parkArt.ring(tick),stage.center,tint);
    }
    if (ring.phase==SmileyPhase::Active) {
        for (unsigned i=0;i<stage.coinCount;++i) {
            if (!ring.collected(i)) sharedActor(view,parkArt.coin(tick),stage.coins[i],coinTint);
        }
    }
    for (unsigned i=0;i<stage.coinCount;++i) {
        if (ring.collected(i) && tick-ring.coinTick[i]<50)
            drawScorePopup(view,stage.coins[i],tick-ring.coinTick[i],ring.coinOrder[i]);
    }
    if (ring.phase==SmileyPhase::Active || ring.phase==SmileyPhase::Passed || ring.phase==SmileyPhase::Failed) {
        const Vec2 center=view.position(stage.center);
        char label[24];
        if (ring.phase==SmileyPhase::Active) {
            std::snprintf(label,sizeof label,"%u",ring.remaining(stage,tick));
            localeText(label,center.x-ViewX,center.y-6,0.9f,White,true);
        } else if (ring.phase==SmileyPhase::Passed) {
            localeText((currentCourse==CosmicFifthCourse || currentCourse==CosmicEleventhCourse || currentCourse==CosmicSeventeenthCourse)?lang("Good!","Отлично!"):"Good!",center.x-ViewX,center.y-12,0.9f,White,true);
            std::snprintf(label,sizeof label,"+%u %s",static_cast<unsigned>(stage.bonus),(currentCourse==CosmicFifthCourse || currentCourse==CosmicEleventhCourse || currentCourse==CosmicSeventeenthCourse)?lang("pts.","очк."):"pts.");
            localeText(label,center.x-ViewX,center.y,0.9f,White,true);
        } else {
            localeText((currentCourse==CosmicFifthCourse || currentCourse==CosmicEleventhCourse || currentCourse==CosmicSeventeenthCourse)?lang("Fail...","Неудача..."):"Fail...",center.x-ViewX,center.y-6,0.9f,White,true);
        }
    }
}

void drawParkObjects(const GameplayView& view,const Round& round) {
    if (!parkArt.pixels()) return;
    if (currentCourse==Elysian512Course) {
        constexpr uint32_t tint=GU_RGBA(119,192,255,255);
        drawSmileyRing(view,ElysianRings512[0],round.smiley,round.elysianRingTick,tint,tint);
    } else if (currentCourse==ArcadeCourse) {
        for (unsigned i=0;i<std::size(ParkRingStages);++i) {
            drawSmileyRing(view,ParkRingStages[i],round.arcadeRings[i],round.ticks,ParkRingTints[i],ParkRingTints[i]);
        }
    } else if (currentCourse==PursuitCourse) {
        for (unsigned i=0;i<std::size(ParkPursuitRingStages);++i)
            drawSmileyRing(view,ParkPursuitRingStages[i],round.pursuitRings[i],round.ticks,
                           ParkPursuitRingTints[i],ParkPursuitRingTints[i]);
    } else if (currentCourse==DoubleDogCourse) {
        drawSmileyRing(view,ParkRings316[0],round.dogRings[0],round.ticks,ParkRingTints316[0],ParkRingTints316[0]);
    } else if (currentCourse==RacetrackCourse) {
        for (unsigned i=0;i<std::size(ParkRings317);++i)
            drawSmileyRing(view,ParkRings317[i],round.dogRings[i],round.ticks,ParkRingTints317[i],ParkRingTints317[i]);
    } else if (parkArt.data.smiley.coinCount) {
        drawSmileyRing(view,parkArt.data.smiley,round.smiley,round.ticks,parkArt.data.tint,parkArt.data.tint);
    }
}

void drawParkAsh(const Round& round) {
    if (!round.parkScare.ashAlpha) return;
    setTexture(GU_PSM_8888,512,256,ashArt.texture());
    const uint32_t tint=GU_RGBA(255,255,255,round.parkScare.ashAlpha);
    for (unsigned i=0;i<ashArt.size();++i) {
        const auto& art=ashArt.patch(i);
        patch(art,ViewX+136+art.x,art.y,1,tint);
    }
}

void drawParkUfo(const Round& round) {
    if (!round.smiley.everStarted || parkArt.data.ufoSpeed<=0) return;
    const auto* pixels=parkArt.pixels();
    if (!pixels) return;
    const auto& stage=parkArt.data;
    const float x=(stage.ufo.x-stage.ufoSpeed*(round.ticks-round.smiley.ufoStart))*GameplayView::RasterScale;
    const float y=stage.ufo.y*GameplayView::RasterScale;
    const auto& sprite=parkArt.ufo(round.ticks);
    if (x+sprite.x>=ViewWidth || x+sprite.x+sprite.width<=0) return;
    setTexture(GU_PSM_8888,ParkArt::Side,ParkArt::Side,pixels);
    patch(sprite,ViewX+x+sprite.x,y+sprite.y);
}

#include "slot_machine_draw.hpp"

void drawMotionTokens(const GameplayView& view,MotionTokenView stage,const StopTokenRun& run,
                      const StopTokenAssets& assets,unsigned ticks,bool frontier=false) {
    if (!stage) return;
    if (frontier) {
        if (!bindFrontier408Chuckles()) return;
    } else {
        setTexture(GU_PSM_8888,32,32,assets.sprite);
    }
    const Patch sprite=frontier?frontier408Chuckles.patch(29):Patch{0,0,17,17,-8,-8};
    const float scale=currentCourse>=CityCourse?view.worldZoom():1.0f;
    for (unsigned i=0;i<stage.count;++i) {
        if (!run.has(i)) actor(view,sprite,stage.positions[i],scale,
                          currentCourse==CosmicFifthCourse?starstormTint047(stage.colors[i]):currentCourse==Elysian510Course?elysianHazardTint040(currentCourse):stage.colors[i]);
    }
    for (unsigned i=0;i<stage.count;++i) {
        if (!run.has(i)) continue;
        const unsigned age=ticks-run.pickedAt[i];
        if (age<50) drawScorePopup(view,stage.positions[i],age,0);
    }
}

void drawPoolWater(const Round& round) {
    const unsigned frame=round.ticks*25/60%PoolAssets::Frames;
    setTexture(GU_PSM_5650,PoolAssets::Width,PoolAssets::Height,poolAssets.water[frame]);
    patch({0,0,44,122,0,0},ViewX+362,2);
}

void drawPoolRipples(const GameplayView& view,const Round& round) {
    setTexture(GU_PSM_8888,256,64,poolAssets.sprites);
    for (const auto& ripple:round.pool.ripples) {
        const unsigned age=round.ticks-ripple.tick;
        if (!ripple.tick || age>=24 || ripple.scale<0.03f) continue;
        actor(view,{static_cast<int16_t>(32+40*(age/6)),0,34,34,-17,-17},ripple.position,
              2*ripple.scale*view.worldZoom(),GU_RGBA(255,255,255,89));
    }
}

void drawPoolSpeech(const GameplayView& view,const Round& round) {
    if (!round.pool.showingSpeech) return;
    const unsigned age=round.ticks-round.pool.speechStart;
    if (!age) return;
    const float scale=std::max(0.08f,std::min(age,30u)/30.0f);
    const auto anchor=view.position({400,-112});
    const unsigned langIndex=russian()?1:0;
    const auto& widths=poolAssets.lineWidth[langIndex];
    unsigned lines=0,totalWidth=0;
    while (lines<std::size(widths) && widths[lines]) totalWidth+=widths[lines++];
    constexpr float fullWidth=198;
    const float fullHeight=lines*16+10;
    const float right=ViewX+ViewWidth-5, bottom=anchor.y-9;
    const float x=right-fullWidth*scale, y=bottom-fullHeight*scale;
    const uint32_t outline=GU_RGBA(59,43,57,255);
    rectangle(anchor.x-3*scale,bottom-1,6*scale,7*scale,outline);
    rectangle(anchor.x-2*scale,bottom-1,4*scale,6*scale,White);
    rectangle(x,y,fullWidth*scale,fullHeight*scale,outline);
    rectangle(x+1,y+1,fullWidth*scale-2,fullHeight*scale-2,White);
    if (age<30) return;
    setTexture(GU_PSM_8888,PoolAssets::SpeechWidth,PoolAssets::SpeechHeight,poolAssets.speech);
    unsigned visible=totalWidth*std::min(age-30,270u)/270;
    for (unsigned row=0;row<lines && visible;++row) {
        const int width=std::min<unsigned>(widths[row],visible);
        patch({0,static_cast<int16_t>(langIndex*128+row*16),static_cast<int16_t>(width),16,0,0},
              x+5,y+5+row*16);
        visible-=width;
    }
}

void drawLiveStats(const Round& round,const uint16_t* backdrop,bool includeBonus=true) {
    const unsigned score=progress.totalScore(),prisms=progress.totalPrisms();
    const unsigned bonus=round.bonus();
    if (backdrop) {
        setTexture(GU_PSM_5650,VisualWidth,VisualHeight,backdrop);
        if (russian()) {
            patchHudGradient(0,74,90,18);
            if (includeBonus) patchHudGradient(0,141,85,13);
        } else {
            if (score) patchHudGradient(20,76,54,14);
            if (includeBonus && bonus) patchHudGradient(55,141,30,13);
        }
        if (prisms) patchHudGradient(111,76,21,13);
    }

    char text[72];
    if (includeBonus && russian()) {
        std::snprintf(text,sizeof text,"Бонус: %u",bonus);
        localeText(text,5,139,SmallUiText,Gold,false,80);
    } else if (includeBonus && bonus) {
        std::snprintf(text,sizeof text,"%u",bonus);
        resultText(text,57,140,Gold);
    }
    if (russian()) {
        std::snprintf(text,sizeof text,"Счёт: %u!",score);
        localeText(text,5,73,SmallUiText,White,false,88);
    } else if (score) {
        std::snprintf(text,sizeof text,"%u !",score);
        resultText(text,20,75);
    }
    if (prisms) {
        std::snprintf(text,sizeof text,"%u",prisms);
        resultText(text,114,76);
    }
}

void drawTv(const uint16_t* tv) {
    if (tv) {
        setTexture(GU_PSM_5650,64,64,tv);
        patch({0,0,58,48,0,0},ViewX+21,186);
    }
}

void drawThoughtAndKey(const Round& round) {
    if (currentCourse==CataclysmCourse && !round.backrooms) drawFrontierThinker417(round);
    else if (currentCourse!=CosmicSeventeenthCourse) drawTv(round.backrooms?backroomsTv:thoughts.frame(round.tvTick()));
    if (!round.terry.running && !round.questKeyTick) return;
    const unsigned age=round.totalTicks-(round.questKeyTick?round.questKeyTick:round.terry.keyTick);
    if (age>=210) return;
    const unsigned alpha=age<=180 ? 255 : (210-age)*255/30;
    if (russian()) {
        rectangle(ViewX+176,102,194,53,GU_RGBA(255,250,240,alpha));
        localeText("О! Найден секретный",273,112,1,GU_RGBA(70,35,101,alpha),true);
        localeText("ключ!",273,130,1,GU_RGBA(70,35,101,alpha),true);
        return;
    }
    setTexture(GU_PSM_8888,512,256,objects.key);
    const Patch key{0,static_cast<int16_t>(128*((round.totalTicks/3)%2)),272,102,0,0};
    patch(key,ViewX+136,85,1,GU_RGBA(255u,255u,255u,alpha));
}

void drawPause(const Round& round) {
    using namespace PauseArt;
    const auto& menu=round.pauseMenu;
    const unsigned age=menu.elapsedMs;
    const auto layer=[](const uint8_t* pixels) {
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(GU_PSM_T8,0,0,GU_TRUE);
        sceGuClutMode(GU_PSM_8888,0,255,0);
        sceGuClutLoad(32,colors);
        sceGuTexImage(0,512,512,Field,pixels);
        patch({0,0,Field,Field,0,0},ViewX+136,0);
    };
    if (round.cleanPause) {
        const unsigned cameraFrame=age/67%64;
        if (cameraFrame>=30) {
            setTexture(GU_PSM_8888,PauseArt::Width,PauseArt::Height,atlas);
            hudPatch(Camera[cameraFrame>=60?cameraFrame-59:0]);
        }
        return;
    }
    layer(darkness);
    setTexture(GU_PSM_8888,PauseArt::Width,PauseArt::Height,atlas);
    hudPatch((russian()?RuHeadline:EnHeadline)[0]);
    for (unsigned index=0;index<4;++index) {
        const bool selected=index==static_cast<unsigned>(menu.choice);
        const Patch* labels=russian()?RuNormal:EnNormal;
        if (selected) {
            if (menu.crossArmed) labels=russian()?RuPressed:EnPressed;
            else labels=russian()?RuHover:EnHover;
        }
        hudPatch(labels[index]);
    }
    hudPatch(Dancer[age/200%4]);
    hudPatch((russian()?RuPrompt:EnPrompt)[0]);
    layer(filterPixels(age*3/100%FilterFrames));
}

void drawFull(const uint16_t* texture) {
    setTexture(GU_PSM_5650,VisualWidth,VisualHeight,texture);
    patch({0,0,ViewWidth,Height,0,0},ViewX,0);
}

void beginFrame() {
    sceGuStart(GU_DIRECT,displayList);
    sceGuClearColor(GU_RGBA(0,0,0,255));
    sceGuClear(GU_COLOR_BUFFER_BIT);
}

#ifdef UCG_VICTORY_PROBE
struct VictoryRenderSample {
    bool active=false;
    unsigned resultMax=0,tvMax=0,geMax=0;
} victoryRenderSample;
#endif

void endFrame() {
    sceGuScissor(0,0,Width,Height);
    setTexture(GU_PSM_5650,128,512,sidebars);
    patch({0,0,ViewX,Height,0,0},0,0);
    patch({64,0,ViewX,Height,0,0},ViewX+ViewWidth,0);
    sceGuFinish();
#ifdef UCG_VICTORY_PROBE
    const unsigned geStart=victoryRenderSample.active?sceKernelGetSystemTimeLow():0;
#endif
    sceGuSync(GU_SYNC_FINISH,GU_SYNC_WHAT_DONE);
#ifdef UCG_VICTORY_PROBE
    if (victoryRenderSample.active)
        victoryRenderSample.geMax=std::max(victoryRenderSample.geMax,sceKernelGetSystemTimeLow()-geStart);
#endif
}

bool selectorTravelling(unsigned elapsedMs) {
    const auto& travel=levelSelect.transition();
    return travel.from!=LevelSelectState::NoCourse &&
           elapsedMs-travel.startMs<SelectorTransitionArt::DurationMs;
}

bool moveLevelSelection(uint32_t pressed,unsigned elapsedMs) {
    if (selectorTravelling(elapsedMs)) return false;
    bool moved=false;
    if (pressed&PSP_CTRL_LTRIGGER) moved=levelSelect.changeWorld(-1,elapsedMs);
    else if (pressed&PSP_CTRL_RTRIGGER) moved=levelSelect.changeWorld(1,elapsedMs);
    else if (pressed&PSP_CTRL_UP) moved=levelSelect.move(LevelSelectDirection::Up);
    else if (pressed&PSP_CTRL_DOWN) moved=levelSelect.move(LevelSelectDirection::Down);
    else if (pressed&PSP_CTRL_LEFT) moved=levelSelect.move(LevelSelectDirection::Left);
    else if (pressed&PSP_CTRL_RIGHT) moved=levelSelect.move(LevelSelectDirection::Right);
    menuCourse=levelSelect.course();
    return moved;
}

bool updateLevelSelector(uint32_t pressed,uint32_t released,uint32_t held,unsigned elapsedMs) {
    selectorGoDown=false;
    if (!selectorGoArmed && moveLevelSelection(pressed,elapsedMs)) audio.play(Sound::Select);
    if (selectorTravelling(elapsedMs) || menuCourse==LevelSelectState::NoCourse) return false;
    if (pressed&PSP_CTRL_CROSS) selectorGoArmed=true;
    selectorGoDown=selectorGoArmed && (held&PSP_CTRL_CROSS);
    if (!selectorGoArmed || !(released&PSP_CTRL_CROSS)) return false;
    selectorGoArmed=false;
    return true;
}

#include "selector_details_draw.hpp"
#include "live_result_draw.hpp"

void preloadTutorialVictory() {
    if (currentCourse>=TutorialCount || clips.count(resultClip(true)) ||
        commonResultArt.hasPreloadedFrames()) return;
    if (!commonResultArt.preloadFrames(3,59,levelSelectArt.victoryWorkspace(),LevelSelectArt::VictoryWorkspaceBytes)) return;
    auto* vram=static_cast<uint8_t*>(sceGeEdramGetAddr())+LiveResultVramOffset;
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    fallbackPageIndex=ScreenPages;
    liveResultLarge=liveResultClear=liveResultConfetti={};
    commonResultArt.prepare(0,liveResultLarge,vram,ResultLargeBytes,scratch,ResultScratchBytes);
    commonResultArt.prepare(russian()?2:1,liveResultClear,vram+ResultLargeBytes,
                            ResultClearBytes,scratch,ResultScratchBytes);
    commonResultArt.closeStream();
}
#include "ride_draw033.hpp"
#include "elysian_mist043_draw.hpp"
#include "elysian_wizard044_draw.hpp"
#include "elysian_finale045_prepare.hpp"
#include "disco_light051.hpp"
#include "elysian_finale045_draw.hpp"
#include "cosmic_ending053.hpp"
#include "cosmic_draw_extra046.hpp"
#include "cosmic_draw051.hpp"
#include "cosmic_draw052.hpp"
#include "cosmic_draw053.hpp"
#include "cosmic_draw046.hpp"
#include "cosmic_selector046.hpp"
#include "starstorm_draw047.hpp"
#include "starstorm_ring047.hpp"
#include "cosmic_draw048.hpp"
#include "cosmic_draw049.hpp"
#include "cosmic_draw050.hpp"

#include "gift_ui.hpp"

void drawSelectorCards(unsigned world,const LevelSelectArt::CardAtlas& atlas,float offset,
                       bool moving=false,bool focused=true) {
    const auto drawCard=[offset,moving](const Patch& art) {
        const float x=ViewX+art.x,y=art.y+offset;
        if (!moving) {
            patch(art,x,y);
            return;
        }
        const float left=std::max(x,ViewX+145.0f),right=std::min(x+art.width,ViewX+277.0f);
        const float top=std::max(y,26.0f),bottom=std::min(y+art.height,268.0f);
        if (left>=right || top>=bottom) return;
        auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
        vertices[0]={art.u+left-x,art.v+top-y,White,left,top,0};
        vertices[1]={art.u+right-x,art.v+bottom-y,White,right,bottom,0};
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,vertices);
    };
    setTexture(GU_PSM_5650,LevelSelectArt::Size,LevelSelectArt::Size,atlas.normal);
    const unsigned count=LevelSelectState::cells(world);
    for (unsigned cell=0;cell<count;++cell) {
        const bool unlocked=levelSelect.unlocked(world,cell);
        const unsigned course=LevelSelectState::firstCourse(world)+cell;
        const unsigned state=unlocked ? std::min(5u,static_cast<unsigned>(progress.best[course].rank)) :
                                        LevelSelectArt::Locked;
        drawCard(levelSelectArt.cards[world][cell][state]);
    }
    const unsigned selected=levelSelect.course(world);
    if (focused && selected!=LevelSelectState::NoCourse) {
        const unsigned rank=std::min(5u,static_cast<unsigned>(progress.best[selected].rank));
        setTexture(GU_PSM_5650,LevelSelectArt::Size,LevelSelectArt::Size,atlas.focus);
        drawCard(levelSelectArt.cards[world][levelSelect.cell(world)][LevelSelectArt::FirstFocused+rank]);
    }
}

void drawLevelSelect(unsigned elapsedMs) {
    if (commonResultArt.hasPreloadedFrames()) {
        commonResultArt.dropPreloadedFrames();
        liveResultLarge=liveResultClear=liveResultConfetti={};
    }
    const unsigned world=levelSelect.world();
    unsigned latest=world;
    while (levelSelect.unlocked(latest+1,0)) ++latest;
    const bool moving=selectorTravelling(elapsedMs);
    if (!moving) selectorTransitionArt.closeStream();
    const auto& travel=levelSelect.transition();
    const auto* cards=levelSelectArt.loadCards(world,moving?travel.from:LevelSelectState::NoCourse);
    if (!cards || !levelSelectArt.loadBase(moving?travel.from:world,latest,russian())) return;

    const auto* outgoing=moving ? levelSelectArt.loadCards(travel.from,world) : nullptr;
    const unsigned frame=moving ? (elapsedMs-travel.startMs)*SelectorTransitionArt::Fps/1000 : 0;
    const auto* panel=outgoing ? selectorTransitionArt.frame(travel.from,world,latest,russian(),frame) : nullptr;
    unsigned details=menuCourse;
    if (panel) {
        setTexture(GU_PSM_5650,VisualWidth,VisualHeight,levelSelectArt.base);
        patch({0,0,136,272,0,0},ViewX,0);
        setTexture(GU_PSM_5650,VisualWidth,VisualHeight,panel);
        patch({0,0,272,272,0,0},ViewX+136,0);

        // Godot lerps the camera and the card strip by 0.15 on each 60 Hz tick.
        constexpr float worldStep=456*17.0f/32.0f;
        const float delta=world>travel.from ? worldStep : -worldStep;
        const float remaining=std::pow(0.85f,2.0f*frame);
        drawSelectorCards(travel.from,*outgoing,delta*(remaining-1.0f),true);
        drawSelectorCards(world,*cards,delta*remaining,true,false);
        details=levelSelect.course(travel.from);
    } else {
        if (moving && !levelSelectArt.loadBase(world,latest,russian())) return;
        drawFull(levelSelectArt.base);
        drawSelectorCards(world,*cards,0);
        if (selectorGoDown) {
            if (const auto* pixels=levelSelectArt.goFrame(world,latest,russian())) {
                setTexture(GU_PSM_5650,64,64,pixels);
                patch({0,0,34,34,0,0},ViewX+366,221);
            }
        }
    }
    drawCosmicWorldTitle046(panel!=nullptr,world,travel.from,frame);
    drawSelectorDetails(selectorDetails,progress,details,russian());
    setTexture(GU_PSM_5650,VisualWidth,VisualHeight,levelSelectArt.base);
    patchHudGradient(0,70,103,29);
    localeText(lang("Total Score","Общий счёт"),4,69,0.79f);
    char total[24];
    std::snprintf(total,sizeof total,"%u!",progress.totalScore());
    localeText(total,5,82,0.93f,White,false,98,true);
}

void drawAnimatedBack(unsigned elapsedMs,bool profile=false) {
    using namespace MenuScroll;
    alignas(64) static uint8_t frame[BackStride*32*4]{};
    if (profile) {
        if (!profileArt.backFrame(elapsedMs,frame)) return;
    } else if (!menuButtonArt.load(russian()) || !decodeBack(menuButtonArt.atlas,elapsedMs,frame)) return;
    sceKernelDcacheWritebackRange(frame,sizeof frame);
    if (!profile)
        rectangle(ViewX+MenuScroll::BackX,MenuScroll::BackY,MenuScroll::BackWidth,
                  MenuScroll::BackHeight,GU_RGBA(32,68,106,255));
    sceGuBlendFunc(GU_ADD,GU_FIX,GU_ONE_MINUS_SRC_ALPHA,0x00ffffff,0);
    setTexture(GU_PSM_8888,MenuScroll::BackStride,32,frame);
    hudPatch({0,0,MenuScroll::BackWidth,MenuScroll::BackHeight,MenuScroll::BackX,MenuScroll::BackY});
    if (profile) {
        setTexture(GU_PSM_8888,BackStride,32,profileArt.backLabel(russian()));
        hudPatch({0,0,BackWidth,BackHeight,BackX,BackY});
    }
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
}

void drawMenuTv() {
    const auto tick=static_cast<unsigned>(sceKernelGetSystemTimeWide()*60/1000000);
    drawTv(thoughts.menuFrame(tick));
}

#include "profile_ui.hpp"
#include "xmb_profile.hpp"
#include "hardware_probe.hpp"

void drawSettings() {
    constexpr int rows[]={63,110,157};
    const auto localeU=static_cast<int16_t>(russian()?112:0);
    setTexture(GU_PSM_8888,256,256,settingsWidgets);
    for (unsigned i=0;i<3;++i) {
        patch({localeU,static_cast<int16_t>(104+18*i),112,17,0,0},ViewX+166,rows[i]+3);
    }
    const auto focused=[](SettingsRow row) { return settingsRow==row; };
    patch({0,static_cast<int16_t>(focused(SettingsRow::Language)?24:0),130,23,0,0},ViewX+263,rows[0]);
    patch({0,static_cast<int16_t>(focused(SettingsRow::Background)?24:0),130,23,0,0},ViewX+263,rows[1]);
    patch({static_cast<int16_t>(languageDraft==Language::Russian?168:148),44,17,13,0,0},
          ViewX+285,rows[0]+5);
    patch({static_cast<int16_t>(backgroundDraft.random?168:148),
           static_cast<int16_t>(focused(SettingsRow::Random)?20:0),18,18,0,0},
          ViewX+348,rows[2]+3);
    patch({0,static_cast<int16_t>(focused(SettingsRow::Save)?74:48),136,25,0,0},ViewX+204,240);
    patch({localeU,158,112,17,0,0},ViewX+216,244);
    localeText(languageDraft==Language::Russian?"Русский":"English",336,rows[0]+2,1,White,true,68,true);
    localeText("←",272,rows[0]+3,SmallUiText);
    localeText("→",379,rows[0]+3,SmallUiText);
    constexpr const char* english[]={"Black","Tutorial","Golf Central","Golf Central 1-08","Chuckle Park",
                                      "Final Frontier","Cosmic Championship","Moon"};
    constexpr const char* russianNames[]={"Чёрный","Обучение","Гольф-центр","Гольф-центр 1-08","Чакл-парк",
                                           "Final Frontier","Cosmic Championship","Луна"};
    static_assert(std::size(english)==RenderedBackgroundCount && std::size(russianNames)==RenderedBackgroundCount);
    char label[96];
    if (activeBackground>=CatalogBackgroundBase) {
        std::snprintf(label,sizeof label,"%03u",activeBackground-CatalogBackgroundBase);
    } else {
        const unsigned index=backgroundIndex(activeBackground);
        const char* name=lang(english[index],russianNames[index]);
        if (activeBackground) std::snprintf(label,sizeof label,"G%02u %s",activeBackground,name);
        else std::snprintf(label,sizeof label,"%s",name);
    }
    localeText(label,329,rows[1]+2,1,White,true,100,true);
    localeText("←",267,rows[1]+2,1);
    localeText("→",381,rows[1]+2,1);

    const char* hint=nullptr;
    switch (settingsRow) {
    case SettingsRow::Language: hint=lang("← / →  Language     × Save","← / →  Язык     × Сохранить"); break;
    case SettingsRow::Background: hint=lang("← / →  Preview     × Save","← / →  Просмотр     × Сохранить"); break;
    case SettingsRow::Random: hint=lang("× Random background at launch","× Случайный фон при запуске"); break;
    case SettingsRow::Save: hint=lang("× Save settings","× Сохранить настройки"); break;
    case SettingsRow::Count: break;
    }
    if (hint) localeText(hint,272,196,1,White,true,240,true);
    if (settingsError) localeText(lang("Could not save settings","Не удалось сохранить настройки"),
                                  272,217,SmallUiText,White,true,246);
    if (settingsPreviewError) localeText(lang("Could not load background","Не удалось загрузить фон"),
                                         272,217,SmallUiText,White,true,246);
}

void beginSettingsDraft() {
    languageDraft=language;
    backgroundDraft=backgroundPreference;
    backgroundBeforeSettings=activeBackground;
    settingsRow=SettingsRow::Language;
    settingsError=settingsPreviewError=false;
}

void updateSettings(Flow& flow,uint32_t pressed,const char* executable,Course& course,
                    const LanguageFile& languageFile,const BackgroundFile& backgroundFile,const char* log,
                    const Game& game) {
    if (pressed&PSP_CTRL_CIRCLE) {
        selectBackground(backgroundBeforeSettings);
        return;
    }
    const bool up=pressed&PSP_CTRL_UP,down=pressed&PSP_CTRL_DOWN;
    constexpr unsigned rows=static_cast<unsigned>(SettingsRow::Count);
    if (up!=down) {
        settingsRow=static_cast<SettingsRow>((static_cast<unsigned>(settingsRow)+(down?1:rows-1))%rows);
        settingsError=settingsPreviewError=false;
        audio.play(Sound::Select);
        return;
    }
    const bool left=pressed&PSP_CTRL_LEFT,right=pressed&PSP_CTRL_RIGHT,confirm=pressed&PSP_CTRL_CROSS;
    if (left!=right && settingsRow==SettingsRow::Language) {
        languageDraft=languageDraft==Language::English?Language::Russian:Language::English;
        settingsError=false;
        audio.play(Sound::Select);
    } else if (left!=right && settingsRow==SettingsRow::Background) {
        const unsigned index=(backgroundIndex(activeBackground)+(right?1:BackgroundCount-1))%BackgroundCount;
        const unsigned id=BackgroundIds[index];
        settingsPreviewError=!selectBackground(id);
        settingsError=false;
        if (!settingsPreviewError) backgroundDraft={id,id,backgroundDraft.random};
        audio.play(Sound::Select);
    }
    if (!confirm) return;
    settingsError=settingsPreviewError=false;
    if (settingsRow==SettingsRow::Random) {
        backgroundDraft={activeBackground,activeBackground,!backgroundDraft.random};
        audio.play(Sound::Select);
        return;
    }
    if (backgroundDraft.encode()!=backgroundPreference.encode()) {
        if (!changeBackground(backgroundFile,backgroundDraft.chosen,backgroundDraft.random)) {
            settingsError=true;
            return;
        }
        backgroundBeforeSettings=activeBackground;
        logBackground(log,"background_saved");
    }
    if (languageDraft!=language) {
        settingsError=!changeLanguage(executable,course,languageFile,languageDraft);
        if (settingsError) return;
        logEvent(log,russian()?"language_saved_ru":"language_saved_en",game);
    }
    flow.screen=Screen::Main;
}

void render(const Flow& flow,const Course& course,const Round& round,unsigned elapsedMs,
            unsigned levelClock=0,unsigned totalClock=0,unsigned resultMs=0,
            unsigned menuTick=UINT32_MAX,unsigned menuElapsedMs=UINT32_MAX,bool fastEnding=false) {
    if (flow.screen!=Screen::Hole) santaBackdropSlots[2]={};
    if (flow.screen!=Screen::LevelSelect) {
        cosmicWorldSlot046={};
        cosmicTitles046.unload();
        cosmicTitles047.unload();
        cosmicTitles048.unload();
        cosmicTitles049.unload();
        cosmicTitles050.unload();
        cosmicTitles051.unload();
        cosmicTitles052.unload();
        cosmicTitles053.unload();
    }
    if (flow.screen!=Screen::Hole) {
        elysianSwitchArt512.closeStream();
        elysianSwitchArt514.closeStream();
        elysianTreesArt515.closeStream();
        elysianOutOfBoundsArt.closeStream();
        liveResultArt.closeStream();
        commonResultArt.closeStream();
    }
    if (flow.screen==Screen::Title && titleArt && elapsedMs>=2400) {
        beginFrame();
        drawTitleArt(elapsedMs);
        localeText(lang("Press × to start","Нажмите × для начала"),272,226,1,White,true);
        localeText("v1.0.3b",142,250,0.85f);
        localeText("SH2K + Canny Crew + SandstoneDev, 2026",185,250,0.8f);
        const auto tick=menuTick==UINT32_MAX ?
            static_cast<unsigned>(sceKernelGetSystemTimeWide()*60/1000000) : menuTick;
        drawTv(thoughts.menuFrame(tick));
        endFrame();
        return;
    }
    if (flow.screen==Screen::ProfileSelect || flow.screen==Screen::ProfileCreate) {
        beginFrame();
        drawProfiles(flow.screen==Screen::ProfileCreate,elapsedMs,menuElapsedMs==UINT32_MAX?elapsedMs:menuElapsedMs);
        drawMenuTv();
        hardwareProbe.draw();
        endFrame();
        return;
    }
    if (flow.screen==Screen::LevelSelect) {
        beginFrame();
        drawLevelSelect(elapsedMs);
        drawAnimatedBack(elapsedMs,levelSelectArt.hasCleanBack() && profileArt.hasBackArt());
        drawMenuTv();
        drawGift(elapsedMs);
        endFrame();
        return;
    }
    if (flow.screen==Screen::Hole && round.wall418.bedActive) {
        const auto* pixels=clips.frame(Clip::EasyEnding,round.wall418.endingTicks*1000/60);
        beginFrame();
        if (pixels) drawFull(pixels);
        drawLiveStats(round,pixels);
        setTexture(GU_PSM_5650,VisualWidth,HudHeight,hud);
        drawStrokes(round.game.strokes);
        drawClock((round.ticks+round.wall418.endingTicks)*5/3,0,false);
        drawClock(totalClock,6,true);
        endFrame();
        return;
    }
    if (flow.screen==Screen::CosmicEnding) {
        drawCosmicEnding053(progress.totalPrisms(),&round);
        return;
    }
    if (flow.screen==Screen::ElysianEnding) {
        drawElysianEnding045(elapsedMs,fastEnding);
        return;
    }
    if (flow.screen==Screen::WorldResults) {
        const bool golfClear=worldResults.world()==CompletedWorld::GolfCentral;
        const bool cityClear=worldResults.world()==CompletedWorld::GlowstickCity;
        const bool parkClear=worldResults.world()==CompletedWorld::ChucklePark;
        const bool frontierClear=worldResults.world()==CompletedWorld::FinalFrontier;
        const Clip clip=frontierClear?(worldResults.firstUnlock()?Clip::WorldResultsFrontier:Clip::WorldResultsFrontierReplay):
                        parkClear?(worldResults.firstUnlock()?Clip::WorldResultsPark:Clip::WorldResultsParkReplay):
                        cityClear?(worldResults.firstUnlock()?Clip::WorldResultsGlowstick:Clip::WorldResultsGlowstickReplay):
                        golfClear?Clip::WorldResultsGolf:Clip::WorldResults;
        const auto* pixels=clips.frame(clip,elapsedMs);
        beginFrame();
        if (pixels) drawFull(pixels);
        drawLiveStats(round,pixels,false);
        setTexture(GU_PSM_5650,VisualWidth,HudHeight,hud);
        drawClock(round.totalTicks*5/3,6,true);
        if (elapsedMs>=worldResults.choiceTime()) {
            rectangle(ViewX+(worldResults.nextSelected()?352:181),263,34,2,Gold);
            if (golfClear && !worldResults.nextAvailable()) {
                rectangle(ViewX+333,210,75,62,GU_RGBA(19,15,24,238));
                localeText(lang("World 2","Мир 2"),370,224,0.78f,Gold,true);
                localeText(lang("Next build","В след. сборке"),370,243,0.68f,White,true);
            }
        }
        endFrame();
        return;
    }
    if (flow.screen==Screen::Settings) {
        const auto* pixels=clips.frame(Clip::SettingsBackdrop,elapsedMs,true);
        beginFrame();
        if (pixels) drawFull(pixels);
        drawSettings();
        hardwareProbe.draw();
        endFrame();
        return;
    }
    if (flow.screen==Screen::Credits) {
        const auto* pixels=clips.frame(Clip::Credits,0);
        beginFrame();
        if (pixels) drawFull(pixels);
        setTexture(GU_PSM_8888,256,128,menuLabels);
        patch({0,0,128,16,0,0},ViewX+4,78);
        if (russian()) localeText("○ Назад",5,98);
        else patch({112,16,86,16,0,0},ViewX+4,100);
        endFrame();
        return;
    }
    const unsigned page=static_cast<unsigned>(flow.screen);
    const bool result=flow.screen==Screen::Hole && (round.game.won || (round.caught && !round.waterDead && !round.backrooms && !round.finaleMineLoss));
    const bool menuScreen=flow.screen==Screen::Main || flow.screen==Screen::GameMenu;
    MenuScroll::Textures menuTextures{};
    const bool scrolled=menuScreen && clips.menu(menuScrollPack.c_str(),
                                                2*static_cast<unsigned>(russian())+(flow.screen==Screen::GameMenu),
                                                menuTextures);
    const Patch* selectedMenuButton=nullptr;
    if (scrolled && menuButtonArt.load(russian())) {
        const unsigned button=flow.screen==Screen::Main?static_cast<unsigned>(flow.mainOption):3;
        selectedMenuButton=&menuButtonArt.part(button,menuButtonDown);
    }
    const uint16_t* animated=nullptr;
    const bool horseGhostLoss=result && round.horseGhostCaught;
    const bool finaleHole=flow.screen==Screen::Hole && currentCourse==Elysian518Course;
    const bool cosmicHole=flow.screen==Screen::Hole && currentCourse>=CosmicFirstCourse;
    const bool cosmicNew=cosmicHole && currentCourse>=CosmicSecondCourse;
    const bool wizardHole=flow.screen==Screen::Hole && currentCourse==Elysian516Course;
    const bool wizardBlind=wizardHole && round.wizard.blind();
    const bool wizardMirror=wizardHole && round.wizard.mirrored();
    const auto resultId=result?resultClip(round.game.won,round.parkScare.lightsOut):Clip::Loading;
    const bool liveResult=result && !clips.count(resultId);
    const bool elysianOutOfBounds=liveResult && (currentCourse==Elysian511Course || currentCourse==Elysian515Course || finaleHole || cosmicNew) && round.fellOut;
    const bool commonResult=liveResult && !isPilotResultClip040(static_cast<unsigned>(resultId)) &&
                            !isNativeLossClip041(static_cast<unsigned>(resultId));
    if (!elysianOutOfBounds) elysianOutOfBoundsArt.closeStream();
    if (!liveResult) {
        liveResultArt.closeStream();
        commonResultArt.closeStream();
    }
    prepareFinaleDraw045(round,flow.screen==Screen::Hole,result);
    prepareCosmicDraw046(round,cosmicNew,result);
    if (cosmicNew && currentCourse>=CosmicSixteenthCourse) prepareCosmicDraw053(round,result);
    else if (cosmicNew && currentCourse>=CosmicFourteenthCourse) prepareCosmicDraw052(round,result);
    if (cosmicNew && (usesMetronome048(currentCourse) || currentCourse==CosmicSeventeenthCourse)) prepareRhythmDraw048(round,result);
    const bool starstorm=cosmicNew && currentCourse==CosmicFifthCourse;
    const bool weatherReady=prepareStarstormDraw047(starstorm);
    static unsigned resultVramRevision=0;
    if (resultVramRevision!=cosmicVramRevision046) {
        resultVramRevision=cosmicVramRevision046;
        liveResultLarge=liveResultClear=liveResultConfetti={};
    }
#ifdef UCG_VICTORY_PROBE
    const unsigned resultTime=victoryRenderSample.active?sceKernelGetSystemTimeLow():0;
#endif
    liveResultPrepared=liveResult && (elysianOutOfBounds?prepareElysianOutOfBounds041():
        prepareLiveResult(round.game.won,horseGhostLoss || cosmicHole,resultMs,commonResult,
                           finaleHole?HorseDefeatOffset518:LiveResultVramOffset,
                           currentCourse<TutorialCount && commonResultArt.hasPreloadedFrames()?
                               levelSelectArt.victoryPixels():nullptr));
    if (result && !liveResult) {
        const auto clipTime=round.horseBroadcast.active()?round.horseBroadcast.elapsedMs():resultMs;
        animated=clips.frame(horseGhostLoss?Clip::Loss:resultId,clipTime);
    } else if (flow.screen!=Screen::Hole && !scrolled) {
        animated=clips.frame(static_cast<Clip>(page),elapsedMs,flow.screen!=Screen::Loading);
    }
#ifdef UCG_VICTORY_PROBE
    if (victoryRenderSample.active)
        victoryRenderSample.resultMax=std::max(victoryRenderSample.resultMax,sceKernelGetSystemTimeLow()-resultTime);
#endif
    const bool elysianSwitchReady=flow.screen==Screen::Hole && (currentCourse==Elysian512Course || currentCourse==Elysian514Course) &&
                                  !round.backrooms && prepareElysianSwitch(round.switches.state);
    const bool elysianTreesReady=flow.screen==Screen::Hole && currentCourse==Elysian515Course &&
                                 !round.backrooms && prepareElysianTrees515();
    if (cosmicNew && result && (currentCourse==CosmicThirdCourse || currentCourse==CosmicFourthCourse))
        prepareCosmicResult046(round.switches.state);
    else if (cosmicNew && result && currentCourse==CosmicTwelfthCourse) {
        const auto name=std::string("cosmic-fallback-6_12-state")+std::to_string(round.switches.state)+".z";
        const auto path=assetPath(cosmicExecutable051,name.c_str());
        if (cosmicFallbackPath046!=path) {
            cosmicFallbackPath046=path;
            cosmicFallback046=nullptr;
        }
        prepareCosmicResult046(round.switches.state);
    } else if (cosmicNew && result && usesMetronome048(currentCourse))
        prepareRhythmResult048(round);
    const uint16_t* backdrop=scrolled?nullptr:(animated && !horseGhostLoss?animated:fallbackBackdrop(page));
    beginFrame();
    if (scrolled) drawMenuScroll(menuTextures,selectedMenuButton,
                                 menuElapsedMs==UINT32_MAX?elapsedMs:menuElapsedMs,elapsedMs);
    else if (backdrop) drawFull(backdrop);
    if (horseGhostLoss && animated) {
        setTexture(GU_PSM_5650,512,512,animated);
        patch({136,0,272,Height,0,0},ViewX+136,0);
    }
    if (flow.screen==Screen::Main) {
        if (scrolled && !russian()) {
            rectangle(ViewX+209,136,155,30,GU_RGBA(65,49,43,255));
            localeText("Take a break, play a minigame",211,134,0.70f,White,false,150,true);
            localeText("or try on a costume.",211,144,0.70f);
        }
        setTexture(GU_PSM_8888,256,128,menuLabels);
        patch({0,0,128,16,0,0},ViewX+4,78);

    }
    if (flow.screen==Screen::GameMenu) {

        if (russian()) {
            setTexture(GU_PSM_8888,256,128,menuLabels);
            patch({0,84,154,14,0,0},ViewX+209,168);
            patch({0,102,154,14,0,0},ViewX+209,217);
        }
    }
    if (flow.screen==Screen::Title || flow.screen==Screen::Main || flow.screen==Screen::GameMenu) {
        const auto tick=menuTick==UINT32_MAX ?
            static_cast<unsigned>(sceKernelGetSystemTimeWide()*60/1000000) : menuTick;
        drawTv(thoughts.menuFrame(tick));
    }
    if (flow.screen==Screen::Hole && !round.backrooms && isHorseCourse039(currentCourse))
        if (!finaleHole || !round.finale518.aftermath()) drawHorseMeter039(round.horse,round.totalTicks);
    if (result && animated) {
        setTexture(GU_PSM_5650,VisualWidth,HudHeight,hud);
        drawClock(totalClock,6,true);
        if (!hidesElysianScoring(currentCourse)) {
            drawStrokes(round.game.strokes);
            drawClock(levelClock,0,false);
            drawParBadge(backdrop,course.par,round.ticks*1000/60+resultMs);
        }
        drawLiveStats(round,backdrop,!hidesElysianScoring(currentCourse));
        if (!hidesElysianScoring(currentCourse) && round.game.won && resultMs>=2000)
            showResultStats(course,round,levelClock,resultMs);
    }
    if (flow.screen==Screen::Hole && (!result || !animated)) {
        const float horseZoom=isHorseCourse039(currentCourse)?round.horse.zoomPulse:1.0f;
        if (wizardBlind)
            rectangle(ViewX+GameplayView::HudWidth,0,ViewWidth-GameplayView::HudWidth,Height,GU_RGBA(0,0,0,255));
        else if (!finaleHole && backdrop && (wizardMirror || !result || (liveResult && round.game.won)))
            drawHorseZoom039(backdrop,horseZoom,wizardMirror);
        const GameplayView view(course,ViewX,currentCourse==MirrorCourse || wizardMirror,
                                finaleHole?finaleZoom518(round):horseZoom,finaleHole?finaleCameraShift518(round):currentCourse==CosmicFifteenthCourse?Vec2{0,-round.santa.cameraY}:Vec2{});
        if (finaleHole) drawFinaleField518(round,view);
        if (cosmicNew && result && (currentCourse==CosmicThirdCourse || currentCourse==CosmicFourthCourse || currentCourse==CosmicTwelfthCourse || usesMetronome048(currentCourse)))
            drawCosmicResult046();
        if (cosmicNew && currentCourse>=CosmicSixteenthCourse) drawCosmicField053(view,round,result);
        else if (cosmicNew && currentCourse>=CosmicFourteenthCourse) drawCosmicField052(view,round,result);
        else if (cosmicNew) drawCosmicField046(view,round);
        if (cosmicNew && !result && (currentCourse==CosmicSeventhCourse || currentCourse==CosmicEleventhCourse)) drawRhythmScenery048();
        if (!cosmicNew && cosmicHole) drawCosmicField601(round);
        if (elysianSwitchReady) {
            sceGuScissor(ViewX+GameplayView::HudWidth,0,ViewX+ViewWidth,Height);
            drawLiveResultSprite(elysianSwitchSlot512,0,&view,result && !round.game.won?1.0f:horseZoom);
            sceGuScissor(0,0,Width,Height);
        }
        setTexture(GU_PSM_5650,VisualWidth,HudHeight,hud);
        drawClock(totalClock,6,true);
        if (!hidesElysianScoring(currentCourse)) {
            drawStrokes(round.game.strokes);
            drawClock(levelClock,0,false);
            drawParBadge(backdrop,course.par,round.ticks*1000/60+resultMs);
        }
        drawLiveStats(round,backdrop,!hidesElysianScoring(currentCourse));
        sceGuScissor(ViewX+GameplayView::HudWidth,0,ViewX+ViewWidth,Height);
        if (currentCourse==PondCourse) {
            const unsigned frame=round.ticks*5/12%PondFrames;
            setTexture(GU_PSM_5650,512,256,pondAtlas);
            patch({static_cast<int16_t>(frame%6*80),static_cast<int16_t>(frame/6*80),78,78,0,0},
                  ViewX+140,191);
        }
        if (currentCourse==ChuckleCoasterCourse) {
            drawRideTrack033(round.ride);
            drawRideScenery033(view,round.ride);
            drawRideObjects033(view,round,false);
        }
        if (currentCourse==PoolCourse) {
            drawPoolWater(round);
            drawPoolRipples(view,round);
        }
        if (currentCourse>=GlowstickFirstCourse && currentCourse<ChuckleParkFirstCourse) drawGlowstick(round);
        else if (currentCourse==JumbleCourse) drawJumbleHouse(round);
        else {
            if (currentCourse==PursuitCourse) drawPursuitDecor(round.ticks);
            if (currentCourse!=RacetrackCourse) drawSwitchParkTiles(round);
            if (const auto* stage=multiDogStage(currentCourse)) {
                drawLateParkDecor(view,round);
                drawMultiDogs(view,round,*stage);
            }
            if (const auto* dogScene=dogSceneAt(currentCourse); dogScene && currentCourse!=CosmicSeventeenthCourse) drawDog(view,round,*dogScene);
            if (currentCourse==FrontierMineCourse && bindFrontierDogArt()) {
                setFrontierTexture(frontierDogArt);
                for (const auto& dog:round.mineDogs)
                    sharedActor(view,frontierDogArt.patch(11),dog.body.center);
            }
            if (currentCourse==Elysian513Course) {
                drawHorseTrails039(view,round.horse,elysianHazardTint040(currentCourse));
                setTexture(GU_PSM_8888,256,128,dogArt.texture());
                for (const auto& body:round.elysianTraffic.bodies)
                    actor(view,dogArt.patch(11),body.center,view.worldScale()*32.768f/22,
                          elysianHazardTint040(currentCourse),true);
            }
            if (currentCourse==CarouselCourse) drawCarouselDecal(view,round.ticks);
            if (currentCourse==DenseCourse && coasterSprings.pixels()) {
                setTexture(GU_PSM_5650,CoasterSpringArt::Width,CoasterSpringArt::Height,coasterSprings.pixels());
                for (unsigned i=0;i<CoasterSpringArt::Springs;++i) {
                    const auto& art=coasterSprings.patch(i,round.ticks);
                    patch(art,ViewX+art.x,art.y);
                }
            }
            if (hasCoaster(currentCourse)) drawParkCoaster(view,round);
            if (currentCourse==FactoryCourse) drawFactoryDecor(round);
            if (round.parkScare.lightsOut) {
                rectangle(ViewX+136,0,272,272,0xff000000u);
                setTexture(GU_PSM_8888,64,64,darkHoleArt.texture());
                const auto& hole=darkHoleArt.patch(0);
                patch(hole,ViewX+136+hole.x,hole.y);
            }
            if (currentCourse!=DenseCourse && currentCourse<PursuitCourse)
                drawGoalFlag(round,0,round.parkScare.lightsOut?1:0,0);
        }
        if (starstorm) drawStarstormRing047(view,round);
        else if (currentCourse==CosmicEleventhCourse && !result) drawCosmicRing611(view,round);
        else if (currentCourse>=ChuckleParkFirstCourse) drawParkObjects(view,round);
        if (currentCourse==SlotMachineCourse) drawSlots(view,round.slots);
        drawTerry(view,round);
        drawBlocks(view,round);
        if (currentCourse==DenseCourse || currentCourse==PursuitCourse ||
            (currentCourse>=BalloonDogCourse && currentCourse<=ScreenDogCourse) || currentCourse>=FinalFrontierFirstCourse)
            if (currentCourse!=ElysianRocksCourse && currentCourse!=HorseBroadcastCourse &&
                !isHorseCourse039(currentCourse) && !cosmicHole)
                drawGoalFlag(round,0,currentCourse==ScreenDogCourse?round.switches.state:0,0);
        if (currentCourse==CosmicEighthCourse) drawCosmicPortals608(view,round.ticks);
        if (currentCourse==CosmicEleventhCourse) drawCosmicPortals611(view,round.ticks);
        if (currentCourse==FrontierMazeCourse)
            drawFrontierPortals(view,frontierMazePortals,FrontierMazePortals,round.ticks);
        if (currentCourse==FrontierPortalCourse)
            drawFrontierPortals(view,frontierPortalArt,frontierPortals(currentCourse),round.ticks);
        if (currentCourse==Elysian515Course)
            drawFrontierPortals<ElysianPortals515Width,ElysianPortals515Height>(
                view,elysianPortalArt515,{round.elysianPortals515,std::size(round.elysianPortals515)},round.horse.age);
        if (currentCourse==WholesomeCourse)
            drawFrontierPortals(view,wholesomePortalArt,WholesomePortals,round.ticks);
        if (currentCourse==CataclysmCourse) {
            drawFrontierProps417(view,round);
            drawFrontierPortals(view,frontierProps417,{FrontierPortals417,std::size(FrontierPortals417)},
                                round.ticks,FrontierPortalPatch417,true);
        } else drawSwitchParkBlocks(view,round);
        if (currentCourse==TheWallCourse) drawWallObjects418(view,round);
        drawKeyDoors(view,round);
        if (currentCourse==CosmicTwelfthCourse) drawCosmicSwitches612(view,round);
        if (currentCourse==CosmicThirteenthCourse) drawCosmicKey613(view,round);
        if (finaleHole) drawMines518(view,round);
        if (currentCourse==CosmicThirteenthCourse) drawMovingGoal613(view,round,result?resultMs:0);
        else if (currentCourse>=CosmicSixteenthCourse) drawCosmicProps053(view,round,result);
        else if (currentCourse>=CosmicFourteenthCourse) drawCosmicGoals052(view,round);
        else if (cosmicNew) drawCosmicFlag046(round);
        else if (cosmicHole) {
            drawCosmicFlag601(view,round);
            drawCosmicGate601(view,round);
        }
        drawJumpPads(view,round);
        if (currentCourse==CosmicFifteenthCourse) drawSantaProps(view,round,result);
        drawCoins(view,round);
        if (frontierNukeItems().count) drawFrontierNukeItem(view,round);
        drawMotionTokens(view,round.speedStage,round.speedRun,speedTokens,round.ticks);
        if (round.stopStage) drawMotionTokens(view,*round.stopStage,round.stopRun,stopTokens,round.ticks,
                                              round.stopStage==&FrontierStop408);
        if (currentCourse==CrimsonOutpostCourse) {
            drawFrontierEnemies416(view,round);
        } else {
            drawSkulls(view,round);
            drawBaddieRun(view,round,round.baddies);
            if (currentCourse==CosmicTenthCourse) drawBaddieRun(view,round,round.cosmicSkulls);
            if (currentCourse>=CoasterCourse) drawChuckles(view,round);
        }
        drawStageLights(view,round);
        if (currentCourse==CosmicSeventeenthCourse && !result) drawHurrahLights053(view,round);
        if (round.controls.aiming) {
            aimLine(view,round.game.position,round.controls.drag,
                    length(round.controls.drag)>=course.minDrag);
        }
        setTexture(GU_PSM_8888,ActorsWidth,ActorsHeight,actors);
        const uint32_t actorTint=finaleHole?finaleTint518(round):wizardBlind?GU_RGBA(105,105,105,255):
            round.parkScare.lightsOut?ParkScareTint:starstorm?starstormTint047(White):currentCourse==CosmicTwelfthCourse?CosmicShade611:
            usesMetronome048(currentCourse)?cosmicShade048(currentCourse):elysianHazardTint040(currentCourse);
        if (!round.game.won && !chillMode && currentCourse!=ChuckleCoasterCourse && currentCourse!=CosmicEighteenthCourse &&
            (cosmicHole || !hasUnskippableFrontierHole(currentCourse)) &&
            (!isHorseCourse039(currentCourse) || (hasHorseCourseGhost040(currentCourse) && !round.horse.flying()))) {
            actor(view,visual.ghost,round.ghost.position,horseZoom,cosmicGhostTint048(round,0,actorTint),true);
            for (unsigned i=0;i<round.extraGhostCount;++i)
                actor(view,visual.ghost,round.extraGhosts[i].position,horseZoom,cosmicGhostTint048(round,i+1,actorTint),true);
        }
        if (!round.caught && (!finaleHole || round.finale518.phase!=FinalePhase518::Attract)) {
            const auto& activeJump=currentCourse==ChuckleCoasterCourse?round.ride.jump:round.jump;
            actor(view,visual.cat,round.game.position,activeJump.scale/JumpRun::BaseScale*horseZoom,actorTint,true);
            if (round.pool.swimming) {
                setTexture(GU_PSM_8888,256,64,poolAssets.sprites);
                sharedActor(view,{0,0,29,29,-17,-16},round.game.position);
                setTexture(GU_PSM_8888,ActorsWidth,ActorsHeight,actors);
            }
        }
        if (!round.game.won) {
            if (!chillMode && currentCourse!=ChuckleCoasterCourse && currentCourse!=CosmicEighteenthCourse && (cosmicHole || !hasUnskippableFrontierHole(currentCourse)) &&
                (!isHorseCourse039(currentCourse) || (hasHorseCourseGhost040(currentCourse) && !round.horse.flying()))) {
                drawGhostIndicator(view,round.ghost.position,cosmicGhostTint048(round,0,actorTint));
                for (unsigned i=0;i<round.extraGhostCount;++i)
                    drawGhostIndicator(view,round.extraGhosts[i].position,usesMetronome048(currentCourse)?cosmicGhostTint048(round,i+1,actorTint):White);
            }
        }
        if (currentCourse==ChuckleCoasterCourse) drawRideObjects033(view,round,true);
        if (wizardHole && !round.game.won)
            drawWizard516(view,round.wizard,wizardBlind?White:actorTint);
        if (finaleHole) drawFinaleHorse518(view,round);
        else if (isHorseCourse039(currentCourse)) {
            const uint32_t horseTint=wizardBlind?White:currentCourse==Elysian517Course?
                GU_RGBA(230,207,255,255):actorTint;
            drawHorseObjects039(view,round.horse,actorTint,currentCourse!=Elysian513Course,horseTint);
        }
        drawBubbles(view,round.ticks);
        drawRoundKeys(view,round);
        if (finaleHole) drawGoldenTee518(view,round);
        if (currentCourse==SlotMachineCourse && round.slots.outcome==SlotOutcome::Eye)
            drawSlotWorldEyes(view,round);
        drawGolfKey(view,round);
        drawSecretRunner(view,round);
        if (currentCourse==MartCourse) drawMarcySpill(view,round);
        drawNews(round);
        if (round.waterDead) drawSplash(view,round);
        if (currentCourse==PoolCourse) drawPoolSpeech(view,round);
        if (currentCourse==CarouselCourse) drawCarouselLight(view,round.ticks);
        if (currentCourse>=ChuckleParkFirstCourse) drawParkBalloons(view,round);
        if (currentCourse==PursuitCourse) drawPursuitBalloons(view,round.ticks);
        if (currentCourse==DogCourse) {
            drawDogScreen(round.ticks);
            drawGoalFlag(round,0,0,0);
        } else if (currentCourse==ScreenDogCourse) {
            drawDogScreen(round.ticks);
        }
        if (cosmicNew && !result) drawCosmicPlants046(view);
        if (currentCourse==RacetrackCourse) drawSwitchParkTiles(round);
        if (multiDogStage(currentCourse)) drawGoalFlag(round,0,0,0);
        if (currentCourse==MartCourse) drawMarcyHud(round);
        if (round.backrooms) {
            if (const auto* field=backroomsBackdrop()) {
                setTexture(GU_PSM_5650,512,512,field);
                patch({0,0,272,Height,0,0},ViewX+136,0);
            }
        }
        if (currentCourse==FrontierMazeCourse || currentCourse==FrontierDogCourse) drawFrontierShade0607(view);
        if (currentCourse==UncannyValleyCourse) drawFrontierValleyLight(view,round);
        if (currentCourse==FrontierPatrolCourse || currentCourse==FrontierMineCourse)
            drawFrontierLighting1011(view,round);
        if (currentCourse==MovieOnceCourse || currentCourse==AbandonedWorkspaceCourse)
            drawFrontierNight1415(view,round);
        else if (currentCourse==WeatherAlertCourse || currentCourse==DownfallCourse)
            drawFrontierNight1213(view,round);
        else if (currentCourse==CrimsonOutpostCourse || currentCourse==CataclysmCourse)
            drawFrontierLighting1617(view,round);
        if (currentCourse==TheWallCourse) drawWallLighting418(view,round);
        if (hidesElysianScoring(currentCourse)) drawElysianClouds501(round.ticks);
        if (currentCourse==CataclysmCourse) drawFrontierUfos417(view,round);
        if (liveResultPrepared && round.game.won && authoredConfetti040())
            drawLiveResultSprite(liveResultConfetti,0,commonResult?&view:nullptr,commonResult?horseZoom:1.0f);
        if (isHorseCourse039(currentCourse)) {
            const float coverage=round.horse.bounds/(256.0f*horseZoom);
            const unsigned cloudTick=round.horse.age+(liveResult && round.game.won?resultMs*60/1000:0);
            if (currentCourse==Elysian507Course)
                drawElysianCloudLayer039<HorseCloudOffset039>(elysianClouds507,cloudTick,coverage,-0.5f*coverage);
            else if (currentCourse==Elysian508Course)
                drawElysianCloudLayer039<HorseCloudOffset039>(elysianClouds508,round.horse.age,coverage,-0.5f*coverage);
            else if (currentCourse==Elysian510Course)
                drawElysianCloudLayer039<HorseCloudOffset039>(elysianClouds510,cloudTick,coverage,-0.5f*coverage);
            else if (currentCourse==Elysian509Course)
                drawElysianCloudLayer039<HorseCloudOffset039>(elysianClouds509,round.horse.age,coverage,-0.5f*coverage);
            else if (currentCourse==Elysian514Course)
                drawElysianCloudLayer039<HorseCloudOffset039>(elysianClouds514,cloudTick,coverage,-0.5f*coverage);
            else if (currentCourse==Elysian517Course)
                drawElysianCloudLayer039<HorseCloudOffset039>(elysianClouds514,cloudTick,coverage,-0.5f*coverage,8.0f/512);
            else if (finaleHole) {
                const float finaleCoverage=2/finaleZoom518(round);
                drawElysianCloudLayer039<HorseCloudOffset039>(frontierClouds501,round.totalTicks,finaleCoverage,
                    -0.5f*finaleCoverage,8.0f/512,false,-finaleCameraShift518(round).y/512);
            } else if (wizardHole)
                drawElysianCloudLayer039<HorseCloudOffset039>(frontierClouds501,cloudTick,coverage,-0.5f*coverage,0,wizardMirror);
            else drawElysianClouds501<HorseCloudOffset039>(cloudTick,coverage,-0.5f*coverage);
        }
        if (elysianTreesReady)
            drawLiveResultSprite(elysianSwitchSlot512,0,&view,result && !round.game.won?1.0f:horseZoom);
        sceGuScissor(ViewX,0,ViewX+ViewWidth,Height);
        if (currentCourse>=ChuckleParkFirstCourse) drawParkUfo(round);
        if (currentCourse==MovieOnceCourse) drawFrontierSnow414(round.ticks,view);
        else if (wizardHole) drawWizardMist516(round.wizard.mistTime());
        else if (currentCourse==Elysian514Course || currentCourse==Elysian515Course) drawElysianMist043(round.horse.age);
        else if (currentCourse==TheWallCourse) drawSnow033(round.ticks,-1.0f,1);
        else if (currentCourse>=FinalFrontierFirstCourse && currentCourse<TheWallCourse)
            drawSnow033(round.ticks,round.frontierWind.active?frontierSnowSeconds(round.ticks):-1.0f);
        if (!result && !hidesElysianScoring(currentCourse)) drawStageIntro(round.ticks);
        if (wizardHole && !result) drawWizardSpell516(round.wizard);
        if (finaleHole) drawTeeFade518(round);
        if (starstorm && weatherReady) drawStarstorm047(round.ticks);
        if (!result && !round.backrooms && (usesMetronome048(currentCourse) || currentCourse==CosmicSeventeenthCourse)) drawRhythmHud048(round);
        sceGuScissor(0,0,Width,Height);
    }
    if (liveResult) {
        const float slide=round.game.won?liveResultSlide(resultMs):0;
        if (liveResultPrepared) {
            sceGuScissor(ViewX+GameplayView::HudWidth,0,ViewX+ViewWidth,Height);
            if (!elysianOutOfBounds || (resultMs/200)%2==0) drawLiveResultSprite(liveResultLarge,slide);
            if (round.game.won) drawLiveResultSprite(liveResultClear,slide);
            sceGuScissor(0,0,Width,Height);
        }
        if (round.game.won && !hidesElysianScoring(currentCourse) && (liveResultPrepared || resultMs>=2000))
            showResultStats(course,round,levelClock,resultMs,!liveResultPrepared,slide);
    }
    if (flow.screen==Screen::Hole) {
#ifdef UCG_VICTORY_PROBE
        const unsigned tvStart=victoryRenderSample.active?sceKernelGetSystemTimeLow():0;
#endif
        drawThoughtAndKey(round);
#ifdef UCG_VICTORY_PROBE
        if (victoryRenderSample.active)
            victoryRenderSample.tvMax=std::max(victoryRenderSample.tvMax,sceKernelGetSystemTimeLow()-tvStart);
#endif
        if (currentCourse==CosmicSeventeenthCourse) drawHurrahHud053(round);
    }
    if (flow.screen==Screen::Hole) {
        drawGiftIndicators(round.totalTicks*1000/60);
        if (frontierNukeItems().count && !result) drawFrontierNukeExplosions(round);
        if (currentCourse==SlotMachineCourse) drawSlotScreenEffect(round.slots);
        if (currentCourse==LightsCourse && !result) drawParkAsh(round);
        if (round.paused) drawPause(round);
    }
    endFrame();
}

Vec2 aimInput(const SceCtrlData& pad) {
    const float x=static_cast<float>(!!(pad.Buttons&PSP_CTRL_RIGHT)-!!(pad.Buttons&PSP_CTRL_LEFT));
    const float y=static_cast<float>(!!(pad.Buttons&PSP_CTRL_DOWN)-!!(pad.Buttons&PSP_CTRL_UP));
    if (x!=0 || y!=0) return {x,y};
    return {(pad.Lx-128)/127.0f,(pad.Ly-128)/127.0f};
}

void persistProgress(SaveFile& save,const Round& round,const char* log) {
    progress.ticks=round.totalTicks;
    if (!save.write(progress)) logEvent(log,"save_error",round.game);
    else xmbProfile.queue(activeProfile,profiles.entry(activeProfile),progress);
}

TerryEvent interactTerry(Round& round,SaveFile& save,const char* log) {
    if (currentCourse!=1 || round.caught || round.game.won) return TerryEvent::Locked;
    const auto event=round.terry.click(progress,round.totalTicks);
    if (event==TerryEvent::Locked) return event;
    thoughts.play(Thought::Terry,round.tvTick());
    if (event==TerryEvent::Grow) {
        audio.play(Sound::TerryClick);
        logEvent(log,"terry_click",round.game);
    } else {
        audio.play(Sound::SecretKey);
        persistProgress(save,round,log);
        logEvent(log,"terry_key",round.game);
    }
    return event;
}

void recordCourseWin(unsigned index,Score score,unsigned clock,bool advance) {
    if (hidesElysianScoring(index)) {
        progress.best[index].rank=5;
        if (advance) progress.next=std::max(progress.next,static_cast<uint32_t>(index+1));
    } else {
        progress.record(index,score,clock,advance);
    }
}

void acceptGoldenTee045(const Course& course,Round& round,SaveFile& save,const char* log) {
    recordCourseWin(currentCourse,scoreFor(course,round),round.ticks*5/3,false);
    round.beginGoldenTee518();
    persistProgress(save,round,log);
    logEvent(log,"golden_tee_drop",round.game);
}

enum class Advance { Failed, Fatal, NextHole, WorldCleared, ElysianEnding, CosmicEnding, EndOfBuild };

Advance advanceCampaign(const char* executable,Course& course,Round& round,SaveFile& save,const char* log,bool skipping=false) {
    const unsigned completed=currentCourse;
    const Score score=scoreFor(course,round);
    const unsigned clock=round.ticks*5/3;
    const unsigned next=completed+1;
    const bool firstUnlock=progress.next<next;
    if (completed==Elysian518Course) {
        if (skipping || !round.finale518.canEnterEnding()) return Advance::Failed;
        progress.next=std::max(progress.next,static_cast<uint32_t>(CosmicFirstCourse));
        persistProgress(save,round,log);
        audio.musicTune(1,1);
        audio.muteMusic();
        logEvent(log,"elysian_world_clear",round.game);
        return Advance::ElysianEnding;
    }
    const auto completedWorld=worldCompletedAt(next);
    const bool clear=completedWorld!=CompletedWorld::None;
    if (skipping && cannotSkipCourse(completed)) return Advance::Failed;
    if (completed==CosmicEighteenthCourse) {
        if (!round.game.won) return Advance::Failed;
        recordCourseWin(completed,score,clock,true);
        persistProgress(save,round,log);
        audio.muteMusic();
        startCosmicEnding053(!firstUnlock);
        logEvent(log,"campaign_complete",round.game);
        return Advance::CosmicEnding;
    }
    if (completed==CosmicSeventeenthCourse) stopLastHurrahSounds053();
    if (skipping) logEvent(log,"skip",round.game);
    if (!clear && next<playableCourseCount && !enterCourse(executable,next,course,round)) {
        logEvent(log,"course_load_error",round.game);
        return loadCourseAt(executable,completed,course) ? Advance::Failed : Advance::Fatal;
    }
    if (skipping) {
        progress.next=std::max(progress.next,static_cast<uint32_t>(next));
        audio.play(Sound::Skip);
    } else {
        recordCourseWin(completed,score,clock,true);
    }
    persistProgress(save,round,log);
    if (clear) {
        if (completed==ChuckleCoasterCourse) {
            stopRideAudio033();
            audio.rideVolume(1,-80,-80);
        }
        worldResults.begin(completedWorld,playableCourseCount,firstUnlock);
        logEvent(log,"world_clear",round.game);
        return Advance::WorldCleared;
    }
    if (next>=playableCourseCount) {
        logEvent(log,"end_of_build",round.game);
        return Advance::EndOfBuild;
    }
    if (completed==HorseBroadcastCourse) audio.stop(Sound::HorseAlarm);
    if (completed==HorseBroadcastCourse || courseMusic(completed)!=courseMusic(next)) startCourseMusic(next);
    logEvent(log,"advance",round.game);
    logEvent(log,"hole",round.game);
    return Advance::NextHole;
}

#ifdef UCG_VISUAL_TEST
bool dumpFrame(const char* executable,const char* name) {
    const auto* vram=reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(sceGeEdramGetAddr())|0x40000000u);
    const std::string path=statePath(executable,name);
    std::FILE* f=std::fopen(path.c_str(),"wb");
    if (!f) return false;
    const bool ok=std::fwrite(vram,FrameBytes,1,f)==1;
    std::fclose(f);
    return ok;
}

#ifdef UCG_TEST_019_ONLY
#include "focused019.hpp"
#elif defined(UCG_TEST_038_FRONTIER)
#include "focused038.hpp"
#elif defined(UCG_TEST_053)
#include "focused053.hpp"
#elif defined(UCG_TEST_052)
#include "focused052.hpp"
#elif defined(UCG_TEST_051)
#include "focused051.hpp"
#elif defined(UCG_TEST_050)
#include "focused050.hpp"
#elif defined(UCG_TEST_049)
#include "focused049.hpp"
#elif defined(UCG_TEST_048)
#include "focused048.hpp"
#elif defined(UCG_TEST_047)
#include "focused047.hpp"
#elif defined(UCG_TEST_046)
#include "focused046.hpp"
#elif defined(UCG_TEST_045)
#include "focused045.hpp"
#elif defined(UCG_TEST_044)
#include "focused044.hpp"
#elif defined(UCG_TEST_043)
#include "focused043.hpp"
#elif defined(UCG_TEST_042)
#include "focused042.hpp"
#elif defined(UCG_TEST_041)
#include "focused041.hpp"
#elif defined(UCG_TEST_040)
#include "focused040.hpp"
#elif defined(UCG_TEST_039)
#include "focused039.hpp"
#elif defined(UCG_TEST_037_ONLY)
#include "focused037.hpp"
#elif defined(UCG_TEST_028_ONLY)
#include "focused028.hpp"
#elif defined(UCG_TEST_029_ONLY)
#include "focused029.hpp"
#elif defined(UCG_TEST_030_ONLY)
#include "focused030.hpp"
#elif defined(UCG_TEST_031_ONLY)
#include "focused031.hpp"
#elif defined(UCG_TEST_032_ONLY)
#include "focused032.hpp"
#elif defined(UCG_TEST_033_FRONTIER)
#include "focused033_frontier.hpp"
#elif defined(UCG_TEST_033_ONLY)
#include "focused033.hpp"
#elif defined(UCG_TEST_035_FRONTIER)
#include "focused035_frontier.hpp"
#elif defined(UCG_TEST_036_FRONTIER)
#include "focused036_frontier.hpp"
#elif defined(UCG_TEST_037_FRONTIER)
#include "focused037_frontier.hpp"
#elif defined(UCG_TEST_036_ONLY)
#include "focused036.hpp"
#elif defined(UCG_TEST_035_ONLY)
#include "focused035.hpp"
#elif defined(UCG_TEST_034_ONLY)
#include "focused034.hpp"
#endif


[[maybe_unused]] bool runVisualTest(const char* executable,Course& course,SaveFile& save,const LanguageFile& config,
                   const BackgroundFile& backgroundFile,const char* log) {
    if (activeBackground!=1 || backgroundPreference.random) return false;
    Flow flow;
    flow.loadingPromptMs=clips.duration(Clip::Loading);
    flow.loadingMs=std::max(7500u,flow.loadingPromptMs);
    Round round(course);
    constexpr const char* names[]={"loading","title","main-menu","game-menu","win","loss",
                                    "win-0_02","loss-0_02","win-0_03","loss-0_03",
                                    "win-0_04","loss-0_04","win-0_05","loss-0_05","world",
                                    "win-1_01","loss-1_01","win-1_02","loss-1_02",
                                    "win-1_03","loss-1_03","win-1_04","loss-1_04","win-1_05","loss-1_05",
                                    "win-1_06","loss-1_06","win-1_07","loss-1_07","win-1_08","loss-1_08",
                                    "win-1_09","loss-1_09","win-1_10","loss-1_10",
                                    "win-1_11","loss-1_11","win-1_12","loss-1_12",
                                    "win-1_13","loss-1_13","win-1_14","loss-1_14","win-1_15","loss-1_15","win-1_16","loss-1_16","win-1_17","loss-1_17","win-1_18","loss-1_18","credits-reference","world-results-golf","win-2_01","loss-2_01","win-2_02","loss-2_02","win-2_03","loss-2_03","win-2_04","loss-2_04","win-2_05","loss-2_05","settings-backdrop","win-2_06","loss-2_06","win-2_07","loss-2_07","win-2_08","loss-2_08","win-2_09","loss-2_09","win-2_10","loss-2_10","win-2_11","loss-2_11","win-2_12","loss-2_12","win-2_13","loss-2_13","win-2_14","loss-2_14","win-2_15","loss-2_15","win-2_16","loss-2_16","win-2_17","loss-2_17","win-2_18","loss-2_18","win-3_01","loss-3_01","world-results-city","world-results-city-replay","win-3_02","loss-3_02","win-3_03","loss-3_03","win-3_04","loss-3_04","win-3_05","loss-3_05","win-3_06","loss-3_06","win-3_07","loss-3_07","win-3_08","loss-3_08","win-3_09","loss-3_09","win-3_10","loss-3_10","win-3_11","loss-3_11","loss-3_10-dark","win-3_12","loss-3_12","win-3_13","loss-3_13","win-3_14","loss-3_14","win-3_15","loss-3_15","win-3_16","loss-3_16","win-3_17","loss-3_17","win-3_18","loss-3_18","win-4_01","loss-4_01","world-results-park","world-results-park-replay","win-4_02","loss-4_02","win-4_03","loss-4_03","win-4_04","loss-4_04","win-4_05","loss-4_05","win-4_06","loss-4_06","win-4_07","loss-4_07","win-4_08","loss-4_08","win-4_09","loss-4_09","win-4_10","loss-4_10","win-4_11","loss-4_11","win-4_12","loss-4_12","win-4_13","loss-4_13","win-4_14","loss-4_14","win-4_15","loss-4_15","win-4_16","loss-4_16","win-4_17","loss-4_17","win-4_18","loss-4_18","win-5_01","loss-5_01",
        "world-results-frontier","world-results-frontier-replay","easy-ending","win-5_02","loss-5_02","fake-win-5_03","loss-5_03",
        "win-5_04","loss-5_04","win-5_05","loss-5_05","win-5_06","loss-5_06","win-5_07","loss-5_07",
        "win-5_08","loss-5_08","win-5_09","loss-5_09","win-5_10","loss-5_10","win-5_11","loss-5_11","win-5_12","loss-5_12","win-5_13","loss-5_13","win-5_14","loss-5_14","win-5_15","loss-5_15","win-5_16","loss-5_16","win-5_17","loss-5_17","win-5_18","loss-5_18","win-6_01","loss-6_01","win-6_02","loss-6_02","win-6_03","loss-6_03","win-6_04","loss-6_04","win-6_05","loss-6_05","win-6_06","loss-6_06","win-6_07","loss-6_07","win-6_08","loss-6_08","win-6_09","loss-6_09","win-6_10","loss-6_10","win-6_11","loss-6_11","win-6_12","loss-6_12","win-6_13","loss-6_13","win-6_14","loss-6_14","win-6_15","loss-6_15","win-6_16","loss-6_16","win-6_17","loss-6_17","win-6_18","loss-6_18"};
    static_assert(std::size(names)==static_cast<unsigned>(Clip::Count));
    for (unsigned i=0;i<static_cast<unsigned>(Clip::Count);++i) {
        const auto id=static_cast<Clip>(i);
        const unsigned count=clips.count(id);
        if (!count) continue;
        for (unsigned index:{0u,count/2,count-1}) {
            const uint16_t* pixels=clips.frame(id,index*50);
            if (!pixels) return false;
            beginFrame();
            drawFull(pixels);
            endFrame();
            char filename[48];
            std::snprintf(filename,sizeof filename,"%s-%03u.565",names[i],index);
            if (!dumpFrame(executable,filename)) return false;
        }
    }
    Flow loading=flow;
    loading.update(flow.loadingPromptMs-1,true,false,false,false);
    if (loading.screen!=Screen::Loading) return false;
    loading.update(flow.loadingPromptMs,true,false,false,false);
    if (loading.screen!=Screen::Title) return false;
    flow.update(flow.loadingMs,false,false,false,false);
    if (flow.screen!=Screen::Title) return false;
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Main) return false;
    render(flow,course,round,2500);
    if (!dumpFrame(executable,"menu-credit.565")) return false;
    flow.update(0,false,false,false,false,true);
    if (flow.screen!=Screen::Credits) return false;
    render(flow,course,round,0);
    if (!dumpFrame(executable,"credits-screen.565")) return false;
    flow.update(0,false,true,false,false);
    if (flow.screen!=Screen::Main) return false;
    logEvent(log,"test_credits_route",round.game);
    audio.play(Sound::Select);
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::GameMenu) return false;
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::ProfileSelect) return false;
    openLevelSelect(flow);
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Hole) return false;
    logEvent(log,"test_menu_route",round.game);
    round.togglePause();
    const unsigned beforePause=round.totalTicks;
    round.step();
    if (round.totalTicks!=beforePause || round.ticks!=0) return false;
    render(flow,course,round,0,0,0);
    if (!dumpFrame(executable,"pause-screen.565")) return false;
    round.cleanPause=true;
    render(flow,course,round,0,0,0);
    if (!dumpFrame(executable,"pause-clean.565")) return false;
    round.togglePause();
    if (round.cleanPause) return false;
    logEvent(log,"test_pause_resume",round.game);
    round.ghost.position={visual.referenceGhostX,visual.referenceGhostY};
    round.ticks=65;
    render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
    if (!dumpFrame(executable,"stage-moving.565")) return false;
    round.ticks=360;
    render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
    if (!dumpFrame(executable,"stage-settled.565")) return false;
    round.restart();
    audio.play(Sound::Shot);
    for (int i=0;i<58;++i) sceDisplayWaitVblankStart();

    const auto shoot=[&](Vec2 aim) {
        round.controls.update(round.game,course,aim,true,false);
        const bool hit=round.controls.update(round.game,course,{},false,false)==ControlEvent::Shot;
        if (hit) round.coins.multiplier=0;
        return hit;
    };
    const auto reachBackrooms=[&]() {
        round.game.position={275,0};
        if (!shoot({0.5f,0})) return false;
        for (unsigned i=0;i<30 && !round.caught && round.game.moving();++i) round.step();
        return round.backrooms && round.caught && !round.waterDead && !round.game.won &&
               !round.game.moving() && round.game.position.x>BackroomsLimit;
    };
    if (!shoot({-1,0})) return false;
    audio.play(Sound::Shot);
    logEvent(log,"test_shot",round.game);
    for (int i=0;i<30 && round.game.bounces==0;++i) round.step();
    if (round.game.bounces==0) return false;
    audio.play(Sound::Bounce);
    logEvent(log,"test_bounce",round.game);
    round.restart();
    if (!shoot({0.57f,0.84f})) return false;
    for (int i=0;i<180 && !round.game.won && !round.caught;++i) round.step();
    if (!round.game.won) return false;
    render(flow,course,round,0,round.ticks*5/3,round.totalTicks*5/3,clips.duration(Clip::Win)-50);
    if (!dumpFrame(executable,"actual-win.565")) return false;
    audio.play(Sound::Win);
    logEvent(log,"test_goal",round.game);
    if (round.controls.update(round.game,course,{},false,true)!=ControlEvent::Restart) return false;
    round.restart();
    logEvent(log,"test_restart_after_goal",round.game);
    for (int i=0;i<1500 && !round.caught;++i) round.step();
    if (!round.caught) return false;
    render(flow,course,round,0,round.ticks*5/3,round.totalTicks*5/3,0);
    if (!dumpFrame(executable,"actual-loss.565")) return false;
    audio.play(Sound::Loss);
    logEvent(log,"test_caught",round.game);
    if (round.controls.update(round.game,course,{},false,true)!=ControlEvent::Restart) return false;
    round.restart();
    logEvent(log,"test_restart_after_loss",round.game);

    const auto finishShot=[&](Vec2 aim,bool expectBounce,bool expectSand=false) {
        if (!shoot(aim)) return false;
        logEvent(log,"test_route_shot",round.game);
        bool touchedSand=false;
        for (int i=0;i<210 && !round.game.won && !round.caught;++i) {
            round.step();
            touchedSand|=round.inSand;
        }
        if (expectSand && !touchedSand) return false;
        if (touchedSand) logEvent(log,"test_sand_route",round.game);
        if (!round.game.won || (expectBounce && !round.game.bounces)) {
            logEvent(log,round.caught?"test_route_caught":"test_route_missed",round.game);
            return false;
        }
        logEvent(log,"test_route_goal",round.game);
        return true;
    };
    if (!finishShot({0.57f,0.84f},false)) return false;
    const Vec2 aims[]={{0.503f,0.308f},{0.036913f,0.845445f},{0,-0.59f},{0.103510f,0.736512f}};
    const Vec2 goals[]={{176,0},{144,176},{0,-192},{-160,-160}};
    for (unsigned i=0;i<std::size(aims);++i) {
        if (canAdvance(RankAnimationEndMs-1) || !canAdvance(RankAnimationEndMs) ||
            advanceCampaign(executable,course,round,save,log)!=Advance::NextHole ||
            round.game.strokes || round.game.won || course.goal.x!=goals[i].x || course.goal.y!=goals[i].y) return false;
        if (currentCourse==3) {
            Progress resumed{};
            if (!save.read(resumed) || resumed.next!=3 || resumed.best[2].score!=progress.best[2].score) return false;
            logEvent(log,"test_resume_0_04",round.game);
        }
        round.ticks=86;
        if (golf.stages[i].bubbleCount || i>=4) round.ghost.position={visual.referenceGhostX,visual.referenceGhostY};
        render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
        char filename[32];
        std::snprintf(filename,sizeof filename,"hole-%s.565",CourseIds[currentCourse]);
        if (!dumpFrame(executable,filename)) return false;
        if (currentCourse==1) {
            if (interactTerry(round,save,log)!=TerryEvent::Locked) return false;
            progress.quest=2;
            for (unsigned n=0;n<15;++n) {
                if (interactTerry(round,save,log)!=TerryEvent::Grow) return false;
            }
            render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
            if (!dumpFrame(executable,"terry-grown.565")) return false;
            if (interactTerry(round,save,log)!=TerryEvent::Key ||
                interactTerry(round,save,log)!=TerryEvent::Locked) return false;
            round.totalTicks+=3;
            render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
            if (!dumpFrame(executable,"terry-key.565")) return false;
        }
        round.restart();
        if (!finishShot(aims[i],i!=2)) return false;
        if (i>=2) {
            if (round.coins.bonus<50 || round.coins.empty()) return false;
            if (std::FILE* f=std::fopen(log,"a")) {
                std::fprintf(f,"build=053 event=test_coin course=%s coins=%u bonus=%u\n",
                             CourseIds[currentCourse],round.coins.multiplier,round.coins.bonus);
                std::fclose(f);
            }
            render(flow,course,round,0,round.ticks*5/3,round.totalTicks*5/3,clips.duration(resultClip(true))-50);
            std::snprintf(filename,sizeof filename,"result-%s.565",CourseIds[currentCourse]);
            if (!dumpFrame(executable,filename)) return false;
        }
        if (round.controls.update(round.game,course,{},false,true)!=ControlEvent::Restart) return false;
        round.restart();
        if (round.coins.bonus || !round.coins.empty()) return false;
        logEvent(log,"test_route_restart",round.game);
        if (!finishShot(aims[i],i!=2)) return false;
    }
    if (advanceCampaign(executable,course,round,save,log)!=Advance::WorldCleared) return false;
    flow.screen=Screen::WorldResults;
    render(flow,course,round,8950);
    if (!dumpFrame(executable,"world-results.565")) return false;
    Progress loaded{};
    if (!save.read(loaded) || loaded.next!=TutorialCount || loaded.terryKey!=1 ||
        loaded.totalScore()!=progress.totalScore() || loaded.totalPrisms()!=progress.totalPrisms() ||
        loaded.best[3].score<=8000 || loaded.best[4].score<=8000) return false;
    logEvent(log,"test_progress_reloaded",round.game);
    leaveWorldResults(flow,false);
    if (flow.screen!=Screen::LevelSelect || menuCourse!=currentCourse) return false;
    flow.screen=Screen::WorldResults;
    leaveWorldResults(flow,true);
    if (flow.screen!=Screen::Hole || menuCourse!=TutorialCount ||
        !enterCourse(executable,menuCourse,course,round)) return false;
    audio.music(Sound::GolfCentral);
    logEvent(log,"test_next_world",round.game);

    const Vec2 golfAims[]={{0.593144f,0.603588f},{-0.729063f,-0.317006f},
                           {-0.672850f,-0.507029f},{0.475f,0}};
    const Vec2 golfGoals[]={{160,-160},{16,0},{192,-104},{192,0},{-160,224},{216,88},{16,-16},{0,-176},
                            {176,0},{-192,192},{192,192},{-224,224},{-224,0},{0,176},{0,192},{355,208},{384,384},{224,-224}};
    const Vec2 shots15[]={{-0.90445805f,-0.06324580f},{-0.96126169f,0.27563736f}};
    const Vec2 shots16[]={{-0.32605916f,0.89584023f},{0.75123686f,0.58693057f},{0.30000001f,0}};
    const Vec2 shots17[]={{-0.97814763f,0.20791161f},{0.49999997f,0.86602545f},{0.86602539f,0.5f},
                         {0.64278764f,0.76604444f},{0.87154388f,0.24991120f},{0.62628490f,-0.02187042f},
                         {0.99756408f,-0.06975638f},{0.95333326f,0},{0.10452836f,-0.99452192f},
                         {-0.13917325f,-0.99026805f},{-0.99939084f,-0.03489963f},{-0.58730084f,0.49280369f}};
    const Vec2 shots19[]={{0.05574849f,0.53041166f},{-0.40673664f,-0.91354543f}};
    const Vec2 shots113[]={{-0.47879738f,-0.76623654f},{0.80656719f,0.02816595f}};
    const Vec2 shots114[]={{-0.63685477f,-0.70729905f},{0.97029573f,0.24192192f}};
    const Vec2 shots115[]={{-0.16527222f,0.93730521f},{0.55943346f,-0.76999366f},
                           {-0.03321608f,0.95118493f},{-0.06639187f,0.94944620f}};
    const Vec2 shots116[]={{0.54977250f,0.65519327f},{0.95176470f,0.0f},{0.90132850f,0.06302702f}};
    const Vec2 shots117[]={{0.84697044f,0.11903394f},{-0.94944626f,0.06639152f}};
    const Vec2 shot118{0.55995119f,0.43748182f};
    const auto playRoute=[&](const Vec2* aims,unsigned count) {
        for (unsigned shot=0;shot+1<count;++shot) {
            if (!shoot(aims[shot])) return false;
            logEvent(log,"test_route_shot",round.game);
            for (unsigned tick=0;tick<210 && round.game.moving() && !round.caught;++tick) round.step();
            if (round.game.moving() || round.caught || round.game.won) {
                logEvent(log,round.caught?"test_route_caught":"test_route_interrupted",round.game);
                return false;
            }
            if (currentCourse==FirstKeyDoorCourse && round.roundKeys.opened) {
                render(flow,course,round,0,round.ticks*5/3,round.totalTicks*5/3);
                if (!dumpFrame(executable,"doors-open-1_13.565")) return false;
            }
        }
        return finishShot(aims[count-1],true);
    };
    static_assert(std::size(golfGoals)==GolfStages);
    for (unsigned i=0;i<GolfStages;++i) {
        if (course.goal.x!=golfGoals[i].x || course.goal.y!=golfGoals[i].y) return false;
        round.ticks=currentCourse==PondCourse?116:86;
        if (golf.stages[i].bubbleCount || i>=4) round.ghost.position={visual.referenceGhostX,visual.referenceGhostY};
        render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
        char filename[32];
        std::snprintf(filename,sizeof filename,"hole-%s.565",CourseIds[currentCourse]);
        if (!dumpFrame(executable,filename)) return false;
        round.ticks=currentCourse==PondCourse?130:146;
        render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
        std::snprintf(filename,sizeof filename,"bubble-%s.565",CourseIds[currentCourse]);
        if (!dumpFrame(executable,filename)) return false;
        round.restart();
        const auto& collectibles=objects.data.courses[currentCourse];
        for (unsigned coin=0;coin<collectibles.count;++coin) {
            if (i>=14) round.ghost.position={-900,-900};
            round.game.position=collectibles.coins[coin];
            round.step();
        }
        if ((i==1 || i==2) && (collectibles.count!=6 || round.coins.bonus!=375)) return false;
        if (i>=4 && i<=6) {
            const unsigned expectedCount=i==4?21:13, expectedBonus=i==4?1825:1025;
            if (collectibles.count!=expectedCount || round.coins.collected[0]!=(1u<<expectedCount)-1 ||
                round.coins.bonus!=expectedBonus) return false;
            if (std::FILE* file=std::fopen(log,"a")) {
                std::fprintf(file,"build=053 event=test_full_coin_set course=%s coins=%u bonus=%u\n",
                             CourseIds[currentCourse],expectedCount,expectedBonus);
                std::fclose(file);
            }
        }
        if (i>=14) {
            constexpr unsigned counts[]={28,38,0,160}, bonuses[]={2525,3525,0,15725};
            const unsigned count=counts[i-14], bonus=bonuses[i-14];
            if (collectibles.count!=count || round.coins.bonus!=bonus) return false;
            for (unsigned coin=0;coin<count;++coin) if (!round.coins.has(coin)) return false;
            if (std::FILE* file=std::fopen(log,"a")) {
                std::fprintf(file,"build=053 event=test_full_coin_set course=%s coins=%u bonus=%u\n",
                             CourseIds[currentCourse],count,bonus);
                std::fclose(file);
            }
        }
        if (collectibles.count) logEvent(log,"test_golf_coins",round.game);
        round.restart();
        if (golf.stages[i].sandCount) {
            round.game.position=i==12?Vec2{64,196}:golf.stages[i].sand[0].points[0];
            round.game.velocity={400,0};
            Game normal(course,&walls);
            normal.position=round.game.position;
            normal.velocity=round.game.velocity;
            normal.step();
            round.step();
            const float ratio=length(round.game.velocity)/length(normal.velocity);
            if (!round.inSand || ratio<0.82f || ratio>0.86f) return false;
            logEvent(log,"test_sand_damping",round.game);
            round.restart();
        }
        if (i==3) {
            round.game.position={105,-160};
            round.step();
            if (!round.waterDead || !round.caught || round.game.won) return false;
            for (unsigned j=0;j<13;++j) round.step();
            render(flow,course,round,0,0,0);
            if (!dumpFrame(executable,"water-splash.565")) return false;
            logEvent(log,"test_water",round.game);
            if (round.controls.update(round.game,course,{},false,true)!=ControlEvent::Restart) return false;
            round.restart();
            if (round.waterDead || round.brokenBlocks || round.blockBonus) return false;
            if (!shoot({-0.9475f,0})) return false;
            unsigned effectTick=0;
            for (unsigned j=0;j<210 && round.game.moving();++j) {
                round.step();
                if (!effectTick && round.brokenBlocks) effectTick=round.ticks+8;
                if (effectTick && round.ticks==effectTick) {
                    render(flow,course,round,0,round.ticks*5/3,round.totalTicks*5/3);
                    if (!dumpFrame(executable,"block-broken.565")) return false;
                }
            }
            if (round.game.moving() || round.caught || round.game.won || round.brokenBlocks!=0x66 ||
                round.blockMultiplier!=4 || round.blockBonus!=100 || !round.game.bounces) return false;
            logEvent(log,"test_blocks",round.game);
        }
        if (i==6) {
            const auto& stage=golf.stages[i];
            if (!stage.hasKey || progress.quest!=2 || progress.golfKey) return false;
            progress.quest=1;
            round.game.position=stage.keyTrigger;
            round.step();
            if (progress.golfKey) return false;
            progress.quest=2;
            round.restart();
            round.game.position=stage.keyTrigger;
            round.step();
            if (!progress.golfKey || !round.questKeyTick) return false;
            persistProgress(save,round,log);
            Progress withKey{};
            if (!save.read(withKey) || withKey.terryKey!=1 || withKey.golfKey!=1) return false;
            logEvent(log,"test_golf_key_saved",round.game);
            round.totalTicks+=20;
            render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
            if (!dumpFrame(executable,"golf-key.565")) return false;
            round.restart();
            if (!playRoute(shots17,std::size(shots17)) || round.brokenBlocks!=0x26a9) return false;
        } else if (i==7) {
            const auto& stage=golf.stages[i];
            if (stage.skullCount!=6) return false;
            bool bounced=false;
            for (unsigned tick=0;tick<86;++tick) {
                Vec2 before[MaxSkulls];
                std::copy_n(round.skulls.velocity,stage.skullCount,before);
                round.skulls.step(walls,stage.skullCount);
                for (unsigned n=0;n<stage.skullCount;++n) {
                    bounced|=length(before[n]-round.skulls.velocity[n])>1.0f;
                }
            }
            if (!bounced) return false;
            logEvent(log,"test_skull_bounce",round.game);
            render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
            if (!dumpFrame(executable,"skulls-moving.565")) return false;
            round.restart();
            if (!shoot({0.62982357f,0.65220141f})) return false;
            logEvent(log,"test_route_shot",round.game);
            bool skullCaptured=false;
            for (unsigned tick=0;tick<210 && !round.game.won && !round.caught;++tick) {
                round.step();
                if (!skullCaptured && round.skulls.bonus) {
                    render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
                    if (!dumpFrame(executable,"skull-hit.565")) return false;
                    skullCaptured=true;
                }
            }
            if (!round.game.won || !round.game.bounces || round.skulls.bonus!=25 || round.game.strokes!=1) return false;
            logEvent(log,"test_skull_defeated",round.game);
            logEvent(log,"test_route_goal",round.game);
        } else if (i==8) {
            if (golf.stages[i].skullCount!=1 || golf.stages[i].bubbleCount!=12) return false;
            for (unsigned tick=0;tick<90;++tick) round.step();
            if (length(round.game.position-course.spawn)<5 || round.game.moving() ||
                round.game.strokes || round.skulls.bonus || round.caught) return false;
            logEvent(log,"test_idle_skull_push",round.game);
            round.restart();
            if (!playRoute(shots19,std::size(shots19)) || round.skulls.bonus!=25) return false;
            logEvent(log,"test_pond_course_skull",round.game);
        } else if (i==9) {
            const Vec2 secondSpawn=round.extraGhosts[0].position;
            if (round.extraGhostCount!=1 || secondSpawn.x!=96 || secondSpawn.y!=-224) return false;
            const Vec2 firstSpawn=round.ghost.position;
            for (unsigned tick=0;tick<24;++tick) round.step();
            if (length(round.ghost.position-firstSpawn)<1 || length(round.extraGhosts[0].position-secondSpawn)<1) return false;
            logEvent(log,"test_two_ghosts_chase",round.game);
            round.extraGhosts[0].position=round.game.position;
            round.step();
            if (!round.caught || round.waterDead || round.game.won) return false;
            logEvent(log,"test_second_ghost_catches",round.game);
            if (round.controls.update(round.game,course,{},false,true)!=ControlEvent::Restart) return false;
            round.restart();
            if (round.caught || length(round.ghost.position-firstSpawn)>0 ||
                length(round.extraGhosts[0].position-secondSpawn)>0) return false;
            logEvent(log,"test_two_ghosts_restart",round.game);
            if (!finishShot({0.71813065f,0.38183686f},true)) return false;
        } else if (i==10 || i==11) {
            const unsigned expected=i==10?2:7;
            const auto& stage=golf.stages[i];
            if (round.extraGhostCount!=expected) return false;
            const Vec2 firstSpawn=round.ghost.position;
            for (unsigned tick=0;tick<24;++tick) round.step();
            if (round.caught || length(round.ghost.position-firstSpawn)<1) return false;
            for (unsigned n=0;n<expected;++n) {
                if (length(round.extraGhosts[n].position-stage.extraGhostSpawns[n])<1) return false;
            }
            logEvent(log,i==10?"test_three_ghosts_chase":"test_eight_ghosts_chase",round.game);
            round.extraGhosts[expected-1].position=round.game.position;
            round.step();
            if (!round.caught || round.waterDead || round.backrooms || round.game.won) return false;
            logEvent(log,"test_last_ghost_catches",round.game);
            if (round.controls.update(round.game,course,{},false,true)!=ControlEvent::Restart) return false;
            round.restart();
            if (round.caught || length(round.ghost.position-firstSpawn)>0) return false;
            for (unsigned n=0;n<expected;++n) {
                if (length(round.extraGhosts[n].position-stage.extraGhostSpawns[n])>0) return false;
            }
            logEvent(log,"test_ghost_roster_restart",round.game);
            if (i==11) {
                if (!reachBackrooms()) return false;
                logEvent(log,"test_backrooms_entered",round.game);
                if (round.controls.update(round.game,course,{},false,true)!=ControlEvent::Restart) return false;
                round.restart();
                if (round.backrooms || round.caught || length(round.game.position-course.spawn)>0) return false;
                logEvent(log,"test_backrooms_restart",round.game);
            }
            const Vec2 aim=i==10?Vec2{-0.79737806f,-0.32216170f}:Vec2{-0.50549519f,-0.69575471f};
            if (!finishShot(aim,true) || round.game.strokes!=1) return false;
        } else if (i==12 || i==13) {
            const unsigned expectedCount=i==12?2:8;
            if (!round.keyStage || round.keyStage->keyCount!=expectedCount ||
                round.keyStage->doorCount!=expectedCount) return false;
            const auto& keyLevel=*round.keyStage;
            if (i==12) {
                const auto& locked=keyLevel.doors[0];
                round.game.position=locked.center+Vec2{locked.halfSize.x+course.playerRadius+5,0};
                if (!shoot({-0.75f,0})) return false;
                for (unsigned tick=0;tick<12 && !round.game.bounces;++tick) round.step();
                if (!round.game.bounces || round.roundKeys.opened || round.roundKeys.collected) return false;
                logEvent(log,"test_locked_door_bounce",round.game);
                round.restart();
            } else {
                KeyDoorRun colors;
                colors.restart(keyLevel);
                const auto end=keyLevel.doorColors+keyLevel.doorCount;
                const auto match=std::find(keyLevel.doorColors,end,keyLevel.keys[0].color);
                if (match==end) return false;
                const unsigned matchingDoor=match-keyLevel.doorColors;
                const unsigned wrongDoor=(matchingDoor+1)%keyLevel.doorCount;
                if (!colors.collect(keyLevel,keyLevel.keys[0].position,course.playerRadius,1) ||
                    colors.held!=1 || colors.open(keyLevel,wrongDoor,2) || colors.bonus ||
                    !colors.open(keyLevel,matchingDoor,2) || colors.held || colors.bonus!=200) return false;
                logEvent(log,"test_key_color_match",round.game);
            }
            const Vec2* shots=i==12?shots113:shots114;
            if (!playRoute(shots,2)) return false;
            const uint32_t expectedOpened=i==12?3:0x70;
            if (round.roundKeys.collected!=(1u<<expectedCount)-1 ||
                round.roundKeys.opened!=expectedOpened || round.roundKeys.bonus!=(i==12?400:600)) return false;
            logEvent(log,"test_key_door_route",round.game);
        } else if (i==14 || i==15) {
            if (!round.keyStage || round.keyStage->keyCount!=(i==14?6u:2u) ||
                round.keyStage->doorCount!=round.keyStage->keyCount) return false;
            if (i==15) {
                round.game.position={368,-180};
                round.step();
                if (!round.caught || !round.waterDead || round.pool.showingSpeech) return false;
                logEvent(log,"test_pool_water_death",round.game);
                if (round.controls.update(round.game,course,{},false,true)!=ControlEvent::Restart) return false;
                round.restart();
                if (round.caught || round.waterDead || round.pool.swimming || !round.coins.empty() ||
                    length(round.game.position-course.spawn)>0) return false;
                logEvent(log,"test_pool_restart",round.game);

                const unsigned oldQuest=progress.quest;
                progress.quest=0;
                canSwim=true;
                round.game.position={368,-180};
                round.step();
                if (round.caught || !round.pool.swimming || !round.pool.showingSpeech || progress.quest!=1)
                    return false;
                persistProgress(save,round,log);
                Progress check{};
                if (!save.read(check) || check.quest!=1 || check.next!=20) return false;
                logEvent(log,"test_pool_swim_quest_saved",round.game);
                canSwim=false;
                progress.quest=oldQuest;
                round.restart();
            }
            const Vec2* aims=i==14?shots115:shots116;
            if (!playRoute(aims,i==14?std::size(shots115):std::size(shots116))) return false;
            const uint32_t opened=i==14?0x2f:1;
            if (round.roundKeys.opened!=opened || round.roundKeys.bonus!=(i==14?1000:200) ||
                round.roundKeys.collected!=(i==14?0x3f:1)) return false;
            logEvent(log,"test_key_door_route",round.game);
        } else if (i==16) {
            if (!round.keyStage || round.keyStage->keyCount!=8 || round.keyStage->doorCount!=8 ||
                round.extraGhostCount!=1 || golf.stages[i].sandCount!=2 ||
                course.left!=-512 || course.right!=512) return false;
            if (!playRoute(shots117,std::size(shots117)) || round.game.strokes!=2 ||
                round.roundKeys.collected!=0x42 || round.roundKeys.opened!=0xa0 || round.roundKeys.bonus!=400)
                return false;
            logEvent(log,"test_key_door_route",round.game);
        } else if (i==17) {
            if (!finishShot(shot118,true) || round.game.strokes!=1) return false;
            logEvent(log,"test_rest_stop_route",round.game);
        } else if (i<std::size(golfAims)) {
            if (!finishShot(golfAims[i],true,i==1)) return false;
        } else {
            const Vec2* aims=i==4?shots15:shots16;
            const unsigned count=i==4?std::size(shots15):std::size(shots16);
            if (!playRoute(aims,count)) return false;
            const unsigned expectedBlocks=i==4?7:13, expectedMask=i==4?0x3c1c0:0x3ffe;
            if (round.brokenBlocks!=expectedMask || round.blockMultiplier!=expectedBlocks ||
                round.blockBonus!=5*expectedBlocks*(expectedBlocks+1)) return false;
            if (std::FILE* file=std::fopen(log,"a")) {
                std::fprintf(file,"build=053 event=test_block_set course=%s broken=%u bonus=%u\n",
                             CourseIds[currentCourse],expectedBlocks,round.blockBonus);
                std::fclose(file);
            }
        }
        const auto outcome=advanceCampaign(executable,course,round,save,log);
        const auto lastGolf=CourseCount>=GlowstickFirstCourse?Advance::WorldCleared:Advance::EndOfBuild;
        if (outcome!=(i+1<GolfStages?Advance::NextHole:lastGolf)) return false;
        if (outcome==Advance::WorldCleared) {
            Flow resultsFlow;
            resultsFlow.screen=Screen::WorldResults;
            render(resultsFlow,course,round,WorldSkipTimeMs);
            if (!dumpFrame(executable,"world-results-golf.565")) return false;
            const unsigned previousMenu=menuCourse;
            leaveWorldResults(resultsFlow,true);
            if (CourseCount==GlowstickFirstCourse &&
                (resultsFlow.screen!=Screen::LevelSelect || menuCourse!=GlowstickFirstCourse-1)) return false;
            if (CourseCount>GlowstickFirstCourse &&
                (resultsFlow.screen!=Screen::Hole || menuCourse!=GlowstickFirstCourse)) return false;
            menuCourse=previousMenu;
            logEvent(log,"test_world_golf_boundary",round.game);
        }
    }
    if (!save.read(loaded) || loaded.next!=GlowstickFirstCourse || resumeCourse()!=GlowstickFirstCourse ||
        loaded.best[5].score==0 || loaded.best[6].score==0 || loaded.best[7].score==0 ||
        loaded.best[8].score==0 || loaded.best[9].score==0 || loaded.best[10].score==0 ||
        loaded.best[11].score==0 || loaded.best[12].score==0 || loaded.best[13].score==0 ||
        loaded.best[14].score==0 || loaded.best[15].score==0 || loaded.best[16].score==0 ||
        loaded.best[17].score==0 || loaded.best[18].score==0 ||
        loaded.best[19].score==0 || loaded.best[20].score==0 ||
        loaded.best[21].score==0 || loaded.best[22].score==0 ||
        loaded.terryKey!=1 || loaded.golfKey!=1) return false;
    logEvent(log,"test_golf_progress_reloaded",round.game);
    if (!enterCourse(executable,0,course,round) || walls.active() || round.game.position.x!=-224) return false;
    logEvent(log,"test_return_to_first",round.game);
    HoldToSkip skip;
    for (unsigned i=1;i<HoldToSkip::RequiredTicks;++i) if (skip.update(true)) return false;
    const Progress beforeSkip=progress;
    if (!skip.update(true) || skip.update(true) ||
        advanceCampaign(executable,course,round,save,log,true)!=Advance::NextHole ||
        progress.totalScore()!=beforeSkip.totalScore() || currentCourse!=1) return false;
    logEvent(log,"test_skip_hold",round.game);

    const unsigned baseline=audio.blocks();
    audio.music(Sound::Menu);
    for (int i=0;i<150 && audio.blocks()-baseline<12;++i) sceDisplayWaitVblankStart();
    if (std::FILE* f=std::fopen(log,"a")) {
        std::fprintf(f,"build=053 audible_pcm_blocks=%u\n",audio.blocks()-baseline);
        std::fclose(f);
    }
    if (audio.blocks()-baseline<12) return false;
    if (!enterCourse(executable,0,course,round)) return false;
    flow.screen=Screen::Main;
    flow.update(0,false,false,false,true);
    flow.update(0,true,false,false,false);
    if (flow.screen!=Screen::Settings) return false;
    languageDraft=Language::Russian;
    render(flow,course,round,1500);
    if (!dumpFrame(executable,"language-select.565")) return false;
    if (!changeLanguage(executable,course,config,languageDraft) ||
        config.read()!=Language::Russian || !russian()) return false;
    logEvent(log,"test_language_saved_ru",round.game);
    render(flow,course,round,1500);
    if (!dumpFrame(executable,"language-russian.565")) return false;
    beginFrame();
    drawFull(gameplayPage);
    rectangle(ViewX+136,0,272,Height,GU_RGBA(38,41,43,255));
    constexpr float fontSamples[]={0.63f,0.75f,0.85f,1.0f,1.12f};
    constexpr const char* sampleLabels[]={"63%","75%","85%","100%","112%"};
    localeText("Читаемость текста",272,12,1.2f,White,true);
    for (unsigned i=0;i<std::size(fontSamples);++i) {
        const float y=43+i*43;
        localeText(sampleLabels[i],147,y,1,Gold);
        localeText("Выбрать лунку, Ёё",194,y,fontSamples[i],White);
        localeText("Бонус: 15725",194,y+16,fontSamples[i],GU_RGBA(161u,255u,116u,255u));
    }
    endFrame();
    if (!dumpFrame(executable,"font-readability.565")) return false;
    flow.screen=Screen::Title;
    render(flow,course,round,3950);
    if (!dumpFrame(executable,"title-russian.565")) return false;
    flow.screen=Screen::Main;
    render(flow,course,round,2300);
    if (!dumpFrame(executable,"main-russian.565")) return false;
    flow.screen=Screen::Credits;
    render(flow,course,round,0);
    if (!dumpFrame(executable,"credits-russian.565")) return false;
    flow.screen=Screen::Hole;
    for (unsigned i=0;i<CourseCount;++i) {
        if (!enterCourse(executable,i,course,round)) return false;
        round.ticks=i==PondCourse?116:120;
        if (i>=BillboardCourse-1) round.ghost.position={visual.referenceGhostX,visual.referenceGhostY};
        render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
        char name[48];
        std::snprintf(name,sizeof name,"russian-%s.565",CourseIds[currentCourse]);
        if (!dumpFrame(executable,name)) return false;
        if (i==BackroomsCourse) {
            round.restart();
            for (unsigned tick=0;tick<180;++tick) round.step();
            if (round.caught) return false;
            render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
            if (!dumpFrame(executable,"ghost-swarm-1_12.565")) return false;
            round.restart();
            if (!reachBackrooms()) return false;
            render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
            if (!dumpFrame(executable,"backrooms-1_12.565")) return false;
            round.restart();
        }
        if (i==0) {
            round.togglePause();
            render(flow,course,round,0);
            if (!dumpFrame(executable,"pause-russian.565")) return false;
            round.togglePause();
        }
        if (i==PoolCourse) {
            round.restart();
            canSwim=true;
            round.ghost.position={-900,-900};
            round.game.position={368,-180};
            round.step();
            if (!round.pool.swimming || !round.pool.showingSpeech) return false;
            round.ticks=round.pool.speechStart+300;
            render(flow,course,round,0,visual.referenceClock,visual.referenceTotal);
            if (!dumpFrame(executable,"pool-speech-russian.565")) return false;
            canSwim=false;
            round.restart();
            round.game.won=true;
            round.game.strokes=3;
            round.coins.bonus=350;
            render(flow,course,round,0,2750,54000,4499);
            if (!dumpFrame(executable,"result-text-russian.565")) return false;
            round.restart();
        }
        if (i>=FirstBaddieCourse) {
            for (bool won:{true,false}) {
                const auto clip=resultClip(won);
                const unsigned count=clips.count(clip);
                for (unsigned frame:{0u,count/2,count-1}) {
                    const auto* pixels=clips.frame(clip,frame*50);
                    if (!pixels) return false;
                    beginFrame();
                    drawFull(pixels);
                    endFrame();
                    std::snprintf(name,sizeof name,"ru-%s-%s-%03u.565",won?"win":"loss",CourseIds[currentCourse],frame);
                    if (!dumpFrame(executable,name)) return false;
                }
            }
        }
    }
    round.game.won=true;
    round.game.strokes=2;
    render(flow,course,round,0,500,900,4550);
    if (!dumpFrame(executable,"result-russian.565")) return false;
    Progress afterLanguage{};
    if (!save.read(afterLanguage) || afterLanguage.next!=GlowstickFirstCourse || !afterLanguage.terryKey || !afterLanguage.golfKey ||
        afterLanguage.totalScore()!=progress.totalScore()) return false;
    if (!changeLanguage(executable,course,config,Language::English) || russian() ||
        config.read()!=Language::English) return false;
    logEvent(log,"test_language_saved_en",round.game);
    if (!changeLanguage(executable,course,config,Language::Russian) ||
        config.read()!=Language::Russian || !russian()) return false;
    logEvent(log,"test_language_kept_progress",round.game);
    flow.screen=Screen::Settings;
    beginSettingsDraft();
    const uint32_t settingsBeforePreview=backgroundFile.read();
    const auto settingsInput=[&](uint32_t pressed) {
        updateSettings(flow,pressed,executable,course,config,backgroundFile,log,round.game);
        return !settingsError;
    };
    if (!settingsInput(PSP_CTRL_DOWN) || settingsRow!=SettingsRow::Background) return false;
    render(flow,course,round,1500);
    if (!dumpFrame(executable,"background-default.565")) return false;
    if (!settingsInput(PSP_CTRL_LEFT) || activeBackground!=0) return false;
    render(flow,course,round,1500);
    if (!dumpFrame(executable,"background-black.565")) return false;
    for (unsigned i=1;i<=BackgroundCount+2;++i) {
        const unsigned id=BackgroundIds[i%BackgroundCount];
        if (!settingsInput(PSP_CTRL_RIGHT) || activeBackground!=id ||
            backgroundFile.read()!=settingsBeforePreview) return false;
        if (id==CatalogBackgroundBase+104) {
            render(flow,course,round,1500);
            if (!dumpFrame(executable,"background-catalog-104.565")) return false;
        }
    }
    const BackgroundFile manualFixture(statePath(executable,"background-manual.cfg"));
    if (!manualFixture.write(backgroundDraft.encode())) return false;
    render(flow,course,round,1500);
    if (!dumpFrame(executable,"background-g02.565")) return false;
    if (!settingsInput(PSP_CTRL_DOWN) || settingsRow!=SettingsRow::Random ||
        !settingsInput(PSP_CTRL_CROSS) || !backgroundDraft.random ||
        backgroundFile.read()!=settingsBeforePreview || activeBackground!=2) return false;
    render(flow,course,round,1500);
    if (!dumpFrame(executable,"background-random.565") ||
        !settingsInput(PSP_CTRL_DOWN) || !settingsInput(PSP_CTRL_CROSS) ||
        backgroundFile.read()!=BackgroundPreference{2,2,true}.encode()) return false;
    Progress afterSettings{};
    if (config.read()!=Language::Russian || !save.read(afterSettings) ||
        afterSettings.next!=GlowstickFirstCourse || afterSettings.totalScore()!=afterLanguage.totalScore() ||
        afterSettings.totalPrisms()!=afterLanguage.totalPrisms() ||
        !afterSettings.terryKey || !afterSettings.golfKey) return false;
    logEvent(log,"test_background_kept_language_and_progress",round.game);
    return true;
}

#endif

} // namespace

int main(int argc,char** argv) {
    const char* executable=argc>0?argv[0]:nullptr;
#ifdef UCG_DISC_BUILD
    sceIoMkdir("ms0:/PSP",0777);
    sceIoMkdir("ms0:/PSP/UCG053-ISO",0777);
    organizedAssets=true;
#else
    SceIoStat dataDirectory{};
    organizedAssets=sceIoGetstat(companion(executable,"data").c_str(),&dataDirectory)>=0 &&
                    FIO_S_ISDIR(dataDirectory.st_mode);
#endif
    const std::string log=statePath(executable,"session.log");
    logMemory(log.c_str(),"bootstrap",0,"w");
    logMemory(log.c_str(),"release_fix_1",0);
    logMemory(log.c_str(),"release_fix_2",0);
    logMemory(log.c_str(),"release_fix_4",0);
    logMemory(log.c_str(),"selector_back_save_icon",0);
#ifndef UCG_VISUAL_TEST
    uint64_t splashStart=0;
    if (showBootSplash()) {
        sceDisplayWaitVblankStart();
        splashStart=sceKernelGetSystemTimeWide();
        logMemory(log.c_str(),"boot_splash_ready",0);
    }
#endif
    const LanguageFile config(statePath(executable,"language.cfg"));
    const BackgroundFile backgroundFile(statePath(executable,"background.cfg"));
#ifdef UCG_DISC_BUILD
    const std::string audioPack=assetPath(executable,"audio.uca");
#else
    const std::string audioPack=assetPath(executable,"audio.pak");
#endif
    const std::string slotAudioPack=assetPath(executable,"slots-audio.pak");
    const std::string parkAudioPack=assetPath(executable,"park-audio.pak");
    const std::string thoughtsPack=assetPath(executable,"thoughts.pak");
    const std::string glowstickPack=assetPath(executable,"glowstick.pak");
    const std::string stopPack=assetPath(executable,"stop-tokens.pak");
    const std::string speedPack=assetPath(executable,"speed-tokens.pak");
    const std::string billboardPack=assetPath(executable,"billboards.pak");
    const std::string fontPack=assetPath(executable,"locale-font.bin");
    const std::string labelsPack=assetPath(executable,"menu-labels.bin");
    const std::string selectPack=assetPath(executable,"level-select.pak");
    const std::string selectDetailsPack=assetPath(executable,"selector-details.bin");
    const std::string selectorTransitionPack=assetPath(executable,"selector-transition.pak");
    const std::string selectorGoPack=assetPath(executable,"selector-go.pak");
    const std::string profilesPack=assetPath(executable,"profiles-art.pak");
    const std::string giftsPack=assetPath(executable,"selector-gifts.bin");
    const std::string giftBlastPack=assetPath(executable,"selector-gift-explosion.bin");
    const GiftFile giftFile(statePath(executable,"gifts.cfg"));
    gifts.load(giftFile);
    menuScrollPack=assetPath(executable,"menu-scroll.pak");
    profiles=Profiles(statePath(executable,""));
    cosmicTitlePath046=assetPath(executable,"selector-046-titles.pak");
    cosmicTitlePath047=assetPath(executable,"selector-047-titles.pak");
    cosmicTitlePath048=assetPath(executable,"selector-048-titles.pak");
    cosmicTitlePath049=assetPath(executable,"selector-049-titles.pak");
    cosmicTitlePath050=assetPath(executable,"selector-050-titles.pak");
    cosmicTitlePath051=assetPath(executable,"selector-051-titles.pak");
    cosmicTitlePath052=assetPath(executable,"selector-052-titles.pak");
    cosmicTitlePath053=assetPath(executable,"selector-053-titles.pak");
    const std::string settingsPack=assetPath(executable,"settings-widgets.bin");
    const std::string decorationsPack=assetPath(executable,"decorations.pak");
    const std::string backroomsPack=assetPath(executable,"backrooms.bin");
    const std::string keyDoorsPack=assetPath(executable,"keydoors.pak");
    const std::string poolPack=assetPath(executable,"pool.pak");
#ifdef UCG_VISUAL_TEST
    SaveFile save(statePath(executable,"progress-test.sav"));
#else
    SaveFile save(statePath(executable,"progress.sav"));
#endif
    const int cb=sceKernelCreateThread("ucg-callback",callbackThread,0x11,0x1000,0,nullptr);
    if (cb>=0) sceKernelStartThread(cb,0,nullptr);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    Course course{};
    const Language savedLanguage=config.read();
    const bool actorsReady=actorAtlas.open(assetPath(executable,"actors.pak").c_str(),CourseCount);
    const bool flagsReady=goalFlags.open(assetPath(executable,"goal-flags.pak").c_str(),2*CourseCount);
    const char* localizedError="localized course/media";
    const bool localizedAssets=actorsReady && flagsReady &&
        (loadLocalizedAssets(executable,course,savedLanguage,&localizedError) ||
         (savedLanguage==Language::Russian && loadLocalizedAssets(executable,course,Language::English,&localizedError)));
    const char* failedAsset=nullptr;
    int logOpenResult=0;
    const auto checkAsset=[&](bool ok,const char* asset) {
        if (ok) return true;
        failedAsset=asset;
        const auto probe=sceIoOpen(log.c_str(),PSP_O_RDONLY,0);
        if (probe<0) logOpenResult=probe;
        else sceIoClose(probe);
        if (auto* out=std::fopen(log.c_str(),"a")) {
            std::fprintf(out,"build=053 event=asset_error file=%s\n",asset);
            if (const auto* reason=audio.failure())
                std::fprintf(out,"build=053 event=audio_open_error part=%s\n",reason);
            std::fclose(out);
        }
        return false;
    };
    if (!checkAsset(actorsReady,"actors.pak") || !checkAsset(flagsReady,"goal-flags.pak") ||
        !checkAsset(localizedAssets,localizedError) ||
        !checkAsset(!clips.hasLiveResults() || liveResultArt.open(assetPath(executable,"results-native.ucr"),123,ResultScratchBytes),
                    "results-native.ucr") ||
        !checkAsset(!clips.hasCommonResults() || commonResultArt.open(assetPath(executable,"results-win.ucr"),62,ResultScratchBytes),
                    "results-win.ucr") ||
        !checkAsset(parkArt.open(assetPath(executable,"park-art.pak").c_str(),
                                 assetPath(executable,"park-stage.pak").c_str()),"park art and stage") ||
        !checkAsset(thoughts.open(thoughtsPack.c_str()),"thoughts.pak") ||
        !checkAsset(glowstickArt.open(glowstickPack.c_str()),"glowstick.pak") ||
        !checkAsset(loadBaddies(assetPath(executable,"baddies.pak").c_str()),"baddies.pak") ||
        !checkAsset(stopTokens.open(stopPack.c_str()),"stop-tokens.pak") ||
        !checkAsset(speedTokens.open(speedPack.c_str()),"speed-tokens.pak") ||
        !checkAsset(billboards.open(billboardPack.c_str()),"billboards.pak") ||
        !checkAsset(loadGimmicks(assetPath(executable,"gimmicks.pak").c_str()),"gimmicks.pak") ||
        !checkAsset(newsArt.open(assetPath(executable,"news.pak").c_str()),"news.pak") ||
        !checkAsset(loadLocaleFont(fontPack.c_str()),"locale-font.bin") ||
        !checkAsset(loadUiTexture(labelsPack.c_str(),menuLabels,sizeof menuLabels),"menu-labels.bin") ||
        !checkAsset(loadUiTexture(settingsPack.c_str(),settingsWidgets,sizeof settingsWidgets),"settings-widgets.bin") ||
        !checkAsset(levelSelectArt.open(selectPack.c_str()),"level-select.pak") ||
        !checkAsset(selectorTransitionArt.open(selectorTransitionPack.c_str()),"selector-transition.pak") ||
        !checkAsset(levelSelectArt.openGo(selectorGoPack.c_str()),"selector-go.pak") ||
        !checkAsset(loadUiTexture(selectDetailsPack.c_str(),selectorDetails,sizeof selectorDetails),"selector-details.bin") ||
        !checkAsset(loadUiTexture(assetPath(executable,"selector-park-titles.bin").c_str(),
                                  selectorParkTitles,sizeof selectorParkTitles),"selector-park-titles.bin") ||
        !checkAsset(loadUiTexture(assetPath(executable,"selector-factory-titles.bin").c_str(),
                                  selectorFactoryTitles,sizeof selectorFactoryTitles),"selector-factory-titles.bin") ||
        !checkAsset(loadUiTexture(assetPath(executable,"selector-late-park-titles.bin").c_str(),
                                  selectorLateParkTitles,sizeof selectorLateParkTitles),"selector-late-park-titles.bin") ||
        !checkAsset(loadUiTexture(assetPath(executable,"selector-next-titles.bin").c_str(),
                                  selectorNextTitles,sizeof selectorNextTitles),"selector-next-titles.bin") ||
        !checkAsset(loadUiTexture(assetPath(executable,"frontier-titles.bin").c_str(),
                                  selectorFrontierTitles,sizeof selectorFrontierTitles),"frontier-titles.bin") ||
        !checkAsset(loadUiTexture(assetPath(executable,"selector-next-world-titles.bin").c_str(),
                                  selectorNextWorldTitles,sizeof selectorNextWorldTitles),"selector-next-world-titles.bin") ||
        !checkAsset(frontier1011Titles.openOnDemand(assetPath(executable,"selector-frontier-1011-titles.pak")),
                    "selector-frontier-1011-titles.pak") ||
        !checkAsset(frontier18501Titles.openOnDemand(assetPath(executable,"selector-frontier-18501-titles.pak")),
                    "selector-frontier-18501-titles.pak") ||
        !checkAsset(elysian0203Titles.openOnDemand(assetPath(executable,"selector-elysian-0203-titles.pak")),
                    "selector-elysian-0203-titles.pak") ||
        !checkAsset(elysian0405Titles.openOnDemand(assetPath(executable,"selector-frontier-050405-titles.pak")),
                    "selector-frontier-050405-titles.pak") ||
        !checkAsset(elysian0607Titles.openOnDemand(assetPath(executable,"selector-elysian-0607-titles.pak")),
                    "selector-elysian-0607-titles.pak") ||
        !checkAsset(elysian0809Titles.openOnDemand(assetPath(executable,"selector-horse-050809-titles.pak")),
                    "selector-horse-050809-titles.pak") ||
        !checkAsset(elysian1011Titles.openOnDemand(assetPath(executable,"selector-horse-051011-titles.pak")),
                    "selector-horse-051011-titles.pak") ||
        !checkAsset(elysian1213Titles.openOnDemand(assetPath(executable,"selector-horse-051213-titles.pak")),
                    "selector-horse-051213-titles.pak") ||
        !checkAsset(elysianSwitchArt512.open(assetPath(executable,"frontier-switch-5_12.ucr"),ElysianSwitch512SpriteCount,ResultScratchBytes),
                    "frontier-switch-5_12.ucr") ||
        !checkAsset(elysian1415Titles.load(assetPath(executable,"selector-horse-051415-titles.pak")) &&
                    elysian1415Titles.size()==SelectorHorse043Count,
                    "selector-horse-051415-titles.pak") ||
        !checkAsset(elysian1617Titles.load(assetPath(executable,"selector-horse-051617-titles.pak")) &&
                    elysian1617Titles.size()==SelectorHorse044Count,"selector-horse-051617-titles.pak") ||
        !checkAsset(finaleTitles045.load(assetPath(executable,"selector-045-titles.pak")) &&
                    finaleTitles045.size()==Selector045Count,"selector-045-titles.pak") ||
        !checkAsset(openFinaleAssets045(executable),"ending-045") ||
        !checkAsset(cosmicWorldTitles046.open(assetPath(executable,"selector-046-world.ucr"),SelectorWorld046Count,ResultScratchBytes),
                    "selector-046-world.ucr") ||
        !checkAsset(openCosmicAssets046(executable),"cosmic-046") ||
        !checkAsset(openCosmicAssets048(executable),"cosmic-048") ||
        !checkAsset(openCosmicAssets049(executable),"cosmic-049") ||
        !checkAsset(openCosmicAssets050(executable),"cosmic-050") ||
        !checkAsset(openCosmicAssets051(executable),"cosmic-051") ||
        !checkAsset(openCosmicAssets052(executable),"cosmic-052") ||
        !checkAsset(openCosmicAssets053(executable),"cosmic-053") ||
        !checkAsset(openCosmicEnding053(executable),"cosmic-ending-053") ||
        !checkAsset(elysianSwitchArt514.open(assetPath(executable,"frontier-switch-5_14.ucr"),ElysianSwitch514SpriteCount,ResultScratchBytes),
                    "frontier-switch-5_14.ucr") ||
        !checkAsset(elysianTreesArt515.open(assetPath(executable,"elysian-trees-5_15.ucr"),ElysianTrees515SpriteCount,ResultScratchBytes),
                    "elysian-trees-5_15.ucr") ||
        !checkAsset(elysianOutOfBoundsArt.open(assetPath(executable,"elysian-ob.ucr"),1,ResultScratchBytes),
                    "elysian-ob.ucr") ||
        !checkAsset(frontier1617Titles.openOnDemand(assetPath(executable,"selector-frontier-1617-titles.pak")),
                    "selector-frontier-1617-titles.pak") ||
        !checkAsset(frontier1415Titles.openOnDemand(assetPath(executable,"selector-frontier-1415-titles.pak")),
                    "selector-frontier-1415-titles.pak") ||
        !checkAsset(frontier1213Titles.openOnDemand(assetPath(executable,"selector-frontier-1213-titles.pak")),
                    "selector-frontier-1213-titles.pak") ||
        !checkAsset(frontier0809Titles.openOnDemand(assetPath(executable,"selector-frontier-final-titles.pak")),
                    "selector-frontier-final-titles.pak") ||
        !checkAsset(frontier0607Titles.load(assetPath(executable,"selector-frontier-0607-titles.pak")),
                    "selector-frontier-0607-titles.pak") ||
        !checkAsset(frontierLateTitles.load(assetPath(executable,"selector-frontier-late-titles.pak")),
                    "selector-frontier-late-titles.pak") ||
        !checkAsset(loadUiTexture(assetPath(executable,"selector-dog-titles.bin").c_str(),
                                  selectorDogTitles,sizeof selectorDogTitles),"selector-dog-titles.bin") ||
        !checkAsset(loadUiTexture(assetPath(executable,"selector-final-park-titles.bin").c_str(),
                                  selectorFinalParkTitles,sizeof selectorFinalParkTitles),"selector-final-park-titles.bin") ||
        !checkAsset(loadUiTexture(assetPath(executable,"selector-extra-park-titles.bin").c_str(),
                                  selectorExtraParkTitles,sizeof selectorExtraParkTitles),"selector-extra-park-titles.bin") ||
        !checkAsset(profileArt.open(profilesPack.c_str()),"profiles-art.pak") ||
        !checkAsset(GiftArt::load(giftsPack.c_str(),giftPixels,GiftArt::IconRows),"selector-gifts.bin") ||
        !checkAsset(loadUiTexture(assetPath(executable,"bart-gift-033.bin").c_str(),
                                  bartGiftPixels,sizeof bartGiftPixels),"bart-gift-033.bin") ||
        !checkAsset(loadUiTexture(assetPath(executable,"selector-gift-burst.bin").c_str(),
                                  burstGiftPixels,sizeof burstGiftPixels),"selector-gift-burst.bin") ||
        !checkAsset(GiftArt::load(giftBlastPack.c_str(),giftBlast[0],GiftArt::ExplosionRows,SelectorGifts::BlastPages),"selector-gift-explosion.bin") ||
        !checkAsset(menuButtonArt.open(menuScrollPack.c_str()),"menu-scroll.pak") ||
        !checkAsset(loadDecorations(decorationsPack.c_str()),"decorations.pak") ||
        !checkAsset(PauseArt::load(assetPath(executable,"pause-menu.bin").c_str()),"pause-menu.bin") ||
        !checkAsset(loadBackrooms(backroomsPack.c_str()),"backrooms.bin") ||
        !checkAsset(loadKeyDoors(keyDoorsPack.c_str()),"keydoors.pak") ||
        !checkAsset(poolAssets.load(poolPack.c_str()),"pool.pak") ||
        (CourseCount>SlotMachineCourse &&
         !checkAsset(slotArt.open(assetPath(executable,"slots.pak")),"slots.pak")) ||
        !checkAsset(audio.open(audioPack.c_str(),slotAudioPack.c_str(),parkAudioPack.c_str(),
                               assetPath(executable,"park-timer-3_11.pcm").c_str()),
                    "audio.pak / slots-audio.pak / park-audio.pak") ||
        !checkAsset(loadBackground(backgroundFile),"backgrounds")) {
        pspDebugScreenInit();
        pspDebugScreenPrintf("Build 040 startup error.\nAsset: %s\n",failedAsset);
        if (const auto* reason=audio.failure()) pspDebugScreenPrintf("Audio: %s\n",reason);
        if (logOpenResult<0) pspDebugScreenPrintf("Log open: 0x%08X\n",static_cast<unsigned>(logOpenResult));
        pspDebugScreenPrintf("TRIANGLE to exit.");
        while (running.load(std::memory_order_relaxed)) {
            sceDisplayWaitVblankStart();
            SceCtrlData pad{};
            sceCtrlPeekBufferPositive(&pad,1);
            if (pad.Buttons&PSP_CTRL_TRIANGLE) break;
        }
        sceKernelExitGame();
        return 1;
    }

    Round round(course);
    Flow flow;
    flow.loadingPromptMs=clips.duration(Clip::Loading);
    flow.loadingMs=std::max(7500u,flow.loadingPromptMs);
#ifndef UCG_VISUAL_TEST
    while (splashStart && sceKernelGetSystemTimeWide()-splashStart<3000000u)
        sceDisplayWaitVblankStart();
#endif
    initializeGraphics();
    logMemory(log.c_str(),"memory_ready",audio.effectMemoryBytes());
    logEvent(log.c_str(),"ready",round.game);
    logEvent(log.c_str(),russian()?"language_loaded_ru":"language_loaded_en",round.game);
    logBackground(log.c_str(),"background_loaded");
#ifdef UCG_VISUAL_TEST
#if defined(UCG_TEST_053)
    const bool pass=runFocused053(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_052)
    const bool pass=runFocused052(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_051)
    const bool pass=runFocused051(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_050)
    const bool pass=runFocused050(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_049)
    const bool pass=runFocused049(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_048)
    const bool pass=runFocused048(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_047)
    const bool pass=runFocused047(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_046)
    const bool pass=runFocused046(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_045)
    const bool pass=runFocused045(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_044)
    const bool pass=runFocused044(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_043)
    const bool pass=runFocused043(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_042)
    const bool pass=runFocused042(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_041)
    const bool pass=runFocused041(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_040)
    const bool pass=runFocused040(executable,course,save,config,log.c_str());
#elif defined(UCG_TEST_039)
    constexpr unsigned scenario=82;
    const bool pass=scenario==82 ? runFocused039(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_038_FRONTIER)
    constexpr unsigned scenario=78;
    const bool pass=scenario==78 ? runFocused038(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_037_ONLY)
    constexpr unsigned scenario=37;
    const bool pass=scenario==37 ? runFocused037(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_037_FRONTIER)
    constexpr unsigned scenario=74;
    const bool pass=scenario==74 ? runFocused037Frontier(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_036_ONLY)
    constexpr unsigned scenario=36;
    const bool pass=scenario==36 ? runFocused036(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_036_FRONTIER)
    constexpr unsigned scenario=70;
    const bool pass=scenario==70 ? runFocused036Frontier(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_035_ONLY)
    constexpr unsigned scenario=35;
    const bool pass=scenario==35 ? runFocused035(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_035_FRONTIER)
    constexpr unsigned scenario=66;
    const bool pass=scenario==66 ? runFocused035Frontier(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_034_ONLY)
    constexpr unsigned scenario=34;
    const bool pass=scenario==34 ? runFocused034(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_033_FRONTIER)
    constexpr unsigned scenario=62;
    const bool pass=scenario==62 ? runFocused033Frontier(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_033_ONLY)
    constexpr unsigned scenario=33;
    const bool pass=scenario==33 ? runFocused033(executable,course,save,config,giftFile,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_032_ONLY)
    constexpr unsigned scenario=32;
    const bool pass=scenario==32 ? runFocused032(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_031_ONLY)
    constexpr unsigned scenario=31;
    const bool pass=scenario==31 ? runFocused031(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_030_ONLY)
    constexpr unsigned scenario=30;
    const bool pass=scenario==30 ? runFocused030(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_029_ONLY)
    constexpr unsigned scenario=29;
    const bool pass=scenario==29 ? runFocused029(executable,course,save,config,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_028_ONLY)
    constexpr unsigned scenario=28;
    const bool pass=scenario==28 ? runFocused028(executable,course,save,config,giftFile,log.c_str()) :
                                  runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#elif defined(UCG_TEST_019_ONLY)
    constexpr bool focused=true;
    const bool pass=focused ? runFocused019(executable,course,save,config,backgroundFile,log.c_str()) :
                              runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#else
    const bool pass=runVisualTest(executable,course,save,config,backgroundFile,log.c_str());
#endif
    logEvent(log.c_str(),pass?"visual_test_pass":"visual_test_fail",round.game);
    audio.stop();
    sceGuTerm();
    sceKernelExitGame();
    return pass?0:2;
#endif
    if (save.read(progress)) {
        if (std::FILE* file=std::fopen(log.c_str(),"a")) {
            std::fprintf(file,"build=053 event=save_loaded next=%u score=%u prisms=%u key=%u golf_key=%u glowstick_key=%u park_key=%u cosmic_key=%u\n",
                         static_cast<unsigned>(progress.next),progress.totalScore(),progress.totalPrisms(),
                         static_cast<unsigned>(progress.terryKey),static_cast<unsigned>(progress.golfKey),
                         static_cast<unsigned>(progress.glowstickKey),static_cast<unsigned>(progress.parkKey),static_cast<unsigned>(progress.cosmicKey));
            std::fclose(file);
        }
    }
    canSwim=earnedSwim045();
    menuCourse=resumeCourse();
    audio.play(Sound::Startup);
    render(flow,course,round,0);
    bool frameReady=true;
    bool waitRelease=false,settingsFromPause=false;
    uint64_t pauseStart=0,tvPauseStart=0;
    uint32_t lastButtons=0;
    Screen pressedMenu=Screen::Loading;
    uint64_t last=sceKernelGetSystemTimeWide(), phaseStart=last, lastRender=last, accumulator=0;
    const uint64_t menuStart=last;
    uint64_t resultStart=0;
    EndingClock045 endingClock;
    const auto titlePath=assetPath(executable,"title.bin");
    const auto loadTitle=[&] {
        if (titleArt || titlePath.empty()) return;
        constexpr unsigned offset=FrontierLightVramOffset;
        static_assert(offset+MenuScroll::TitleBytes<=2*1024*1024);
        titleReusedCourseVram=true;
        if (!loadVramRaw045(titlePath,offset,MenuScroll::TitleBytes)) {
            logEvent(log.c_str(),"title_layers_unavailable",round.game);
            return;
        }
        titleArt=vram045(offset);
    };
    loadTitle();
    audio.primeMusic(Sound::Menu);
    bool titleImpact=false, resultHit=false, resultRank=false;
#ifdef UCG_VICTORY_PROBE
    uint64_t victoryProbeStart=0,titleProbeStart=0;
    unsigned titleRenderMax=0;
    unsigned victoryProbeCourse=0,victoryFrames=0,victoryRenderMax=0;
    const auto finishVictoryProbe=[&](uint64_t now) {
        if (!victoryProbeStart) return;
        audio.probe(AudioProbePhase::Off);
        const auto& sample=audio.timing(AudioProbePhase::Victory);
        if (auto* file=std::fopen(log.c_str(),"a")) {
            std::fprintf(file,"build=053 event=victory_profile course=%s duration_ms=%u frames=%u render_max_us=%u audio_blocks=%u audio_read_max_us=%u audio_mix_max_us=%u audio_gap_max_us=%u audio_late=%u music_underruns=%u effect_underruns=%u read_errors=%u output_error=%d\n",
                         CourseIds[victoryProbeCourse],static_cast<unsigned>((now-victoryProbeStart)/1000),
                         victoryFrames,victoryRenderMax,sample.blocks.load(),sample.readMax.load(),
                         sample.mixMax.load(),sample.gapMax.load(),sample.late.load(),sample.underruns.load(),
                         sample.effectUnderruns.load(),sample.readErrors.load(),sample.outputError.load());
            std::fprintf(file,"build=053 event=victory_assets course=%s confetti_cached=%u result_us=%u tv_us=%u ge_wait_us=%u\n",
                         CourseIds[victoryProbeCourse],static_cast<unsigned>(commonResultArt.hasPreloadedFrames()),
                         victoryRenderSample.resultMax,victoryRenderSample.tvMax,victoryRenderSample.geMax);
            std::fprintf(file,"build=053 event=victory_audio_io music_calls=%u effect_calls=%u music_io_max_us=%u effect_io_max_us=%u music_work_max_us=%u effect_work_max_us=%u music_io_total_us=%u effect_io_total_us=%u\n",
                         sample.musicIoCalls.load(),sample.effectIoCalls.load(),sample.musicIoMax.load(),
                         sample.effectIoMax.load(),sample.musicWorkMax.load(),sample.effectWorkMax.load(),
                         sample.musicIoUs.load(),sample.effectIoUs.load());
            std::fclose(file);
        }
        victoryProbeStart=0;
        victoryRenderSample.active=false;
    };
#endif

    while (running.load(std::memory_order_relaxed) && !flow.quit) {
        sceDisplayWaitVblankStart();
        if (frameReady) {
            screenDrawBuffer=sceGuSwapBuffers();
            frameReady=false;
        }
        const uint64_t now=sceKernelGetSystemTimeWide();
        const uint64_t delta=std::min<uint64_t>(now-last,100000);
        last=now;
        SceCtrlData pad{};
        sceCtrlPeekBufferPositive(&pad,1);
        const uint32_t pressed=pad.Buttons&~lastButtons;
        const uint32_t released=lastButtons&~pad.Buttons;
        lastButtons=pad.Buttons;
        const bool keyboardActive=profileKeyboard.active();
        const bool menuIdle=!pad.Buttons && !pressed && !released && now-phaseStart>=1000000 &&
            (flow.screen==Screen::LevelSelect || flow.screen==Screen::ProfileSelect ||
             flow.screen==Screen::Main || flow.screen==Screen::GameMenu);
        const unsigned oskMode=hardwareProbe.osk30?1:0;
#ifdef UCG_VICTORY_PROBE
        if (victoryProbeStart && (currentCourse!=victoryProbeCourse || flow.screen!=Screen::Hole ||
                                  !round.game.won || now-victoryProbeStart>=5000000))
            finishVictoryProbe(now);
        if (titleProbeStart && (flow.screen!=Screen::Title || now-titleProbeStart>=5000000)) {
            audio.probe(AudioProbePhase::Off);
            const auto& sample=audio.timing(AudioProbePhase::Title);
            if (auto* file=std::fopen(log.c_str(),"a")) {
                std::fprintf(file,"build=053 event=title_profile duration_ms=%u render_max_us=%u audio_blocks=%u music_priming=%u music_underruns=%u effect_underruns=%u audio_read_max_us=%u\n",
                             static_cast<unsigned>((now-titleProbeStart)/1000),titleRenderMax,
                             sample.blocks.load(),sample.priming.load(),sample.underruns.load(),
                             sample.effectUnderruns.load(),sample.readMax.load());
                std::fclose(file);
            }
            titleProbeStart=0;
        }
        audio.probe(titleProbeStart?AudioProbePhase::Title:victoryProbeStart?AudioProbePhase::Victory:
                    keyboardActive?(oskMode?AudioProbePhase::Osk30:AudioProbePhase::Osk60):
                    flow.screen==Screen::ProfileCreate?AudioProbePhase::Profile:AudioProbePhase::Off);
#endif
        if (keyboardActive) {
            if (!oskMode || now-lastRender>=32000) {
                const unsigned renderStart=sceKernelGetSystemTimeLow();
                render(flow,course,round,static_cast<unsigned>((now-phaseStart)/1000));
                const unsigned oskStart=sceKernelGetSystemTimeLow();
                profileKeyboard.draw(oskMode+1);
                hardwareProbe.renderMax[oskMode]=std::max(hardwareProbe.renderMax[oskMode],oskStart-renderStart);
                hardwareProbe.oskMax[oskMode]=std::max(hardwareProbe.oskMax[oskMode],sceKernelGetSystemTimeLow()-oskStart);
                frameReady=true;
                lastRender=now;
            }
            continue;
        }
#ifdef UCG_VICTORY_PROBE
        if (flow.screen==Screen::ProfileSelect || flow.screen==Screen::ProfileCreate || flow.screen==Screen::Settings) {
            if (pressed&PSP_CTRL_SELECT) {
                hardwareProbe.visible=!hardwareProbe.visible;
                if (hardwareProbe.visible && hardwareProbe.opened==HardwareProbe::NotRun)
                    hardwareProbe.checkStorage(executable,log.c_str());
            }
            if (flow.screen==Screen::ProfileCreate && (pressed&PSP_CTRL_LTRIGGER))
                hardwareProbe.osk30=!hardwareProbe.osk30;
        }
#endif

        if (xmbProfile.busy()) {
            if (now-lastRender>=32000) {
                render(flow,course,round,static_cast<unsigned>((now-phaseStart)/1000),
                       0,0,0,UINT32_MAX,static_cast<unsigned>((now-menuStart)/1000%MenuScroll::PeriodMs));
                frameReady=true;
                lastRender=now;
            }
            xmbProfile.update(false,log.c_str(),round.game);
            continue;
        }
        xmbProfile.update(menuIdle,log.c_str(),round.game);

        const Screen before=flow.screen;
        if (round.paused && (before==Screen::Hole || settingsFromPause))
            round.tvPausedTicks=static_cast<unsigned>((now-tvPauseStart)*60/1000000);
        unsigned elapsedMs=static_cast<unsigned>((now-phaseStart)/1000);
        bool confirm=pressed&PSP_CTRL_CROSS;
        const bool buttonMenu=before==Screen::Main || before==Screen::GameMenu;
        if (buttonMenu && confirm) pressedMenu=before;
        menuButtonDown=buttonMenu && pressedMenu==before && (pad.Buttons&PSP_CTRL_CROSS);
        if (buttonMenu) confirm=pressedMenu==before && (released&PSP_CTRL_CROSS);
        if (before==Screen::Title && !titleImpact) {
            if (confirm && elapsedMs<1400) {
                phaseStart=now-1400000;
                elapsedMs=1400;
                confirm=false;
            }
            if (elapsedMs>=1400) {
                titleImpact=true;
                audio.play(Sound::TitleImpact);
                audio.play(Sound::TitleCheer);
            }
        }
        if (before==Screen::Hole && round.wall418.bedActive &&
            ((pressed&PSP_CTRL_START) || round.wall418.endingFinished())) {
            persistProgress(save,round,log.c_str());
            openLevelSelect(flow,TheWallCourse);
            audio.music(Sound::Menu);
            phaseStart=now;
            accumulator=0;
            waitRelease=true;
            logEvent(log.c_str(),"easy_ending_finished",round.game);
            continue;
        }
        if (before==Screen::ElysianEnding) {
            const bool fast=pad.Buttons&PSP_CTRL_RTRIGGER;
            if (updateElysianEnding045(flow,endingClock,delta,fast,pressed&PSP_CTRL_START)) {
                phaseStart=now;
                logEvent(log.c_str(),"cosmic_world_unlocked",round.game);
            }
        }
        if (before==Screen::CosmicEnding &&
            updateCosmicEnding053(static_cast<unsigned>(delta/1000),pressed,pad.Buttons)) {
            resetCosmicEnding053();
            openLevelSelect(flow,CosmicEighteenthCourse);
            audio.music(Sound::Menu);
            phaseStart=now;
            waitRelease=true;
            logEvent(log.c_str(),"campaign_results_closed",round.game);
            continue;
        }
        const bool canPause=before==Screen::Hole && !round.game.won && !round.caught && !round.wall418.bedActive &&
            !(currentCourse==Elysian518Course &&
              (round.finale518.phase==FinalePhase518::Defeat || round.finale518.phase==FinalePhase518::Attract));
        const bool pauseInput=canPause && !round.horse.flying() && (round.paused || (pressed&PSP_CTRL_START));
        const auto togglePause=[&]() {
            round.togglePause();
            if (round.paused) tvPauseStart=now;
            pauseStart=now;
            waitRelease=true;
            accumulator=0;
            audio.play(Sound::Pause);
            logEvent(log.c_str(),round.paused?"pause":"resume",round.game);
        };
        if (pauseInput) {
            using Choice=PauseMenuState::Choice;
            if (!round.paused || (pressed&(PSP_CTRL_START|PSP_CTRL_CIRCLE))) {
                togglePause();
            } else if (pressed&PSP_CTRL_SELECT) {
                round.cleanPause=!round.cleanPause;
                round.pauseMenu.crossArmed=false;
                pauseStart=now;
                logEvent(log.c_str(),round.cleanPause?"pause_clean":"pause_overlay",round.game);
            } else {
                const auto previous=round.pauseMenu.choice;
                const auto choice=round.pauseMenu.update(pressed,released,round.cleanPause);
                if (round.pauseMenu.choice!=previous) audio.play(Sound::Select);
                switch (choice) {
                case Choice::Resume: togglePause(); break;
                case Choice::Settings: settingsFromPause=true; flow.screen=Screen::Settings; break;
                case Choice::LevelSelect: flow.screen=Screen::LevelSelect; break;
                case Choice::Main: flow.screen=Screen::Main; break;
                case Choice::None: break;
                }
            }
            if (round.paused) round.pauseMenu.elapsedMs=static_cast<unsigned>((now-pauseStart)/1000);
            confirm=false;
        }
        const bool giftBlocked=before==Screen::LevelSelect &&
            gifts.update(giftFile,now,confirm || (pressed&PSP_CTRL_CIRCLE));
        if (before==Screen::LevelSelect && !giftBlocked)
            confirm=updateLevelSelector(pressed,released,pad.Buttons,elapsedMs);
        if (before==Screen::WorldResults) {
            const auto update=worldResults.update(elapsedMs,pressed&PSP_CTRL_START,
                                                  pressed&PSP_CTRL_LEFT,pressed&PSP_CTRL_RIGHT,
                                                  confirm,pressed&PSP_CTRL_CIRCLE);
            if (update.elapsedMs!=elapsedMs) {
                elapsedMs=update.elapsedMs;
                phaseStart=now-static_cast<uint64_t>(elapsedMs)*1000;
            }
            playWorldCues(update.cues);
            if (update.choice!=WorldResultsChoice::None) {
                leaveWorldResults(flow,update.choice==WorldResultsChoice::NextWorld);
                if (flow.screen==Screen::LevelSelect) audio.music(Sound::Menu);
            }
        }
        if (before==Screen::ProfileSelect || before==Screen::ProfileCreate) {
            updateProfiles(flow,pressed,save,log.c_str(),round.game);
            if (flow.screen==Screen::LevelSelect)
                xmbProfile.queue(activeProfile,profiles.entry(activeProfile),progress);
        }
        if (before==Screen::Settings) {
            updateSettings(flow,pressed,executable,course,config,backgroundFile,log.c_str(),round.game);
            if (settingsFromPause && ((pressed&PSP_CTRL_CIRCLE) || flow.screen==Screen::Main)) {
                flow.screen=Screen::Hole;
                settingsFromPause=false;
                pauseStart=now-round.pauseMenu.elapsedMs*1000ull;
            }
        }
        if (flow.screen==before && !giftBlocked && !pauseInput) flow.update(elapsedMs,confirm && before!=Screen::WorldResults,pressed&PSP_CTRL_CIRCLE && before!=Screen::WorldResults && !(before==Screen::Hole && (round.wall418.bedActive || round.horseBroadcast.active() || round.horse.flying())),
                    !menuButtonDown && (pressed&PSP_CTRL_UP),!menuButtonDown && (pressed&PSP_CTRL_DOWN));
        if (flow.screen!=before) {
            if (before==Screen::LevelSelect) selectorTransitionArt.closeStream();
            if (before==Screen::Hole && flow.screen!=Screen::Settings && currentCourse==CosmicFifthCourse) {
                audio.stageCue(StageCue::None);
                audio.rideVolume(1,-80,-80);
            }
            if (before==Screen::Hole && currentCourse==CosmicThirteenthCourse && flow.screen!=Screen::Settings)
                audio.stageCue(StageCue::None);
            if (before==Screen::Hole && currentCourse==ChuckleCoasterCourse && flow.screen!=Screen::Settings) {
                stopRideAudio033();
                audio.rideVolume(1,-80,-80);
                audio.pauseStageCue(false);
            }
            pressedMenu=Screen::Loading;
            menuButtonDown=false;
            phaseStart=now;
            accumulator=0;
            if (pressed&PSP_CTRL_CIRCLE) audio.play(Sound::Back);
            else if (before!=Screen::Loading) audio.play(Sound::Select);
            if (before==Screen::Hole && flow.screen!=Screen::Hole && currentCourse==CosmicSeventeenthCourse) stopLastHurrahSounds053();
            if (before==Screen::Hole && flow.screen!=Screen::Hole && usesRingTimers(currentCourse))
                audio.stopRingTimers();
            if (flow.screen==Screen::Title) {
                loadTitle();
                if (titleArt) logEvent(log.c_str(),"title_layers_ready",round.game);
#ifdef UCG_VICTORY_PROBE
                audio.resetTiming(AudioProbePhase::Title);
                titleProbeStart=sceKernelGetSystemTimeWide();
                titleRenderMax=0;
                audio.probe(AudioProbePhase::Title);
#endif
                audio.music(Sound::Menu);
                audio.play(Sound::TitleHit);
                titleImpact=false;
            } else if (flow.screen==Screen::Main && before==Screen::Hole) {
                persistProgress(save,round,log.c_str());
                audio.stageCue(StageCue::None);
                audio.music(Sound::Menu);
            } else if (flow.screen==Screen::Settings) {
                beginSettingsDraft();
            } else if (flow.screen==Screen::Credits) {
                logEvent(log.c_str(),"credits",round.game);
            } else if (flow.screen==Screen::ProfileSelect) {
                openProfiles(flow);
            } else if (flow.screen==Screen::LevelSelect) {
                if (before==Screen::Hole) {
                    persistProgress(save,round,log.c_str());
                    openLevelSelect(flow,currentCourse);
                    audio.stageCue(StageCue::None);
                    audio.music(Sound::Menu);
                }
                if (gifts.queue(giftFile,progress)) audio.play(Sound::GiftAward);
                logEvent(log.c_str(),"level_select",round.game);
            } else if (flow.screen==Screen::Hole && before!=Screen::Settings) {
                if (!enterCourse(executable,menuCourse,course,round)) {
                    logEvent(log.c_str(),"course_load_error",round.game);
                    if (before!=Screen::WorldResults || !loadCourseAt(executable,currentCourse,course)) break;
                    playableCourseCount=std::min(playableCourseCount,menuCourse);
                    leaveWorldResults(flow,false);
                    audio.music(Sound::Menu);
                    continue;
                }
                round.totalTicks=progress.ticks;
                thoughts.begin(round.tvTick());
                resultStart=0;
                waitRelease=true;
                startCourseMusic(currentCourse);
                audio.play(Sound::Join);
                logEvent(log.c_str(),"hole",round.game);
            }
        }
        if (flow.screen==Screen::Hole && !round.paused && !pauseInput) {
#ifdef UCG_VICTORY_PROBE
            if ((pressed&PSP_CTRL_SELECT) && !round.game.won) logEvent(log.c_str(),"select_in_play",round.game);
#endif
            const bool liftHorseInput=round.horse.flying() && (pressed&PSP_CTRL_CROSS);
            round.liftHorse=round.liftHorse || liftHorseInput;
            if (!(pad.Buttons&PSP_CTRL_CROSS)) waitRelease=false;
            const bool oldAim=round.controls.aiming;
            if (pressed&PSP_CTRL_SQUARE) {
                interactTerry(round,save,log.c_str());
                if (currentCourse==CosmicFirstCourse && !round.caught && !round.game.won &&
                    length(round.game.position-Vec2{-312,88})<80) round.playMoonCow601();
            }
            const bool retry=!round.horseBroadcast.active() &&
                ((pressed&PSP_CTRL_TRIANGLE) || (round.caught && confirm));
            const bool wonBeforeRetry=round.game.won;
            const Score retryScore=retry && wonBeforeRetry ? scoreFor(course,round) : Score{};
            const unsigned retryClock=round.ticks*5/3;
            const auto event=round.updateControls(aimInput(pad),
                !waitRelease && !round.caught && !round.ride.parryFrozen() &&
                    (pad.Buttons&PSP_CTRL_CROSS),retry);
            if (event==ControlEvent::Restart) {
                if (round.backrooms) startCourseMusic(currentCourse);
                if (wonBeforeRetry) {
                    recordCourseWin(currentCourse,retryScore,retryClock,false);
                    if (gifts.queue(giftFile,progress)) audio.play(Sound::GiftAward);
                } else {
                    ++progress.resets;
                }
                persistProgress(save,round,log.c_str());
                if (currentCourse>=Elysian518Course &&
                    (!loadFinaleCourse045(executable,currentCourse) || !loadCosmicCourse046(executable,currentCourse))) break;
                round.restart();
                resultStart=0;
                waitRelease=true;
                audio.play(Sound::Join);
                logEvent(log.c_str(),"restart",round.game);
            } else if (event==ControlEvent::Shot) {
                round.coins.multiplier=0;
                audio.play(Sound::Shot);
                thoughts.play(Thought::Moving,round.tvTick());
                logEvent(log.c_str(),"shot",round.game);
            } else if (!oldAim && round.controls.aiming) {
                audio.play(Sound::Grab);
            } else if (oldAim && !round.controls.aiming) {
                audio.play(Sound::Cancel);
            }
            const unsigned previousBounces=round.game.bounces,previousBonus=round.coins.bonus;
            const auto previousBlocks=round.brokenBlocks;
            const uint32_t previousGolfKey=progress.golfKey;
            const bool secretStarted=round.secret.startedAt!=0,marcyStarted=round.marcy.startedAt!=0;
            const uint32_t previousGlowstickKey=progress.glowstickKey,previousParkKey=progress.parkKey;
            const uint32_t previousFrontierKey=progress.frontierKey,previousElysianKey=progress.elysianKey,previousCosmicKey=progress.cosmicKey;
            const unsigned previousChuckles=round.chuckles.defeatedCount;
            const uint32_t previousQuest=progress.quest,previousTokens=round.stopRun.collected;
            const uint64_t previousSpeed=round.speedRun.collected;
            const auto previousSlotOutcome=round.slots.outcome;
            const uint32_t previousKeys=round.roundKeys.collected,previousDoors=round.roundKeys.opened;
            const unsigned previousSkullBonus=round.skulls.bonus,previousBaddieDefeats=round.baddies.defeatedCount;
            const bool wasWon=round.game.won,wasCaught=round.caught,wasInSand=round.inSand;
            const bool wasAirborne=round.jump.airborne(), policeTriggered=round.police.triggeredAt!=0;
            accumulator+=delta*60;
            while (accumulator>=1000000) {
                round.step(pad.Buttons&PSP_CTRL_RTRIGGER,
                           (pressed&PSP_CTRL_SQUARE) && gifts.opened(GiftReward::Parry),round.liftHorse);
                round.liftHorse=false;
                accumulator-=1000000;
            }
            if (!wasInSand && round.inSand) logEvent(log.c_str(),"sand",round.game);
            if (wasAirborne!=round.jump.airborne())
                logEvent(log.c_str(),round.jump.airborne()?"jump_pad":"jump_landed",round.game);
            if (!policeTriggered && round.police.triggeredAt) logEvent(log.c_str(),"police_trigger",round.game);
            if (!secretStarted && round.secret.startedAt) logEvent(log.c_str(),"secret_runner_started",round.game);
            if (!marcyStarted && round.marcy.startedAt) logEvent(log.c_str(),"marcy_started",round.game);
            if (round.chuckles.defeatedCount!=previousChuckles) logEvent(log.c_str(),"chuckle_defeated",round.game);
            if (round.coins.bonus>previousBonus) logEvent(log.c_str(),"coin",round.game);
            if (round.stopRun.collected!=previousTokens) logEvent(log.c_str(),"stop_token",round.game);
            if (round.slots.outcome!=previousSlotOutcome) {
                if (auto* out=std::fopen(log.c_str(),"a")) {
                    std::fprintf(out,"event=slots_resolved course=%s frames=%u,%u,%u outcome=%u bonus=%u tick=%u\n",
                                 CourseIds[currentCourse],static_cast<unsigned>(round.slots.frames[0]),
                                 static_cast<unsigned>(round.slots.frames[1]),static_cast<unsigned>(round.slots.frames[2]),
                                 static_cast<unsigned>(round.slots.outcome),round.slots.bonus,round.slots.outcomeTick);
                    std::fclose(out);
                }
            }
            if (round.speedRun.collected!=previousSpeed) logEvent(log.c_str(),"speed_token",round.game);
            if (round.brokenBlocks!=previousBlocks) logEvent(log.c_str(),"block_broken",round.game);
            if (round.keyStage && round.roundKeys.collected!=previousKeys) logEvent(log.c_str(),"round_key",round.game);
            if (round.keyStage && round.roundKeys.opened!=previousDoors) logEvent(log.c_str(),"key_door_opened",round.game);
            if (round.skulls.bonus!=previousSkullBonus) logEvent(log.c_str(),"skull_defeated",round.game);
            if (round.baddies.defeatedCount!=previousBaddieDefeats) logEvent(log.c_str(),"baddie_hit",round.game);
            if (progress.quest!=previousQuest) {
                persistProgress(save,round,log.c_str());
                logEvent(log.c_str(),"pool_quest_saved",round.game);
            }
            if (progress.golfKey!=previousGolfKey || progress.glowstickKey!=previousGlowstickKey ||
                progress.parkKey!=previousParkKey || progress.frontierKey!=previousFrontierKey || progress.elysianKey!=previousElysianKey || progress.cosmicKey!=previousCosmicKey) {
                persistProgress(save,round,log.c_str());
                const char* key=progress.cosmicKey!=previousCosmicKey?"cosmic_key":
                                progress.elysianKey!=previousElysianKey?"elysian_key":
                                progress.frontierKey!=previousFrontierKey?"frontier_key":
                                progress.parkKey!=previousParkKey?"park_key":
                                progress.glowstickKey!=previousGlowstickKey?"glowstick_key":"golf_key";
                logEvent(log.c_str(),key,round.game);
            }
            if (round.game.bounces>previousBounces) {
                if (currentCourse<CosmicFirstCourse && round.brokenBlocks==previousBlocks && round.baddies.defeatedCount==previousBaddieDefeats &&
                    round.skulls.bonus==previousSkullBonus && round.chuckles.defeatedCount==previousChuckles)
                    audio.play(Sound::Bounce);
                // Collision sounds are queued without touching the card.
            }
            if (usesRingTimers(currentCourse) && currentCourse!=Elysian512Course && ((!wasWon && round.game.won) || (!wasCaught && round.caught)))
                audio.stopRingTimers();
            if (!wasWon && round.game.won) {
                if (currentCourse==CosmicSeventeenthCourse) stopLastHurrahSounds053();
                resultStart=now;
                resultHit=resultRank=false;
#ifdef UCG_VICTORY_PROBE
                audio.resetTiming(AudioProbePhase::Victory);
                victoryProbeStart=now;
                victoryProbeCourse=currentCourse;
                victoryFrames=victoryRenderMax=0;
                victoryRenderSample={true,0,0,0};
                audio.probe(AudioProbePhase::Victory);
#endif
                audio.play(Sound::Win);
                if (round.horseBroadcast.active()) logEvent(log.c_str(),"horse_broadcast",round.game);
                else {
                    thoughts.play(Thought::Win,round.tvTick());
                    thoughts.setFallback(Thought::Win);
                    logEvent(log.c_str(),"goal",round.game);
                }
            }
            if (!wasCaught && round.caught) {
                if (currentCourse==CosmicSeventeenthCourse) stopLastHurrahSounds053();
                if (currentCourse==CosmicThirteenthCourse) audio.stageCue(StageCue::None);
                resultStart=now;
                if (!round.backrooms) {
                    thoughts.play(Thought::Death,round.tvTick());
                    thoughts.setFallback(Thought::Death);
                }
                if (round.backrooms) audio.muteMusic();
                else if (round.horse.phase==HorsePhase039::Lost && !round.horseGhostCaught && !round.fellOut && !round.waterDead && !round.finaleMineLoss) audio.play(Sound::HorseJumpscare);
                else if (!round.waterDead && !round.finaleMineLoss) audio.play(round.fellOut?Sound::OutOfBounds:Sound::Loss);
                logEvent(log.c_str(),round.backrooms?"backrooms":round.waterDead?"water":
                         round.fellOut?"jump_out_of_bounds":"caught",round.game);
            }
            unsigned resultMs=round.game.won?static_cast<unsigned>((now-resultStart)/1000):0;
            if (currentCourse==Elysian518Course && round.game.won && confirm && canAdvance(resultMs)) {
                acceptGoldenTee045(course,round,save,log.c_str());
                resultStart=0;
            }
            if (round.horseBroadcast.ready() ||
                (currentCourse==Elysian518Course && round.finale518.canEnterEnding()) ||
                (currentCourse!=Elysian518Course && round.game.won && confirm && !liftHorseInput && canAdvance(resultMs))) {
                if (usesRingTimers(currentCourse)) audio.stopRingTimers();
                const auto outcome=advanceCampaign(executable,course,round,save,log.c_str(),false);
                if (outcome==Advance::Fatal) break;
                if (outcome==Advance::Failed && round.horseBroadcast.active()) {
                    audio.stop(Sound::HorseAlarm);
                    audio.music(Sound::Menu);
                    openLevelSelect(flow,currentCourse);
                    phaseStart=now;
                    continue;
                }
                if (gifts.queue(giftFile,progress)) audio.play(Sound::GiftAward);
                if (outcome==Advance::CosmicEnding) {
                    flow.screen=Screen::CosmicEnding;
                    phaseStart=now;
                    continue;
                }
                if (outcome==Advance::ElysianEnding) {
                    flow.screen=Screen::ElysianEnding;
                    endingClock={};
                    phaseStart=now;
                    continue;
                }
                if (outcome==Advance::EndOfBuild) {
                    if (currentCourse==CosmicFifthCourse) {
                        audio.stageCue(StageCue::None);
                        audio.rideVolume(1,-80,-80);
                    }
                    openLevelSelect(flow,currentCourse);
                    audio.stageCue(StageCue::None);
                    phaseStart=now;
                    audio.music(Sound::Menu);
                    continue;
                }
                if (outcome==Advance::NextHole || outcome==Advance::WorldCleared) {
                    resultStart=0;
                    accumulator=0;
                    waitRelease=true;
                    if (outcome==Advance::WorldCleared) {
                        flow.screen=Screen::WorldResults;
                        phaseStart=now;
                        const auto world=worldResults.world();
                        audio.music(world==CompletedWorld::FinalFrontier?Sound::WorldClearFrontier:
                                    world==CompletedWorld::ChucklePark?Sound::WorldClearPark:
                                    world==CompletedWorld::GlowstickCity?Sound::WorldClearCity:
                                    world==CompletedWorld::GolfCentral?Sound::WorldClearGolf:Sound::WorldClear);
                    } else {
                        audio.play(Sound::Join);
                    }
                    continue;
                }
            }
            if (round.game.won && !hidesElysianScoring(currentCourse)) {
                if (confirm && !liftHorseInput && resultMs<2000) {
                    resultStart=now-2000000;
                    resultMs=2000;
                } else if (confirm && !liftHorseInput && resultMs<4400) {
                    resultStart=now-4400000;
                    resultMs=4400;
                }
                if (!resultHit && resultMs>=2000) {
                    resultHit=true;
                    audio.play(Sound::ResultsHit);
                }
                if (!resultRank && resultMs>=4400) {
                    resultRank=true;
                    audio.play(Sound::ResultsBang);
                    const unsigned rank=scoreFor(course,round).rank;
                    audio.play(static_cast<Sound>(static_cast<unsigned>(Sound::RankUncanny)+rank));
                }
            }
        }
        if (now-lastRender>=32000) {
#ifdef UCG_VICTORY_PROBE
            const unsigned renderStart=(victoryProbeStart || titleProbeStart)?sceKernelGetSystemTimeLow():0;
#endif
            render(flow,course,round,flow.screen==Screen::ElysianEnding?endingClock.ms():
                   static_cast<unsigned>((now-phaseStart)/1000),
                   round.ticks*5/3,round.totalTicks*5/3,
                   resultStart?static_cast<unsigned>((now-resultStart)/1000):0,
                   UINT32_MAX,static_cast<unsigned>((now-menuStart)/1000%MenuScroll::PeriodMs),endingClock.fast);
#ifdef UCG_VICTORY_PROBE
            if (titleProbeStart) titleRenderMax=std::max(titleRenderMax,sceKernelGetSystemTimeLow()-renderStart);
            if (victoryProbeStart) {
                ++victoryFrames;
                victoryRenderMax=std::max(victoryRenderMax,sceKernelGetSystemTimeLow()-renderStart);
            }
#endif
            frameReady=true;
            lastRender=now;
        }
    }
#ifdef UCG_VICTORY_PROBE
    finishVictoryProbe(sceKernelGetSystemTimeWide());
#endif
    if (flow.screen==Screen::Hole) persistProgress(save,round,log.c_str());
    audio.stop();
    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
