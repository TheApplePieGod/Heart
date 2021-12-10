#include "FrameBuffer.glsl"

struct Material {
    vec4 baseColor;
    vec4 emissiveFactor;
    vec4 texCoordTransform; // [0-1]: scale, [2-3]: offset
    vec4 hasPBRTextures; // [0]: hasAlbedo, [1]: hasMetallicRoughness
    vec4 hasTextures; // [0]: hasNormal, [1]: hasEmissive, [2]: hasOcclusion
    vec4 scalars; // [0]: metalness, [1]: roughness, [2]: alphaClipThreshold
};

struct Light {
    vec4 position;
    vec4 direction;
    vec4 color;
    int lightType;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};

layout(location = 0) in vec2 texCoord;
layout(location = 1) in flat int entityId;
layout(location = 2) in float depth;
layout(location = 3) in vec3 worldPos;
layout(location = 4) in vec3 normal;
layout(location = 5) in vec3 tangent;
layout(location = 6) in vec3 bitangent;
layout(location = 7) in flat int instance;

layout(binding = 2) readonly buffer MaterialBuffer {
    Material materials[];
} materialBuffer;
layout(binding = 3) readonly buffer LightingBuffer {
    Light lights[];
} lightingBuffer;

layout(binding = 4) uniform sampler2D albedoTex;
layout(binding = 5) uniform sampler2D metallicRoughnessTex;
layout(binding = 6) uniform sampler2D normalTex;
layout(binding = 7) uniform sampler2D emissiveTex;
layout(binding = 8) uniform sampler2D occlusionTex;
layout(binding = 9) uniform samplerCube irradianceMap;
layout(binding = 10) uniform samplerCube prefilterMap;
layout(binding = 11) uniform sampler2D brdfLUT;

#define PI 3.1415926
#define DIRECTIONAL 1
#define POINT 2

// adapted from https://learnopengl.com/PBR/Lighting
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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
    vec4 color = materialBuffer.materials[instance].baseColor;
    if (materialBuffer.materials[instance].hasPBRTextures[0] == 1.f) // has albedo
        color *= texture(albedoTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy);
    return color;
}

float GetMetalness()
{
    float metalness = materialBuffer.materials[instance].scalars[0];
    if (materialBuffer.materials[instance].hasPBRTextures[1] == 1.f) // has metallicRoughness
        metalness *= texture(metallicRoughnessTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy).b;
    return metalness;
}

float GetRoughness()
{
    float roughness = materialBuffer.materials[instance].scalars[1];
    if (materialBuffer.materials[instance].hasPBRTextures[1] == 1.f) // has metallicRoughness
        roughness *= texture(metallicRoughnessTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy).g;
    return roughness;
}

vec3 GetEmissive()
{
    vec3 emissive = materialBuffer.materials[instance].emissiveFactor.xyz;
    if (materialBuffer.materials[instance].hasTextures[1] == 1.f) // has emissive
        emissive *= texture(emissiveTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy).rgb;
    return emissive;
}

float GetOcclusion()
{
    float occlusion = 1.f;
    if (materialBuffer.materials[instance].hasTextures[2] == 1.f) // has occlusion
        occlusion = texture(occlusionTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy).r;
    return occlusion;
}

void Clip(float alpha)
{
    if (alpha <= materialBuffer.materials[instance].scalars[2]) // alpha clip threshold
        discard;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), vec3(0.f), vec3(1.f));
}

vec3 CalcLightContribution(vec3 albedo, vec3 F0, vec3 lightColor, float intensity, float attenuation, vec3 L, vec3 V, vec3 N, float metalness, float roughness)
{
    vec3 H = normalize(V + L);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3 radiance = lightColor * attenuation * intensity;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;

    float NdotL = max(dot(N, L), 0.0); 
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec4 GetFinalColor()
{
    vec4 baseColor = GetAlbedo();
    baseColor.rgb = pow(baseColor.rgb, vec3(2.2)); // compensate for gamma correction
    Clip(baseColor.a);

    float roughness = GetRoughness();
    float metalness = GetMetalness();
    vec3 emissive = GetEmissive();
    float occlusion = GetOcclusion();

    vec3 V = normalize(frameBuffer.data.cameraPos.xyz - worldPos);
    vec3 nt = normalize(tangent);
    vec3 nb = normalize(bitangent);
    vec3 nn = normalize(normal);

    mat3x3 tbn = mat3x3(nb, nt, nn);

    vec3 N = nn;
    if (materialBuffer.materials[instance].hasTextures[0] == 1.f) // has normal
        N = normalize(tbn * (texture(normalTex, (texCoord + materialBuffer.materials[instance].texCoordTransform.zw) * materialBuffer.materials[instance].texCoordTransform.xy).xyz * 2.0 - 1.0));

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 R = reflect(-V, N); 

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor.rgb, metalness);

    vec3 finalContribution = vec3(0.f);

    // contribution from all lights
    int lightCount = int(lightingBuffer.lights[0].position.x);
    for (int i = 1; i <= lightCount; i++)
    {
        float attenuation = 1.f;
        if (lightingBuffer.lights[i].lightType == POINT)
        {
            float dist = length(lightingBuffer.lights[i].position.xyz - worldPos);
            attenuation = 1.f / (lightingBuffer.lights[i].constantAttenuation + lightingBuffer.lights[i].linearAttenuation * dist + lightingBuffer.lights[i].quadraticAttenuation * dist * dist);
        }

        vec3 L = normalize(lightingBuffer.lights[i].position.xyz - worldPos);
        if (lightingBuffer.lights[i].lightType == DIRECTIONAL)
            L = lightingBuffer.lights[i].direction.xyz;
            
        vec3 H = normalize(V + L);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 lightColor = lightingBuffer.lights[i].color.rgb;
        float intensity = lightingBuffer.lights[i].color.a;
        vec3 radiance = lightColor * attenuation * intensity;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = min(vec3(1.f), numerator / denominator); // limit specular intensity for bloom

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metalness;

        float NdotL = max(dot(N, L), 0.0); 
        finalContribution += (kD * baseColor.rgb / PI + specular) * radiance * NdotL;
    }
    
    // ambient lighting
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metalness;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * baseColor.rgb;
    
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;   
    vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = min(vec3(1.f), prefilteredColor * (F * envBRDF.x + envBRDF.y)); // limit specular intensity for bloom
    vec3 ambient = (kD * diffuse + specular) * occlusion; // specular

    vec3 finalColor = ambient + finalContribution;

    // tonemapping
    //finalColor = ACESFilm(finalColor);
    // finalColor = finalColor / (finalColor + vec3(1.0));

    // gamma correction
    //finalColor = pow(finalColor, vec3(0.4545));

    return vec4(finalColor + emissive, baseColor.a);
}