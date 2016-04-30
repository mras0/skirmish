#include "mat.h"

namespace skirmish {

static_assert(sizeof(mat<3, 3, float, void>) == sizeof(float)*9, "");

} // namespace skirmish