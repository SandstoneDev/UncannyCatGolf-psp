#pragma once

namespace PhonePronouns054 {

enum class Label : unsigned {
    TheyThem, TheyHim, TheyHer, TheyIts, HeThem, HeHim, HeHer, HeIts,
    SheThem, SheHim, SheHer, SheIts, ItThem, ItHim, ItHer, ItIts, Any, Ask
};

// random(inclusiveMax) draws independently from [0, inclusiveMax].
// Each actor keeps the result obtained when it is created.
template<class Random>
Label draw(Random&& random,bool allowOverrides) {
    const unsigned subject=random(3);
    const unsigned object=random(3);
    auto label=static_cast<Label>(subject*4+object);
    if (allowOverrides) {
        if (random(10)==10) label=Label::Any;
        if (random(10)==10) label=Label::Ask;
    }
    return label;
}

inline const char* text(Label label) {
    constexpr const char* labels[]={
        "They/Them","They/Him","They/Her","They/Its","He/Them","He/Him","He/Her","He/Its",
        "She/Them","She/Him","She/Her","She/Its","It/Them","It/Him","It/Her","It/Its","Any","Ask!"
    };
    return labels[static_cast<unsigned>(label)];
}

}
