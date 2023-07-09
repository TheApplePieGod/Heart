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
vec3 ImportanceSampleGGX(vec2 rand, vec3 N, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0f * PI * rand.x;
    float cosTheta = sqrt((1.0f - rand.y) / ((a*a - 1.0f) * rand.y + 1.0f));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // Spherical -> cartesian
    vec3 offset = vec3(
        sinTheta * cos(phi),
        sinTheta * sin(phi),
        cosTheta
    );

    // Convert to N space, be careful about axes
    vec3 up = abs(N.z) < 0.999f ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = cross(up, N);
    vec3 converted = normalize(tangent * offset.x + up * offset.y + N * offset.z);

    return converted;
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

    // Convert to N space, be careful about axes
    vec3 up = abs(N.z) < 0.999f ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = cross(up, N);
    vec3 converted = normalize(tangent * offset.x + up * offset.y + N * offset.z);

    return vec4(converted, PDF);
}

// https://jcgt.org/published/0007/04/01/paper.pdf
// https://github.com/NVIDIAGameWorks/Falcor/blob/47f4fbf4fc3cedb329e1a452a58d9d8f42acef69/Source/Falcor/Experimental/Scene/Material/Microfacet.slang
vec3 ImportanceSampleGGXVNDF(vec2 rand, vec3 N, vec3 V, float roughness)
{
    float a = roughness * roughness;

    // Orthonormal N basis
    mat3 TBN;
    TBN[0] = normalize(V - N * dot(N, V));
    TBN[1] = cross(N, TBN[0]);
    TBN[2] = N;

    // Convert V to tangent space
    V = V * TBN;
    
    // Transform the view vector to the hemisphere configuration
    vec3 Vh = normalize(vec3(a * V.x, a * V.y, V.z));

    // Construct orthonormal basis (Vh,T1,T2).
    vec3 T1 = (Vh.z < 0.9999f) ? normalize(cross(vec3(0, 0, 1), Vh)) : vec3(1, 0, 0);
    vec3 T2 = cross(Vh, T1);

    // Parameterization of the projected area of the hemisphere.
    float r = sqrt(rand.x);
    float phi = (2.f * PI) * rand.y;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1.f + Vh.z);
    t2 = (1.f - s) * sqrt(1.f - t1 * t1) + s * t2;

    // Reproject onto hemisphere.
    vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.f, 1.f - t1 * t1 - t2 * t2)) * Vh;

    // Transform the normal back to the ellipsoid configuration. This is our half vector.
    vec3 h = normalize(vec3(a * Nh.x, a * Nh.y, max(0.f, Nh.z)));

    // Convert out of tangent space
    return TBN[0] * h.x + TBN[1] * h.y + TBN[2] * h.z;
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

vec3 ComputeBRDF(vec3 diffuse, float roughness, float NdotL, vec3 F0, vec3 N, vec3 V, vec3 H)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    vec3 F = FresnelSchlick(VdotH, F0);
    diffuse = ComputeDiffuseBRDF(diffuse);
    vec3 specular = ComputeSpecularBRDF(roughness, F, NdotV, NdotL, NdotH);

    return (vec3(1.0) - F) * diffuse + specular;
}

#endif
