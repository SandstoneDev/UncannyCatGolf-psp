#pragma once
#include "release_paths.hpp"
#include "editor_builder_present.hpp"
#include "editor_builder_gift.hpp"
#include <functional>
#include <string>
#include <utility>
// Late include after the module's GU helpers, EditorArt and EditorRender.
namespace EditorBuilderModule {
using Sound=std::function<bool(unsigned,float,uint32_t)>;
class Adapter {
 EditorBuilder::Store store;
 EditorBuilder::Presentation presentation{store};
 EditorBuilderGift::Reader bank;
 Sound sound;
 std::string root,error;
 int residentFrame=-1;
 unsigned residentToken=0;
 bool initialized=false,cueFailed=false;
 std::string path(const char* file)const{return std::string_view(file).rfind("data/",0)==0?root+file:ReleasePaths::stateFromDirectory(root,file);}
 bool fail(const std::string& message,std::string& e){error=e=message;++stats.errors;return false;}
 bool resident(unsigned frame)const{return residentFrame==int(frame)&&EditorArt::cached[0]==-1&&EditorArt::ages[0]==residentToken;}
 bool prepare(unsigned frame,std::string& e){
  if(resident(frame)){++stats.cacheHits;return true;}
  flushBatch(); // Submit AND fence all queued readers before reusing slot0.
  residentFrame=-1;EditorArt::cached[0]=-1;residentToken=EditorArt::ages[0]=++EditorArt::age;
  if(!bankOpen){bankOpen=bank.open(path(EditorBuilderGift::BankPath).c_str());if(!bankOpen){EditorRender::ensureBatch();return fail("Cannot open original Builder gift bank",e);}}
  auto* target=EditorArt::video(EditorArt::CacheStart);
  if(!bank.decode(frame,target,EditorArt::SlotBytes,EditorArt::workspace,EditorArt::InflateWorkspaceRequired)){
   EditorArt::textureDirty=true;EditorRender::markTextureDirty();EditorRender::ensureBatch();return fail("Cannot decode original Builder gift spin",e);
  }
  sceKernelDcacheWritebackRange(target,EditorBuilderGift::Data::Records[frame].raw);
  EditorArt::textureDirty=true;EditorRender::markTextureDirty();residentFrame=int(frame);++stats.decodes;
  EditorRender::ensureBatch();return true;
 }
 bool bankOpen=false;
public:
 struct Stats{uint64_t decodes=0,cacheHits=0,draws=0,getAttempts=0,errors=0;}stats;
 Adapter()=default;Adapter(const Adapter&)=delete;Adapter&operator=(const Adapter&)=delete;
 bool init(const std::string& gameRoot,Sound playSound,std::string& e){
  shutdown();root=gameRoot;if(!root.empty()&&root.back()!='/'&&root.back()!='\\')root+='/';sound=std::move(playSound);error.clear();e.clear();stats={};
  store.save=[this](uint32_t value,std::string& message){EditorBuilder::File file(path(EditorBuilder::FileName));if(file.write(value))return true;message="Cannot persist pending Builder gift";return false;};
  EditorBuilder::File file(path(EditorBuilder::FileName));if(!store.restore(file.read(),e))return fail(e,e);
  presentation.restorePending();
  presentation.onSound=[this](EditorBuilder::Cue cue){
   if(cue!=EditorBuilder::Cue::Get)return;
   ++stats.getAttempts;if(!sound||!sound(359,2.f,0)){cueFailed=true;error="Cannot queue Builder Get sound";}
  };
  initialized=true;return true;
 }
 bool queue(std::string& e){
  e.clear();error.clear();cueFailed=false;
  if(!initialized)return fail("Builder module adapter is not initialized",e);
  if(!presentation.queue(e))return fail(e,e);
  // Pending was already committed before the cue. Never discard/replay it if
  // audio rejects; an idempotent retry succeeds without issuing another Get.
  if(cueFailed)return fail(error,e);
  return true;
 }
 void update(uint32_t deltaMs){if(initialized)presentation.advance(deltaMs,false);}
 bool setSlot(unsigned slot,std::string& e){if(!initialized)return fail("Builder module adapter is not initialized",e);return presentation.setSlot(slot,e);}
 EditorBuilder::View view()const{return presentation.view();}
 bool pending()const{return store.pending();}
 bool owned()const{return store.owned();}
 const std::string& getError()const{return error;}
 bool draw(std::string& e){
  e.clear();error.clear();if(!initialized)return fail("Builder module adapter is not initialized",e);
  const auto v=presentation.view();if(!v.indicator)return true;
  if(!EditorArt::ready||!EditorArt::workspace)return fail("Builder indicator requires loaded EditorArt workspace",e);
  EditorRender::ensureBatch();if(!prepare(v.spinFrame,e))return false;
  const auto previous=EditorArt::uiScissor;EditorArt::scissor(36,0,444,272);
  auto* target=EditorArt::video(EditorArt::CacheStart);const auto& record=EditorBuilderGift::Data::Records[v.spinFrame];
  sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(32,target+record.pw*record.ph);setTexture(GU_PSM_T8,record.pw,record.ph,target);sceGuTexFilter(GU_NEAREST,GU_NEAREST);sceGuTexFlush();
  EditorBuilderGift::Params params;params.x=v.x;params.y=v.y;params.scale=v.scale;params.spinFrame=v.spinFrame;params.shown=true;
  EditorBuilderGift::visit(params,[&](const EditorBuilderGift::Command& command){
   if(command.kind!=EditorBuilderGift::Kind::Bank)return;
   auto* vertices=static_cast<TextureVertex*>(sceGuGetMemory(4*sizeof(TextureVertex)));
   for(unsigned i=0;i<4;++i){const bool right=i&1,bottom=i&2;vertices[i]={command.source.x+(right?command.source.w:0),command.source.y+(bottom?command.source.h:0),command.tint,36+(command.dest.x+(right?command.dest.w:0))*EditorArt::Scale,(command.dest.y+(bottom?command.dest.h:0))*EditorArt::Scale,0};}
   sceGuDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,vertices);++stats.draws;
  });
  EditorArt::scissor(previous[0],previous[1],previous[2],previous[3]);return true;
 }
 // Call before EditorArt/GU shutdown. No scene-level clear hook is provided:
 // owner0 Get and the persisted pending indicator survive scene transitions.
 // Graphics-only OSK lifecycle. Default call is BEFORE sceGuTerm. If false,
 // the caller must already have submitted/fenced prior readers before GU died.
 // Pending/pulse/spin age, persistence, bank FILE and sound ownership survive.
 void invalidateGraphics(bool guAlive=true){
  if(residentFrame>=0&&EditorArt::cached[0]==-1&&EditorArt::ages[0]==residentToken){
   if(guAlive)flushBatch();
   EditorArt::cached[0]=-1;EditorArt::ages[0]=++EditorArt::age;
   EditorArt::textureDirty=true;EditorRender::markTextureDirty();
  }
  residentFrame=-1;residentToken=0;
 }
 void shutdown(){
  invalidateGraphics();bank.close();bankOpen=false;initialized=false;sound={};presentation.onSound={};
 }
};
}
