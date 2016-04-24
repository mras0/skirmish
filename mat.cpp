#include "mat.h"

namespace skirmish {

static_assert(sizeof(mat3f<void>) == sizeof(float)*9, "");

} // namespace skirmish