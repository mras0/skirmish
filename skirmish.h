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

static constexpr world_normal world_up{0, 0, 1};

template<typename T>
static constexpr float pi_v = T(3.1415926535897932384626433832795028841968);

static constexpr auto pi_f = pi_v<float>;

} // namespace skirmish

#endif