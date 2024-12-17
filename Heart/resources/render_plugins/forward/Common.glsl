#define FRAME_BUFFER_BINDING 0
#define FRAME_BUFFER_SET 0
#include "../frame_data/FrameBuffer.glsl"

#define LIGHT_BUFFER_BINDING 2
#define LIGHT_BUFFER_SET 0
#include "../lighting_data/LightingBuffer.glsl"

#define MATERIAL_BUFFER_BINDING 3
#define MATERIAL_BUFFER_SET 0
#define MATERIAL_TEXTURES_BINDING 0
#define MATERIAL_TEXTURES_SET 1
#include "../collect_materials/MaterialBuffer.glsl"

#define CLUSTER_LIGHT_INDICES_BINDING 4
#define CLUSTER_LIGHT_GRID_BINDING 5
#define CLUSTER_DATA_BINDING 6
#define CLUSTER_BUFFER_SET 0
#include "../clustered_lighting/ClusterBuffer.glsl"

#include "../util/Misc.glsl"
#include "../util/Colormap.glsl"
#include "../util/Lighting.glsl"
#include "../util/BRDF.glsl"

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in flat uint inMaterialId;
layout(location = 5) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

layout(binding = 7) uniform sampler2D brdfTex;
layout(binding = 8) uniform samplerCube prefilterMap;
layout(binding = 9) uniform samplerCube irradianceMap;

void WriteFinalColor()
{
    vec4 albedo = GetAlbedo(inMaterialId, inTexCoord, vec4(0.f));
    if (AlphaClip(inMaterialId, albedo.a))
        discard;

    // Gamma correction
    albedo.rgb = pow(albedo.rgb, vec3(2.2f));

    vec3 emissive = GetEmissive(inMaterialId, inTexCoord, vec4(0.f));
    vec3 normal = GetNormal(inTangent, inBitangent, inNormal, inMaterialId, inTexCoord, vec4(0.f));
    float metalness = GetMetalness(inMaterialId, inTexCoord, vec4(0.f));
    float roughness = GetRoughness(inMaterialId, inTexCoord, vec4(0.f));
    float occlusion = GetOcclusion(inMaterialId, inTexCoord, vec4(0.f));

    vec3 P = inWorldPos;
    vec3 N = normal;
    vec3 V = normalize(frameBuffer.data.cameraPos.xyz - P);

    vec3 F0 = mix(vec3(0.04), albedo.rgb, metalness);
    vec3 diffuse = mix(albedo.rgb * (vec3(1.0) - F0), vec3(0.0), metalness);

    // Direct lighitng

    // Compute cluster indices
    // Flip the depth when appropriate so the close value is always zero
    float depth = gl_FragCoord.z;
    uint clusterZ = GetClusterZIndex(
        frameBuffer.data.reverseDepth ? 1 - depth : depth,
        frameBuffer.data.clipPlanes.x,
        frameBuffer.data.clipPlanes.y,
        GET_CLUSTER_DATA().clusterScale,
        GET_CLUSTER_DATA().clusterBias
    );
    uint clusterIdx = GetClusterIndex(
        gl_FragCoord.xy,
        frameBuffer.data.screenSize.xy,
        GET_CLUSTER_DATA().clusterDims,
        clusterZ
    );

    LightGrid grid = GetLightGrid(clusterIdx);
    vec3 finalContribution = vec3(0.f);
    for (uint i = 0; i < grid.lightCount; i++)
    {
        uint lightIndex = GetGridLightIndex(grid.offset, i) + 1;

        LightEvalData data;
        if (GET_LIGHT(lightIndex).lightType == LIGHT_POINT)
            GetPointLightEvalData(data, GET_LIGHT(lightIndex), P, N);
        else if (GET_LIGHT(lightIndex).lightType == LIGHT_DIRECTIONAL)
            GetDirectionalLightEvalData(data, GET_LIGHT(lightIndex), N);

        if (data.nDotL <= 0)
            continue;

        finalContribution += EvaluateLightBRDF(data, N, V, F0, diffuse, roughness);
    }
    
    // Indirect lighting
    {
        vec3 R = reflect(-V, N);
        vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metalness;

        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * 4.0).rgb;   

        vec3 diffuse = irradiance * albedo.rgb;
        vec2 brdf = texture(brdfTex, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

        float finalOcclusion = occlusion;
        finalContribution += (kD * diffuse + specular) * finalOcclusion;
    }

    finalContribution += emissive * albedo.rgb;

    outColor = vec4(finalContribution, 1.f);
}
