#pragma once

#include <psprtc.h>
#include <psputility_sysparam.h>
#include "club_physics054.hpp"
#include "robert-animation054.hpp"
#include "club_text054.hpp"

namespace Club054 {

constexpr unsigned RoomOffset=2*FrameBytes;
constexpr unsigned RoomBytes=512*272*2;
constexpr unsigned RobertOffset=RoomOffset+RoomBytes;
constexpr unsigned RecitalFrames=18697191;
constexpr uint32_t LoungeFrames=11907000,BasementFrames=6047360;
constexpr unsigned ExitHoldMs=1600;

// Exact linear RGBA crop of approved costume bank: u45..63/v26..44; active 17x17 at (1,1).
alignas(64) const uint32_t CannyFallbackPixels[32*32] = {
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff434650u,0xff272c38u,0xff202430u,0xff232936u,0xff4b5564u,0xff676f80u,0xff697283u,
    0xff616a7du,0xff646e82u,0xff717c94u,0xff7f8ea8u,0xff8799b5u,0xff91a3beu,0xff596472u,0xff353f4cu,
    0xff3b4654u,0xff48515du,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff747580u,0xff696d7bu,0xff5c606bu,0xff555b68u,0xff737c90u,0xff788193u,0xff8691a7u,
    0xff8994abu,0xff838fa5u,0xff8d98b2u,0xff6e7788u,0xff737e90u,0xff94a0bau,0xff6d7387u,0xff586070u,
    0xff444e5au,0xff4c5560u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff7f818bu,0xff737784u,0xff757b89u,0xff7c8290u,0xff5a5d68u,0xff2c2f35u,0xff2f333bu,
    0xff2b2d35u,0xff292b33u,0xff272930u,0xff31333bu,0xff707184u,0xff7f7c97u,0xff70728eu,0xff5f6173u,
    0xff292425u,0xff252125u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xffa3a2afu,0xff9294a0u,0xff868995u,0xff8b8e9au,0xff63646au,0xff3e3c3fu,0xff3d3b3eu,
    0xff504e53u,0xff524f52u,0xff504d51u,0xff807e8du,0xff8b87a4u,0xff7d7c98u,0xff747997u,0xff737480u,
    0xff7c7474u,0xff2c2424u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xffa09ca6u,0xffb3b3beu,0xffb9bbc4u,0xffbcc0c7u,0xffc0c3ccu,0xffc6c9d2u,0xffc4c7d0u,
    0xffc6c8d1u,0xffc3c6cfu,0xffc2c2cdu,0xffb5b3c1u,0xff9c99a9u,0xff8d8ea5u,0xff7c7f95u,0xff39363au,
    0xff363133u,0xff151214u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xffbdb9c2u,0xffbebdc7u,0xffbcbdc7u,0xffbbbcc6u,0xffb7b9c2u,0xffb7b9c2u,0xffb8b9c2u,
    0xffb3b4bfu,0xffb6b6c1u,0xffbab9c4u,0xffbabac4u,0xffbdbcc6u,0xffb4b3bfu,0xff83828eu,0xff0b0809u,
    0xff070506u,0xff0e0c0du,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff6b666bu,0xff4c494du,0xff5e5b61u,0xff8e8d96u,0xffaaa8b3u,0xffb7b8c2u,0xffb9b9c2u,
    0xffa5a3afu,0xff8f8c94u,0xff615e64u,0xff4b484eu,0xff5c606cu,0xffa6a7b3u,0xffb1b1b9u,0xff373336u,
    0xff040000u,0xff090606u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff2a2424u,0xff000000u,0xff000000u,0xff5c585du,0xffafadb8u,0xffb8b9c2u,0xffb8b9c2u,
    0xffafacb5u,0xff626168u,0xff090b10u,0xff030204u,0xff333944u,0xffa2a4b0u,0xffc2c3ccu,0xffa2a0a8u,
    0xff272930u,0xff1c1f27u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff9e9ca1u,0xff747074u,0xff7c777bu,0xff98949cu,0xffb4b3beu,0xffbcbcc5u,0xffb7b8c2u,
    0xffb3b2bcu,0xff92929cu,0xff898b96u,0xff8b8d95u,0xffafb1bau,0xffbebdc5u,0xffacabb4u,0xffb1b0bau,
    0xff636570u,0xff0c0f16u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xffc0c3cbu,0xffc9cfd5u,0xffc6cad1u,0xffb1afb9u,0xffadaab6u,0xffb6b4c0u,0xffb7b6c2u,
    0xffafadb9u,0xffb2b0bau,0xffc5c7ceu,0xffc6c6ceu,0xffb9b9c1u,0xffa5a2aau,0xff97959fu,0xffa3a1abu,
    0xff5d5f6au,0xff151a2bu,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xffa49fa7u,0xffb1b1b9u,0xffbcbdc5u,0xff9f9ea7u,0xff79778eu,0xff928eaau,0xff7c7995u,
    0xff6b6d83u,0xffacabb5u,0xffb8b7bfu,0xffb0aeb7u,0xff9f9ba4u,0xff7a767bu,0xff7c7b81u,0xff9999a3u,
    0xff545258u,0xff14131au,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff989298u,0xffa7a3a9u,0xffb1afb8u,0xffafadb6u,0xff686a7au,0xff3e4062u,0xff3f435eu,
    0xff7c7f8cu,0xffb0aeb8u,0xffaba7afu,0xffa39fa5u,0xff9a949au,0xff747175u,0xff757377u,0xff92929bu,
    0xff4b4548u,0xff0b0808u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff8a8489u,0xff928c91u,0xff908a8fu,0xff999398u,0xff918b8fu,0xff454654u,0xff666770u,
    0xff989296u,0xff8f898eu,0xff888388u,0xff857f84u,0xff868085u,0xff807b7fu,0xff85838bu,0xffa3a2adu,
    0xff5f5b60u,0xff110e0fu,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff7f797cu,0xff6d686cu,0xff5a565du,0xff4d4a51u,0xff39373du,0xff1b1c22u,0xff24252du,
    0xff3b3c44u,0xff454751u,0xff54565fu,0xff595e64u,0xff6a6b71u,0xff828189u,0xff908f99u,0xffadacb7u,
    0xffb4b4bdu,0xff5b575bu,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff6d696eu,0xff5a585eu,0xff464651u,0xff373743u,0xff32333fu,0xff40424bu,0xff45464eu,
    0xff3c424du,0xff3e4552u,0xff434b58u,0xff505963u,0xff6e7279u,0xff8a8c96u,0xffa6a5afu,0xffb2b0bbu,
    0xffb8b8c1u,0xffb0afb7u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff7c797fu,0xff716d72u,0xff605e63u,0xff53525bu,0xff4b4d58u,0xff4c4f5au,0xff464a57u,
    0xff424857u,0xff48505cu,0xff585e68u,0xff71737au,0xff8e8d97u,0xffa5a3aeu,0xffb0aeb9u,0xffb4b2bdu,
    0xffb4b3bdu,0xffb6b5bfu,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0xff9595a0u,0xff8c8b94u,0xff807d83u,0xff6f6c71u,0xff5f6066u,0xff53565eu,0xff494f5bu,
    0xff515761u,0xff64676du,0xff77777du,0xff8a8a92u,0xff9c9aa6u,0xffa8a6b1u,0xffadacb6u,0xffb1afbbu,
    0xffb4b3bdu,0xffb4b3bdu,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
    0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,
};

std::string roomPath,robertPath,robertAtlas;
SaveFile* questSave=nullptr;

enum class Scene { Exploring, CloseRoom, Turn, Speak, Recital, Bow, WalkAway };
struct RoomRecord { int16_t x,y; uint16_t variant,reserved; uint32_t offset,bytes; };
static_assert(sizeof(RoomRecord)==16);

Physics physics;
AvatarIdle059::Speech speech059;
Scene scene=Scene::Exploring;
unsigned line=0,sceneMs=0,clockMs=0,exitMs=0,currentSpeaker=UINT_MAX;
unsigned dialogueStart=0,roomDialogStart=0;
unsigned cachedVariant=UINT_MAX;
int cachedX=INT_MAX,cachedY=INT_MAX,roomX=0,roomY=0;
uint64_t stepCredit=0;
bool waitRelease=true,aiming=false,retire=false,saved=false;
bool ready=false,robertArtReady=false,showUncanny=false,showShiverDog=false;
bool dialogueContact[std::size(Geometry::shapes)]{};
const char* dialogue=nullptr;
Vec2 drag{};
Sound music=Sound::Count;
float gain=1,fadeFrom=1,fadeTo=1;
unsigned fadeMs=1000;

#include "club_visit054.hpp"
#include "club_npc_draw054.hpp"

void setPaths(const char* executable) {
    NpcDraw::path=assetPath(executable,"club-npcs-054.bin");
    roomPath=assetPath(executable,"club-scenes-054.pak");
    robertPath=assetPath(executable,"club-robert-054.pak");
    robertAtlas=assetPath(executable,"robert-atlas.rgba");

}

bool remember(unsigned phase) {
    if (questPhase()>=phase) return true;
    if(world.rememberQuest)return world.rememberQuest(world.context,phase);
    Progress updated=progress;
    updated.quest=phase;
    if (!questSave || !questSave->write(updated)) return false;
    progress=updated;
    return true;
}

void track(Sound id,unsigned position=0) {
    music=id;
    gain=fadeFrom=fadeTo=1;
    fadeMs=1000;
    if (id==Sound::Count) audio.muteMusic();
    else audio.musicAt(id,position);
}

void fade(float to) {
    fadeFrom=gain;
    fadeTo=to;
    fadeMs=0;
}

void show(const char* key,unsigned speaker=UINT_MAX) {
    if(speaker==UINT_MAX)visit.silence();
    dialogue=key?ClubText054::get(key,russian()):nullptr;
    if(speaker!=UINT_MAX && dialogue)
        visit.typeFor(static_cast<Actor>(speaker),(1000*ClubText054::characterCount(dialogue)+29)/30);
    currentSpeaker=speaker;
    dialogueStart=clockMs;
}

void roomDialogue() {
    for (const auto& entry:Geometry::roomDialogues)
        if (entry.x==roomX && entry.y==roomY) { show(entry.dialogue); return; }
}

void enter(VillaUnlocks::Flags flags) {
    physics={};speech059.clearSpeech();
    enterVisit();
    NpcDraw::reset();
    showUncanny=physics.uncanny=VillaUnlocks::unlocked(flags,VillaUnlocks::Id::Panic);
    showShiverDog=physics.shiverDog=VillaUnlocks::unlocked(flags,VillaUnlocks::Id::BortBash);
    scene=Scene::Exploring;
    line=sceneMs=clockMs=exitMs=roomDialogStart=0;
    dialogue=nullptr;
    currentSpeaker=UINT_MAX;
    roomX=roomY=0;
    ready=robertArtReady=aiming=retire=saved=false;
    drag={};
    waitRelease=true;
    stepCredit=0;
    std::fill(std::begin(dialogueContact),std::end(dialogueContact),false);
    Costumes054::loadBank();
    if (Costumes054::selected==Costumes054::Id::Canny && !Costumes054::actorPixels)
        sceKernelDcacheWritebackRange(const_cast<uint32_t*>(CannyFallbackPixels),sizeof CannyFallbackPixels);
    track(Sound::ClubLoungeMusic);
    roomDialogue();
}

void leave() {
    audio.musicTune(1,1);
    ready=false;
    NpcDraw::reset();
}

unsigned robertFrame(unsigned animation,unsigned timeMs) {
    const auto& track=Geometry::robertAnimations[animation];
    const auto* frames=Geometry::robertFrames+track.first;
    unsigned frameTick=timeMs*60/1000;
    unsigned duration=0;
    for (unsigned i=0;i<track.count;++i) duration+=frames[i].ticks;
    if (track.loop) frameTick%=duration;
    for (unsigned i=0;i<track.count;++i) {
        if (frameTick<frames[i].ticks) return frames[i].variant;
        frameTick-=frames[i].ticks;
    }
    return frames[track.count-1].variant;
}

unsigned robertVariant(int x,int y) {
    if (x==Geometry::robertRoomX && y==Geometry::robertRoomY) {
        switch (scene) {
        case Scene::Exploring:
            return questPhase()<ClubText054::Robert::CompleteQuestPhase?Geometry::robertEntry:0;
        case Scene::CloseRoom:
            return sceneMs<500?Geometry::robertEntry:robertFrame(0,0);
        case Scene::Turn:
            return robertFrame(1,sceneMs);
        case Scene::Speak: {
            const unsigned typing=ClubText054::Robert::typingMs(ClubText054::Robert::Keys[line],russian());
            return robertFrame(sceneMs<typing?2:3,sceneMs);
        }
        case Scene::Recital:
            return robertFrame(4,sceneMs);
        case Scene::Bow:
            return sceneMs<ClubText054::Robert::BowMs?robertFrame(5,sceneMs):
                   robertFrame(6,sceneMs-ClubText054::Robert::BowMs);
        case Scene::WalkAway:
            return sceneMs<ClubText054::Robert::WalkAwayMs?robertFrame(7,sceneMs):Geometry::robertClosed;
        }
    }
    if (physics.entranceClosed &&
        ((x==Geometry::robertCorridorRoomX && y==Geometry::robertRoomY) || (x==-2 && y>=-2 && y<=0)))
        return Geometry::robertClosed;
    return 0;
}

unsigned variant(int x,int y) {
    const unsigned robert=robertVariant(x,y);
    if (robert) return robert>=12?Geometry::robertClosed:robert;
    if (x==-2 && y==0 && physics.breakerDoor) return 1;
    if (physics.breakerOn && ((x==-2 && y==-2) || (x==3 && y==4))) return 2;
    if (x==-1 && y==-2 && (showUncanny || showShiverDog)) return showUncanny?(showShiverDog?5:3):4;
    return 0;
}

bool loadRoom(int x,int y,unsigned version) {
    const auto file=openAssetFile((version>=Geometry::robertEntry?robertPath:roomPath).c_str());
    if (!file || std::fseek(file.get(),0,SEEK_END)!=0) return false;
    const long size=std::ftell(file.get());
    uint32_t header[2]{};
    if (size<=8 || std::fseek(file.get(),0,SEEK_SET)!=0 ||
        std::fread(header,sizeof header,1,file.get())!=1 || header[0]!=0x31434355u ||
        header[1]<1 || header[1]>256) return false;
    RoomRecord match{},fallback{};
    uint32_t expected=8+header[1]*sizeof(RoomRecord);
    for (unsigned i=0;i<header[1];++i) {
        RoomRecord record{};
        if (std::fread(&record,sizeof record,1,file.get())!=1 || record.reserved ||
            record.offset!=expected || expected>static_cast<unsigned>(size) ||
            !record.bytes || record.bytes>static_cast<unsigned>(size)-expected) return false;
        expected+=record.bytes;
        if (record.x!=x || record.y!=y) continue;
        if (record.variant==version) match=record;
        if (!record.variant) fallback=record;
    }
    if (!match.bytes && version<Geometry::robertEntry) match=fallback;
    if (expected!=static_cast<unsigned>(size) || !match.bytes || match.bytes>sizeof fallbackPage ||
        std::fseek(file.get(),static_cast<long>(match.offset),SEEK_SET)!=0 ||
        std::fread(fallbackPage,match.bytes,1,file.get())!=1) return false;
    auto* pixels=reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(vram045(RoomOffset))|0x40000000u);
    fallbackPageIndex=ScreenPages;
    horseMeter039.invalidateShared();
    unsigned long decoded=RoomBytes,packed=match.bytes;
    return uncompress2(pixels,&decoded,reinterpret_cast<uint8_t*>(fallbackPage),&packed)==Z_OK &&
           decoded==RoomBytes && packed==match.bytes && NpcDraw::clean(x,y);
}

