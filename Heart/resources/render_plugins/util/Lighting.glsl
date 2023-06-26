#ifndef UTIL_LIGHTING
#define UTIL_LIGHTING

#include "Constants.glsl"
#include "BRDF.glsl"

vec3 GetLightRadiance(Light light, float attenuation)
{
    return light.color.rgb * light.color.a * attenuation;
}

vec3 EvaluatePointLightBRDF(Light light, vec3 P, vec3 N, vec3 V, vec3 F0, vec3 diffuse, float roughness)
{
    float dist = length(light.position.xyz - P);
    float attenuation = 1.f / (
        light.constantAttenuation + light.linearAttenuation * dist + light.quadraticAttenuation * dist * dist
    );

    vec3 L = normalize(light.position.xyz - P);
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL == 0)
        return vec3(0.0);

    vec3 H = normalize(V + L);
    vec3 BRDF = ComputeBRDF(diffuse, roughness, F0, N, V, H, L);
    return BRDF * GetLightRadiance(light, attenuation) * NdotL;
}

vec3 EvaluateDirectionalLightBRDF(Light light, vec3 P, vec3 N, vec3 V, vec3 F0, vec3 diffuse, float roughness)
{
    vec3 L = normalize(light.direction.xyz);
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL == 0)
        return vec3(0.0);

    vec3 H = normalize(V + L);
    vec3 BRDF = ComputeBRDF(diffuse, roughness, F0, N, V, H, L);
    return BRDF * GetLightRadiance(light, 1.0) * NdotL;
}

#endif
