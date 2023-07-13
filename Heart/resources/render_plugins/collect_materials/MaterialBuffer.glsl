#ifndef MATERIAL_BUFFER
#define MATERIAL_BUFFER

struct MaterialData {
    vec4 baseColor;
    vec4 emissiveFactor;
    vec4 texCoordTransform; // [0-1]: scale, [2-3]: offset
    vec4 hasPBRTextures; // [0]: hasAlbedo, [1]: hasMetallicRoughness
    vec4 hasTextures; // [0]: hasNormal, [1]: hasEmissive, [2]: hasOcclusion
    vec4 scalars; // [0]: metalness, [1]: roughness, [2]: alphaClipThreshold
};

struct MaterialInfo {
    MaterialData data;
    int albedoIndex;
    int metallicRoughnessIndex;
    int normalIndex;
    int emissiveIndex;
    int occlusionIndex;
};

layout(
    std430,
    binding = MATERIAL_BUFFER_BINDING,
    set = MATERIAL_BUFFER_SET
) readonly buffer MaterialBuffer {
    MaterialInfo data[];
} materialBuffer;

#define MAX_MATERIAL_TEXTURES 1000
layout(
    binding = MATERIAL_TEXTURES_BINDING,
    set = MATERIAL_TEXTURES_SET
) uniform sampler2D materialTextures[MAX_MATERIAL_TEXTURES];

#define GET_MATERIAL(materialId) materialBuffer.data[materialId]

vec4 SampleMaterialTexture(uint textureId, uint materialId, vec2 texCoord, vec4 lod)
{
    #ifdef MATERIAL_EXPLICIT_LOD
    return textureGrad(
    #else
    return texture(
    #endif
        materialTextures[textureId],
        (texCoord + materialBuffer.data[materialId].data.texCoordTransform.zw)
           * materialBuffer.data[materialId].data.texCoordTransform.xy
    #ifdef MATERIAL_EXPLICIT_LOD
        , lod.xy, lod.zw
    #endif
    );
}

vec4 GetAlbedo(uint materialId, vec2 texCoord, vec4 lod)
{
    vec4 color = GET_MATERIAL(materialId).data.baseColor;
    if (GET_MATERIAL(materialId).albedoIndex != -1)
        color *= SampleMaterialTexture(GET_MATERIAL(materialId).albedoIndex, materialId, texCoord, lod);
    return color;
}

float GetMetalness(uint materialId, vec2 texCoord, vec4 lod)
{
    float metalness = GET_MATERIAL(materialId).data.scalars[0];
    if (GET_MATERIAL(materialId).metallicRoughnessIndex != -1)
        metalness *= SampleMaterialTexture(GET_MATERIAL(materialId).metallicRoughnessIndex, materialId, texCoord, lod).b;
    return metalness;
}
    
float GetRoughness(uint materialId, vec2 texCoord, vec4 lod)
{
    float roughness = GET_MATERIAL(materialId).data.scalars[1];
    if (GET_MATERIAL(materialId).metallicRoughnessIndex != -1)
        roughness *= SampleMaterialTexture(GET_MATERIAL(materialId).metallicRoughnessIndex, materialId, texCoord, lod).g;
    return roughness;
}

vec3 GetEmissive(uint materialId, vec2 texCoord, vec4 lod)
{
    vec3 emissive = GET_MATERIAL(materialId).data.emissiveFactor.xyz;
    if (GET_MATERIAL(materialId).emissiveIndex != -1)
        emissive *= SampleMaterialTexture(GET_MATERIAL(materialId).emissiveIndex, materialId, texCoord, lod).xyz;
    return emissive;
}

float GetOcclusion(uint materialId, vec2 texCoord, vec4 lod)
{
    float occlusion = 1.f;
    if (GET_MATERIAL(materialId).occlusionIndex != -1)
        occlusion = SampleMaterialTexture(GET_MATERIAL(materialId).occlusionIndex, materialId, texCoord, lod).r;
    return occlusion;
}

vec3 GetNormal(vec3 tangent, vec3 bitangent, vec3 normal, uint materialId, vec2 texCoord, vec4 lod)
{
    if (GET_MATERIAL(materialId).normalIndex != -1)
    {
        mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));
        return normalize(tbn * (
            SampleMaterialTexture(GET_MATERIAL(materialId).normalIndex, materialId, texCoord, lod).xyz * 2.0 - 1.0
        ));
    }
    return normalize(normal);
}

bool AlphaClip(uint materialId, float alpha)
{
    return alpha <= GET_MATERIAL(materialId).data.scalars[2];
}

#endif
