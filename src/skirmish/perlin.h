#ifndef SKIRMISH_PERLIN_H
#define SKIRMISH_PERLIN_H

namespace skirmish { namespace perlin {

constexpr int max_number_of_octaves = 23;

float noise_2d(float x, float y, float persistence, int number_of_octaves);

} } // namespace skirmish::perlin

#endif