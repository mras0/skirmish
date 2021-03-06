#ifndef SKIRMISH_MATH_CONSTANTS_H
#define SKIRMISH_MATH_CONSTANTS_H

namespace skirmish {

template<typename T>
static constexpr T pi_v    = T(3.1415926535897932384626433832795028841968);

template<typename T>
static constexpr T exp1_v  = T(2.71828182845904523536028747135266249775);

template<typename T>
static constexpr T sqrt2_v = T(1.414213562373095048801688724209698078569);

static constexpr auto pi_f    = pi_v<float>;
static constexpr auto exp1_f  = exp1_v<float>;
static constexpr auto sqrt2_f = sqrt2_v<float>;

} // namespace skirmish

#endif
