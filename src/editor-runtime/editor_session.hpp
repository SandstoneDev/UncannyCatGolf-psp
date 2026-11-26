#pragma once
#include "editor_play_results.hpp"
#include "editor_native_shot069.hpp"
#include "editor_cues.hpp"
#include "editor_play_fx.hpp"
#include "editor_fixed_step.hpp"
#include "avatar_editor_speech059.hpp"
namespace EditorSession {
using Point = ucg::editor::Vec2;
struct Session {
  EditorPlay::Scene scene;
  EditorController::Controller view;
  EditorCues::Loops loops;
  bool active = false, test = false, paused = false, dragging = false,
       goalReported = false;
  unsigned pauseChoice = 0;
  unsigned modifierHudEpoch=0;
  EditorPlayHud::State state;
  double powerSensitivity = 1;
  bool invertShot = false, bufferShots = false;
  unsigned totalCentiseconds = 0, prisms = 0;
  int64_t totalScore = 0;
  EditorTiming::FixedStep timing;
  unsigned skipTicks = 0;
  unsigned audioRejected=0;
  unsigned playingMusic=~0u;std::string playingExternal;bool musicReady=false,restartBlocked=false;
  std::function<void()> onSettings;
  Point pointer{512, 256};
  ::Vec2 nativeDrag{};
  std::function<void(int64_t, unsigned, double, int)> onComplete;
  std::function<void()> onExit, onSkip;
  std::string error;
  bool start(const ucg::editor::Document &doc, const std::string &path,
             bool testing, std::string &e) {
    stop(true);
    EditorRender::reclaimPageMemory();
    EditorRender::invalidate();
    test = testing;
    view.path = path;
    error.clear();
    view.model.document.root = ucg::editor::Value::dict();
    for (const auto &field : doc.root.object) {
      bool omit = field.first == "nodes" || field.first == "deco";
      for (int layer = 0; layer < 6; ++layer)
        omit |= field.first == ucg::editor::tile_layer_name(layer);
      if (!omit)
        view.model.document.root[field.first] = field.second;
    }
    scene.decorationCollider = [&](const EditorPlay::Decoration &d,
                                   std::vector<std::vector<::Vec2>> &out,
                                   std::string &failure) {
      auto folder = EditorFiles::base(view.path);
      std::string custom;
      if (!d.imageFilename.empty() &&
          EditorFiles::join(folder, d.imageFilename, custom) &&
          EditorFiles::exists(custom)) {
        EditorAlpha::Polygons polygons, triangles;
        if (!EditorAlpha::contours(custom, polygons, failure) ||
            !EditorAlpha::triangulate(polygons, triangles, failure))
          return false;
        for (const auto &p : triangles) {
          std::vector<::Vec2> v;
          for (auto q : p)
            v.push_back({q.x, q.y});
          out.push_back(std::move(v));
        }
        return true;
      }
      std::string texture = d.texture;
      if (texture.rfind("res://", 0) == 0)
        texture.erase(0, 6);
      for (unsigned i = 0; i < std::size(EditorArt::Packed::Images); ++i)
        if (texture == EditorArt::Packed::Images[i].path) {
          struct Context {
            std::vector<std::vector<::Vec2>> *out;
            float w, h;
          } ctx{&out, EditorArt::Packed::Images[i].width * .5f,
                EditorArt::Packed::Images[i].height * .5f};
          bool ok = EditorArt::Collision::load(
              executable.c_str(), i, &ctx,
              [](void *opaque, const EditorArt::Collision::Point *points,
                 unsigned n) {
                auto &context = *static_cast<Context *>(opaque);
                std::vector<::Vec2> p;
                for (unsigned j = 0; j < n; ++j)
                  p.push_back(
                      {points[j].x - context.w, points[j].y - context.h});
                context.out->push_back(std::move(p));
                return true;
              });
          if (!ok)
            failure = "Cannot read decoration collision bank";
          return ok;
        }
      failure = "Unknown decoration image: " + d.texture;
      return false;
    };
    Course base{};
    base.cells = 1;
    base.left = base.top = -256;
    base.right = base.bottom = 256;
    base.spawn = {-100, 0};
    base.goal = {100, 0};
    base.playerRadius = 15.8f;
    base.goalRadius = 16;
    base.maxDrag = 175;
    base.minDrag = 20;
    base.dragMultiplier = 5;
    base.frictionCoeff = .04f;
    base.frictionPow = 1.06f;
    base.stopSpeed = 20;
    base.fieldSize = 512;
    base.catWidth = base.catHeight = 32;
    if (!scene.load(doc, base, e)) {
      view.model.clear_memory();
      return false;
    }
    scene.testMode=testing;
    std::string external;
    double loop = 0;
    unsigned song = 0;
    if (const auto *s = doc.root.find("song"))song=unsigned(s->numeric());
    const auto *customSong = doc.root.find("customSongName");
    if (song==5&&customSong && !customSong->text.empty()) {
      if (!EditorFiles::join(EditorFiles::base(path), customSong->text,
                             external) ||
          !EditorFiles::exists(external))
        external.clear();
      if (const auto *l = doc.root.find("loopPoint"))
        loop = l->numeric();
      if (!std::isfinite(loop) || loop < 0)
        loop = 0;
      if (!external.empty()) {
        ucg::packed_audio::Bank<ucg::editor_audio::Count> probe;
        // Use the reader's probe transaction to reproduce Godot's seek-past-end
        // -> 0.
        auto bank = companion(executable.c_str(), "data/audio/audio.uca");
        const bool bankReady=probe.open(bank.c_str())==decltype(probe)::Open::Ready;
        if(!bankReady)external.clear();
        else if(!probe.overrideMusic(ucg::editor_audio::ExternalMusic,external.c_str(),loop)){
          // Godot seeks beyond EOF back to zero. Keep a valid custom track;
          // only a second failed load uses musOops.
          loop=0;
          if(!probe.overrideMusic(ucg::editor_audio::ExternalMusic,external.c_str(),0))external.clear();
        }
      }
    }
    unsigned rings=0,missiles=0,bees=0;
    for(const auto&entity:scene.entities) {
      rings+=entity.objectId==8;missiles+=entity.objectId==28;bees+=entity.objectId==30;
      if(entity.objectId==25){missiles+=entity.mod==7;bees+=entity.mod==11?20:0;}
    }
    const unsigned music = external.empty() ? (song<std::size(ucg::editor_audio::LevelMusic)?ucg::editor_audio::LevelMusic[song]:ucg::editor_audio::FallbackMusic):ucg::editor_audio::ExternalMusic;
    const bool continueMusic=!testing&&musicReady&&music==playingMusic&&external==playingExternal;
    // Capture immediately before reopening the level's effect pools, after its
    // synchronous geometry load; the previous Master music keeps running there.
    const unsigned musicFrame=continueMusic?audio.musicPositionFrames(static_cast<Sound>(music)):0;
    if (!openAudio(external.empty() ? nullptr : external.c_str(), loop,rings,missiles,bees/20)) {
      e = std::string("Audio: ") + audio.failure();
      scene.clear_memory();
      view.model.clear_memory();
      openAudio();
      return false;
    }
    if(continueMusic)audio.musicAt(static_cast<Sound>(music),musicFrame);
    else audio.music(static_cast<Sound>(music));
    playingMusic=music;playingExternal=external;musicReady=!testing;
    audioRejected=audio.editorOneShotRejected();
    scene.bufferShots = bufferShots;
    EditorPlayHud::clearFX();
    state.reset();
    active = true;
    ++modifierHudEpoch;
    timing.reset();
    paused = dragging = goalReported = false;
    pauseChoice = 0;
    skipTicks=0;
    event("play", testing, unsigned(scene.entities.size()));
    e.clear();
    return true;
  }
  void stop(bool preserveMusic=false) {
    EditorRender::GroundRuntime::sceneChanged();
    restartBlocked=false;
    if(!preserveMusic){musicReady=false;playingMusic=~0u;playingExternal.clear();}
    loops.stop();
    audio.editorClearSceneEffects();
    audio.editorPauseEffects(false);
    active = false;
    timing.reset();
    paused = dragging = false;
    scene.clear_memory();
    view.model.clear_memory();
    EditorPlayHud::clearFX();
    state.reset();
  }
  void restart() {
    loops.stop();
    if(!audio.editorClearSceneEffects()){
      restartBlocked=true;paused=true;dragging=false;audio.editorPauseEffects(true);
      error=lang("Audio reset pending. SELECT+Triangle retries; Circle exits.","Сброс звука не завершён. SELECT+Triangle — повтор; Circle — выход.");
      return;
    }
    restartBlocked=false;error.clear();
    audioRejected=audio.editorOneShotRejected();
    EditorRender::GroundRuntime::sceneChanged();
    scene.restart();
    ++modifierHudEpoch;
    timing.reset();
    audio.editorPauseEffects(false);
    audio.musicTune(1,1);
    scene.bufferShots = bufferShots;
    goalReported = false;
    pauseChoice = 0;
    skipTicks=0;
    paused = dragging = false;
    state.reset();
    EditorPlayHud::clearFX();
    event("restart");
  }
  Point world(Point p) const {
    double z = 256 / scene.bounds;
    return {(p.x - 384) / z + scene.camera.x, (p.y - 256) / z + scene.camera.y};
  }
  void resume() {
    timing.reset();
    paused = false;
    audio.editorPauseEffects(false);
    audio.musicTune(1, 1);
    state.reset();
    event("resumed");
  }
  void update(const EditorInput::Actions &a, Point p, Point previous,
              unsigned pressed, ::Vec2 nativeAim) {
    if (!active) return;
    pointer = p;
    const auto frame=timing.advance(sceKernelGetSystemTimeWide(),paused||restartBlocked);
    const unsigned rejected=audio.editorOneShotRejected();
    if(rejected!=audioRejected){
      audioRejected=rejected;restartBlocked=true;paused=true;dragging=false;loops.stop();audio.editorPauseEffects(true);
      error=lang("Too many simultaneous sounds or an audio read error. SELECT+Triangle retries; Circle exits.","Слишком много одновременных звуков или ошибка чтения. SELECT+Triangle — повтор; Circle — выход.");
      event("audio_limit",rejected);
    }
    if(restartBlocked){
      if(a.undo)restart();
      else if(a.back){stop();if(onExit)onExit();}
      return;
    }
    if (a.back && test) {
      stop();
      if (onExit) onExit();
      return;
    }
    bool resuming=false;
    if (!test && scene.ready_for_input() && (a.pause || a.back)) {
      if (paused){resume();resuming=true;}
      else {
        paused = true;
        dragging = false;
        audio.editorPauseEffects(true);
        audio.musicTune(1, .75f);
        EditorCues::raw(ucg::editor_audio::PauseSound,0,1);
        event("paused");
      }
    }
    if (paused) {
      const unsigned count = 3;
      if (pressed & EditorInput::Up) pauseChoice = (pauseChoice + count - 1) % count;
      if (pressed & EditorInput::Down) pauseChoice = (pauseChoice + 1) % count;
      if (p.x != previous.x || p.y != previous.y) {
        const float x = p.x * EditorArt::Scale, y = p.y * EditorArt::Scale;
        for(unsigned i=0;i<count;++i) {
          const auto &row = (russian()?PauseArt::RuNormal:PauseArt::EnNormal)[i];
          if(x>=136 && x<408 && y>=row.y-2 && y<row.y+row.height+2) pauseChoice=i;
        }
      }
    } else if(!resuming) {
      const auto at = world(p);
      scene.pointerScreen = {float(p.x), float(p.y)};
      scene.pointer_delta({float((p.x - previous.x)*powerSensitivity), float((p.y - previous.y)*powerSensitivity)});
      if (a.primaryDown && !scene.dead && !scene.game.won) {
        scene.click_pointer({float(p.x), float(p.y)});
        nativeDrag = {};
        dragging = scene.begin_drag();
      }
      if (dragging && a.primary)
        nativeDrag = EditorNativeShot069::drag(nativeDrag,nativeAim,bool(scene.modifier_mask()&(1u<<9)));
      if (dragging && a.primaryUp) {
        dragging = false;
        const double factor=powerSensitivity*(invertShot?-1:1);
        scene.shoot({float(nativeDrag.x*factor),float(nativeDrag.y*factor)});
      }
      if (a.secondaryDown) scene.action();
      if (!test && (pressed & EditorInput::Triangle) && !a.undo)
        scene.teleport({float(at.x),float(at.y)});
      for(unsigned tick=0;tick<frame.physics;++tick){
        scene.step60();
        EditorCues::events(scene);
        loops.sync(scene);
        if (!EditorPlayHud::advanceEvents(scene)) {
          error=EditorPlayHud::error; paused=true; loops.stop();break;
        }
        if (!scene.fault.empty()) {
          error = scene.fault;
          paused = true;
          loops.stop();
          event("play_fault");break;
        }
      }
      if(!scene.game.won&&(scene.ready_for_input()||scene.dead)){
        skipTicks=a.skip?std::min(skipTicks+frame.physics,31u):0;
        if(skipTicks>30&&!test){stop(true);if(onSkip)onSkip();return;}
      }else skipTicks=0;
    }
    if ((scene.dead || scene.game.won) && dragging) dragging=false;
    if (scene.game.won && !goalReported) {
      goalReported=true;
      event("goal",unsigned(scene.game.strokes),unsigned(scene.elapsedTicks));
    }
    const auto command=state.update(scene.game.won,scene.dead,paused,frame.milliseconds,
                                     a.primaryDown,a.undo&&(test||scene.ready_for_input()||scene.dead||scene.game.won),pauseChoice);
    state.armed=a.primary;
    EditorPlayHud::sounds(scene,state,EditorCues::raw);
    using Command=EditorPlayHud::Command;
    switch(command) {
    case Command::Restart: restart(); break;
    case Command::Resume: resume(); break;
    case Command::Settings: if(onSettings)onSettings(); break;
    case Command::Skip: stop(); if(onExit)onExit(); break;
    case Command::Exit: stop(); if(onExit)onExit(); break;
    case Command::Continue:
      if(test) { stop(); if(onExit)onExit(); }
      else {
        const auto score=scene.final_score(); const auto strokes=scene.game.strokes;
        const double seconds=scene.elapsed_seconds(); const int rank=EditorPlayHud::rank(scene);
        stop(true); if(onComplete)onComplete(score,strokes,seconds,rank);
      }
      break;
    default: break;
    }
  }
  void draw() {
    if (!active) return;
    EditorRender::renderWorld(view,&scene);
    drawAim();
    EditorPlayHud::drawWorldFX(view,scene);
    AvatarEditorSpeech059::draw(view,scene);
    EditorPlayHud::drawOverlayFX();
    EditorPlayHud::draw(view,scene,totalCentiseconds,totalScore,prisms);
    if(paused)EditorPlayHud::drawSkipping(skipTicks);
    EditorPlayHud::drawState(scene,state);
    if(!paused)EditorPlayHud::drawSkipping(skipTicks);
    if (!error.empty()) EditorArt::textScaled(3,error.c_str(),264,490,White,496,1.75f);
  }
  void drawAim() {
    if(!dragging||paused||scene.dead||scene.game.won)return;
    const double sensitivity=powerSensitivity*(invertShot?-1:1);
    Point delta{nativeDrag.x*sensitivity,nativeDrag.y*sensitivity};
    double power=std::hypot(delta.x,delta.y);
    if(!(scene.modifier_mask()&(1u<<9))&&power>175){delta.x*=175/power;delta.y*=175/power;power=175;}
    if(power<=0)return;
    const auto tint=power<20?ucg::editor::Color{.5,0,0,1}:EditorColors::fromHsv({360*.69*(1-(power-20)/155),.68,1,.5});
    Point scale{1,1};for(const auto&actor:scene.entities)if(actor.objectId==0){scale={actor.scale.x,actor.scale.y};break;}
    const Point normal{-delta.y/power*2,delta.x/power*2};
    const Point points[]={{normal.x,normal.y},{-normal.x,-normal.y},{delta.x+normal.x,delta.y+normal.y},{delta.x-normal.x,delta.y-normal.y}};
    EditorRender::View camera(view,&scene);
    auto*vertices=static_cast<ColorVertex*>(sceGuGetMemory(4*sizeof(ColorVertex)));
    for(unsigned i=0;i<4;++i){const auto p=camera.vertex({scene.game.position.x+points[i].x*scale.x,scene.game.position.y+points[i].y*scale.y});vertices[i]={EditorRender::rgba(tint),p.x,p.y,0};}
    EditorArt::scissor(ViewX+136,0,ViewX+408,272);sceGuDisable(GU_TEXTURE_2D);
    sceGuDrawArray(GU_TRIANGLE_STRIP,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,vertices);
    EditorArt::fullView();
  }
};
} // namespace EditorSession
