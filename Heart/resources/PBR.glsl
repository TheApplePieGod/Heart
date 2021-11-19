struct Material {
    vec4 baseColor;
    float roughness;
    float metalness;
    vec2 texCoordScale;
    vec2 texCoordOffset;
    bool hasAlbedo;
    bool hasRoughness;
    bool hasMetalness;
    bool hasNormal;
    vec2 padding;
};

layout(location = 0) in vec2 texCoord;
layout(location = 1) in flat int entityId;
layout(location = 2) in float depth;

layout(binding = 2) readonly buffer MaterialBuffer {
    Material material;
} materialBuffer;
layout(binding = 3) uniform sampler2D albedo;

vec4 GetAlbedo()
{
    vec4 color = materialBuffer.material.baseColor;
    if (materialBuffer.material.hasAlbedo)
        color *= texture(albedo, (texCoord + materialBuffer.material.texCoordOffset) * materialBuffer.material.texCoordScale);
    return color;
}