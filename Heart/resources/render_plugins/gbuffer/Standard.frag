#version 460

#include "Common.glsl"

void main() {
    vec4 albedo = GetAlbedo(inMaterialId, inTexCoord, vec4(0.f));
    if (AlphaClip(inMaterialId, albedo.a))
        discard;

    vec3 normal = GetNormal(inTangent, inBitangent, inNormal, inMaterialId, inTexCoord, vec4(0.f));
    float metalness = GetMetalness(inMaterialId, inTexCoord, vec4(0.f));
    float roughness = GetRoughness(inMaterialId, inTexCoord, vec4(0.f));

    WriteOutputs(albedo.rgb, normal, roughness, metalness);
}
