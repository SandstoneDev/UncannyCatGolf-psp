#pragma once
#include <cstddef>
namespace ucg { namespace editor {
struct VariableDef { const char* name; const char* varname; const char* type; const char* default_json; const char* enum_names_json; double min_value,max_value; bool bounded; };
struct ObjectDef {int id;const char* name;const char* scene_path;const char* uid;const char* credit;const VariableDef* variables;size_t variable_count;const char* defaults_json;bool polygon,player;};
inline const VariableDef object_variables_1[] = {
{"Speed Limit","speed_limit","number","40.0","[]",0,0,false},
{"Acceleration","acceleration","number","0.35","[]",0,0,false},
};
inline const VariableDef object_variables_3[] = {
{"Enemy to spawn","eEnemy","enum","0","[\"Car\", \"Skull\", \"Chuckle\"]",0,2,true},
{"Speed","b_speed","number","0","[]",0,0,false},
{"Rotation","b_rotate","number","0.0","[]",0,0,false},
{"Enemy Score","b_score_value","number","0","[]",0,0,false},
{"Spawn rate","spawn_rate","number","0","[]",0,0,false},
{"Initial Spawn Countdown","spawn_countdown","number","0","[]",0,0,false},
{"Max enemies","max_baddies","number","0","[]",0,0,false},
{"Dir X","dirx","number","0","[]",0,0,false},
{"Dir Y","diry","number","0","[]",0,0,false},
};
inline const VariableDef object_variables_4[] = {
{"Score","score_value","number","50","[]",0,0,false},
};
inline const VariableDef object_variables_8[] = {
{"Redoable","redoable","bool","false","[]",0,0,false},
{"Time","time_given","number","0.0","[]",0,0,false},
{"Score","score_value","number","2000","[]",0,0,false},
};
inline const VariableDef object_variables_9[] = {
{"Score","score_value","number","0","[]",0,0,false},
};
inline const VariableDef object_variables_12[] = {
{"inactive","inactive","bool","false","[]",0,0,false},
};
inline const VariableDef object_variables_13[] = {
{"X size","sizex","number","32","[]",0,0,false},
{"Y size","sizey","number","32","[]",0,0,false},
};
inline const VariableDef object_variables_15[] = {
{"Speed","speed","number","0","[]",0,0,false},
{"Dir X","dirx","number","0","[]",0,0,false},
{"Dir Y","diry","number","0","[]",0,0,false},
};
inline const VariableDef object_variables_17[] = {
{"Speed","speed","number","100","[]",0,0,false},
{"Score","score_value","number","25","[]",0,0,false},
{"Dir X","dirx","number","0","[]",0,0,false},
{"Dir Y","diry","number","0","[]",0,0,false},
};
inline const VariableDef object_variables_18[] = {
{"Speed","speed","number","0","[]",0,0,false},
{"Score","score_value","number","25","[]",0,0,false},
{"Dir X","dirx","number","0","[]",0,0,false},
{"Dir Y","diry","number","0","[]",0,0,false},
};
inline const VariableDef object_variables_20[] = {
{"Floor Type","deco","enum","0","[\"Tiles\", \"Golf\", \"Dirt\", \"Asphalt\", \"Chuckle\", \"Mart\", \"Arcade\", \"Snow\", \"Metal\", \"Grass\", \"Golf Central\"]",0,10,true},
};
inline const VariableDef object_variables_22[] = {
{"Direction","direction","number","0.0","[]",0,0,false},
{"Lock Speed","speed_granted","number","0.0","[]",0,0,false},
};
inline const VariableDef object_variables_24[] = {
{"Enable Smoothing","smoothing","bool","false","[]",0,0,false},
{"Smoothing Speed","smoothing_speed","number","10.0","[]",0,0,false},
{"Segment Size (X)","segment_size_x","number","512","[]",0,0,false},
{"Segment Size (Y)","segment_size_y","number","512","[]",0,0,false},
};
inline const VariableDef object_variables_25[] = {
{"Modifier","mod","enum","0","[\"Hard Mode\", \"Burst\", \"Telekinesis\", \"Buddy\", \"Parry\", \"Jumpy\", \"Dizzy\", \"Missiles\", \"Sticky\", \"God\", \"Borts\", \"Bees\"]",0,11,true},
};
inline const ObjectDef object_definitions[] = {
{0,"Player","res://assetsNEW/scenes/objects/player/player.tscn","uid://b68tiqag02djh","",nullptr,0,"{\"credits_mode\":false,\"buffered_shot_frame\":0,\"wall_death_count\":0,\"wall_death_timer\":0,\"portal_sick_timer\":0,\"portal_sick\":false,\"jump_death_buffer\":0,\"additional_velocity\":\"(0,0)\",\"truevel\":\"(0,0)\",\"has_keys\":[],\"key_rotation_offset\":\"(48.0, 0.0)\",\"afterimage_cooldown\":0.0,\"jump_collision_mask\":65536,\"pause_glass\":false,\"glass_health\":3,\"parry_active\":0,\"parry_timer\":0,\"tk_strokes\":0.0}",false,true},
{1,"Ghost","res://assetsNEW/scenes/objects/ghost/ghost.tscn","uid://wgr23py8fg1b","",object_variables_1,2,"{\"acceleration\":0.35,\"speed_limit\":40.0,\"parry_stun\":0,\"afterimage_cooldown\":0.0}",false,false},
{2,"Goal","res://assetsNEW/scenes/objects/important/goal.tscn","uid://bbp84ahibwsn3","",nullptr,0,"{\"has_flag\":true}",false,false},
{3,"BaddieSpawner","res://assetsNEW/scenes/objects/baddies/baddie_spawner.tscn","uid://pckk4rg2hklu","",object_variables_3,9,"{\"death_FX\":\"poof\",\"do_thought\":\"\",\"show_light\":false,\"score_value\":1000,\"coin_multiply\":false,\"coin_multiply_value\":0,\"does_speed\":false,\"set_speed\":0.0,\"spawn_rate\":0,\"spawn_countdown\":0,\"max_baddies\":0,\"b_speed\":0,\"b_score_value\":0,\"b_rotate\":0.0,\"b_ignorey_caronly\":false,\"colorful\":false,\"spawned_baddies\":[],\"dirx\":0,\"diry\":0,\"eEnemy\":0}",false,false},
{4,"Coin","res://assetsNEW/scenes/objects/collectibles/coin.tscn","uid://piiexm01bud6","",object_variables_4,1,"{\"death_FX\":\"NULL\",\"do_thought\":\"Money\",\"show_light\":false,\"score_value\":50,\"coin_multiply\":true,\"coin_multiply_value\":5,\"does_speed\":false,\"set_speed\":0.0}",false,false},
{5,"SpeedToken","res://assetsNEW/scenes/objects/collectibles/speed_token.tscn","uid://olhdu3o67heb","",nullptr,0,"{\"death_FX\":\"\",\"do_thought\":\"\",\"show_light\":false,\"score_value\":0,\"coin_multiply\":false,\"coin_multiply_value\":0,\"does_speed\":true,\"set_speed\":875.0}",false,false},
{6,"StopToken","res://assetsNEW/scenes/objects/collectibles/stop_token.tscn","uid://djb664usmchpa","",nullptr,0,"{\"death_FX\":\"NULL\",\"do_thought\":\"\",\"show_light\":false,\"score_value\":0,\"coin_multiply\":false,\"coin_multiply_value\":0,\"does_speed\":true,\"set_speed\":0.0}",false,false},
{7,"Nuke","res://assetsNEW/scenes/objects/collectibles/nuke.tscn","uid://cuoktmd6vc16v","",nullptr,0,"{\"death_FX\":\"NULL\",\"do_thought\":\"\",\"show_light\":false,\"score_value\":0,\"coin_multiply\":false,\"coin_multiply_value\":0,\"does_speed\":false,\"set_speed\":0.0}",false,false},
{8,"SmileyRing","res://assetsNEW/scenes/objects/collectibles/smiley_ring.tscn","uid://cyt4gpulwqebl","",object_variables_8,3,"{\"death_FX\":\"NULL\",\"do_thought\":\"\",\"show_light\":false,\"score_value\":2000,\"coin_multiply\":false,\"coin_multiply_value\":0,\"does_speed\":false,\"set_speed\":0.0,\"redoable\":false,\"ring_bell\":true,\"time_given\":0.0,\"hasKey\":false,\"coins_grabbed\":0,\"keyDone\":false}",false,false},
{9,"SmileyCoin","res://assetsNEW/scenes/objects/collectibles/smiley_coin.tscn","uid://cpj64lw6y6p03","",object_variables_9,1,"{\"death_FX\":\"NULL\",\"do_thought\":\"\",\"show_light\":false,\"score_value\":0,\"coin_multiply\":false,\"coin_multiply_value\":0,\"does_speed\":false,\"set_speed\":0.0}",false,false},
{10,"Key","res://assetsNEW/scenes/objects/collectibles/key.tscn","uid://c1e10ljuoyiei","",nullptr,0,"{\"death_FX\":\"NULL\",\"do_thought\":\"\",\"show_light\":false,\"score_value\":0,\"coin_multiply\":false,\"coin_multiply_value\":0,\"does_speed\":false,\"set_speed\":0.0,\"accel\":-0.2,\"follow_mode\":false,\"rotate_offset\":\"(64.0, 0.0)\"}",false,false},
{11,"DBlock","res://assetsNEW/scenes/objects/blocks/d_block.tscn","uid://bqsrs6wh6gc17","",nullptr,0,"{\"size\":\"(32, 32)\",\"death_FX\":\"poof\",\"score_value\":10,\"dblock_score\":true}",false,false},
{12,"Portal","res://assetsNEW/scenes/objects/gimmicks/portal.tscn","uid://cae2pkqfmb8go","",object_variables_12,1,"{\"inactive\":false}",false,false},
{13,"KeyDoor","res://assetsNEW/scenes/objects/blocks/key_door.tscn","uid://bdnhpxy3ifxk","",object_variables_13,2,"{\"size\":\"(32, 32)\",\"death_FX\":\"poof\",\"score_value\":200,\"dblock_score\":false,\"sizex\":32,\"sizey\":32,\"color\":\"(1, 1, 1, 1)\"}",false,false},
{14,"SwitchBlock","res://assetsNEW/scenes/objects/blocks/switch_block.tscn","uid://d278t8ghf4m3y","",nullptr,0,"{\"size\":\"(32, 32)\",\"death_FX\":\"\",\"score_value\":0,\"dblock_score\":false,\"keep_face\":false}",false,false},
{15,"DogGolf","res://assetsNEW/scenes/objects/gimmicks/dog_golf.tscn","uid://gam8ay0mjyoy","",object_variables_15,3,"{\"winner\":false,\"speed\":0,\"bounce_off_player\":true,\"dirx\":0,\"diry\":0}",false,false},
{16,"DogGoal","res://assetsNEW/scenes/objects/gimmicks/dog_goal.tscn","uid://c0vgg11xdulcd","",nullptr,0,"{\"taken\":false}",false,false},
{17,"Skull","res://assetsNEW/scenes/objects/baddies/skull.tscn","uid://ea5qc4x5roc5","",object_variables_17,4,"{\"dead\":false,\"death_sound_pitch\":\"(0.8, 1.55)\",\"death_FX\":\"poof\",\"do_thought\":\"Skull\",\"speed\":100,\"score_value\":25,\"rotate\":0.0,\"chaser\":false,\"chase_accel\":4.0,\"chase_limit\":200.0,\"ignorey\":false,\"dirx\":0,\"diry\":0}",false,false},
{18,"Chuckle","res://assetsNEW/scenes/objects/baddies/chuckle.tscn","uid://dwmxq32y2nysn","",object_variables_18,4,"{\"dead\":false,\"death_sound_pitch\":\"(0.7, 1.3)\",\"death_FX\":\"poof\",\"do_thought\":\"Chuckle\",\"speed\":0,\"score_value\":25,\"rotate\":0.0,\"chaser\":true,\"chase_accel\":4.0,\"chase_limit\":200.0,\"ignorey\":false,\"dirx\":0,\"diry\":0}",false,false},
{19,"SandPit","res://assetsNEW/scenes/objects/gimmicks/sand_pitEditor.tscn","uid://cihdfsewiymh","",nullptr,0,"{\"points\":[],\"deco\":0}",true,false},
{20,"DecoFloor","res://assetsNEW/scenes/objects/gimmicks/floorDecoEditor.tscn","uid://b1je84ep5orrq","",object_variables_20,1,"{\"points\":[],\"deco\":0,\"variables\":{\"deco\":0}}",true,false},
{21,"light","res://assetsNEW/scenes/objects/deco/editorLight.tscn","uid://d0yoqxxx42kuv","",nullptr,0,"{\"energy\":1}",false,false},
{22,"JumpPad","res://assetsNEW/scenes/objects/gimmicks/jump_pad.tscn","uid://x3mtyoq3osrw","",object_variables_22,2,"{\"direction\":0.0,\"speed_granted\":0.0}",false,false},
{23,"WaterDeath","res://assetsNEW/scenes/objects/gimmicks/water_deathEditor.tscn","uid://c0kajas8arpxj","",nullptr,0,"{\"points\":[],\"deco\":0,\"always_kill\":true,\"make_ripples\":false,\"ripple_countdown\":0.1}",true,false},
{24,"CameraFollower","res://assetsNEW/scenes/editor/camera_follower.tscn","uid://c2qatmcpu1uks","Mira",object_variables_24,4,"{\"actually_in_level\":false,\"smoothing\":false,\"smoothing_speed\":10.0,\"segment_size_x\":512,\"segment_size_y\":512}",false,false},
{25,"modifierCollect","res://assetsNEW/scenes/editor/modifierCollectible.tscn","uid://b33adjfusx0u","",object_variables_25,1,"{\"death_FX\":\"\",\"do_thought\":\"\",\"show_light\":false,\"score_value\":0,\"coin_multiply\":false,\"coin_multiply_value\":0,\"does_speed\":false,\"set_speed\":0.0,\"mod\":0}",false,false},
};
inline constexpr size_t object_definition_count=sizeof(object_definitions)/sizeof(object_definitions[0]);
struct MetadataDef {const char* key;const char* default_json;};
inline const MetadataDef metadata_definitions[] = {
{"levelName","\"\""},
{"levelAuthor","\"\""},
{"world","\"\""},
{"id","\"\""},
{"rank1","7500.0"},
{"rank2","6500.0"},
{"rank3","5500.0"},
{"rank4","5000.0"},
{"levelBounds","256.0"},
{"light","\"(1, 1, 1, 1)\""},
{"song","0"},
{"customSongName","\"\""},
{"bg","0"},
{"tiles","[]"},
{"switchTiles1","[]"},
{"switchTiles2","[]"},
{"switchTiles3","[]"},
{"switchTiles4","[]"},
{"dogtiles","[]"},
{"nodes","[]"},
{"deco","[]"},
{"tBright","\"(1, 1, 1, 1)\""},
{"tNormal","\"(0.721569, 0.721569, 0.721569, 1)\""},
{"tDark","\"(0.419608, 0.419608, 0.419608, 1)\""},
{"tExtra","\"(1, 1, 1, 1)\""},
{"par","0"},
{"time","0"},
{"bpm","0"},
{"loopPoint","0"},
{"customtiles","\"\""},
{"switch1customtiles","\"\""},
{"switch2customtiles","\"\""},
{"switch3customtiles","\"\""},
{"switch4customtiles","\"\""},
{"dogcustomtiles","\"\""},
{"mods","\"\""},
{"customfloor","\"\""},
};
inline constexpr size_t metadata_definition_count=sizeof(metadata_definitions)/sizeof(metadata_definitions[0]);
}}
