#include "perlin.h"
#include <math.h>

float interpolate(float a, float b, float x)
{
    return a + (b - a) * x;
}

float noise_1(int x, int y)
{
    unsigned n = static_cast<unsigned>(x + y * 57);
    n = (n<<13) ^ n;
    return 0.5f + 0.5f * ( 1.0f - ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

float smoothed_noise(int x, int y)
{
    float corners = ( noise_1(x-1, y-1)+noise_1(x+1, y-1)+noise_1(x-1, y+1)+noise_1(x+1, y+1) ) / 16;
    float sides   = ( noise_1(x-1, y)  +noise_1(x+1, y)  +noise_1(x, y-1)  +noise_1(x, y+1) ) /  8;
    float center  =  noise_1(x, y) / 4;
    return corners + sides + center;
}

float interpolated_noise(float x, float y)
{
    const int integer_X    = int(x);
    const float fractional_X = x - integer_X;

    const int integer_Y    = int(y);
    const float fractional_Y = y - integer_Y;

    const float v1 = smoothed_noise(integer_X, integer_Y);
    const float v2 = smoothed_noise(integer_X + 1, integer_Y);
    const float v3 = smoothed_noise(integer_X, integer_Y + 1);
    const float v4 = smoothed_noise(integer_X + 1, integer_Y + 1);

    const float i1 = interpolate(v1, v2, fractional_X);
    const float i2 = interpolate(v3, v4, fractional_X);

    return interpolate(i1, i2, fractional_Y);
}

float perlin_noise_2d(float x, float y, float persistence, int number_of_octaves)
{
    x = fabsf(x);
    y = fabsf(y);

    float total     = 0;
    float frequency = 1;
    float amplitude = 1;
    float max_value = 0;
    for (int i = 0; i < number_of_octaves; ++i) {
        total     += interpolated_noise(x * frequency, y * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= 2;
    }

    return total / max_value;
}