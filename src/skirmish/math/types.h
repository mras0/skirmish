#ifndef SKIRMISH_MATH_TYPES_H
#define SKIRMISH_MATH_TYPES_H

#include "vec.h"
#include "mat.h"

namespace skirmish {

struct world_pos_tag;
struct world_normal_tag;

using world_pos    = vec<3, float, world_pos_tag>;
using world_normal = vec<3, float, world_normal_tag>;

static constexpr world_normal world_up{0, 0, 1};

} // namespace skirmish

#endif
