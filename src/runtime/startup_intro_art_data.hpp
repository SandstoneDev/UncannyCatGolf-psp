#pragma once
#include <cstdint>
namespace StartupIntroArt {
inline constexpr unsigned Width=512,Height=128,Bytes=Width*Height*4,ScratchBytes=104720;
struct Image{unsigned offset,bytes,width,height,x,y;uint32_t crc;};
inline constexpr Image Images[]={
    {0u,18533u,256u,32u,0u,80u,0x980bb129u},
    {18533u,1292u,374u,70u,0u,0u,0x6126bf41u},
    {19825u,142u,96u,16u,0u,112u,0x2d90d803u},
    {19967u,139u,96u,16u,96u,112u,0x6427ea45u},
    {20106u,33u,16u,16u,192u,112u,0x2709372bu},
    {20139u,36u,16u,16u,208u,112u,0x769d3e05u},
    {20175u,24u,16u,16u,224u,112u,0x51182b2cu},
    {20199u,35u,16u,16u,240u,112u,0x380772cdu},
    {20234u,31u,12u,10u,256u,112u,0x503d3cfeu},
};
inline constexpr unsigned CompressedBytes=20265,LoadScratchBytes=ScratchBytes+CompressedBytes;
}
