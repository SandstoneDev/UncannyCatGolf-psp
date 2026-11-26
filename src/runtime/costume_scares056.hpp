#pragma once
namespace CostumeScares056 {
inline LiveResultArt art;
inline bool open(const char* executable){return art.open(assetPath(executable,"costume-scares056.bin"),VillaCostumes::Art::SpriteCount,ResultScratchBytes);}
inline bool prepare(unsigned offset){
    if(static_cast<unsigned>(sceGeEdramGetSize())<offset+ResultLargeBytes)return false;
    const auto pair=Costumes054::screenPair056();
    auto* vram=static_cast<uint8_t*>(sceGeEdramGetAddr())+offset;
    auto* scratch=reinterpret_cast<uint8_t*>(fallbackPage)+ResultScratchOffset;
    fallbackPageIndex=ScreenPages;
    const bool result=art.prepare(pair.ghost,liveResultLarge,vram,ResultLargeBytes,scratch,ResultScratchBytes);
    art.closeStream();return result;
}
}