bool same(const char* a,const char* b) { return std::strcmp(a,b)==0; }
bool starts(const char* text,const char* prefix) { return std::strncmp(text,prefix,std::strlen(prefix))==0; }

bool updateNamedArea(const char* path) {
    if(starts(path,"Level/Stoppers/SpecialStopper")) {physics.velocity={};return true;}
    if(same(path,"Level/Stoppers/JumpPad/CollisionShape2D")) {
        visit.armApple();physics.launch();audio.play(Sound::JumpPad);return true;
    }
    if(same(path,"Level/Crew/Greenhead/CollisionShape2D")) {
        if(visit.explode())audio.play(Sound::ClubGreenheadBoom);
        return true;
    }
    if (same(path,"Level/Stuff/Exit/CollisionShape2D")) { retire=true; return true; }
    if (same(path,"Level/BreakerDoorHitbox/CollisionShape2D")) {
        if (questPhase()>=ClubText054::Quest::BreakerQuestPhase && !physics.breakerDoor) {
            physics.breakerDoor=true;
            audio.play(Sound::ClubBreakerDoor);
        }
        return true;
    }
    if (same(path,"Level/Breaker/BreakerArea/CollisionShape2D")) {
        if (!physics.breakerOn) {
            physics.breakerOn=true;
            audio.play(Sound::ClubBreakerSpark);
        }
        return true;
    }
    if (starts(path,"Level/Basement/RobertCutscene/PlayerDetector/")) {
        if (questPhase()<ClubText054::Robert::CompleteQuestPhase && scene==Scene::Exploring) {
            scene=Scene::CloseRoom;
            sceneMs=0;
            physics.entranceClosed=true;
            show(nullptr);
            fade(0);
        }
        return true;
    }
    constexpr const char* base="Level/Basement/MusicChangers/";
    if (!starts(path,base)) return false;
    const char* area=path+std::strlen(base);
    if (starts(area,"ToDeepBasement/")) {
        track(Sound::ClubBasementMusic,static_cast<unsigned>((clockMs*441ull/10)%BasementFrames));
        audio.musicTune(0,1);
        gain=0;
        fade(1);
    } else if (starts(area,"FromDeepBasement/") || starts(area,"ToBasement/") || starts(area,"ToZendo/")) {
        if (scene==Scene::Exploring) fade(0);
    } else if (scene==Scene::Exploring) {
        if (music!=Sound::ClubLoungeMusic || gain==0) {
            track(Sound::ClubLoungeMusic,static_cast<unsigned>((clockMs*441ull/10)%LoungeFrames));
            audio.musicTune(0,1);
            gain=0;
        }
        fade(1);
    }
    return true;
}

