#pragma once
#include <cstdint>
namespace EditorBuilderGiftSourceData {
inline constexpr const char* BankPath="data/world/editor-gift-source.bin";
struct Image{const char* path;uint32_t width,height,colors,paletteOffset,paletteCRC,rowsOffset;};
inline constexpr Image Images[]={
{"data/world/editor-aux/player/costume_builder.png",768u,768u,213u,64u,4036157856u,1104u},
{"data/world/editor-aux/minigames/bartbash/boom.png",1207u,100u,47u,916u,860153006u,10320u},
};
inline constexpr uint32_t BankBytes=155951u,PayloadBegin=11520u;
}
