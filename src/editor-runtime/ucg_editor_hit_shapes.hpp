#pragma once
#include <cstddef>
namespace ucg {
namespace editor {
struct HitShape {
    enum Kind { Circle, Rectangle, Polygon };
    Kind kind;
    double x, y;
};
inline constexpr HitShape object_hit_shapes[] = {
    {HitShape::Circle, 15.8, 0},       {HitShape::Circle, 16.2788, 0},
    {HitShape::Circle, 16.0, 0},       {HitShape::Rectangle, 16.0, 16.0},
    {HitShape::Circle, 11.1803, 0},    {HitShape::Circle, 16.0, 0},
    {HitShape::Circle, 10.05, 0},      {HitShape::Circle, 16.0, 0},
    {HitShape::Circle, 24.0, 0},       {HitShape::Circle, 20, 0},
    {HitShape::Circle, 16.0, 0},       {HitShape::Rectangle, 16.0, 16.0},
    {HitShape::Rectangle, 8.0, 8.0},   {HitShape::Rectangle, 16.0, 16.0},
    {HitShape::Rectangle, 16.0, 16.0}, {HitShape::Circle, 15.8, 0},
    {HitShape::Circle, 16.0, 0},       {HitShape::Circle, 16.0, 0},
    {HitShape::Circle, 16.0, 0},       {HitShape::Polygon, 0, 0},
    {HitShape::Polygon, 0, 0},         {HitShape::Circle, 20, 0},
    {HitShape::Circle, 20, 0},         {HitShape::Polygon, 0, 0},
    {HitShape::Rectangle, 32.0, 32.0}, {HitShape::Circle, 11.1803, 0},
};
struct DecoHitImage {
    const char *path;
    double width, height;
};
inline constexpr DecoHitImage deco_hit_images[] = {
    {"res://assetsNEW/graphics/stage/deco/W0/light.png", 500, 500},
    {"res://assetsNEW/graphics/stage/deco/W1/bubble.png", 200, 200},
    {"res://assetsNEW/graphics/stage/deco/W1/cautionsign.png", 500, 500},
    {"res://assetsNEW/graphics/stage/deco/W1/foliage1.png", 1920, 1733},
    {"res://assetsNEW/graphics/stage/deco/W1/golfball.png", 360, 360},
    {"res://assetsNEW/graphics/stage/deco/W1/poolcanny.png", 1024, 1024},
    {"res://assetsNEW/graphics/stage/deco/W1/textfun.png", 474, 101},
    {"res://assetsNEW/graphics/stage/deco/W1/restarea.png", 300, 159},
    {"res://assetsNEW/graphics/stage/deco/W1/skullemoji.png", 230, 254},
    {"res://assetsNEW/graphics/stage/deco/W1/roundaboutsign.png", 400, 400},
    {"res://assetsNEW/graphics/stage/deco/W1/textcraig.png", 318, 146},
    {"res://assetsNEW/graphics/stage/deco/W1/golfcentral.png", 398, 82},
    {"res://assetsNEW/graphics/stage/deco/W2/bank_text.png", 693, 210},
    {"res://assetsNEW/graphics/stage/deco/W2/trashbag.png", 896, 896},
    {"res://assetsNEW/graphics/stage/deco/W2/parking_sign.png", 500, 500},
    {"res://assetsNEW/graphics/stage/deco/W2/blue_dumpster.png", 508, 479},
    {"res://assetsNEW/graphics/stage/deco/W2/green_dumpster.png", 370, 370},
    {"res://assetsNEW/graphics/stage/deco/W3/chuckle_mart.png", 256, 32},
    {"res://assetsNEW/graphics/stage/deco/W2/stoplight.png", 192, 172},
    {"res://assetsNEW/graphics/stage/deco/W2/glowstick_welcome.png", 352, 80},
    {"res://assetsNEW/graphics/stage/deco/W3/trespassing_sign.png", 79, 58},
    {"res://assetsNEW/graphics/stage/deco/W3/chuckle_ufo.png", 700, 1152},
    {"res://assetsNEW/graphics/stage/deco/W3/chuckle_aisle.png", 128, 32},
    {"res://assetsNEW/graphics/stage/deco/W5/real_rock_1.png", 2483, 2077},
    {"res://assetsNEW/graphics/stage/deco/W5/real_rock_2.png", 1281, 758},
    {"res://assetsNEW/graphics/stage/deco/W5/real_tree_1.png", 2000, 2037},
    {"res://assetsNEW/graphics/stage/deco/W0/terry.png", 89, 129},
    {"res://assetsNEW/graphics/stage/deco/W3/mr_bones.png", 75, 100},
    {"res://assetsNEW/graphics/stage/deco/W4/bonepiledeco.png", 423, 412},
    {"res://assetsNEW/graphics/stage/deco/W6/mooncow.png", 192, 140},
    {"res://assetsNEW/graphics/stage/deco/W3/smileydisc.png", 774, 42},
    {"res://assetsNEW/graphics/stage/deco/W1/cool_A.png", 107, 112},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/B.png", 65, 62},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/C.png", 78, 99},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/D.png", 71, 87},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/E.png", 71, 71},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/F.png", 93, 92},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/G.png", 103, 104},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/H.png", 56, 64},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/I.png", 84, 109},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/J.png", 62, 76},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/K.png", 73, 89},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/L.png", 72, 84},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/M.png", 73, 71},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/N.png", 75, 84},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/O.png", 73, 82},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/P.png", 87, 131},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/Q.png", 52, 69},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/R.png", 116, 105},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/S.png", 78, 114},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/T.png", 63, 76},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/U.png", 80, 94},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/V.png", 88, 94},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/W.png", 106, 90},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/X.png", 128, 148},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/Y.png", 112, 155},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/Z.png", 81, 108},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/exclamation.png", 66, 114},
    {"res://assetsNEW/graphics/stage/deco/EDITOR/question.png", 99, 127},
    {"res://assetsNEW/graphics/stage/deco/W3/color_tube.png", 190, 161},
    {"res://assetsNEW/graphics/stage/deco/W3/dog_logo.png", 350, 172},
    {"res://assetsNEW/graphics/stage/deco/W4/bed.png", 224, 160},
    {"res://assetsNEW/graphics/stage/deco/W3/coaster_poster_5.png", 172, 108},
    {"res://assetsNEW/graphics/stage/deco/W3/factory_gears.png", 462, 146},
    {"res://assetsNEW/graphics/stage/deco/W3/factory_worker.png", 64, 32},
    {"res://assetsNEW/graphics/stage/deco/W3/hit_switch_sign.png", 128, 192},
};
} // namespace editor
} // namespace ucg
