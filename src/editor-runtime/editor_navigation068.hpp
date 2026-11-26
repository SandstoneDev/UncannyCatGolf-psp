#pragma once
#include <array>
#include <cstdint>
namespace EditorNavigation068 {
// Main-thread-only, no allocation or I/O. One checkpoint per Editor tab or gameplay visit.
struct State {
 std::array<unsigned,6> frames{};unsigned saved=0,previous=~0u,inputs=0;uint64_t idleAt=0;bool previousWelcome=false;
 bool observe(unsigned context,unsigned tab,bool active,uint64_t now,bool ready=true,bool welcome=false){
  if(context!=previous||(welcome&&!previousWelcome)){frames={};saved=inputs=0;idleAt=now;previous=context;}
  previousWelcome=welcome;
  if(context==3)tab=0;
  if((context!=2&&context!=3)||tab>=frames.size())return false;
  // Modal frames cannot consume a world checkpoint or satisfy its warmup.
  if(!ready){frames[tab]=0;idleAt=now;return false;}
  if(active){idleAt=now;++inputs;}
  if(frames[tab]<120)++frames[tab];
  if(frames[tab]<120||active||now-idleAt<500000||(saved&(1u<<tab)))return false;
  saved|=1u<<tab;return true;
 }
};
static_assert(sizeof(State)<64,"bounded navigation checkpoint gate");
}
