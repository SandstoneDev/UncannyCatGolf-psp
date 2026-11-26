#ifndef UCG_EDITOR_PROJECTION_MATH_HPP
#define UCG_EDITOR_PROJECTION_MATH_HPP
#include <cstdint>
#include <cstring>
#ifndef __PSP__
#include <cfenv>
#endif
// Binary64 renderer-only identities. No reassociation, allocations or globals.
namespace EditorProjectionMath {
inline bool nativeNearestEven() {
#ifdef __PSP__
    uint32_t control;
    __asm__ volatile("cfc1 %0, $31":"=r"(control));
    return (control&3u)==0;
#else
    return std::fegetround()==FE_TONEAREST;
#endif
}
inline uint64_t bits(double value) { uint64_t result;std::memcpy(&result,&value,8);return result; }
inline double number(uint64_t value) { double result;std::memcpy(&result,&value,8);return result; }
inline bool zero(uint64_t value) { return (value<<1)==0; }
inline bool finite(uint64_t value) { return (value&UINT64_C(0x7ff0000000000000))!=UINT64_C(0x7ff0000000000000); }
inline bool finiteNonzero(uint64_t value) { return finite(value) && !zero(value); }
inline bool normal(uint64_t value) { const auto e=(value>>52)&2047;return e && e!=2047; }
inline bool power2(uint64_t value) { return normal(value) && !(value&UINT64_C(0xfffffffffffff)); }
inline bool withinProjectionRange(double value) {
    // |value| <=2^40, including signedzero/subnormal. Rejects infinities/NaNs.
    return (bits(value)&UINT64_C(0x7fffffffffffffff))<=UINT64_C(0x4270000000000000);
}
namespace detail {
// Keep exceptional operands on one opaque original operation. This also
// prevents host optimizer operand commuting from changing a NaN's payload
// when the surrounding renderer expression has multiple distinct NaNs.
__attribute__((noinline)) inline double multiply(double a,double b) { volatile double x=a,y=b;return x*y; }
__attribute__((noinline)) inline double divide(double a,double b) { volatile double x=a,y=b;return x/y; }
__attribute__((noinline)) inline double subtract(double a,double b) { volatile double x=a,y=b;return x-y; }
__attribute__((noinline)) inline double add(double a,double b) { volatile double x=a,y=b;return x+y; }
}
inline bool scaled(uint64_t value,uint64_t power,bool division,double& output) {
    if(!normal(value) || !power2(power)) return false;
    const int exponent=int((value>>52)&2047)+(division?1023-int((power>>52)&2047):int((power>>52)&2047)-1023);
    if(exponent<=0 || exponent>=2047) return false;
    output=number(((value^power)&UINT64_C(0x8000000000000000))|
        (uint64_t(exponent)<<52)|(value&UINT64_C(0xfffffffffffff)));
    return true;
}
inline bool tryMultiplyPower2(double a,double b,double& output) {
    const auto ab=bits(a),bb=bits(b);
    return scaled(ab,bb,false,output) || scaled(bb,ab,false,output);
}
inline bool tryDividePower2(double a,double b,double& output) { return scaled(bits(a),bits(b),true,output); }
// Exact nearest-even binary64 result for17/32. P=17*M has at most58bits;
// discarded bits are explicitly rounded before carrying the53bit significand.
// This optional path assumes the renderer's default round-to-nearest-even mode.
inline bool tryScale17by32(double a,double& output) {
    const uint64_t ab=bits(a);
    if(!normal(ab)) return false;
    const uint64_t m=(ab&UINT64_C(0xfffffffffffff))|UINT64_C(0x10000000000000);
    const uint64_t p=(m<<4)+m;
    const unsigned shift=p>=UINT64_C(0x200000000000000)?5:4;
    int exponent=int((ab>>52)&2047)+int(shift)-5;
    // Keep every pre-round underflow path on the original floating operator,
    // even if rounding that tiny value could carry back into the normal range.
    if(exponent<=0 || exponent>=2047) return false;
    uint64_t significand=p>>shift;
    const uint64_t remainder=p&((uint64_t(1)<<shift)-1),half=uint64_t(1)<<(shift-1);
    if(remainder>half || (remainder==half && (significand&1))) ++significand;
    if(significand==UINT64_C(0x20000000000000)) { significand>>=1;++exponent; }
    if(exponent<=0 || exponent>=2047) return false;
    output=number((ab&UINT64_C(0x8000000000000000))|(uint64_t(exponent)<<52)|
        (significand&UINT64_C(0xfffffffffffff)));
    return true;
}
inline double add(double a,double b) {
    const auto ab=bits(a),bb=bits(b);
    if(!finite(ab) || !finite(bb)) return detail::add(a,b);
    if(zero(bb) && finiteNonzero(ab)) return a;
    if(zero(ab) && finiteNonzero(bb)) return b;
    return a+b;
}
inline double subtract(double a,double b) {
    if(!finite(bits(a)) || !finite(bits(b))) return detail::subtract(a,b);
    if(zero(bits(b)) && finiteNonzero(bits(a))) return a;
    return a-b;
}
inline double scale17by32(double a) { double output;if(tryScale17by32(a,output)) return output;return a*(17./32); }
inline double scale17by32Checked(double a,bool nearestEven) {
    return nearestEven?scale17by32(a):detail::multiply(a,17./32);
}
inline double multiply(double a,double b) {
    const auto ab=bits(a),bb=bits(b);
    if(!finite(ab) || !finite(bb)) return detail::multiply(a,b);
    if((zero(ab) && finite(bb)) || (zero(bb) && finite(ab))) return number((ab^bb)&UINT64_C(0x8000000000000000));
    if(bb==UINT64_C(0x3ff0000000000000) && finite(ab)) return a;
    if(ab==UINT64_C(0x3ff0000000000000) && finite(bb)) return b;
    double output;
    if(tryMultiplyPower2(a,b,output)) return output;
    // Keep17/32 explicit at the renderer call site rather than expanding the
    // generic multiply's rounding-mode contract.
    return a*b;
}
inline double divide(double a,double b) {
    if(!finite(bits(a)) || !finite(bits(b))) return detail::divide(a,b);
    double output;if(tryDividePower2(a,b,output)) return output;return a/b;
}
inline bool zeroRotation(double x,double y,double angle) { return zero(bits(angle)) && finiteNonzero(bits(x)) && finiteNonzero(bits(y)); }
}
#endif
