#pragma once
namespace EditorRender { namespace HudAssets {
struct Image {const char*path;unsigned width,height;};
inline constexpr Image Sprites[]={
{"data/world/editor-aux/player/costume_canny.png",512,512},
{"data/world/editor-aux/player/costume_uncanny.png",512,512},
{"data/world/editor-aux/player/costume_mscanny.png",512,512},
{"data/world/editor-aux/player/costume_retro.png",512,512},
{"data/world/editor-aux/player/costume_dog.png",512,512},
{"data/world/editor-aux/player/costume_veteran.png",852,852},
{"data/world/editor-aux/player/costume_cannydigo.png",870,870},
{"data/world/editor-aux/player/costume_rainbow_canny.png",512,512},
{"data/world/editor-aux/player/costume_peak.png",634,634},
{"data/world/editor-aux/player/costume_catfish.png",512,512},
{"data/world/editor-aux/player/costume_builder.png",768,768},
{"data/world/editor-aux/player/costume_nothing.png",512,512},
{"data/world/editor-aux/player/costume_arle.png",512,512},
{"data/world/editor-aux/player/costume_minnie.png",512,512},
{"data/world/editor-aux/player/costume_rudy.png",512,512},
{"data/world/editor-aux/player/costume_bingus.png",512,512},
{"data/world/editor-aux/player/costume_chen.png",512,512},
{"data/world/editor-aux/player/costume_pig.png",512,512},
{"data/world/editor-aux/player/costume_ploobie.png",512,512},
{"data/world/editor-aux/player/costume_robert.png",512,512},
{"data/world/editor-aux/player/costume_swag.png",512,512},
{"data/world/editor-aux/player/costume_true.png",512,512},
{"data/world/editor-aux/player/costume_wires.png",512,512},
{"data/world/editor-aux/player/costume_arle_ghost.png",512,512},
{"data/world/editor-aux/player/costume_bingus_ghost.png",512,512},
{"data/world/editor-aux/player/costume_chen_ghost.png",512,512},
{"data/world/editor-aux/player/costume_peak_ghost.png",512,512},
{"data/world/editor-aux/player/costume_horse.png",512,512},
{"data/world/editor-aux/player/costume_mscanny_ghost.png",512,512},
{"data/world/editor-aux/player/costume_ploobie_ghost.png",512,512},
{"data/world/editor-aux/player/costume_uncanny.png",512,512},
{"data/world/editor-aux/player/costume_retro_ghost.png",512,512},
{"data/world/editor-aux/player/costume_catfish_ghost.png",512,512},
{"data/world/editor-aux/player/costume_veteran_ghost.png",512,512},
{"data/world/editor-aux/player/costume_true_ghost.png",512,512},
{"data/world/editor-aux/player/costume_wires_ghost.png",512,512},
{"",0,0},
};
inline constexpr Image out_of_bounds={"data/world/editor-aux/stage/misc/out_of_bounds.png",254,108};
inline constexpr Image backrooms={"data/world/editor-aux/stage/misc/backrooms.png",512,512};
inline constexpr Image costume_horse={"data/world/editor-aux/player/costume_horse.png",512,512};
inline constexpr unsigned Rainbow[64]={0xffb280ffu,0xffad81feu,0xffa682feu,0xff9d84feu,0xff9486feu,0xff8987feu,0xff7f89feu,0xff758afeu,0xff6d8bffu,0xff658bffu,0xff5d8afeu,0xff5589feu,0xff4e88feu,0xff4788feu,0xff4089feu,0xff3b8bfeu,0xff3690ffu,0xff3497ffu,0xff359fffu,0xff37a9ffu,0xff3ab4ffu,0xff3ebfffu,0xff41c9ffu,0xff44d2feu,0xff45d9f7u,0xff40e1edu,0xff37e8e0u,0xff2aeed0u,0xff1bf3beu,0xff14f7a9u,0xff21fa94u,0xff38fa7fu,0xff54f86cu,0xff71f45bu,0xff8eed4cu,0xffa9e540u,0xffc2db3au,0xffd7d03au,0xffe9c43eu,0xfff6b943u,0xfffeae49u,0xffffa14fu,0xffff9555u,0xffff885au,0xffff7b60u,0xffff6f65u,0xfffd656bu,0xfffc5c71u,0xfffc5678u,0xfffe5181u,0xffff4f8cu,0xffff4e97u,0xffff4ea3u,0xffff50aeu,0xffff53b9u,0xffff56c2u,0xfffa5acbu,0xfff160d3u,0xffe767dcu,0xffdb6ee4u,0xffcf74ecu,0xffc479f3u,0xffba7dfau,0xffb380ffu};
inline constexpr Image timer={"data/world/editor-aux/HUD/timer.png",360,200};
inline constexpr Image prism={"data/world/editor-aux/menu/prism.png",320,32};
inline constexpr Image prism_black={"data/world/editor-aux/menu/prism-black.png",320,32};
}}
