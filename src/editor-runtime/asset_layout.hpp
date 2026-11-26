#pragma once

#include <string>
#include <string_view>

namespace AssetLayout {

inline bool begins(std::string_view name,std::string_view prefix) {
    return name.substr(0,prefix.size())==prefix;
}

inline bool isField(std::string_view name) {
    const auto digit=[](char value) { return value>='0' && value<='9'; };
    return name.size()==8 && digit(name[0]) && name[1]=='_' && digit(name[2]) &&
           digit(name[3]) && name.substr(4)==".bin";
}

inline std::string relative(std::string_view original) {
    const bool russian=begins(original,"ru/");
    const auto name=russian?original.substr(3):original;
    const auto path=[&](const char* group,bool localized) {
        std::string output="data/";
        output+=group;
        if (localized) output+=russian?"/ru":"/en";
        output+='/';
        output.append(name);
        return output;
    };
    if (isField(name)) return path("courses",true);
#define ASSET_NAME(filename,group,localized) if (name==filename) return path(group,localized);
#define ASSET_PREFIX(prefix,group,localized) if (begins(name,prefix)) return path(group,localized);
#include "asset_layout.inc"
#undef ASSET_PREFIX
#undef ASSET_NAME
    return path("world",false);
}

}
