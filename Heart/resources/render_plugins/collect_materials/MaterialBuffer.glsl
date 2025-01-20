#ifndef MATERIAL_BUFFER
#define MATERIAL_BUFFER

#extension GL_EXT_nonuniform_qualifier : require

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

vec4 SampleMaterialTexture(uint textureId, MaterialData matData, vec2 texCoord, vec4 lod)
{
    //return vec4(float(textureId), float(materialId), texCoord.x, texCoord.y);
    #ifdef MATERIAL_EXPLICIT_LOD
    return textureGrad(
    #else
    return texture(
    #endif
        nonuniformEXT(materialTextures[textureId]),
        (texCoord + matData.texCoordTransform.zw)
           * matData.texCoordTransform.xy
    #ifdef MATERIAL_EXPLICIT_LOD
        , lod.xy, lod.zw
    #endif
    );
}

vec4 GetAlbedo(MaterialInfo material, vec2 texCoord, vec4 lod)
{
    vec4 color = material.data.baseColor;
    #ifndef MATERIAL_DISABLE_ALBEDO_TEX
    if (material.albedoIndex != -1)
        color *= SampleMaterialTexture(material.albedoIndex, material.data, texCoord, lod);
    #endif
    return color;
}

vec2 GetMetalnessRoughness(MaterialInfo material, vec2 texCoord, vec4 lod)
{
    float metalness = material.data.scalars[0];
    float roughness = material.data.scalars[1];
    #ifndef MATERIAL_DISABLE_MR_TEX
    if (material.metallicRoughnessIndex != -1)
    {
        vec4 sampled = SampleMaterialTexture(material.metallicRoughnessIndex, material.data, texCoord, lod);
        metalness *= sampled.b;
        roughness *= sampled.g;
    }
    #endif
    return vec2(metalness, roughness);
}
    
vec3 GetEmissive(MaterialInfo material, vec2 texCoord, vec4 lod)
{
    vec3 emissive = material.data.emissiveFactor.xyz;
    #ifndef MATERIAL_DISABLE_EMISSIVE_TEX
    if (material.emissiveIndex != -1)
        emissive *= SampleMaterialTexture(material.emissiveIndex, material.data, texCoord, lod).xyz;
    #endif
    return emissive;
}

float GetOcclusion(MaterialInfo material, vec2 texCoord, vec4 lod)
{
    float occlusion = 1.f;
    #ifndef MATERIAL_DISABLE_OCCLUSION_TEX
    if (material.occlusionIndex != -1)
        occlusion = SampleMaterialTexture(material.occlusionIndex, material.data, texCoord, lod).r;
    #endif
    return occlusion;
}

vec3 GetNormal(vec3 tangent, vec3 bitangent, vec3 normal, MaterialInfo material, vec2 texCoord, vec4 lod)
{
    #ifndef MATERIAL_DISABLE_NORMAL_TEX
    if (material.normalIndex != -1)
    {
        mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normalize(normal));
        return normalize(tbn * (
            SampleMaterialTexture(material.normalIndex, material.data, texCoord, lod).xyz * 2.0 - 1.0
        ));
    }
    #endif
    return normalize(normal);
}

bool AlphaClip(MaterialInfo material, float alpha)
{
    return alpha <= material.data.scalars[2];
}

#endif
