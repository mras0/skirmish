#ifndef SKIRMISH_SKIRMISH_H
#define SKIRMISH_SKIRMISH_H

#include "vec.h"
#include "mat.h"

namespace skirmish {

struct world_pos_tag;
struct world_normal_tag;

using world_pos = vec<3, float, world_pos_tag>;
using world_mat = mat<3, 3, float, world_pos_tag>;

using world_normal = vec<3, float, world_normal_tag>;


} // namespace skirmish

#endif