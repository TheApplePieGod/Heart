#include "FrameBuffer.glsl"

struct Material {
    vec4 baseColor;
    vec4 texCoordTransform; // [0-1]: scale, [2-3]: offset
    vec4 hasTextures; // [0]: hasAlbedo, [1]: hasMetallicRoughness, [2]: hasNormal
    vec4 scalars; // [0]: metalness, [1]: roughness
};

layout(location = 0) in vec2 texCoord;
layout(location = 1) in flat int entityId;
layout(location = 2) in float depth;
layout(location = 3) in vec3 worldPos;
layout(location = 4) in vec3 normal;
layout(location = 5) in vec3 tangent;
layout(location = 6) in vec3 bitangent;

layout(binding = 2) readonly buffer MaterialBuffer {
    Material material;
} materialBuffer;
layout(binding = 3) uniform sampler2D albedoTex;
layout(binding = 4) uniform sampler2D metallicRoughnessTex;
layout(binding = 5) uniform sampler2D normalTex;

#define PI 3.1415926

// adapted from https://learnopengl.com/PBR/Lighting
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec4 GetAlbedo()
{
    vec4 color = materialBuffer.material.baseColor;
    if (materialBuffer.material.hasTextures[0] == 1.f) // has albedo
        color *= texture(albedoTex, (texCoord + materialBuffer.material.texCoordTransform.zw) * materialBuffer.material.texCoordTransform.xy);
    return color;
}

float GetMetalness()
{
    float metalness = materialBuffer.material.scalars[0];
    if (materialBuffer.material.hasTextures[1] == 1.f) // has metallicRoughness
        metalness *= texture(metallicRoughnessTex, (texCoord + materialBuffer.material.texCoordTransform.zw) * materialBuffer.material.texCoordTransform.xy).b;
    return metalness;
}

float GetRoughness()
{
    float roughness = materialBuffer.material.scalars[1];
    if (materialBuffer.material.hasTextures[1] == 1.f) // has metallicRoughness
        roughness *= texture(metallicRoughnessTex, (texCoord + materialBuffer.material.texCoordTransform.zw) * materialBuffer.material.texCoordTransform.xy).g;
    return roughness;
}

vec4 GetFinalColor()
{
    vec3 L = normalize(vec3(0.5f, 0.5f, 0.f));
    vec3 V = normalize(frameData.cameraPos.xyz - worldPos);
    vec3 H = normalize(V + L);
    vec3 nt = normalize(tangent);
    vec3 nb = normalize(bitangent);
    vec3 nn = normalize(normal);

    mat3x3 tbn = mat3x3(nb, nt, nn);

    vec3 N = nn;
    if (materialBuffer.material.hasTextures[2] == 1.f) // has normal
        N = tbn * (texture(normalTex, (texCoord + materialBuffer.material.texCoordTransform.zw) * materialBuffer.material.texCoordTransform.xy).xyz * 2.0 - 1.0);
    
    vec4 baseColor = GetAlbedo();
    float roughness = GetRoughness();
    float metalness = GetMetalness();

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor.rgb, metalness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;

    float NdotL = max(dot(N, L), 0.0);        
    vec3 finalContribution = (kD * baseColor.rgb / PI + specular) * 1.f * NdotL;

    vec3 ambient = vec3(0.01) * baseColor.rgb; //* ao;
    vec3 finalColor = ambient + finalContribution;

    finalColor = finalColor / (finalColor + vec3(1.0));
    finalColor = pow(finalColor, vec3(1.0/2.2));

    return vec4(finalColor, baseColor.a);
}