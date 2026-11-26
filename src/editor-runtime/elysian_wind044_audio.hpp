#pragma once

#include <cstdint>

inline constexpr unsigned WizardWindFractionBits=15;
inline constexpr unsigned WizardWindUnity=1u<<WizardWindFractionBits;

inline int32_t wizardWindSample044(int16_t from,int16_t to,unsigned fraction) {
    return from+(static_cast<int32_t>(to)-from)*static_cast<int32_t>(fraction)/
                static_cast<int32_t>(WizardWindUnity);
}
