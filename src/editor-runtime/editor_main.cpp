#include "home_exit067.hpp"
#include "asset_layout.hpp"
#include "release_paths.hpp"
#include "release_loadexec.hpp"
#include "storage_hooks.hpp"
#include "controls.hpp"
#include "editor_loading_thread066.hpp"
#include "frame_profile066.hpp"
#include "editor_perf067.hpp"
#include "editor_navigation068.hpp"
#include "game.hpp"
#include "locale.hpp"
#include "media.hpp"
#include "editor_allocation_probe.hpp"
#include "editor_ground_cache.hpp"
#include "editor_side_background068.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <malloc.h>
#include <pspctrl.h>
#include <pspiofilemgr.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <psploadexec.h>
#include <psprtc.h>
#include <psppower.h>
#include <psputility.h>
#include <string>
#include <sys/stat.h>
#include <vector>
PSP_MODULE_INFO("UCG_EDITOR", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_MAIN_THREAD_STACK_SIZE_KB(256);
PSP_HEAP_SIZE_KB(-1);
PSP_HEAP_THRESHOLD_SIZE_KB(192);
namespace {
constexpr unsigned ScreenWidth = 480, ScreenHeight = 272, Stride = 512,
                   FrameBytes = Stride * ScreenHeight * 2;
constexpr int ViewX = 36;
constexpr uint32_t White = 0xffffffffu;
struct TextureVertex {
  float u, v;
  uint32_t color;
  float x, y, z;
};
struct ColorVertex {
  uint32_t color;
  float x, y, z;
};
alignas(64) unsigned displayList[256 * 1024];
alignas(64) uint16_t fallbackPage[512 * 272];
std::string executable;
bool drawOpen = false;
FrameProfile066::State phaseProfile066;
EditorPerf067::State editorPerf067;
EditorNavigation068::State editorNavigation068;
volatile uint32_t editorCheckpointUs068=0;
EditorPerf067::Stamp editorPerfStamp067(){const uint32_t now=sceKernelGetSystemTimeLow();return {now,phaseProfile066.buildUs+(drawOpen?now-phaseProfile066.buildStamp:0),phaseProfile066.syncUs};}
std::string companion(const char*,const char*);
void flushPhaseProfile066(){
 if(editorPerf067.count||editorPerf067.pending.frames){
  auto detail=companion(executable.c_str(),"editor-detail067.log");
  if(auto*f=std::fopen(detail.c_str(),"a")){bool good=editorPerf067.dump(f);int closed=std::fclose(f);if(good&&!closed)editorPerf067.clear();}
 }
 if(!phaseProfile066.count&&!phaseProfile066.pending.frames)return;
 auto path=companion(executable.c_str(),"editor-profile066.log");
 if(auto*f=std::fopen(path.c_str(),"a")){bool good=phaseProfile066.dump(f);int closed=std::fclose(f);if(good&&!closed)phaseProfile066.clear();}
}
uint64_t batchEpoch=0;
EditorGroundCache::Graphics groundGraphics{};
#ifdef UCG_EDITOR_CAPTURE
std::string companion(const char *name,const char *file);
unsigned captureOffset=0,captureSerial=0;
bool captureNext=false;
int startResults[4]{},finishResult=0,syncResult=0;unsigned startCount=0;
void captureFrame(){
  if(!captureNext)return;
  captureNext=false;
  const unsigned serial=captureSerial++;
  for(unsigned buffer=0;buffer<2;++buffer){
  auto path=companion(executable.c_str(),("capture-"+std::to_string(serial)+"-"+std::to_string(buffer)+".565").c_str());
  if(auto*f=std::fopen(path.c_str(),"wb")){
    std::setvbuf(f,nullptr,_IONBF,0);
    const auto*frame=reinterpret_cast<const void*>(0x44000000u+buffer*FrameBytes);
    std::fwrite(frame,1,FrameBytes,f);std::fclose(f);
  }
  }
  auto trace=companion(executable.c_str(),"capture-trace.txt");
  if(auto*f=std::fopen(trace.c_str(),"a")){std::fprintf(f,"capture=%u offset=%u starts=%u recent=%d,%d,%d,%d finish=%d sync=%d\n",serial,captureOffset,startCount,startResults[0],startResults[1],startResults[2],startResults[3],finishResult,syncResult);std::fclose(f);}
}
#endif
struct Probe {
  uint32_t mode, tab, nodes, deco, tiles, selected, profile;
  float x, y;
  uint32_t frames, free, largest;
  uint32_t browserMode,sceneTick,strokes,dead,won;
  float playerX,playerY,velocityX,velocityY;
  uint32_t heapFree,heapUsed,textureReads,customReads,failedImages,cacheBytes,renderError,settings,hudReady;
};
volatile Probe editorProbe{};
volatile uint32_t editorCursorVisible067=0,editorFocusTarget067=~0u;
volatile double pointerFrameScale=1;
struct MemoryProbe{uint32_t requested,maxRequested,reclaims,failed;};
volatile MemoryProbe editorMemoryProbe{};
volatile uint32_t editorMemoryLargest=0;
volatile uint32_t editorMemoryLargeBlocks=0;
struct AudioProbe{uint32_t rejected,underruns;};
struct AudioDiagnosticsProbe{uint32_t requests,starts,initialWaitBlocks,underrunBlocks,maxStartUsec,lateStarts;};
volatile AudioDiagnosticsProbe editorAudioDiagnostics{};
#ifdef UCG_EDITOR_CAPTURE
volatile AudioDiagnosticsProbe editorAudioSources[ucg::editor_audio::Count]{};
volatile uint32_t editorAudioSourceCount=ucg::editor_audio::Count;
struct ReaderDiagnosticsProbe{
  uint32_t musicSlices,maxMusicSliceUsec,serviceCalls,maxServiceUsec,pendingAdmissions,
    pendingObservations,lowReserveObservations,primingObservations,startedObservations,
    minQueuedPending,maxQueuedPending;
};
volatile ReaderDiagnosticsProbe editorReaderProbe{};
struct CacheDiagnosticsProbe{
  uint32_t total,hot,cold,hits,stores,storeFailures,storeSkips,stageDecodes,
    stageHits,promotions,allocationFailures,workspacePeak;
};
volatile CacheDiagnosticsProbe editorCacheProbe{};
struct ColdPoolDiagnosticsProbe{
  uint32_t segments,resident,used,high,requests,allocationFailures,evictions,
    invalidReleases,initializationAttempts,initializationFailures,
    hotReserveDenials,coldReserveDenials;
};
volatile ColdPoolDiagnosticsProbe editorColdPoolProbe{};
struct SampleCacheDiagnosticsProbe{uint32_t bytes,entries,hits,misses,stores,evictions,allocationFailures,sourceFailures,bypasses,clears;};
volatile SampleCacheDiagnosticsProbe editorSampleCacheProbe{},editorLitCacheProbe{};
struct BuilderDiagnosticsProbe{uint32_t pending,owned,spinFrame,draws,getAttempts,errors;};
volatile BuilderDiagnosticsProbe editorBuilderProbe{};
struct FileDiagnosticsProbe{uint32_t open,selected,scroll,count,relativeHash,nameHash;};
volatile FileDiagnosticsProbe editorFileProbe{};
struct LegacyAudioDiagnosticsProbe{
  uint32_t blocks,readMax,mixMax,gapMax,late,priming,underruns,effectUnderruns,
    readErrors,lastMissingEffect,musicBufferMin;
  int32_t outputError;
  uint32_t musicIoUs,effectIoUs,musicIoCalls,effectIoCalls,musicIoMax,effectIoMax,
    musicWorkMax,effectWorkMax,queuedFrames,queuedLowerBound;
};
volatile LegacyAudioDiagnosticsProbe editorLegacyAudioProbe{};
#endif
volatile AudioProbe editorAudioProbe{};
unsigned profile = 0;
std::string companion(const char *name, const char *file) {
  return ReleasePaths::moduleCompanion(name,file);
}
void setTexture(unsigned format, unsigned width, unsigned height,
                const void *pixels) {
  sceGuEnable(GU_TEXTURE_2D);
  sceGuTexMode(format, 0, 0, 1);
  sceGuTexImage(0, width, height, width, pixels);
  sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
}
void rectangle(float x, float y, float w, float h, uint32_t color) {
  sceGuDisable(GU_TEXTURE_2D);
  auto *v = static_cast<ColorVertex *>(sceGuGetMemory(2 * sizeof(ColorVertex)));
  v[0] = {color, x, y, 0};
  v[1] = {color, x + w, y + h, 0};
  sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                 2, nullptr, v);
}
void beginBatch() {
  if(phaseProfile066.enabled)phaseProfile066.begin(sceKernelGetSystemTimeLow());
  const int started=sceGuStart(GU_DIRECT, displayList);
#ifdef UCG_EDITOR_CAPTURE
  startResults[startCount++%4]=started;
#else
  (void)started;
#endif
  drawOpen = true;
  ++batchEpoch; // Every prior list has been submitted and fenced before reuse.
}
void flushBatch() {
  if (!drawOpen)
    return;
  if(editorPerf067.active)editorPerf067.noteList(unsigned(sceGuCheckList()));
  if(phaseProfile066.enabled)phaseProfile066.built(sceKernelGetSystemTimeLow());
  const int finished=sceGuFinish();
  const int synced=sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
  if(phaseProfile066.enabled)phaseProfile066.synced(sceKernelGetSystemTimeLow());
  groundGraphics.fenceOK=finished>=0&&synced>=0;
#ifdef UCG_EDITOR_CAPTURE
  finishResult=finished;syncResult=synced;
#else
  (void)finished;(void)synced;
#endif
  drawOpen = false;
}
bool restoreAfterArtWrite=false;
void artWriteBegin(){restoreAfterArtWrite=drawOpen;flushBatch();}
void artWriteEnd(){const bool restore=restoreAfterArtWrite;restoreAfterArtWrite=false;if(restore&&!drawOpen)beginBatch();}
void beginFrame() {
  beginBatch();
  sceGuScissor(0, 0, ScreenWidth, ScreenHeight);
  sceGuClearColor(0xff000000u);
  sceGuClear(GU_COLOR_BUFFER_BIT);
}
void endFrame();
AudioMixer audio;
Language language = Language::English;
bool russian() { return language == Language::Russian; }
const char *lang(const char *en, const char *ru) { return russian() ? ru : en; }
void event(const char *name, unsigned a = 0, unsigned b = 0) {
  auto path = companion(executable.c_str(), "editor-session.log");
  if (auto *f = std::fopen(path.c_str(), "a")) {
    std::fprintf(f, "event=editor_%s a=%u b=%u\n", name, a, b);
    std::fclose(f);
  }
}
unsigned selectedEditorGhostCue();
bool openAudio(const char *external = nullptr, double loop = 0,unsigned rings=0,unsigned missiles=0,unsigned beeGroups=0) {
  audio.probe(AudioProbePhase::Off);
  audio.stop();
  auto root = companion(executable.c_str(), "data/audio/");
  const bool ready=audio.open(
      (root + "audio.uca").c_str(), (root + "slots-audio.pak").c_str(),
      (root + "park-audio.pak").c_str(), (root + "park-timer-3_11.pcm").c_str(),
      nullptr, true, external, loop,rings,missiles,beeGroups,selectedEditorGhostCue());
  if(ready)audio.probe(AudioProbePhase::Map);
  return ready;
}
} // namespace
#include "editor_alpha.hpp"
#include "editor_art.hpp"
#include "editor_browser.hpp"
#include "editor_collision.hpp"
#include "editor_colors.hpp"
#include "editor_controller.hpp"
#include "editor_files.hpp"
#include "editor_images.hpp"
#include "editor_input.hpp"
#include "editor_keyboard.hpp"
#include "editor_panel.hpp"
#include "editor_play.hpp"
#include "editor_render.hpp"
#include "editor_builder_module.hpp"
#include "editor_legacy_text.hpp"
#include "editor_widget_text.hpp"
#include "editor_color_picker_art.hpp"
#include "editor_menu_art.hpp"
#include "editor_panel_color_art.hpp"
#include "tv_caption059.hpp"
#include "editor_play_hud.hpp"
#include "editor_modifier_hud062.hpp"
#include "editor_browser_text.hpp"
#include "editor_browser_rich.hpp"
#include "editor_browser_art.hpp"
#include "editor_browser_backdrop.hpp"
#include "editor_session.hpp"
#include "editor_settings_panel.hpp"
#include "editor_focus067.hpp"
#include "editor_original_cursor067.hpp"
#include "ucg_editor_catalog.hpp"
#include "ucg_editor_model.hpp"
namespace {
unsigned selectedEditorGhostCue(){return ucg::editor_audio::costumeSound(EditorPlayHud::pair.ghost);}
#include "editor_ground_gu.hpp"
#include "editor_ground_proof_publish.hpp"
EditorSideBackground068::Image sideBackground068;
void drawSideBackground068(){
  if(!sideBackground068.ready)return;
  if(!drawOpen)beginBatch();
  const auto clip=EditorArt::uiScissor;
  sceGuScissor(0,0,ScreenWidth,ScreenHeight);
  setTexture(GU_PSM_5650,128,512,sideBackground068.pixels);
  sceGuTexFilter(GU_NEAREST,GU_NEAREST);
  sceGuTexFlush();
  for(const auto& border:EditorSideBackground068::Borders){
    auto* v=static_cast<TextureVertex*>(sceGuGetMemory(2*sizeof(TextureVertex)));
    v[0]={float(border.u),0,White,float(border.x),0,0};
    v[1]={float(border.u+border.width),float(ScreenHeight),White,float(border.x+border.width),float(ScreenHeight),0};
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,nullptr,v);
  }
  sceGuScissor(clip[0],clip[1],clip[2],clip[3]);
}
void endFrame(){if(drawOpen)EditorModifierHud062::present(sceKernelGetSystemTimeWide());drawSideBackground068();flushBatch();EditorRender::GroundRuntime::finishFrame(groundGraphics.fenceOK);
#ifdef UCG_EDITOR_CAPTURE
 publishGroundProof();
#endif
}
void initializeGraphics() {
  scePowerSetClockFrequency(333,333,166);
#ifdef UCG_EDITOR_CAPTURE
  captureOffset=0;
#endif
  sceGuInit();
  beginBatch();
  sceGuDrawBuffer(GU_PSM_5650, nullptr, Stride);
  sceGuDispBuffer(ScreenWidth, ScreenHeight, reinterpret_cast<void *>(FrameBytes), Stride);
  sceGuOffset(2048 - ScreenWidth / 2, 2048 - ScreenHeight / 2);
  sceGuViewport(2048, 2048, ScreenWidth, ScreenHeight);
  sceGuScissor(0, 0, ScreenWidth, ScreenHeight);
  sceGuEnable(GU_SCISSOR_TEST);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuDisable(GU_CULL_FACE);
  sceGuDisable(GU_DITHER);
  sceGuEnable(GU_BLEND);
  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
  sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
  sceGuTexFilter(GU_NEAREST, GU_NEAREST);
  sceGuTexWrap(GU_CLAMP, GU_CLAMP);
  flushBatch();
  sceDisplayWaitVblankStart();
  sceGuDisplay(GU_TRUE);
  groundGraphics.drawOffset=0;bindGroundGraphics();
}
EditorBuilderModule::Adapter builder;
bool leaving = false;
void bootTrace065(const char*phase,const char*detail=nullptr){
  const bool first=std::strcmp(phase,"entry")==0;
  const int fd=sceIoOpen("ms0:/PSP/GAME/UCG054/editor-boot.log",PSP_O_WRONLY|PSP_O_CREAT|(first?PSP_O_TRUNC:PSP_O_APPEND),0777);
  if(fd<0)return;
  static uint64_t firstAt=0,lastAt=0;const uint64_t at=sceKernelGetSystemTimeWide();if(first){firstAt=lastAt=at;}
  const auto heap=mallinfo();char line[448];
  const int n=std::snprintf(line,sizeof line,"build=066 phase=%s elapsed_us=%llu phase_us=%llu free=%u largest=%u heap_used=%u heap_free=%u detail=%.120s\n",phase,(unsigned long long)(at-firstAt),(unsigned long long)(at-lastAt),unsigned(sceKernelTotalFreeMemSize()),unsigned(sceKernelMaxFreeMemSize()),unsigned(heap.uordblks),unsigned(heap.fordblks),detail?detail:"-");
  if(n>0)sceIoWrite(fd,line,std::min(unsigned(n),unsigned(sizeof(line)-1)));
  sceIoClose(fd);lastAt=at;
}
void checkpointNavigation068(unsigned tab,bool cursor){
  // The current frame has already been observed and the GE fenced. These
  // diagnostic writes are excluded from frame aggregates, never from game time.
  const uint32_t start=sceKernelGetSystemTimeLow();
  const auto&timing=audio.timing(AudioProbePhase::Map);
  const auto ones=audio.editorOneShotDiagnostics();
  const auto reader=audio.editorReaderDiagnostics();
  auto relaxed=[](const auto&v){return unsigned(v.load(std::memory_order_relaxed));};
  const unsigned blocks=relaxed(timing.blocks),late=relaxed(timing.late),underruns=relaxed(timing.underruns),readErrors=relaxed(timing.readErrors),musicIo=relaxed(timing.musicIoCalls),musicMax=relaxed(timing.musicIoMax),gapMax=relaxed(timing.gapMax),queued=audio.queuedMusicFrames();
  flushPhaseProfile066();
  const unsigned profileIo=sceKernelGetSystemTimeLow()-start;
  auto path=companion(executable.c_str(),"editor-navigation069.log");
  if(auto*f=std::fopen(path.c_str(),"a")){
    std::fprintf(f,"build=069 reason=warm_idle context=%u tab=%u cursor=%u tab_frames=%u input_frames=%u source_reads=%u custom_reads=%u cold_decodes=%u art_decodes=%u font_reads=%u font_cache_bytes=%u cache_bytes=%u ground_valid=%u ground_key=%llu audio_blocks=%u audio_late=%u audio_underruns=%u audio_read_errors=%u audio_music_io_calls=%u audio_music_io_max_us=%u audio_gap_max_us=%u audio_queued_frames=%u oneshot_requests=%u oneshot_starts=%u oneshot_underruns=%u reader_music_slices=%u reader_music_slice_max_us=%u reader_service_max_us=%u profile_checkpoint_us=%u previous_checkpoint_total_us=%u\n",editorNavigation068.previous,tab,unsigned(cursor),editorNavigation068.frames[tab],editorNavigation068.inputs,EditorRender::textureReads,EditorRender::customReads,EditorRender::coldStageDecodes,EditorArt::inflateRecords067,EditorRender::FontMasks::reads,EditorRender::FontMasks::cacheBytes(),EditorRender::cacheTotalBytes(),unsigned(EditorRender::ground.valid),(unsigned long long)EditorRender::ground.key,blocks,late,underruns,readErrors,musicIo,musicMax,gapMax,queued,ones.requests,ones.starts,ones.underrunBlocks,reader.musicSlices,reader.maxMusicSliceUsec,reader.maxServiceUsec,profileIo,unsigned(editorCheckpointUs068));
    std::fclose(f);
  }
  editorCheckpointUs068=sceKernelGetSystemTimeLow()-start;
}
void returnMain() {
  flushPhaseProfile066();
  const uint64_t age=builder.view().animationMs;
  const uint32_t carriedGiftAge=uint32_t(age<500?age:500+(age-500)%2000);
  builder.shutdown();
  audio.stop();
  EditorSettingsArt::unload();
  EditorPlayHud::unloadFX();
  EditorPlayHud::unloadState();
  EditorPlayHud::unload();
  EditorRender::shutdown();
  EditorArt::unload();
  EditorRender::GroundRuntime::invalidate();groundGraphics.contextReady=false;sceGuTerm();
  auto path = companion(executable.c_str(), "EBOOT.PBP");
  std::string args = path;
  args.push_back(0);
  args += "editor-return";
  args.push_back(0);
  args += std::to_string(profile);
  args.push_back(0);
  args += std::to_string(carriedGiftAge);
  args.push_back(0);
  SceKernelLoadExecParam parameters{};
  parameters.size = sizeof parameters;
  parameters.args = args.size();
  parameters.argp = const_cast<char *>(args.data());
  int result = ReleaseLoadExec::execute(path.c_str(), &parameters);
  event("return_failed", unsigned(result));
  sceKernelExitGame();
  leaving = true;
}
} // namespace
int main(int argc, char **argv) {
    if(HomeExit067::install()<0){sceKernelExitGame();return 1;}
  bootTrace065("entry",argc>0&&argv&&argv[0]?argv[0]:"missing argv0");
  executable = argc ? argv[0] : "ms0:/PSP/GAME/UCG053EDIT/EDITOR.PBP";
  const std::string moduleSuffix = "/data/editor/EBOOT.PBP";
  if (executable.size() >= moduleSuffix.size() &&
      executable.compare(executable.size() - moduleSuffix.size(),
                         moduleSuffix.size(), moduleSuffix) == 0)
    executable.resize(executable.size() - moduleSuffix.size()),
        executable += ReleasePaths::disc(executable) ? "/EBOOT.BIN" : "/EBOOT.PBP";
  ucg_storage::configure(executable.c_str());
  if (argc > 2)
    profile = std::min(7, std::max(0, std::atoi(argv[2])));
  editorProbe.profile = profile;
  sceCtrlSetSamplingCycle(0);
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
  bootTrace065("graphics_begin");
  initializeGraphics();
  bootTrace065("graphics_done");
  const auto loading066=EditorLoadingThread066::start(reinterpret_cast<uint16_t*>(reinterpret_cast<uintptr_t>(sceGeEdramGetAddr())|0x40000000u),unsigned(sceGeEdramGetSize()),fallbackPage,sizeof fallbackPage);
  bootTrace065("loading_presented",loading066.spinning?"original_prism_center_black_spinning":loading066.shown?"original_prism_center_black_static":"failed");
  AvatarIdle059::cosmetic.state=sceKernelGetSystemTimeLow();AvatarEditorSpeech059::initialize();TvCaption059::publish();
  EditorArt::pageWriteBegin=artWriteBegin;
  EditorArt::pageWriteEnd=artWriteEnd;
  language = Language::English; // Owner preference: Editor only; Main locale file unchanged.
  EditorController::Controller c;
  EditorPanel::Panel panel(c);
  EditorBrowser::Browser browser;
  browser.clock=[](){return EditorPlayHud::shaderTime;};
  browser.onBeforeRefresh=[](){EditorRender::releaseCaches();};
  browser.onBeforeResults=[](){
    EditorRender::reclaimPageMemory();
    EditorBrowserRender::release();
  };
  EditorSession::Session session;
  EditorSettings::Panel settings;
  EditorTiming::FixedStep pointerTiming;
#ifdef UCG_EDITOR_CAPTURE
  const auto memoryLogPath=companion(executable.c_str(),"editor-memory.log");
  FILE*memoryLog=std::fopen(memoryLogPath.c_str(),"wb");
  if(memoryLog){std::setvbuf(memoryLog,nullptr,_IONBF,0);std::fputs("phase=memory-log-ready\n",memoryLog);}
  auto memoryTrace=[&](const char*phase,size_t requested){
    if(!memoryLog)return;
    const auto heap=mallinfo();char line[192];
    const int length=std::snprintf(line,sizeof line,"phase=%s requested=%u largest=%u large_blocks=%u used=%u free=%u cache=%u frame=%u\n",phase,unsigned(requested),unsigned(editorMemoryLargest),unsigned(editorMemoryLargeBlocks),unsigned(heap.uordblks),unsigned(heap.fordblks),EditorRender::cacheBytes(),unsigned(editorProbe.frames));
    if(length>0)std::fwrite(line,1,std::min<unsigned>(unsigned(length),sizeof(line)-1),memoryLog);
  };
#else
  auto memoryTrace=[](const char*,size_t){};
#endif
  bool editing = false;
  unsigned editorMusicFrame=0;
  bootTrace065("art_begin",executable.c_str());
  if (!EditorArt::load(executable.c_str(), fallbackPage, sizeof fallbackPage))
    panel.say("Cannot load editor artwork");
  bootTrace065("art_done",EditorArt::ready?(EditorArt::menuOverlay.active()?"ready overlay066":"ready overlay_fallback"):"failed");
  EditorLegacyText::install();EditorWidgetText::install();
  bootTrace065("hud_begin");
  if(!EditorPlayHud::load(executable.c_str(),profile,russian())||!EditorPlayHud::loadTV(executable.c_str())||!EditorPlayHud::loadState(executable.c_str())||!EditorPlayHud::loadFX(executable.c_str()))panel.say(EditorPlayHud::error);
  bootTrace065("hud_done",EditorPlayHud::ready&&EditorPlayHud::tvReady?"ready":EditorPlayHud::error.c_str());
  EditorRender::playerHook=EditorPlayHud::player;
  EditorRender::ghostHook=EditorPlayHud::ghost;
  EditorRender::ghostPlainHook=EditorPlayHud::ghostPlain;
  bootTrace065("audio_begin");
  if (!openAudio())
    panel.say(std::string("Cannot open audio: ") + audio.failure());
  else
    audio.music(static_cast<Sound>(ucg::editor_audio::BrowserMusic));
  bootTrace065("audio_done");
  auto applySettings=[&](){
    session.powerSensitivity=settings.values.sensitivity;
    session.invertShot=settings.values.invert;
    session.bufferShots=session.scene.bufferShots=settings.values.buffer;
    audio.editorBusVolumes(float(settings.values.master),float(settings.values.music),float(settings.values.sounds));
  };
  settings.open(companion(executable.c_str(),"editor-settings.json"));
  settings.onChange=applySettings;
  settings.onClose=[&](){pointerTiming.reset();session.timing.reset();};
  applySettings();
  if(!settings.error.empty())panel.say(settings.error);
  session.onSettings=[&](){EditorRender::setCachePagesLimit(1);EditorRender::invalidate();settings.show();};
  std::string error;
  bootTrace065("browser_begin");
  if(!builder.init(companion(executable.c_str(),""),[](unsigned id,float gain,uint32_t owner){return audio.editorOneShot(id,owner,0,gain,1.f);},error))panel.say(error);
  EditorFiles::installExample(
      companion(executable.c_str(), "data/world/editor-examples/ExampleWorld"),
      companion(executable.c_str(), "custom_levels"), error);
  if (!error.empty())
    panel.say(error);
  const auto customRoot = companion(executable.c_str(), "custom_levels");
  const auto statsPath =
      companion(executable.c_str(),
                "editor-stats.json");
  if (!browser.open(customRoot, statsPath, russian(), error))
    panel.say(error);
  browser.onReturnMain = returnMain;
  auto configureHitTests=[&](){
    c.model.before_reserve=[&](const ucg::editor::AllocationBudget&budget,std::string&e){
      const size_t additional=budget.total_bytes;
      if(!additional)return true;
      editorMemoryLargest=unsigned(budget.largest_block);
      editorMemoryLargeBlocks=unsigned(budget.large_blocks);
      memoryTrace("guard-request",additional);
      editorMemoryProbe.requested=unsigned(additional);
      editorMemoryProbe.maxRequested=std::max(unsigned(editorMemoryProbe.maxRequested),unsigned(additional));
      auto probe=[&](){
        return EditorMemory::probe(additional,budget.largest_block,budget.large_blocks);
      };
      if(probe()){memoryTrace("guard-pass",additional);return true;}
      const bool resumeList=drawOpen;
      auto restoreList=[&](){if(resumeList&&!drawOpen)beginBatch();};
      editorMemoryProbe.reclaims=editorMemoryProbe.reclaims+1;
      EditorRender::reclaimPageMemory();
      if(probe()){restoreList();memoryTrace("guard-pages-pass",additional);return true;}
      EditorRender::releaseCaches();EditorRender::invalidate();
      if(probe()){restoreList();memoryTrace("guard-release-pass",additional);return true;}
      restoreList();
      editorMemoryProbe.failed=editorMemoryProbe.failed+1;
      memoryTrace("guard-denied",additional);
      e=lang("Not enough memory for this level edit; document unchanged.","„N„u„t„€„ƒ„„„p„„„€„‰„~„€ „„p„}„‘„„„y „t„|„‘ „y„x„}„u„~„u„~„y„‘ „…„‚„€„r„~„‘; „t„€„{„…„}„u„~„„ „~„u „y„x„}„u„~„v„~.");return false;
    };
    c.decorationSize=[&](const ucg::editor::Value&item,double&w,double&h){
    if(item.array.size()>10&&!item.array[10].text.empty()){std::string path;EditorImages::Info info;if(EditorFiles::join(EditorFiles::base(c.path),item.array[10].text,path)&&EditorImages::inspect(path.c_str(),info)){w=info.width;h=info.height;return true;}return false;}
    std::string path=item.array[9].text;if(path.rfind("res://",0)==0)path.erase(0,6);for(const auto&image:EditorArt::Packed::Images)if(path==image.path){w=image.width;h=image.height;return true;}return false;
  };};configureHitTests();
  browser.onOpenEditor = [&]() {
    editing = true;
    c = EditorController::Controller();
    configureHitTests();
    panel.showWelcome();
    audio.music(static_cast<Sound>(ucg::editor_audio::EditorMusic));
    event("opened");
  };
  browser.onLaunchWorldLevel = [&](const std::string &folder,
                                   const std::string &file, std::string &e) {
    ucg::editor::Document doc;
    if (!ucg::editor::load_file(folder + "/" + file, doc, e))
      return false;
    return session.start(doc, folder + "/" + file, false, e);
  };
  browser.onBuilderReward = [&](std::string &e) {
    const bool ready=builder.queue(e);
    if(ready)event(builder.pending()?"builder_pending":"builder_owned",profile);
    return ready;
  };
  panel.onExit = [&]() {
    // The confirmed exit discards the PC editor scene; play-test keeps it.
    c.model.clear_memory();
    c.stroke = ucg::editor::Document();
    c.dragOriginal = ucg::editor::Document();
    c.dragging = c.painting = c.erasing = false;
    panel.showWelcome();
    EditorRender::releaseCaches();
    editing = false;
    browser.mode = EditorBrowser::Browser::Mode::Menu;
    audio.music(static_cast<Sound>(ucg::editor_audio::BrowserMusic));
    event("closed");
  };
  panel.onPlay = [&](std::string &e) {
    editorMusicFrame=audio.musicPositionFrames(static_cast<Sound>(ucg::editor_audio::EditorMusic));
    const bool ready=session.start(c.model.document,c.path,true,e);
    if(!ready&&c.music)audio.musicAt(static_cast<Sound>(ucg::editor_audio::EditorMusic),editorMusicFrame);
    return ready;
  };
  session.onComplete = [&](int64_t score, unsigned strokes, double seconds,
                           int rank) {
    browser.notifyFinished(score, strokes, seconds, rank);
    if(browser.mode==EditorBrowser::Browser::Mode::LevelResults)browser.next();
    if(browser.mode==EditorBrowser::Browser::Mode::Results)audio.music(static_cast<Sound>(ucg::editor_audio::ResultsMusic));
  };
  session.onExit = [&]() {
    if (session.test) {
      editing = true;
      if (!openAudio())
        panel.say(audio.failure());
      if (c.music)
        audio.musicAt(static_cast<Sound>(ucg::editor_audio::EditorMusic),editorMusicFrame);
    } else {
      editing = false;
      browser.returnToWorlds();
      if (!openAudio())
        panel.say(audio.failure());
      audio.music(static_cast<Sound>(ucg::editor_audio::BrowserMusic));
    }
  };
  session.onSkip = [&]() { browser.notifySkipped();if(browser.mode==EditorBrowser::Browser::Mode::Playing&&session.active)EditorCues::play(322);if(browser.mode==EditorBrowser::Browser::Mode::Results)audio.music(static_cast<Sound>(ucg::editor_audio::ResultsMusic)); };
  panel.thumbnail = EditorRender::drawThumbnail;
  panel.onSampleColor=[](ucg::editor::Vec2 at){
    ucg::editor::Color color{0,0,0,1};
    void* pixels=nullptr;int stride=0,format=0;
    if(sceDisplayGetFrameBuf(&pixels,&stride,&format,PSP_DISPLAY_SETBUF_IMMEDIATE)<0||!pixels||stride<=0)return color;
    const int x=std::clamp(ViewX+int(at.x*EditorArt::Scale),0,479),y=std::clamp(int(at.y*EditorArt::Scale),0,271);
    const auto* memory=reinterpret_cast<const uint8_t*>(reinterpret_cast<uintptr_t>(pixels)|0x40000000u);
    uint32_t value=0;
    if(format==PSP_DISPLAY_PIXEL_FORMAT_565){const uint16_t p=reinterpret_cast<const uint16_t*>(memory)[y*stride+x];color={double(p&31)/31,double(p>>5&63)/63,double(p>>11&31)/31,1};return color;}
    if(format==PSP_DISPLAY_PIXEL_FORMAT_8888)value=reinterpret_cast<const uint32_t*>(memory)[y*stride+x];else return color;
    color={double(value&255)/255,double(value>>8&255)/255,double(value>>16&255)/255,double(value>>24)/255};return color;
  };
  panel.onKeyboardClosed=[&](){
    pointerTiming.reset();session.timing.reset();
    builder.invalidateGraphics();
    EditorRender::GroundRuntime::invalidate();groundGraphics.contextReady=false;sceGuTerm();initializeGraphics();
    EditorRender::invalidate();
    if(!EditorArt::load(executable.c_str(),fallbackPage,sizeof fallbackPage))panel.say("Cannot restore editor UI after keyboard");
    EditorLegacyText::install();EditorWidgetText::install();
  };
  EditorBrowser::Canvas canvas;
  canvas.rect = EditorPanel::box;
  canvas.richText=EditorBrowserRich::draw;
  canvas.titleFont=1003;canvas.bodyFont=1001;canvas.authorFont=1000;canvas.sublevelFont=1004;canvas.scoreFont=1005;
  canvas.descriptionFont=2008;canvas.smallFont=2009;canvas.resultFont=2000;canvas.resultTitleFont=1002;
  canvas.fontMetrics=[](unsigned font){
    if(font>=2000){const auto&f=EditorRender::HudFonts::Fonts[font-2000];return std::make_pair(f.ascent,f.descent);}
    if(font>=1000){const auto&f=EditorRender::BrowserFonts::Fonts[font-1000];return std::make_pair(f.ascent,f.descent);}
    return std::make_pair(16.f,3.f);
  };
  canvas.text=[](unsigned font,const char*text,float x,float baseline,uint32_t tint,float width){
    if(font>=2000){const auto&f=EditorRender::HudFonts::Fonts[font-2000];EditorPlayHud::FontText::label(text,font-2000,x,baseline-f.ascent,width,0,false,false,false,3,tint);}
    else if(font>=1000){const auto&f=EditorRender::BrowserFonts::Fonts[font-1000];EditorBrowserText::FontText::label(text,font-1000,x,baseline-f.ascent,width,0,0,false,false,3,tint);}
    else EditorArt::text(font,text,x,baseline,tint,width);
  };
  canvas.measure=[](unsigned font,const char*text){return font>=2000?EditorPlayHud::FontText::measure(text,font-2000):font>=1000?EditorBrowserText::FontText::measure(text,font-1000):EditorArt::textWidth(font,text);};
  canvas.label=[](unsigned font,const char*text,float x,float y,float w,float h,unsigned align,bool middle,bool wrap,uint32_t tint){
    if(font>=2000)EditorPlayHud::FontText::label(text,font-2000,x,y,w,h,align==1,middle,wrap,3,tint);
    else EditorBrowserText::FontText::label(text,font-1000,x,y,w,h,align,middle,wrap,3,tint);
  };
  canvas.skin=[](unsigned skin,float x,float y,float w,float h,uint32_t tint){EditorBrowserArt::draw(static_cast<EditorBrowserArt::Skin>(skin),x,y,w,h,tint);};
  canvas.focus=[](float x,float y,float w,float h,uint32_t tint){EditorBrowserArt::drawFocus(x,y,w,h,tint);};
  canvas.scrollbar=[](bool horizontal,unsigned state,float x,float y,float w,float h){EditorBrowserArt::drawScroll(horizontal,state,x,y,w,h);};
  canvas.back=[&](bool ru,float time,float y){EditorSettingsArt::drawBack(executable.c_str(),ru,time,y);};
  canvas.background=[](bool results,float){EditorBrowserRender::backdrop(results,float(EditorPlayHud::shaderTime));};
  canvas.fontByPath=[](const std::string&path,unsigned current){if(path=="res://assetsNEW/fonts/impact.ttf")return 2010u;return path.empty()?current:~0u;};
  canvas.worldIcon=[](const std::string&path,bool results,float x,float y,float w,float h,uint32_t tint){
    const auto file=path.empty()?companion(executable.c_str(),results?EditorRender::HudAssets::Sprites[0].path:EditorBrowserArt::DefaultIcon.path):path;
    EditorRender::ThumbnailRuntime::draw(file,x,y,w,h,results?512:96,tint);
  };
  canvas.clip = [](float x, float y, float w, float h) {
    EditorArt::scissor(ViewX + int(x * EditorArt::Scale),
                       int(y * EditorArt::Scale),
                       ViewX + int((x + w) * EditorArt::Scale),
                       int((y + h) * EditorArt::Scale));
  };
  canvas.resetClip = EditorArt::fullView;
  std::vector<std::array<int,4>> browserClips;
  canvas.pushClip=[&](float x,float y,float w,float h){
    const auto previous=EditorArt::uiScissor;browserClips.push_back(previous);canvas.clip(x,y,w,h);
    const auto next=EditorArt::uiScissor;EditorArt::scissor(std::max(previous[0],next[0]),std::max(previous[1],next[1]),std::min(previous[2],next[2]),std::min(previous[3],next[3]));
  };
  canvas.popClip=[&](){if(browserClips.empty())return;const auto previous=browserClips.back();browserClips.pop_back();EditorArt::scissor(previous[0],previous[1],previous[2],previous[3]);};
  canvas.menu = [](bool ru, unsigned selected, unsigned state) {
    if(!EditorArt::frameResident(EditorArt::Packed::EditorMenus[unsigned(ru)][selected%2][state%3]))flushBatch();
    bool ready = EditorArt::prepareEditorMenu(ru, selected, state);
    if(!drawOpen)beginBatch();
    if (ready)
      EditorArt::drawEditorMenu(ru, selected, state);
  };
  canvas.image = [](const std::string &path, float x, float y, float w,
                    float h) {
    return EditorRender::drawCustomThumbnail(path, x, y, w, h);
  };
  panel.onBeforeLoad=[&](){
    editorNavigation068={}; // A new or retried document load starts fresh warmup.
    const auto heap=mallinfo();event("before_load",unsigned(heap.uordblks),unsigned(heap.fordblks));
    EditorRender::reclaimPageMemory();
  };
  panel.onReload = [&]() {
    EditorRender::invalidate();
    std::vector<EditorFiles::Entry> entries;
    std::string e;
    auto folder = EditorFiles::base(c.path);
    if (folder.empty())
      return;
    if (!EditorFiles::list(folder, "", entries, e)) {
      panel.say(e);
      return;
    }
    c.customDeco.clear();
    for (const auto &f : entries)
      if (!f.directory && (EditorFiles::extension(f.name, ".png") ||
                           EditorFiles::extension(f.name, ".jpg")))
        c.customDeco.push_back(f.name);
  };
  panel.onMusic = [&]() {
    if (c.music)
      audio.music(static_cast<Sound>(ucg::editor_audio::EditorMusic));
    else
      audio.muteMusic();
  };
  const bool loadingStopped066=EditorLoadingThread066::stop();
  char loadingStatus066[96];std::snprintf(loadingStatus066,sizeof loadingStatus066,"%s frames=%u changes=%u error=%d",loadingStopped066?"joined_display1":"failed",unsigned(EditorLoadingThread066::presented.load()),unsigned(EditorLoadingThread066::changes.load()),int(EditorLoadingThread066::failure.load()));bootTrace065("loading_stopped",loadingStatus066);
  if(!loadingStopped066){sceKernelExitGame();return 1;}
  const bool sideReady068=sideBackground068.load(executable.c_str());
  if(sideReady068)sceKernelDcacheWritebackRange(sideBackground068.pixels,sizeof sideBackground068.pixels);
  event("side_background",sideBackground068.active,sideReady068);
  bootTrace065("ready");
  event("ready", sceKernelTotalFreeMemSize(), sceKernelMaxFreeMemSize());
  EditorInput::Decoder decoder;
  unsigned previous = 0;
  ucg::editor::Vec2 pointer{512, 256}, oldPointer = pointer;
  UiModality067::State uiMode067;UiModality067::Repeat uiRepeat067;EditorFocus067::State uiFocus067;
  auto pointerCaptured=[&](){
    if(!session.active||session.paused||settings.active)return false;
    const auto phase=session.scene.visual_state().phase;
    return phase==EditorPlay::VisualState::Phase::Telekinetic ||
       (phase==EditorPlay::VisualState::Phase::Jump&&(session.scene.modifier_mask()&(1u<<2)));
  };
  uint64_t builderClock=sceKernelGetSystemTimeWide();
  uint64_t builderRemainder=0;
  phaseProfile066.enabled=true;unsigned previousProfileContext066=~0u;
  while (!leaving) {
    const uint32_t frameStart066=sceKernelGetSystemTimeLow();
    const unsigned profileContext066=settings.active?5u:session.active?(session.paused?4u:3u):editing?2u:16u+unsigned(browser.mode);
    if(profileContext066!=previousProfileContext066){if(profileContext066!=3u)flushPhaseProfile066();previousProfileContext066=profileContext066;}
    const auto perfCounters067=[](){return EditorPerf067::Counters{EditorRender::textureReads,EditorRender::customReads,EditorRender::coldStageDecodes,EditorArt::inflateRecords067,unsigned(batchEpoch),EditorRender::FontMasks::reads};};
    editorPerf067.begin(profileContext066,perfCounters067());
    const uint64_t builderNow=sceKernelGetSystemTimeWide();
    builderRemainder+=builderNow-builderClock;builderClock=builderNow;
    builder.update(uint32_t(std::min<uint64_t>(builderRemainder/1000,UINT32_MAX)));builderRemainder%=1000;
    editorAudioProbe.rejected=audio.editorOneShotRejected();
    editorAudioProbe.underruns=audio.editorOneShotUnderruns();
    EditorPlayHud::updateTime();
    SceCtrlData pad{};
    sceCtrlReadBufferPositive(&pad, 1);
    unsigned held = pad.Buttons, pressed = held & ~previous,
             released = previous & ~held;
    previous = held;
    auto actions = decoder.step(held, pressed, released);
#ifdef UCG_EDITOR_CAPTURE
    if((held&(EditorInput::L|EditorInput::R|EditorInput::Select|EditorInput::Start))==(EditorInput::L|EditorInput::R|EditorInput::Select|EditorInput::Start)&&(pressed&EditorInput::Start)){
      captureNext=true;actions={};pressed=released=0;
    }
#endif
    auto axis = [](unsigned value) {
      float n = (int(value) - 128) / 127.f;
      return std::abs(n) < .16f ? 0.f
                                : std::copysign((std::abs(n) - .16f) / .84f, n);
    };
    oldPointer = pointer;
    const auto pointerFrame=pointerTiming.advance(sceKernelGetSystemTimeWide());
    pointerFrameScale=pointerFrame.milliseconds*.06;
    const bool pausePointerBlocked=session.active&&!settings.active&&(session.paused||
      (!session.test&&session.scene.ready_for_input()&&(actions.pause||actions.back)));
    const bool browserButtons=!session.active&&!editing&&!settings.active&&
      (browser.mode==EditorBrowser::Browser::Mode::Menu||browser.mode==EditorBrowser::Browser::Mode::Worlds);
    if(editing&&EditorPanel::PanelTestAccess::pointerRequired(panel))uiMode067.cursor=true;
    const bool uiNavigation067=!session.active||session.paused||settings.active;
    const bool precise067=pointerCaptured()||actions.middle||(editing&&EditorPanel::PanelTestAccess::precise(panel));
    if(uiNavigation067)uiMode067.observe(UiModality067::analog(pad.Lx,pad.Ly),held,precise067);else uiMode067.cursor=true;
    double speed = pausePointerBlocked?0.:(!session.active&&actions.shift ? 1.5 : 5.)*pointerFrameScale;
    const double dx = (uiMode067.cursor?axis(pad.Lx)*speed:0.) +
                               (session.active&&!settings.active&&!browserButtons&&!(held & EditorInput::Select)
                                    ? ((held & EditorInput::Right ? 1 : 0) -
                                       (held & EditorInput::Left ? 1 : 0)) *
                                          speed
                                    : 0);
    const double dy = (uiMode067.cursor?axis(pad.Ly)*speed:0.) +
                               (session.active&&!settings.active&&!browserButtons&&!(held & EditorInput::Select)
                                    ? ((held & EditorInput::Down ? 1 : 0) -
                                       (held & EditorInput::Up ? 1 : 0)) *
                                          speed
                                    : 0);
    const auto moved=EditorInput::movePointer(pointer.x,pointer.y,dx,dy,pointerCaptured()&&!pausePointerBlocked);
    pointer={moved.x,moved.y};oldPointer={moved.previousX,moved.previousY};
    const unsigned repeat067=uiRepeat067.step(uiMode067.cursor?0:held,pressed,sceKernelGetSystemTimeWide());
    if(!uiMode067.cursor&&uiNavigation067)pressed=(pressed&~UiModality067::Directions)|repeat067;
    auto interaction067=pointer;uiFocus067.valid=false;
    if(!uiMode067.cursor&&!session.active&&!settings.active){
      const unsigned nav067=((held&EditorInput::Select)||precise067)?0:pressed&UiModality067::Directions;
      if(editing){EditorPanel::PanelTestAccess::prepare(panel,uiFocus067,nav067);interaction067=uiFocus067.point(pointer);pressed&=~UiModality067::Directions;if(!uiFocus067.valid)actions.primaryDown=false;}
      else if(browserButtons){EditorBrowser::FocusAccess067::native(browser);interaction067={-100,-100};}
      else{EditorBrowser::FocusAccess067::prepare(browser,uiFocus067,nav067);interaction067=uiFocus067.point(pointer);pressed&=~UiModality067::Directions;if(!uiFocus067.valid)actions.primaryDown=false;}
    }
    if(!uiMode067.cursor&&settings.active)settings.keyFocus=true;
    editorCursorVisible067=uiMode067.cursor&&(!session.active||settings.active||session.scene.dead||session.scene.game.won);editorFocusTarget067=uiFocus067.valid?unsigned(uiFocus067.focus.id):~0u;
    const bool wasPlaying=session.active;
    const bool wasModal=panel.modal();
    if (settings.active)
      settings.update(actions,pointer,oldPointer,pressed);
    else if (session.active)
      session.update(actions, pointer, oldPointer, pressed, EditorNativeShot069::aim(held,pad.Lx,pad.Ly));
    else if (editing)
      panel.update(actions, interaction067, pressed, held);
    else
      browser.update(actions, interaction067, pressed, held);
    editorProbe.mode=session.active?2:editing?1:0;editorProbe.browserMode=unsigned(browser.mode);
    editorProbe.sceneTick=session.scene.tick;editorProbe.strokes=session.scene.game.strokes;editorProbe.dead=session.scene.dead;editorProbe.won=session.scene.game.won;
    editorProbe.playerX=session.scene.game.position.x;editorProbe.playerY=session.scene.game.position.y;editorProbe.velocityX=session.scene.game.velocity.x;editorProbe.velocityY=session.scene.game.velocity.y;
    editorProbe.x = float(pointer.x);
    editorProbe.y = float(pointer.y);
    editorProbe.tab = c.tab;
    editorProbe.nodes = c.model.document.nodes().size();
    editorProbe.deco = c.model.document.decorations().size();
    editorProbe.tiles = 0;
    for (unsigned i = 0; i < 6; ++i)
      editorProbe.tiles += c.model.document.tiles(i).size();
    editorProbe.selected =
        c.model.selection_valid ? uint32_t(c.model.selection_index) : ~0u;
    ++editorProbe.frames;
    editorProbe.free = sceKernelTotalFreeMemSize();
    editorProbe.largest = sceKernelMaxFreeMemSize();
    const auto heap=mallinfo();
    editorProbe.heapFree=heap.fordblks;editorProbe.heapUsed=heap.uordblks;
    editorProbe.textureReads=EditorRender::textureReads;editorProbe.customReads=EditorRender::customReads;
    editorProbe.failedImages=EditorRender::failedImages;editorProbe.cacheBytes=EditorRender::cacheBytes();
    editorProbe.renderError=!EditorRender::getError().empty();editorProbe.settings=settings.active;
    editorProbe.hudReady=EditorPlayHud::ready&&EditorPlayHud::stateReady&&EditorPlayHud::fxReady;
    auto storeDiagnostics=[](volatile AudioDiagnosticsProbe&out,const auto&d){out.requests=d.requests;out.starts=d.starts;out.initialWaitBlocks=d.initialWaitBlocks;out.underrunBlocks=d.underrunBlocks;out.maxStartUsec=d.maxStartUsec;out.lateStarts=d.lateStarts;};
    storeDiagnostics(editorAudioDiagnostics,audio.editorOneShotDiagnostics());
#ifdef UCG_EDITOR_CAPTURE
    editorAudioSourceCount=ucg::editor_audio::Count;
    for(unsigned id=0;id<ucg::editor_audio::Count;++id)storeDiagnostics(editorAudioSources[id],audio.editorOneShotDiagnostics(id));
    const auto reader=audio.editorReaderDiagnostics();
    editorReaderProbe.musicSlices=reader.musicSlices;editorReaderProbe.maxMusicSliceUsec=reader.maxMusicSliceUsec;
    editorReaderProbe.serviceCalls=reader.serviceCalls;editorReaderProbe.maxServiceUsec=reader.maxServiceUsec;
    editorReaderProbe.pendingAdmissions=reader.pendingAdmissions;editorReaderProbe.pendingObservations=reader.pendingObservations;
    editorReaderProbe.lowReserveObservations=reader.lowReserveObservations;editorReaderProbe.primingObservations=reader.primingObservations;
    editorReaderProbe.startedObservations=reader.startedObservations;editorReaderProbe.minQueuedPending=reader.minQueuedPending;
    editorReaderProbe.maxQueuedPending=reader.maxQueuedPending;
    editorCacheProbe.total=EditorRender::cacheTotalBytes()+EditorRender::reservedSampleBytes();editorCacheProbe.hot=EditorRender::cacheBytes();
    editorCacheProbe.cold=EditorRender::coldCache.allocatedBytes();editorCacheProbe.hits=EditorRender::coldCache.hits;
    editorCacheProbe.stores=EditorRender::coldCache.stores;editorCacheProbe.storeFailures=EditorRender::coldStoreFailures;
    editorCacheProbe.storeSkips=EditorRender::coldStoreSkips;editorCacheProbe.stageDecodes=EditorRender::coldStageDecodes;
    editorCacheProbe.stageHits=EditorRender::coldStageHits;editorCacheProbe.promotions=EditorRender::coldPromotions;
    editorCacheProbe.allocationFailures=EditorRender::cacheAllocationFailures;editorCacheProbe.workspacePeak=EditorRender::coldCache.workspacePeak;
    const auto&samples=EditorRender::sampleCache.stats();
    editorSampleCacheProbe.bytes=EditorRender::sampleCache.accountedBytes();editorSampleCacheProbe.entries=EditorRender::sampleCache.entryCount();
    editorSampleCacheProbe.hits=uint32_t(samples.hits);editorSampleCacheProbe.misses=uint32_t(samples.misses);editorSampleCacheProbe.stores=uint32_t(samples.stores);editorSampleCacheProbe.evictions=uint32_t(samples.evictions);
    editorSampleCacheProbe.allocationFailures=uint32_t(samples.allocationFailures);editorSampleCacheProbe.sourceFailures=uint32_t(samples.sourceFailures);editorSampleCacheProbe.bypasses=uint32_t(samples.bypasses);editorSampleCacheProbe.clears=uint32_t(samples.clears);
#ifdef UCG_EDITOR_ENABLE_LIT_MEMO
    const auto&lit=EditorRender::litCache.stats();
    editorLitCacheProbe.bytes=EditorRender::litCache.accountedBytes();editorLitCacheProbe.entries=EditorRender::litCache.entryCount();
    editorLitCacheProbe.hits=uint32_t(lit.hits);editorLitCacheProbe.misses=uint32_t(lit.misses);editorLitCacheProbe.stores=uint32_t(lit.stores);editorLitCacheProbe.evictions=uint32_t(lit.evictions);
    editorLitCacheProbe.allocationFailures=uint32_t(lit.allocationFailures);editorLitCacheProbe.sourceFailures=uint32_t(lit.sourceFailures);editorLitCacheProbe.bypasses=uint32_t(lit.bypasses);editorLitCacheProbe.clears=uint32_t(lit.clears);
#endif
    editorBuilderProbe.pending=builder.pending();editorBuilderProbe.owned=builder.owned();editorBuilderProbe.spinFrame=builder.view().spinFrame;
    editorBuilderProbe.draws=uint32_t(builder.stats.draws);editorBuilderProbe.getAttempts=uint32_t(builder.stats.getAttempts);editorBuilderProbe.errors=uint32_t(builder.stats.errors);
    const auto fileState=panel.captureFileState();
    editorFileProbe.open=fileState[0];editorFileProbe.selected=fileState[1];editorFileProbe.scroll=fileState[2];editorFileProbe.count=fileState[3];
    editorFileProbe.relativeHash=fileState[4];editorFileProbe.nameHash=fileState[5];
    editorColdPoolProbe.segments=EditorRender::Cold::pool.segmentCount;
    editorColdPoolProbe.resident=EditorRender::Cold::pool.residentBytes();
    editorColdPoolProbe.used=EditorRender::Cold::pool.usedSlots;editorColdPoolProbe.high=EditorRender::Cold::pool.highSlots;
    editorColdPoolProbe.requests=EditorRender::Cold::pool.requests;editorColdPoolProbe.allocationFailures=EditorRender::Cold::pool.failures;
    editorColdPoolProbe.evictions=EditorRender::Cold::pool.evictions;editorColdPoolProbe.invalidReleases=EditorRender::Cold::pool.invalidReleases;
    editorColdPoolProbe.initializationAttempts=EditorRender::coldPoolInitializationAttempts;
    editorColdPoolProbe.initializationFailures=EditorRender::coldPoolInitializationFailures;
    editorColdPoolProbe.hotReserveDenials=EditorRender::hotReserveDenials;editorColdPoolProbe.coldReserveDenials=EditorRender::coldReserveDenials;
    const auto&legacy=audio.timing(AudioProbePhase::Map);
    editorLegacyAudioProbe.blocks=legacy.blocks.load(std::memory_order_relaxed);
    editorLegacyAudioProbe.readMax=legacy.readMax.load(std::memory_order_relaxed);editorLegacyAudioProbe.mixMax=legacy.mixMax.load(std::memory_order_relaxed);
    editorLegacyAudioProbe.gapMax=legacy.gapMax.load(std::memory_order_relaxed);editorLegacyAudioProbe.late=legacy.late.load(std::memory_order_relaxed);
    editorLegacyAudioProbe.priming=legacy.priming.load(std::memory_order_relaxed);editorLegacyAudioProbe.underruns=legacy.underruns.load(std::memory_order_relaxed);
    editorLegacyAudioProbe.effectUnderruns=legacy.effectUnderruns.load(std::memory_order_relaxed);editorLegacyAudioProbe.readErrors=legacy.readErrors.load(std::memory_order_relaxed);
    editorLegacyAudioProbe.lastMissingEffect=legacy.lastMissingEffect.load(std::memory_order_relaxed);editorLegacyAudioProbe.musicBufferMin=legacy.musicBufferMin.load(std::memory_order_relaxed);
    editorLegacyAudioProbe.outputError=legacy.outputError.load(std::memory_order_relaxed);
    editorLegacyAudioProbe.musicIoUs=legacy.musicIoUs.load(std::memory_order_relaxed);editorLegacyAudioProbe.effectIoUs=legacy.effectIoUs.load(std::memory_order_relaxed);
    editorLegacyAudioProbe.musicIoCalls=legacy.musicIoCalls.load(std::memory_order_relaxed);editorLegacyAudioProbe.effectIoCalls=legacy.effectIoCalls.load(std::memory_order_relaxed);
    editorLegacyAudioProbe.musicIoMax=legacy.musicIoMax.load(std::memory_order_relaxed);editorLegacyAudioProbe.effectIoMax=legacy.effectIoMax.load(std::memory_order_relaxed);
    editorLegacyAudioProbe.musicWorkMax=legacy.musicWorkMax.load(std::memory_order_relaxed);editorLegacyAudioProbe.effectWorkMax=legacy.effectWorkMax.load(std::memory_order_relaxed);
    const unsigned queued=audio.queuedMusicFrames();editorLegacyAudioProbe.queuedFrames=queued;editorLegacyAudioProbe.queuedLowerBound=queued>4096?queued-4096:0;
#endif
    if (editing && uiMode067.cursor && !session.active && !wasPlaying && !wasModal && !panel.blocksWorld(pointer)) {
      if (actions.wheel) {
        std::string e;
        if (!c.scale(actions.wheel, actions, e))
          panel.say(e);
      }
      if (actions.middle && actions.shift) {
        c.camera.x -= (pointer.x - oldPointer.x) / c.zoom();
        c.camera.y -= (pointer.y - oldPointer.y) / c.zoom();
      }
      auto world = c.world(pointer);
      std::string e;
      if (!c.selectMode) {
        if (c.tab == 1) {
          if ((actions.primaryDown || actions.secondaryDown) && !c.painting){
            if(!c.beginStroke(world,actions.secondary,e))panel.say(e);
          }
          if (c.painting) {
            c.continueStroke(world);
            if (!actions.primary && !actions.secondary && !c.endStroke(e))
              panel.say(e);
          }
        } else if (actions.primaryDown && c.tab == 2) {
          if (!c.place(world, actions.shift||(actions.alt&&actions.middle), actions.alt && actions.middle, e))
            panel.say(e);
        } else if (actions.primaryDown && c.tab == 3) {
          const char *texture = nullptr, *filename = "";
          std::string path;
          if (c.deco < 66) {
            auto image = EditorArt::Packed::Deco[c.deco].image;
            path =
                "res://" + std::string(EditorArt::Packed::Images[image].path);
            texture = path.c_str();
          } else if (c.deco - 66 < c.customDeco.size())
            filename = c.customDeco[c.deco - 66].c_str();
          if (!c.placeDeco(world, texture, filename, actions.shift, e))
            panel.say(e);
        }
      } else {
        if (actions.primaryDown) {
          bool canDrag=true;
          c.point = c.pointAt(world);
          if (c.point < 0) {
            if (actions.alt && c.model.selected()) {
              if (!c.model.copy_selected(e)||!c.model.paste(world,e)){
                panel.say(e);canDrag=false;
              }
            } else{
              c.selectAt(world, e);
              c.point=c.pointAt(world);
            }
          }
          if (canDrag)if(auto*n=c.model.selected()) {
            ucg::editor::Vec2 pos;
            ucg::editor::read_vector(n->array[0], pos);
            c.dragOffset = {pos.x - world.x, pos.y - world.y};
            memoryTrace("begin-drag",0);
            if(!c.beginDrag(e))panel.say(e);
            memoryTrace("begun-drag",0);
          }
        }
        if (c.dragging && actions.primary) {
          if (c.point >= 0) {
            if (!c.moveDragPoint(unsigned(c.point), world, actions.shift, e))
              panel.say(e);
          } else if (auto *n = c.model.selected()) {
            (void)n;
            ucg::editor::Vec2 p{world.x + c.dragOffset.x,
                                world.y + c.dragOffset.y};
            if (!actions.shift)
              p = EditorController::snap(p, 8);
            if (!c.moveDrag(p, e))
              panel.say(e);
          }
        }
        if (actions.primaryUp){
          memoryTrace("finish-drag",0);
          if(!c.finishDrag(e))panel.say(e);
          memoryTrace("finished-drag",0);
        }
      }
      if(actions.secondaryDown&&c.tab!=1){if(!c.finishDrag(e))panel.say(e);if(!c.eraseAt(world,e)&&!e.empty())panel.say(e);}
    } else if (editing && c.painting &&
               (!actions.primary && !actions.secondary)) {
      std::string e;
      if (!c.endStroke(e))
        panel.say(e);
    }
    if (c.dragging && actions.primaryUp) {
      std::string e;
      if (!c.finishDrag(e))
        panel.say(e);
    }
    if(!settings.active&&(session.active||editing)&&EditorSettingsArt::archive)EditorSettingsArt::unload();
    const unsigned targetCache=settings.active?1:EditorRender::PageCacheCount;
    if(EditorRender::pageLimit!=targetCache)EditorRender::setCachePagesLimit(targetCache);
    if(session.active||editing)EditorBrowserRender::release();
    EditorModifierHud062::state.observe(session.active&&!session.scene.game.won?
        ModifierHud061::Context::Editor:ModifierHud061::Context::None,session.modifierHudEpoch,
        ModifierHud061::fromEditor(session.scene.modifier_mask()),sceKernelGetSystemTimeWide());
    EditorModifierHud062::framePresented=false;
    const uint32_t renderStart066=sceKernelGetSystemTimeLow();
    const uint32_t updateUs066=renderStart066-frameStart066;phaseProfile066.buildUs=phaseProfile066.syncUs=0;
    // Context reflects the state that is actually drawn after this update.
    const unsigned renderContext067=settings.active?5u:session.active?(session.paused?4u:3u):editing?2u:16u+unsigned(browser.mode);
    auto zoneStamp067=editorPerfStamp067();
    auto markZone067=[&](unsigned zone){auto now=editorPerfStamp067();editorPerf067.measure(zone,zoneStamp067,now);zoneStamp067=now;};
    beginFrame();
    if(settings.active){
      // Settings covers the viewport; retain simulation state without repopulating world texture caches.
    }else if (session.active)
      session.draw();
    else if (editing) {
      EditorRender::renderWorld(c);
      markZone067(EditorPerf067::Canvas);
      if (uiMode067.cursor && !panel.modal() && !c.selectMode && pointer.x >= 256)
        EditorRender::renderPreview(c, c.world(pointer));
      markZone067(EditorPerf067::Preview);
      panel.draw();
      if (c.tab == 1 && !panel.modal())
        EditorRender::drawTilePicker(c);
      markZone067(EditorPerf067::Panel);
    } else {
      if(browser.mode==EditorBrowser::Browser::Mode::Menu)EditorPlayHud::backdrop(editorProbe.frames*1000/60);
      else EditorPlayHud::leftBackdrop(uint32_t(EditorPlayHud::shaderTime*1000));
      browser.draw(canvas);
    }
    if(!editing||session.active||settings.active)markZone067(EditorPerf067::Canvas);
    if(settings.active)settings.draw(EditorPlayHud::shaderTime);

    if(drawOpen)EditorModifierHud062::present(sceKernelGetSystemTimeWide());
    {std::string giftError;if(!builder.draw(giftError))panel.say(giftError);}
    if(!uiMode067.cursor&&!session.active&&!settings.active)uiFocus067.draw();
    if(uiMode067.cursor&&(!session.active||settings.active||session.scene.dead||session.scene.game.won)&&!pointerCaptured()&&!(session.active&&session.paused&&!settings.active))
      EditorOriginalCursor067::draw(float(pointer.x),float(pointer.y));
    endFrame();
    markZone067(EditorPerf067::Overlay);
#ifdef UCG_EDITOR_CAPTURE
    captureFrame();
#endif
    const uint32_t renderUs066=sceKernelGetSystemTimeLow()-renderStart066;
    panel.drawKeyboard();
    sceDisplayWaitVblankStart();
    swappedGroundGraphics(sceGuSwapBuffers());
    editorPerf067.observe(renderContext067,perfCounters067());
    phaseProfile066.observe(2,renderContext067,sceKernelGetSystemTimeLow()-frameStart066,updateUs066,renderUs066,phaseProfile066.buildUs,phaseProfile066.syncUs,editorProbe.nodes);
    // A held button or deflected analog stick prevents checkpoint I/O during navigation.
    if(editorNavigation068.observe(renderContext067,c.tab,held!=0||std::abs(int(pad.Lx)-128)>=21||std::abs(int(pad.Ly)-128)>=21,sceKernelGetSystemTimeWide(),(renderContext067==3||!panel.modal()),editing&&!session.active&&panel.isWelcome()))checkpointNavigation068(renderContext067==3?0:c.tab,uiMode067.cursor);
#ifdef UCG_EDITOR_CAPTURE
    captureOffset^=FrameBytes;
#endif
  }
  flushPhaseProfile066();
  sceKernelExitGame();
  return 0;
}
