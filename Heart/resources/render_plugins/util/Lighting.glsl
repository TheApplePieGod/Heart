#ifndef UTIL_LIGHTING
#define UTIL_LIGHTING

#include "Constants.glsl"
#include "BRDF.glsl"

struct LightEvalData
{
    vec3 l;
    float nDotL;
    vec3 radiance;
    float dist;
};

vec3 GetLightRadiance(Light light, float attenuation)
{
    return light.color.rgb * light.color.a * attenuation;
}

void GetPointLightEvalData(inout LightEvalData data, Light light, vec3 P, vec3 N)
{
    float dist = length(light.position.xyz - P);
    float attenuation = 1.f / (
        light.constantAttenuation + light.linearAttenuation * dist + light.quadraticAttenuation * dist * dist
    );
    data.dist = dist;
    data.radiance = GetLightRadiance(light, attenuation);
    data.l = normalize(light.position.xyz - P);
    data.nDotL = max(dot(N, data.l), 0.0);
}

void GetDirectionalLightEvalData(inout LightEvalData data, Light light, vec3 N)
{
    data.dist = 10000;
    data.radiance = GetLightRadiance(light, 1.0);
    data.l = normalize(light.direction.xyz);
    data.nDotL = max(dot(N, data.l), 0.0);
}

vec3 EvaluateLightBRDF(inout LightEvalData evalData, vec3 N, vec3 V, vec3 F0, vec3 diffuse, float roughness)
{
    vec3 H = normalize(V + evalData.l);
    vec3 BRDF = ComputeBRDF(diffuse, roughness, evalData.nDotL, F0, N, V, H);
    return BRDF * evalData.radiance * evalData.nDotL;
}

#endif
