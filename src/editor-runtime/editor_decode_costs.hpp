#pragma once
// Diagnostic-only attribution. Normal candidate contains no timing/table code.
#ifdef UCG_EDITOR_CAPTURE
struct DecodeCost {unsigned coldReads=0,coldUsec=0,sourceReads=0,sourceUsec=0,indexedReads=0,indexedUsec=0;};
inline std::array<DecodeCost,EditorArt::Packed::Records+1> decodeCosts{};
struct DecodeCostScope {
  unsigned page,kind,start;
  DecodeCostScope(unsigned id,unsigned type):page(std::min(id,EditorArt::Packed::Records)),kind(type),start(sceKernelGetSystemTimeLow()){}
  ~DecodeCostScope(){
    auto&c=decodeCosts[page];const unsigned elapsed=sceKernelGetSystemTimeLow()-start;
    if(kind==0){++c.coldReads;c.coldUsec+=elapsed;}
    else if(kind==1){++c.sourceReads;c.sourceUsec+=elapsed;}
    else{++c.indexedReads;c.indexedUsec+=elapsed;}
  }
};
#define UCG_DECODE_COST(id,kind) DecodeCostScope decodeCostScope(id,kind)
#else
#define UCG_DECODE_COST(id,kind)
#endif
