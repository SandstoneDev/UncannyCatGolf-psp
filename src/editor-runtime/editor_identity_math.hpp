#ifndef UCG_EDITOR_IDENTITY_MATH_HPP
#define UCG_EDITOR_IDENTITY_MATH_HPP
#include <cstdint>
#include <cstring>
namespace EditorIdentityMath {
inline uint64_t bits(double value) {
  uint64_t result;
  std::memcpy(&result, &value, sizeof(result));
  return result;
}
inline bool zero(uint64_t value) { return (value << 1) == 0; }
inline bool finite(uint64_t value) {
  return (value & UINT64_C(0x7ff0000000000000)) != UINT64_C(0x7ff0000000000000);
}
inline bool finiteNonzero(uint64_t value) {
  return finite(value) && !zero(value);
}
inline double add(double a, double b) {
  const auto ab = bits(a), bb = bits(b);
  if (zero(bb) && finiteNonzero(ab))
    return a;
  if (zero(ab) && finiteNonzero(bb))
    return b;
  return a + b;
}
inline double multiply(double a, double b) {
  const auto ab = bits(a), bb = bits(b);
  if (bb == UINT64_C(0x3ff0000000000000) && finite(ab))
    return a;
  if (ab == UINT64_C(0x3ff0000000000000) && finite(bb))
    return b;
  return a * b;
}
inline bool zeroRotation(double x, double y, double angle) {
  return zero(bits(angle)) && finiteNonzero(bits(x)) && finiteNonzero(bits(y));
}
} // namespace EditorIdentityMath
#endif
