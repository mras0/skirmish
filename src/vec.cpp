#include "vec.h"

namespace skirmish {

static_assert(sizeof(vec<3, float, void>) == sizeof(float)*3, "");

} // namespace skirmish