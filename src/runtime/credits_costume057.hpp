#pragma once
#include "credits_patch057.hpp"
namespace CreditsCostume057 {
CreditsPatch057::Reader patches;
CreditsPatch057::FirstBank firstBank;
std::unique_ptr<uint8_t[]> firstPixels;
CreditsComposite057::Image firstImage;
LiveResultArt::Slot scare;
bool ready=false,frameReady=false;
bool open(const char* executable){
 return patches.open(assetPath(executable,"ending-costume057.bin"))&&firstBank.open(assetPath(executable,"ending-player-first057.bin"));
}
void reset(){patches.reset();firstBank.reset();CostumeScares056::art.closeStream();scare={};ready=frameReady=false;firstPixels.reset();}
bool ensure(){
 if(Costumes054::selected==VillaCostumes::Id::Canny)return false;
 if(ready)return true;
 if(!Costumes054::screenActorReady056())return false;
 if(!firstPixels)firstPixels.reset(new(std::nothrow) uint8_t[firstBank.capacity()]);
 if(!firstPixels)return false;
 unsigned generation=0;
 // Some lossless native UCR tiles need193379compressed bytes. The full clip
 // compressed arena is available here before any movie/patch decoding.
 constexpr unsigned EntryScratch=ElysianEnding045::NativeBytes+ElysianEnding045::MaxPayload;
 auto* scratch=clips.compressedTextureWorkspace<EntryScratch>(generation);
 const auto pair=Costumes054::screenPair056();
 if(!firstBank.load(pair.player,firstPixels.get(),scratch,firstImage))return false;
 if(static_cast<unsigned>(sceGeEdramGetSize())<FrontierLightVramOffset+ResultLargeBytes)return false;
 // The movie's XOR key lives in fallbackPage. This decode borrows only the
 // clip compressed workspace, before the movie initializes/uses its key.
 const bool prepared=CostumeScares056::art.prepare(pair.ghost,scare,vram045(FrontierLightVramOffset),ResultLargeBytes,scratch,EntryScratch);
 CostumeScares056::art.closeStream();if(!prepared)return false;
 ready=true;return true;
}
CreditsComposite057::Image small(bool ghost){
 const auto& bank=Costumes054::screenBank056;const unsigned i=ghost?1:0;
 return {reinterpret_cast<const uint8_t*>(bank.pixels[i]),bank.width[i],bank.height[i],128,-.5f*bank.width[i],-.5f*bank.height[i],true};
}
bool prepare(unsigned ms,uint16_t* movie){
 frameReady=false;
 if(!ready||!movie)return false;
 unsigned generation=0;
 auto* decoded=clips.textureWorkspace<CreditsPatch057::Reader::DecodedMax>(generation);
 auto* packed=clips.compressedTextureWorkspace<CreditsPatch057::Reader::PackedMax>(generation);
 const auto* frame=patches.prepare(std::min(3173u,ms/50),decoded,packed);if(!frame)return false;
 const auto gradient=[](unsigned x){return CostumeShader056::colorAt(ViewX+136+x);};
 const CreditsComposite057::Image ink{ArtistCopycat::pixels.data(),128,128,128,-64,-64,true,ArtistInk::Palette};
 const auto* custom=Costumes054::selected==VillaCostumes::Id::Copycat&&ArtistCopycat::drawn?&ink:nullptr;
 CreditsPatch057::compose(movie,*frame,firstImage,small(false),small(true),Costumes054::selected==VillaCostumes::Id::Rainbow,gradient,custom);
 sceKernelDcacheWritebackRange(movie,ElysianEnding045::FrameBytes);
 frameReady=true;return frame->scare;
}
void drawScare(bool show){if(show&&ready)drawLiveResultSprite(scare,0,nullptr,1,White,Costumes054::selected==VillaCostumes::Id::Rainbow);}
}
