#version 460

#define MATERIAL_BUFFER_BINDING 2
#define MATERIAL_BUFFER_SET 0
#define MATERIAL_TEXTURES_BINDING 0
#define MATERIAL_TEXTURES_SET 1
#include "../collect_materials/MaterialBuffer.glsl"

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in flat uint inMaterialId;
layout(location = 5) in flat uint inEntityId;
layout(location = 6) in vec4 inClipPos;
layout(location = 7) in vec4 inPrevClipPos;

layout(location = 0) out vec4 outGBuffer1; // RGB: Albedo, A: Metallic
layout(location = 1) out vec4 outGBuffer2; // RGB: WSNormal, A: Roughness
layout(location = 2) out vec4 outGBuffer3; // RG: Motion Vector
layout(location = 3) out float outEntityId;

vec2 ComputeMotionVector(vec4 prevPos, vec4 newPos)
{
    // Normalize
    vec2 new = (newPos.xy / newPos.w);
    vec2 prev = (prevPos.xy / prevPos.w);

    // Remap to [0, 1]
    new = new * 0.5 + 0.5;
    prev = prev * 0.5 + 0.5;

    return prev - new;
}

void main() {
    outEntityId = float(inEntityId);

    vec4 albedo = GetAlbedo(inMaterialId, inTexCoord);
    if (AlphaClip(inMaterialId, albedo.a))
        discard;

    // Compensate for gamma correction
    // TODO: fix? this is probably because textures are unorm
    outGBuffer1.rgb = pow(albedo.rgb, vec3(2.2)); 

    outGBuffer2.rgb = GetNormal(inTangent, inBitangent, inNormal, inMaterialId, inTexCoord);

    outGBuffer1.a = 1.0;//GetMetalness(inMaterialId, inTexCoord);
    outGBuffer2.a = GetRoughness(inMaterialId, inTexCoord);

    float linearZ = gl_FragCoord.z / gl_FragCoord.w;
    outGBuffer3.rg = ComputeMotionVector(inPrevClipPos, inClipPos);
    outGBuffer3.b = linearZ;
    outGBuffer3.a = 1.0;

    // TODO: revisit this. Potentially might just have to store materialId
    // in gbuffer and sample these later. Mid because sampling here allows
    // for proper miplevels
    /*
    vec3 emissive = GetEmissive(inMaterialId, inTexCoord);
    float occlusion = GetOcclusion(inMaterialId, inTexCoord);
    */
}