void contactAreas() {
    if(physics.jumpTicks) {
        std::fill(std::begin(dialogueContact),std::end(dialogueContact),false);
        return;
    }
    for(unsigned i=0;i<std::size(Geometry::shapes);++i) {
        const auto& shape=Geometry::shapes[i];
        if(shape.flags&(Geometry::staticBody|Geometry::initiallyDisabled) || !(shape.mask&2) || physics.hidden(shape.path))continue;
        Vec2 delta{};
        const bool touching=physics.enabled(shape,delta) && intersects(shape,physics.position-delta);
        const bool entered=touching && !dialogueContact[i];
        dialogueContact[i]=touching;
        if(!entered)continue;
        if(updateNamedArea(shape.path)) {
            if(physics.jumpTicks)break;
            continue;
        }
        const auto owner=ClubTracks::shapeOwners[i];
        if(owner.actor<0)continue;
        const auto actor=static_cast<Actor>(owner.actor);
        const bool ouch=owner.part==2;
        const auto time=ouch && actor==Actor::AppleJohn && visit.appleArmed?readClock():ClubEvents::Clock{};
        const auto reaction=visit.contact(actor,ouch,time,1+roll(2),{world.context,world.write});
        if(reaction.stop)physics.velocity={};
        if(reaction.retry) {
            visit.silence();
            dialogue=lang("Could not save Club progress.","Не удалось сохранить события клуба.");
            currentSpeaker=UINT_MAX;dialogueStart=clockMs;
        } else if(reaction.key)show(reaction.key,unsigned(actor));
        playReaction(actor,reaction.cue);
    }
}

