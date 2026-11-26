#pragma once
#include "editor_audio_bindings.hpp"
#include "editor_costume_sounds.hpp"
namespace EditorCues {
struct Cue {
  const char *key;
  unsigned id;
};
inline constexpr Cue cues[] = {
    {"sfxBurst", 298},         {"sfxLaunched", 336},  {"sfxLaunchedDizzy", 337},
    {"sfxWallBump", 342},      {"jump", 226},         {"sfxWaterDeath", 343},
    {"sfxWallDeath", 244},     {"ghost_caught", 260}, {"dblock_hit", 294},
    {"keydoor_open", 296},     {"switch_hit", 297},   {"chuckle_death", 309},
    {"car_death", 306},        {"skull_death", 310},  {"sfxParry", 339},
    {"dog_bounce", 342},       {"dog_goal", 325},     {"spawner_collect", 299},
    {"ring_win", 302},         {"ring_lose", 301},    {"sfxWaterSplash", 344},
    {"sfxPortal", 328},        {"goal", 313},         {"ring_start", 304},
    {"smiley_coin", 303},      {"coin_collect", 299}, {"speed_collect", 298},
    {"stop_collect", 298},     {"nuke_collect", 326}, {"key_collect", 300},
    {"modifier_collect", 222}, {"bee_buzz", 230},     {"buddy_die", 260},
    {"sfxJoin",312}, {"sfxGrab",335}, {"sfxGrabCancel",334}, {"sfxPickUp",338},
    {"sfxPush",340}, {"sfxStick",341}, {"dog_bark",324}, {"car_horn_a",307},
    {"car_horn_b",308}, {"out_of_bounds",311}, {"horse_caught",323},
    {"missile_ding", 227},     {"missile_boom", 228}, {"bort_ow", 262}};
inline unsigned randomState = 0x053ed;
inline float random(float a, float b) {
  randomState ^= randomState << 13;
  randomState ^= randomState >> 17;
  randomState ^= randomState << 5;
  return a + (b - a) * float(randomState & 65535) / 65535;
}
// Explicit original AudioStreamPlayer identity, independent of stream alias.
inline unsigned sourceNode(const char* key,unsigned fallback) {
  if(!std::strcmp(key,"car_horn_a")||!std::strcmp(key,"car_horn_b"))return 1001;
  return fallback+1;
}
inline void raw(unsigned id,float gain,float pitch) {
  audio.playTuned(static_cast<Sound>(id),gain,pitch);
}
inline void rawOwned(unsigned id,float gain,float pitch,uint32_t owner,unsigned node) {
  if(node)audio.editorOneShot(id,owner,node,gain,pitch);
  else audio.playTuned(static_cast<Sound>(id),gain,pitch);
}
inline void play(unsigned id,float pitchOverride=0,float gainAdd=0,uint32_t owner=0,unsigned node=0) {
  float gain = 0, pitch = 1;
  for (const auto &b : ucg::editor_audio::bindings)
    if (b.id == id) {
      gain = b.gainDb + random(-b.randomVolumeDb, b.randomVolumeDb);
      pitch =
          b.pitch *
          (b.randomPitch > 1 ? random(1 / b.randomPitch, b.randomPitch) : 1);
      break;
    }
  rawOwned(id,gain+gainAdd,pitchOverride>0?pitchOverride:pitch,owner,node);
}
inline void stopLoops() {
  audio.stopBortOw();
  audio.editorDropLoops();
  for (unsigned i : {227, 230, 305, 333})
    audio.stop(static_cast<Sound>(i));
}
inline void events(const EditorPlay::Scene &s) {
  audio.editorBeginOneShotBatch();
  for (const auto &e : s.events)
    if(e.type==EditorPlay::Event::StopAudioOwner)audio.editorStopOneShotOwner(e.voiceOwner);
    else if(e.type==EditorPlay::Event::Effect&&e.objectId==7&&e.sound&&!std::strcmp(e.sound,"nuke"))audio.editorStopBaddieOneShots();
    else if (e.type == EditorPlay::Event::Sound) {
      if(!std::strcmp(e.sound,"bee_buzz")||!std::strcmp(e.sound,"missile_ding"))continue;
      if(!std::strcmp(e.sound,"bort_ow")){audio.playBortOw(1);continue;}
      if((!std::strcmp(e.sound,"sfxPush")||!std::strcmp(e.sound,"sfxStick"))&&audio.editorEffectPlaying(static_cast<Sound>(!std::strcmp(e.sound,"sfxPush")?340:341)))continue;
      if(!std::strcmp(e.sound,"ghost_caught")||!std::strcmp(e.sound,"ghost_graze")||!std::strcmp(e.sound,"buddy_die")){rawOwned(ucg::editor_audio::costumeSound(EditorPlayHud::pair.ghost),e.gainDb,e.pitch,e.voiceOwner,!std::strcmp(e.sound,"ghost_graze")?1002:!std::strcmp(e.sound,"buddy_die")?1003:1004);continue;}
      for (unsigned cueIndex=0;cueIndex<sizeof(cues)/sizeof(*cues);++cueIndex) {
        const auto& cue=cues[cueIndex];
        if (!std::strcmp(e.sound, cue.key)) {
          unsigned id = cue.id;
          if (id == 226 && e.objectId == 22)
            id = 327;
          if (id == 344)
            id += unsigned(random(0, 2.999f));
          play(id,e.pitch!=1?e.pitch:0,e.gainDb,e.voiceOwner,sourceNode(e.sound,cueIndex));
          break;
        }
      }
    }
  audio.editorEndOneShotBatch();
}
struct Loops {
  bool tk = false;
  void sync(const EditorPlay::Scene &s) {
    unsigned ringVoice=0,missileVoice=0,beeIndex=0;
    for (const auto &e : s.entities) {
      if(e.objectId==8)audio.editorLoop(305,ringVoice++,e.running,e.ringAudioPitch);
      if(e.objectId==28)audio.editorLoop(227,missileVoice++,e.active&&e.timer<=60&&e.timer>0,1);
      if(e.objectId==30){if(beeIndex%20==0)audio.editorLoop(230,beeIndex/20,e.active,1);++beeIndex;}
    }
    bool nextTk = s.telekinesis_audio_active();
    auto change = [](bool &before, bool now, unsigned id) {
      if (now && !before)
        play(id);
      else if (before && !now)
        audio.stop(static_cast<Sound>(id));
      before = now;
    };
    change(tk, nextTk, 333);
    if(tk)audio.retuneEffect(static_cast<Sound>(333),4,s.tkAudioPitch);
  }
  void stop() {
    stopLoops();
    tk = false;
  }
};
} // namespace EditorCues
