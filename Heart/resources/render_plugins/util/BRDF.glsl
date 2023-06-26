#ifndef UTIL_BRDF
#define UTIL_BRDF

#include "Constants.glsl"

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

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

#endif