void cutscene(unsigned ms) {
    if (scene==Scene::Exploring) return;
    sceneMs+=ms;
    switch (scene) {
    case Scene::CloseRoom:
        if (sceneMs>=ClubText054::Robert::RoomCloseMs) { scene=Scene::Turn; sceneMs=0; }
        break;
    case Scene::Turn:
        if (sceneMs>=ClubText054::Robert::TurnMs) {
            scene=Scene::Speak;
            line=sceneMs=0;
            show(ClubText054::Robert::Keys[0]);
        }
        break;
    case Scene::Speak:
        if (sceneMs<ClubText054::Robert::PauseMs+ClubText054::Robert::typingMs(ClubText054::Robert::Keys[line],russian())) break;
        if (line+1<ClubText054::Robert::LineCount) {
            show(ClubText054::Robert::Keys[++line]);
            sceneMs=0;
        } else {
            if (!remember(ClubText054::Robert::CompleteQuestPhase)) break;
            saved=true;
            scene=Scene::Recital;
            sceneMs=0;
            show(nullptr);
            track(Sound::ClubRecitalMusic);
        }
        break;
    case Scene::Recital:
        if (audio.musicPositionFrames(Sound::ClubRecitalMusic)>=RecitalFrames) { scene=Scene::Bow; sceneMs=0; }
        break;
    case Scene::Bow:
        if (sceneMs>=ClubText054::Robert::BowMs+ClubText054::Robert::TurnBackMs) { scene=Scene::WalkAway; sceneMs=0; }
        break;
    case Scene::WalkAway:
        break;
    default:
        break;
    }
}

