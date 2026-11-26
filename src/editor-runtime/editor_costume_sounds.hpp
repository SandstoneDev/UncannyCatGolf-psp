#pragma once
#include "editor_audio.hpp"
namespace ucg::editor_audio {
struct CostumeSound {const char* resource;const char* source;unsigned id;};
inline constexpr CostumeSound CostumeSounds[]={
{"canny","sounds/ghost/cannyJumpscare.ogg",350u},
{"uncanny","sounds/ghost/defaultJumpscare.ogg",260u},
{"ms_canny","sounds/ghost/cannyJumpscare.ogg",350u},
{"retro","sounds/ghost/cannyJumpscare.ogg",350u},
{"dog","sounds/ghost/cannyJumpscare.ogg",350u},
{"veteran","sounds/ghost/cannyJumpscare.ogg",350u},
{"cannydigo","sounds/ghost/cannyJumpscare.ogg",350u},
{"rainbow","sounds/ghost/cannyJumpscare.ogg",350u},
{"king","sounds/ghost/cannyJumpscare.ogg",350u},
{"catfish","sounds/ghost/cannyJumpscare.ogg",350u},
{"builder","sounds/ghost/cannyJumpscare.ogg",350u},
{"nothing","sounds/ghost/cannyJumpscare.ogg",350u},
{"arleling","sounds/ghost/cannyJumpscare.ogg",350u},
{"minnie","sounds/ghost/cannyJumpscare.ogg",350u},
{"rudy","sounds/ghost/cannyJumpscare.ogg",350u},
{"bingus","sounds/ghost/cannyJumpscare.ogg",350u},
{"chen","sounds/ghost/chenJumpscare.ogg",351u},
{"pig","sounds/ghost/cannyJumpscare.ogg",350u},
{"ploobie","sounds/ghost/cannyJumpscare.ogg",350u},
{"robert","sounds/ghost/cannyJumpscare.ogg",350u},
{"swag","sounds/ghost/cannyJumpscare.ogg",350u},
{"true","sounds/ghost/cannyJumpscare.ogg",350u},
{"wires","sounds/ghost/cannyJumpscare.ogg",350u},
{"ghost_arleling","sounds/ghost/defaultJumpscare.ogg",260u},
{"ghost_bingus","sounds/ghost/alienJumpscare.ogg",349u},
{"ghost_chen","sounds/ghost/chenJumpscare.ogg",351u},
{"ghost_diavolo","sounds/ghost/assholeJumpscare.ogg",194u},
{"ghost_horse","sounds/ghost/horseJumpscare.ogg",323u},
{"ghost_miss","sounds/ghost/defaultJumpscare.ogg",260u},
{"ghost_ploobie","sounds/gimmicks/tickle.ogg",255u},
{"ghost_rainbow","sounds/ghost/defaultJumpscare.ogg",260u},
{"ghost_retro","sounds/ghost/retroJumpscare.ogg",354u},
{"ghost_shoebill","sounds/gimmicks/slots_bad.ogg",356u},
{"ghost_super","sounds/ghost/plusJumpscare.ogg",353u},
{"ghost_true","sounds/ghost/saxJumpscare.ogg",355u},
{"ghost_wires","sounds/ghost/evilJumpscare.ogg",352u},
{"_blank","sounds/ghost/cannyJumpscare.ogg",350u},
};
inline constexpr unsigned CostumeSoundCount=sizeof CostumeSounds/sizeof CostumeSounds[0];
inline constexpr unsigned costumeSound(unsigned sprite){return CostumeSounds[sprite<CostumeSoundCount?sprite:1].id;}
}
