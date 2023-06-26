
#define E 2.71828

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
uint HaltonIndex(uvec2 pixel, uint seed)
{
    uint p2 = uint(ceil(log2(frameBuffer.data.screenSize.x)));
    uint w = uint(pow(2, p2));
    uint p3 = uint(ceil(log2(frameBuffer.data.screenSize.y)/log2(3)));
    uint h = uint(pow(3, p3));

    uint increment = 256*256;
    return ((Halton2Inverse(pixel.x % 256, 8) * 76545 +
        Halton3Inverse(pixel.y % 256, 6) * 110080) % increment) + 0;//seed * increment;
}

void HaltonInit(inout HaltonState state, uvec2 pixel, uint seed)
{
    state.dimension = 2;
    state.sequenceIndex = HaltonIndex(pixel, seed);
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

/*
 * Returns the fraction of microfacets on a surface with a given roughness that
 * point in the direction H
 */
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

/*
 * Returns a vector in spherical coordinates that represents a probabilistic offset
 * from the reflection vector computed based on the roughness of the surface and a random
 * input value vec2(0.0-1.0)
 *
 * https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-20-gpu-based-importance-sampling
 * https://blog.tobias-franke.eu/2014/03/30/notes_on_importance_sampling.html
 */
vec2 ImportanceSampleGGX(vec2 rand, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0f * PI * rand.x;
    float theta = acos(
        sqrt((1.0f - rand.y) / ((a*a - 1.0f) * rand.y + 1.0f))
    );
    return vec2(phi, theta);
}

// TODO: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/reflections/reflections_ray_trace.rgen
vec3 test(vec3 pos, vec3 N, float roughness)
{
    vec3 V = normalize(gl_WorldRayOriginEXT - pos);
    vec3 R = reflect(gl_WorldRayDirectionEXT, N);
    vec3 H = normalize(V + R);

    uint haltonSeed = frameBuffer.data.frameCount;
    HaltonState hstate;
    HaltonInit(hstate, gl_LaunchIDEXT.xy, haltonSeed);

    vec2 offsetVector = ImportanceSampleGGX(
        vec2(fract(HaltonNext(hstate)), fract(HaltonNext(hstate))),
        roughness
    );
    vec3 offsetCartesian = vec3(
        sin(offsetVector.y) * cos(offsetVector.x),
        cos(offsetVector.y),
        sin(offsetVector.y) * sin(offsetVector.x)
    );
    
    // Could be optimized because upAxis and R are normal
    vec3 upAxisCartesian = vec3(0.0, 1.0, 0.0);
    vec3 rotAxis = cross(upAxisCartesian, R);
    float angle = acos(dot(upAxisCartesian, R));
    vec3 RRotated =
        offsetCartesian * cos(angle) + cross(rotAxis, offsetCartesian) * sin(angle) + rotAxis * dot(rotAxis, offsetCartesian) * (1 - cos(angle));

    return RRotated;
}