void step(Flow& flow,Vec2 input,unsigned held,unsigned pressed,unsigned ms) {
    ms=std::min(ms,100u);
    clockMs+=ms;speech059.tick(ms*1000);
    visit.advance(ms);
    if(visit.greenhead==ClubEvents::Greenhead::Explosion &&
       (world.finishedBoom?world.finishedBoom(world.context):
        uint64_t(visit.boomMs)*44100>=uint64_t(audio.totalFrames(Sound::ClubGreenheadBoom))*1000))
        visit.finishBoom();
    if(visit.bobScare && visit.bobMs>=2000) {flow.quit=true;return;}
    exitMs=held&(PSP_CTRL_START|PSP_CTRL_CIRCLE)?exitMs+ms:0;
    if (exitMs>=ExitHoldMs || retire) {
        flow.screen=Screen::Villa;
        return;
    }
    if (!(held&PSP_CTRL_CROSS)) waitRelease=false;
    if (!waitRelease && (pressed&PSP_CTRL_CROSS) && length(physics.velocity)<=20) {
        aiming=true;
        drag={};
        audio.play(Sound::Grab);
    }
    if (aiming && (held&PSP_CTRL_CROSS)) {
        const float magnitude=length(input);
        drag=magnitude>0.18f?input*(175.0f*(std::min(magnitude,1.0f)-0.18f)/(0.82f*magnitude)):Vec2{};
    } else if (aiming) {
        aiming=false;
        if (length(drag)>=20) {
            physics.velocity=drag*5;
            audio.play(Sound::Shot);
        } else audio.play(Sound::Cancel);
    }
    stepCredit+=ms*60;
    while (stepCredit>=1000) {
        stepCredit-=1000;
        visit.physicsStep();
        AvatarIdle059::Context idle059;idle059.active=true;idle059.moving=length(physics.velocity)>20;idle059.airborne=physics.jumpTicks>0;
        AvatarIdle059::step(AvatarIdle059::cosmetic,idle059,speech059);
        const bool bumped=physics.tick();
        if (bumped) audio.play(Sound::Bounce);
        const int x=roomIndex(physics.position.x),y=roomIndex(physics.position.y);
        if (x!=roomX || y!=roomY) {
            roomX=x;
            roomY=y;
            if (scene==Scene::Exploring) {
                show(nullptr);
                roomDialogStart=clockMs+250;
            }
        }
        contactAreas();
    }
    if (roomDialogStart && clockMs>=roomDialogStart) {
        if (!dialogue) roomDialogue();
        roomDialogStart=0;
    }
    if (fadeMs<1000) {
        fadeMs=std::min(1000u,fadeMs+ms);
        const float t=fadeMs/1000.0f;
        const float ease=scene==Scene::CloseRoom?t:(t<0.5f?4*t*t*t:1-4*(1-t)*(1-t)*(1-t));
        gain=fadeFrom+(fadeTo-fadeFrom)*ease;
        audio.musicTune(gain,1);
    }
    cutscene(ms);
}

