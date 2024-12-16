#define MATERIAL_BUFFER_BINDING 2
#define MATERIAL_BUFFER_SET 0
#define MATERIAL_TEXTURES_BINDING 0
#define MATERIAL_TEXTURES_SET 1
#include "../collect_materials/MaterialBuffer.glsl"

#include "./Util.glsl"

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in flat uint inMaterialId;
layout(location = 5) in flat uint inEntityId;
layout(location = 6) in vec4 inClipPos;
layout(location = 7) in vec4 inPrevClipPos;

layout(location = 0) out vec4 outNormalData; // RG: NormalWS, BA: Motion vector
layout(location = 1) out uvec4 outColorData; // RGBA: Packed color data
layout(location = 2) out vec4 outEmissiveData; // RGB: emissive, A: occlusion
layout(location = 3) out float outEntityId;

layout(push_constant) uniform PushConstants
{
    uint storeMotionVectors;
    uint storeColorAndEmissive;
    uint storeEntityIds;
} constants;

#define MIN_ROUGHNESS 0.001
#define MAX_ROUGHNESS 0.75

vec2 ComputeMotionVector(vec4 prevPos, vec4 newPos)
{
    // Normalize
    vec2 new2d = (newPos.xy / newPos.w);
    vec2 prev2d = (prevPos.xy / prevPos.w);

    // Remap to [0, 1]
    new2d = new2d * 0.5 + 0.5;
    prev2d = prev2d * 0.5 + 0.5;

    return prev2d - new2d;
}

void WriteOutputs(vec3 normal, vec3 albedo, vec3 emissive, float roughness, float metalness, float occlusion)
{
    if (constants.storeEntityIds == 1)
        outEntityId = float(inEntityId);

    vec4 normalOutput = vec4(0.f);
    normalOutput.rg = OctahedronEncode(normal);
    if (constants.storeMotionVectors == 1)
        normalOutput.ba = ComputeMotionVector(inPrevClipPos, inClipPos);
    outNormalData = normalOutput;

    if (constants.storeColorAndEmissive == 1)
    {
        outColorData = PackColorData(albedo, metalness, clamp(roughness, MIN_ROUGHNESS, MAX_ROUGHNESS));
        outEmissiveData = vec4(emissive, occlusion);
    }
}
