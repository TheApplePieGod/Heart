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

#define MAX_MATERIAL_TEXTURES 5000
layout(
    binding = MATERIAL_TEXTURES_BINDING,
    set = MATERIAL_TEXTURES_SET
) uniform sampler2D materialTextures[MAX_MATERIAL_TEXTURES];

#define GET_MATERIAL(materialId) materialBuffer.data[materialId]

vec4 SampleMaterialTexture(uint textureId, uint materialId, vec2 texCoord)
{
    return texture(
        materialTextures[textureId],
        (texCoord + materialBuffer.data[materialId].data.texCoordTransform.zw)
           * materialBuffer.data[materialId].data.texCoordTransform.xy
    );
}

vec4 GetAlbedo(uint materialId, vec2 texCoord)
{
    vec4 color = GET_MATERIAL(materialId).data.baseColor;
    if (GET_MATERIAL(materialId).albedoIndex != -1)
        color *= SampleMaterialTexture(GET_MATERIAL(materialId).albedoIndex, materialId, texCoord);
    return color;
}

float GetMetalness(uint materialId, vec2 texCoord)
{
    float metalness = GET_MATERIAL(materialId).data.scalars[0];
    if (GET_MATERIAL(materialId).metallicRoughnessIndex != -1)
        metalness *= SampleMaterialTexture(GET_MATERIAL(materialId).metallicRoughnessIndex, materialId, texCoord).b;
    return metalness;
}
    
float GetRoughness(uint materialId, vec2 texCoord)
{
    float roughness = GET_MATERIAL(materialId).data.scalars[1];
    if (GET_MATERIAL(materialId).metallicRoughnessIndex != -1)
        roughness *= SampleMaterialTexture(GET_MATERIAL(materialId).metallicRoughnessIndex, materialId, texCoord).g;
    return roughness;
}

vec3 GetEmissive(uint materialId, vec2 texCoord)
{
    vec3 emissive = GET_MATERIAL(materialId).data.emissiveFactor.xyz;
    if (GET_MATERIAL(materialId).emissiveIndex != -1)
        emissive *= SampleMaterialTexture(GET_MATERIAL(materialId).emissiveIndex, materialId, texCoord).xyz;
    return emissive;
}

float GetOcclusion(uint materialId, vec2 texCoord)
{
    float occlusion = 1.f;
    if (GET_MATERIAL(materialId).occlusionIndex != -1)
        occlusion = SampleMaterialTexture(GET_MATERIAL(materialId).occlusionIndex, materialId, texCoord).r;
    return occlusion;
}

vec3 GetNormal(vec3 wsNormal, vec3 wsTangent, vec3 wsBitangent, uint materialId, vec2 texCoord)
{
    if (GET_MATERIAL(materialId).normalIndex != -1)
    {
        mat3 tbn = mat3(normalize(wsNormal), normalize(wsBitangent), normalize(wsTangent));
        return normalize(tbn * (
            SampleMaterialTexture(GET_MATERIAL(materialId).normalIndex, materialId, texCoord).xyz * 2.0 - 1.0
        ));
    }
    return wsNormal;
}

bool AlphaClip(uint materialId, float alpha)
{
    return alpha <= GET_MATERIAL(materialId).data.scalars[2];
}

#endif
