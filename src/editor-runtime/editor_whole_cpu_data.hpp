#pragma once
#include <cstdint>
namespace EditorWholeCpu {
struct Spec {uint32_t id,width,height,indexWidth,colors,indexBytes,paletteBytes,payloadBytes,payloadCrc,rgbaCrc;};
inline constexpr Spec Specs[2]={
{0u,512u,512u,1u,150u,262144u,600u,262744u,1531410651u,1168532716u},
{1u,512u,512u,2u,10305u,524288u,41220u,565508u,2964831407u,4161206663u},
};
}
