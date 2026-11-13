#pragma once

#include "elysian_horse039.hpp"
#include "elysian_horse050809.hpp"
#include "elysian_geometry041.hpp"
#include "elysian_geometry042.hpp"
#include "elysian_geometry043.hpp"
#include "elysian_geometry044.hpp"
#include "elysian_geometry045.hpp"

inline constexpr HorseCourse039 ElysianHorseCourses040[]={
    {256.0f,{}, {1,-0.3f},3,150.0f,300,{{
        {{0,-224},1,true}, {{0,224},2,false}, {{224,0},-1,false}
    }}},
    {320.0f,{144,176},{-1,1},4,150.0f,300,{{
        {{0,-192},1,true}, {{192,0},-1,false}, {{-192,0},3,true}, {{0,192},-1,false}
    }}},
    {320.0f,{-248,8},{0,1},6,125.0f,400,{{
        {{-96,-288},-1,true}, {{0,-288},-1,true}, {{96,-288},-1,true},
        {{-96,288},-1,true}, {{0,288},-1,true}, {{96,288},-1,true}
    }}},
    {384.0f,{}, {0,-1},7,150.0f,300,{{
        {{-192,288},-1,true}, {{-320,-64},-1,true}, {{128,-256},-1,true},
        {{192,128},-1,true}, {{320,0},-1,true}, {{-160,-352},-1,true}, {{-64,32},-1,true}
    }}},
    ElysianHorse508,
    ElysianHorse509,
    ElysianHorse510,
    ElysianHorse511,
    ElysianHorse512,
    ElysianHorse513,
    ElysianHorse514,
    ElysianHorse515,
    ElysianHorse516,
    ElysianHorse517,
    ElysianHorse518
};
