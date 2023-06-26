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

layout(location = 0) out vec4 outGBuffer1; // RGB: Albedo, A: Metallic
layout(location = 1) out vec4 outGBuffer2; // RGB: WSNormal, A: Roughness
layout(location = 2) out float outEntityId;

void main() {
    outEntityId = float(inEntityId);

    vec4 albedo = GetAlbedo(inMaterialId, inTexCoord);
    if (AlphaClip(inMaterialId, albedo.a))
        discard;

    // Compensate for gamma correction
    // TODO: fix? this is probably because textures are unorm
    albedo.rgb = pow(albedo.rgb, vec3(2.2)); 

    outGBuffer2.rgb = GetNormal(inTangent, inBitangent, inNormal, inMaterialId, inTexCoord);

    outGBuffer1.a = GetMetalness(inMaterialId, inTexCoord);
    outGBuffer2.a = GetRoughness(inMaterialId, inTexCoord);

    // TODO: revisit this. Potentially might just have to store materialId
    // in gbuffer and sample these later. Mid because sampling here allows
    // for proper miplevels
    /*
    vec3 emissive = GetEmissive(inMaterialId, inTexCoord);
    float occlusion = GetOcclusion(inMaterialId, inTexCoord);
    */
}
