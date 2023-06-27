#ifndef UTIL_HALTON
#define UTIL_HALTON

const uint HALTON_PRIMES[] = uint[](
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43,
    47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103,
    107, 109, 113, 127, 131
);

// https://link.springer.com/content/pdf/10.1007/978-1-4842-4427-2_25.pdf
struct HaltonState
{
    uint dimension;
    uint sequenceIndex;
};

struct HaltonData
{
    uint p2;
    uint p3;
    uint w;
    uint h;
    uint mX;
    uint mY;
};

uint Halton3Inverse(uint index, uint digits)
{
    uint result = 0;
    for (uint d = 0; d < digits; ++d)
    {
        result = result * 3 + index % 3;
        index /= 3;
    }
    return result;
}

uint Halton2Inverse(uint index, uint digits)
{
    index = (index << 16) | (index >> 16);
    index = ((index & 0x00ff00ff) << 8) | ((index & 0xff00ff00) >> 8);
    index = ((index & 0x0f0f0f0f) << 4) | ((index & 0xf0f0f0f0) >> 4);
    index = ((index & 0x33333333) << 2) | ((index & 0xcccccccc) >> 2);
    index = ((index & 0x55555555) << 1) | ((index & 0xaaaaaaaa) >> 1);
    return index >> (32 - digits);
}

// https://github.com/lgruen/halton/blob/main/halton_enum.h#L122
// TODO: implement full alg on CPU side beforehand
uint HaltonIndex(uvec2 pixel, HaltonData data, uint seed)
{
    uint increment = data.w * data.h;
    return ((Halton2Inverse(pixel.x, data.p2) * data.mX  +
        Halton3Inverse(pixel.y, data.p3) * data.mY) % increment) + seed * increment;
}

void HaltonInit(inout HaltonState state, uvec2 pixel, HaltonData data, uint seed)
{
    state.dimension = 2;
    state.sequenceIndex = HaltonIndex(pixel, data, seed);
}

float HaltonSample(uint dimension, uint index)
{
    uint base = HALTON_PRIMES[dimension];
    float invBase = 1.0 / base;

    float res = 0.0;
    float factor = invBase;
    while (index > 0) {
        res += factor * float(index % base);
        index = uint(index * invBase);
        factor *= invBase;
    }

    return res;
}

float HaltonNext(inout HaltonState state)
{
    return HaltonSample(state.dimension++, state.sequenceIndex);
}

#endif
