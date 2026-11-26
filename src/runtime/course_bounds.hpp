#pragma once

// Original regular level bounds and flags, in CourseIds order.
// Source provenance: ../source-audit/bounds-manifest.json.
#include "game.hpp"
#include "campaign.hpp"
namespace CourseBounds {
struct Policy { float bounds; bool always; bool never; };
inline constexpr Policy Policies[113] = {
    {256.0f, false, false}, // 0 0_01 root:459
    {256.0f, false, false}, // 1 0_02 root:479
    {256.0f, false, false}, // 2 0_03 root:470
    {256.0f, false, false}, // 3 0_04 root:459
    {256.0f, false, false}, // 4 0_05 root:471
    {256.0f, false, false}, // 5 1_01 root:461
    {256.0f, false, false}, // 6 1_02 root:462
    {256.0f, false, false}, // 7 1_03 root:463
    {256.0f, false, false}, // 8 1_04 root:591
    {256.0f, false, false}, // 9 1_05 root:494
    {256.0f, false, false}, // 10 1_06 root:471
    {256.0f, false, false}, // 11 1_07 root:493
    {256.0f, false, false}, // 12 1_08 root:462
    {256.0f, false, false}, // 13 1_09 root:583
    {256.0f, false, false}, // 14 1_10 root:459
    {256.0f, false, false}, // 15 1_11 root:459
    {256.0f, false, false}, // 16 1_12 root:460
    {256.0f, false, false}, // 17 1_13 root:480
    {256.0f, false, false}, // 18 1_14 root:532
    {256.0f, false, false}, // 19 1_15 root:516
    {448.0f, false, false}, // 20 1_16 root:991
    {512.0f, false, false}, // 21 1_17 root:533
    {256.0f, false, false}, // 22 1_18 root:461
    {256.0f, false, false}, // 23 2_01 root:500
    {256.0f, false, false}, // 24 2_02 root:450
    {256.0f, false, false}, // 25 2_03 root:451
    {256.0f, false, false}, // 26 2_04 root:450
    {256.0f, false, false}, // 27 2_05 root:463
    {304.0f, false, false}, // 28 2_06 root:462
    {256.0f, false, false}, // 29 2_07 root:454
    {256.0f, false, false}, // 30 2_08 root:497
    {256.0f, false, false}, // 31 2_09 root:451
    {320.0f, false, false}, // 32 2_10 root:486
    {320.0f, false, false}, // 33 2_11 root:645
    {320.0f, false, false}, // 34 2_12 root:450
    {512.0f, false, false}, // 35 2_13 root:609
    {384.0f, false, false}, // 36 2_14 root:497
    {384.0f, false, false}, // 37 2_15 root:468
    {448.0f, false, false}, // 38 2_16 root:646
    {384.0f, false, false}, // 39 2_17 root:516
    {256.0f, false, false}, // 40 2_18 root:458
    {256.0f, false, false}, // 41 3_01 root:471
    {256.0f, false, false}, // 42 3_02 root:655
    {256.0f, false, false}, // 43 3_03 root:477
    {320.0f, false, false}, // 44 3_04 root:452
    {256.0f, false, false}, // 45 3_05 root:707
    {256.0f, false, false}, // 46 3_06 root:894
    {320.0f, false, false}, // 47 3_07 root:660
    {256.0f, false, false}, // 48 3_08 root:610
    {320.0f, false, false}, // 49 3_09 root:453
    {256.0f, false, false}, // 50 3_10 root:447
    {512.0f, false, false}, // 51 3_11 root:524
    {384.0f, false, false}, // 52 3_12 root:888
    {256.0f, false, false}, // 53 3_13 root:496
    {256.0f, false, false}, // 54 3_14 root:495
    {256.0f, false, false}, // 55 3_15 root:496
    {256.0f, false, false}, // 56 3_16 root:599
    {448.0f, false, false}, // 57 3_17 root:549
    {320.0f, false, false}, // 58 3_18 root:1307
    {512.0f, false, false}, // 59 4_01 root:538
    {512.0f, false, false}, // 60 4_02 root:554
    {384.0f, false, false}, // 61 4_03 root:509
    {384.0f, false, false}, // 62 4_04 root:545
    {512.0f, false, false}, // 63 4_05 root:542
    {512.0f, false, false}, // 64 4_06 root:573
    {384.0f, false, false}, // 65 4_07 root:559
    {512.0f, false, false}, // 66 4_08 root:545
    {384.0f, false, false}, // 67 4_09 root:527
    {256.0f, false, false}, // 68 4_10 root:531
    {320.0f, false, false}, // 69 4_11 root:520
    {312.0f, false, false}, // 70 4_12 root:663
    {384.0f, false, false}, // 71 4_13 root:661
    {384.0f, false, false}, // 72 4_14 root:850
    {384.0f, false, false}, // 73 4_15 root:593
    {320.0f, false, false}, // 74 4_16 root:526
    {512.0f, false, false}, // 75 4_17 root:875
    {512.0f, false, false}, // 76 4_18 root:577
    {256.0f, false, false}, // 77 5_01 root:454
    {256.0f, false, false}, // 78 5_02 root:456
    {256.0f, false, false}, // 79 5_03 root:1144
    {256.0f, false, false}, // 80 5_04 root:465
    {320.0f, false, false}, // 81 5_05 root:466
    {320.0f, false, false}, // 82 5_06 root:475
    {384.0f, false, false}, // 83 5_07 root:469
    {320.0f, false, false}, // 84 5_08 root:468
    {384.0f, false, false}, // 85 5_09 root:478
    {384.0f, false, false}, // 86 5_10 root:486
    {512.0f, false, false}, // 87 5_11 root:471
    {320.0f, false, false}, // 88 5_12 root:486
    {384.0f, false, false}, // 89 5_13 root:468
    {384.0f, false, false}, // 90 5_14 root:545
    {448.0f, false, false}, // 91 5_15 root:768
    {384.0f, false, false}, // 92 5_16 root:799
    {320.0f, false, false}, // 93 5_17 root:466
    {512.0f, false, false}, // 94 5_18 root:811
    {384.0f, false, false}, // 95 6_01 root:548
    {320.0f, false, false}, // 96 6_02 root:580
    {512.0f, true, false}, // 97 6_03 root:572
    {384.0f, true, false}, // 98 6_04 root:532
    {512.0f, true, false}, // 99 6_05 root:737
    {256.0f, false, false}, // 100 6_06 root:1054
    {384.0f, false, false}, // 101 6_07 root:870
    {448.0f, false, false}, // 102 6_08 root:830
    {256.0f, true, false}, // 103 6_09 root:1067
    {192.0f, true, false}, // 104 6_10 root:1081
    {384.0f, false, false}, // 105 6_11 root:812
    {384.0f, true, false}, // 106 6_12 root:592
    {512.0f, false, false}, // 107 6_13 root:805
    {384.0f, false, false}, // 108 6_14 root:496
    {256.0f, false, true}, // 109 6_15 root:2788
    {384.0f, false, false}, // 110 6_16 root:574
    {640.0f, true, false}, // 111 6_17 root:1417
    {256.0f, false, false}, // 112 6_18 root:537
};
static_assert(sizeof(Policies)/sizeof(Policies[0])==113, "113 regular course policies");

// level.gd uses a strict square bound plus OB_RADIUS, independently of wall geometry.
enum class Death { None, Backrooms, OutOfBounds };
inline Death death(unsigned index,Vec2 position,bool airborne,unsigned deathBuffer,bool santaBoundsArmed=false) {
    if (index>=sizeof(Policies)/sizeof(Policies[0])) return Death::None;
    Policy policy=Policies[index];
    // Santa's script enables bounds and forces O_B only after its notice/exit trigger.
    if (index==CosmicFifteenthCourse) {
        policy.never=!santaBoundsArmed;
        policy.always=santaBoundsArmed;
    }
    if (policy.never || std::max(std::abs(position.x),std::abs(position.y))<=policy.bounds+24.0f)
        return Death::None;
    return policy.always || airborne || deathBuffer ? Death::OutOfBounds : Death::Backrooms;
}
} // namespace CourseBounds
