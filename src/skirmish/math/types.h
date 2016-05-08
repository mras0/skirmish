#ifndef SKIRMISH_MATH_TYPES_H
#define SKIRMISH_MATH_TYPES_H

#include "vec.h"
#include "mat.h"

namespace skirmish {

struct world_tag;
struct view_tag;
struct projection_tag;

using world_pos         = vec<3, float, world_tag>;
using world_matrix      = mat<4, 4, float, world_tag>;
using view_matrix       = mat<4, 4, float, view_tag>;
using projection_matrix = mat<4, 4, float, projection_tag>;

static constexpr world_pos world_up{0, 0, 1};

} // namespace skirmish

#endif