void drawDialogue() {
    if (!dialogue) return;
    const unsigned revealed=(clockMs-dialogueStart)*30/1000;
    // Source DialogText: PC x320..704, y88. Keep complete line positions
    // fixed while revealing the existing translation at 30 characters/s.
    localeParagraph(dialogue,272,88*Villa054::Scale,1.0f,204,13,true,revealed);
}

void drawRobert() {
    const unsigned pose=robertVariant(roomX,roomY);
    if (pose<12) return;
    if (!robertArtReady) {
        const auto file=openAssetFile(robertAtlas.c_str());
        auto* pixels=reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(vram045(RobertOffset))|0x40000000u);
        robertArtReady=file && std::fread(pixels,256*128*4,1,file.get())==1 && std::fgetc(file.get())==EOF;
    }
    if (!robertArtReady) return;
    sceGuScissor(ViewX+136,0,ViewX+408,Height);
    sceGuTexFlush();
    setTexture(GU_PSM_8888,256,128,vram045(RobertOffset));
    if (scene==Scene::WalkAway) {
        const float t=std::min(1.0f,sceneMs/float(ClubText054::Robert::WalkAwayMs));
        const float x=(3888+1063*t-roomX*Side)*Villa054::Scale;
        const float y=(2432-815*t-roomY*Side)*Villa054::Scale;
        patch({static_cast<int16_t>(pose==23?0:40),80,34,34,0,0},ViewX+136+x-17,y-17);
    } else {
        const unsigned index=pose-12;
        patch({static_cast<int16_t>(index%6*40),static_cast<int16_t>(index/6*40),40,40,0,0},
              ViewX+136+144,186);
    }
    sceGuScissor(0,0,Width,Height);
}

