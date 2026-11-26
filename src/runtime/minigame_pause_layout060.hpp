#pragma once
namespace MinigamePauseLayout060 {
inline constexpr float Scale=1.5f;
inline constexpr float Centers[]={110.5f,144.5f,161.5f};
inline constexpr float left(float width){return 272-width*Scale*.5f;}
inline constexpr float top(float height,unsigned row){return Centers[row]-height*Scale*.5f;}
}
