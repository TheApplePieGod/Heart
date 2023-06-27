#ifndef UTIL_BRDF
#define UTIL_BRDF

#include "Constants.glsl"

/*
 * Returns the fraction of microfacets on a surface with a given roughness that
 * point in the direction H
 */
float DistributionGGX(float NdotH, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
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

// Combines DistributionGGX with ImportanceSampleGGX and returns a rough N
// vector along with its probability
vec4 ImportanceSampleGGXPDF(vec2 rand, vec3 N, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a*a;
    float phi = 2.0f * PI * rand.x;
    float cosTheta = sqrt((1.0f - rand.y) / ((a2 - 1.0f) * rand.y + 1.0f));
    float cosTheta2 = cosTheta * cosTheta;
    float sinTheta = sqrt(1.0f - cosTheta2);

    // Spherical -> cartesian
    vec3 offset = vec3(
        sinTheta * cos(phi),
        sinTheta * sin(phi),
        cosTheta
    );
    
    float d = cosTheta2 * (a2 - 1) + 1;
    float D = a2 / (PI * d * d);
    float PDF = D * cosTheta;

    // Convert to N space
    vec3 up = vec3(0.0, 0.0, 1.0);
    vec3 tangent = cross(up, N);
    vec3 converted = normalize(tangent * offset.x + up * offset.y + N * offset.z);

    return vec4(converted, PDF);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 ComputeSpecularBRDF(float roughness, vec3 F, float NdotV, float NdotL, float NdotH)
{
    float NDF = DistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    return min(vec3(1.f), (NDF * G * F) / denominator);
}

vec3 ComputeDiffuseBRDF(vec3 diffuse)
{
    return diffuse / PI;
}

vec3 ComputeBRDF(vec3 diffuse, float roughness, vec3 F0, vec3 N, vec3 V, vec3 H, vec3 L)
{
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    vec3 F = FresnelSchlick(VdotH, F0);
    diffuse = ComputeDiffuseBRDF(diffuse);
    vec3 specular = ComputeSpecularBRDF(roughness, F, NdotV, NdotL, NdotH);

    return (vec3(1.0) - F) * diffuse + specular;
}

#endif