void draw() {
    const unsigned version=variant(roomX,roomY);
    if (!ready || roomX!=cachedX || roomY!=cachedY || version!=cachedVariant) {
        ready=loadRoom(roomX,roomY,version);
        cachedX=roomX;
        cachedY=roomY;
        cachedVariant=version;
    }
    if(ready && !NpcDraw::prepare())ready=false;
    beginFrame();
    Villa054::drawHud(clockMs*60/1000);
    if (ready) {
        sceGuTexFlush();
        setTexture(GU_PSM_5650,512,512,vram045(RoomOffset));
        patch({0,0,272,272,0,0},ViewX+136,0);
        NpcDraw::drawScareFloor();
    } else localeText(lang("Club room could not be read","Не удалось прочитать комнату"),272,128,0.72f,White,true,260);
    if (ready) {drawRobert();NpcDraw::draw(false);}
    const Vec2 at{ViewX+136+(physics.position.x-roomX*Side)*Villa054::Scale,
                  (physics.position.y-roomY*Side)*Villa054::Scale};
    if (aiming) {
        const Vec2 direction=drag*Villa054::Scale;
        const float size=length(direction);
        if (size>0) {
            const Vec2 normal=Vec2{-direction.y,direction.x}*(1.5f/size);
            sceGuDisable(GU_TEXTURE_2D);
            auto* line=static_cast<ColorVertex*>(sceGuGetMemory(4*sizeof(ColorVertex)));
            line[0]={Gold,at.x+normal.x,at.y+normal.y,0};
            line[1]={Gold,at.x-normal.x,at.y-normal.y,0};
            line[2]={Gold,at.x+direction.x+normal.x,at.y+direction.y+normal.y,0};
            line[3]={Gold,at.x+direction.x-normal.x,at.y+direction.y-normal.y,0};
            sceGuDrawArray(GU_TRIANGLE_STRIP,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,4,nullptr,line);
        }
    }
    if (Costumes054::actorPixels) {
        const auto pair=VillaCostumes::actorPair(Costumes054::selected);
        const auto& image=Costumes054::Art::Actors[pair.player];
        sceGuScissor(ViewX+136,0,ViewX+408,Height);
        sceGuTexFlush();
        setTexture(GU_PSM_8888,Costumes054::Art::ActorsWidth,Costumes054::Art::ActorsHeight,Costumes054::actorPage());
        if (image.width>0) patch(image,at.x+image.x*physics.jumpScale,at.y+image.y*physics.jumpScale,physics.jumpScale);
        sceGuScissor(0,0,Width,Height);
    }
    else if (Costumes054::selected==Costumes054::Id::Canny) {
        sceGuScissor(ViewX+136,0,ViewX+408,Height);
        sceGuTexFlush();
        sceGuEnable(GU_TEXTURE_2D);
        // setTexture uses swizzle=true; this embedded mini-atlas is explicitly linear.
        sceGuTexMode(GU_PSM_8888,0,0,GU_FALSE);
        sceGuTexImage(0,32,32,32,CannyFallbackPixels);
        patch({1,1,17,17,-8,-8},at.x-8*physics.jumpScale,at.y-8*physics.jumpScale,physics.jumpScale);
        sceGuScissor(0,0,Width,Height);
    }
    if(speech059.remaining){
        Course speechCourse{};speechCourse.top=-256;speechCourse.bottom=256;
        const GameplayView view(speechCourse,ViewX,false,1,{-float(roomX*Side)-256,-float(roomY*Side)-256});
        AvatarSpeech058::draw(view,physics.position,avatarIdleLayout059);
    }
    if(ready)NpcDraw::draw(true);
    drawDialogue();
    localeText(lang("Hold START to leave","Удерживайте START для выхода"),272,4,1.0f,White,true,258,true);
    if (exitMs) rectangle(ViewX+146,16,252.0f*exitMs/ExitHoldMs,3,Gold);
    endFrame();
}

} // namespace Club054





