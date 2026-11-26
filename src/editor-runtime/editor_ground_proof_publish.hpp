#pragma once
// Included inside Module anonymous namespace after Renderer/GU definitions.
#ifdef UCG_EDITOR_CAPTURE
inline void publishGroundProof(){
 namespace P=::EditorGroundNativeProof;
 auto&out=P::probe;const auto&cache=EditorRender::GroundRuntime::storage.cache;
 const auto&key=cache.diagnosticKey();const auto&s=cache.stats();
 static_assert(sizeof(EditorGroundCache::Key)==4128&&offsetof(EditorGroundCache::Key,size)==4096&&offsetof(EditorGroundCache::Key,stamp)==4104,"live-key read ABI");
 const uint32_t sequence=(out.words[0]+1)|1u;out.words[0]=sequence;
 asm volatile("":::"memory");
 out.words[1]=P::Magic;out.words[2]=P::Abi;out.words[3]=P::FixedBytes;
#if defined(UCG_EDITOR_DISABLE_GROUND_CACHE) && UCG_EDITOR_DISABLE_GROUND_CACHE
 out.words[4]=0;
#else
 out.words[4]=1;
#endif
 out.words[5]=editorProbe.frames;out.words[6]=editorProbe.sceneTick;out.words[7]=editorProbe.mode;
 out.words[8]=groundGraphics.drawOffset;out.words[9]=groundGraphics.fenceOK;
 out.words[10]=cache.ready();out.words[11]=cache.awaitingPublication();
 auto pair=[&](unsigned at,uint64_t n){out.words[at]=uint32_t(n);out.words[at+1]=uint32_t(n>>32);};
 pair(12,key.stamp.scene);pair(14,key.stamp.document);pair(16,key.stamp.assets);
 out.words[18]=uint32_t(reinterpret_cast<uintptr_t>(&key));out.words[19]=groundGraphics.contextReady;
 pair(20,s.hits);pair(22,s.misses);pair(24,s.captures);pair(26,s.published);
 pair(28,s.invalidations);pair(30,s.copyFailures);pair(32,s.fenceFailures);pair(34,s.cancelled);
 out.words[36]=cache.ready()?key.size:0;out.words[37]=editorProbe.mode==2?out.words[47]:0;
 out.words[38]=EditorRender::failedImages;out.words[39]=!EditorRender::getError().empty();
 out.words[40]=editorProbe.strokes;out.words[41]=editorProbe.dead;out.words[42]=editorProbe.won;
 auto bits=[](float f){uint32_t n;std::memcpy(&n,&f,4);return n;};
 out.words[43]=bits(editorProbe.playerX);out.words[44]=bits(editorProbe.playerY);
 out.words[45]=bits(editorProbe.velocityX);out.words[46]=bits(editorProbe.velocityY);
 if(out.words[36]<=P::KeyBytes)for(unsigned i=0;i<out.words[36];++i)out.key[i]=key.bytes[i];
 asm volatile("":::"memory");out.words[0]=sequence+1;
}
#endif
