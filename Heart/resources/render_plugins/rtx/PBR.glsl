layout(binding = 3, set = 1) uniform samplerCube irradianceMap;
layout(binding = 4, set = 1) uniform samplerCube prefilterMap;
layout(binding = 5, set = 1) uniform sampler2D brdfLUT;

#define PI 3.1415926

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

vec4 SampleTexture(uint materialId, uint offset, vec2 texCoord)
{
    return texture(
        textures[materialId * 5 + offset],
        (texCoord + materialBuffer.materials[materialId].texCoordTransform.zw)
           * materialBuffer.materials[materialId].texCoordTransform.xy
    );
}

vec4 GetAlbedo(uint materialId, vec2 texCoord)
{
    vec4 color = materialBuffer.materials[materialId].baseColor;
    if (materialBuffer.materials[materialId].hasPBRTextures[0] == 1.f) // has albedo
        color *= SampleTexture(materialId, 0, texCoord);
    return color;
}

float GetMetalness(uint materialId, vec2 texCoord)
{
    float metalness = materialBuffer.materials[materialId].scalars[0];
    if (materialBuffer.materials[materialId].hasPBRTextures[1] == 1.f) // has metallicRoughness
        metalness *= SampleTexture(materialId, 1, texCoord).r;
    return metalness;
}

float GetRoughness(uint materialId, vec2 texCoord)
{
    float roughness = materialBuffer.materials[materialId].scalars[1];
    if (materialBuffer.materials[materialId].hasPBRTextures[1] == 1.f) // has metallicRoughness
        roughness *= SampleTexture(materialId, 1, texCoord).g;
    return roughness;
}

vec3 GetEmissive(uint materialId, vec2 texCoord)
{
    vec3 emissive = materialBuffer.materials[materialId].emissiveFactor.xyz;
    if (materialBuffer.materials[materialId].hasTextures[1] == 1.f) // has emissive
        emissive *= SampleTexture(materialId, 3, texCoord).rgb;
    return emissive;
}

float GetOcclusion(uint materialId, vec2 texCoord)
{
    float occlusion = 1.f;
    if (materialBuffer.materials[materialId].hasTextures[2] == 1.f) // has occlusion
        occlusion *= SampleTexture(materialId, 4, texCoord).r;
    return occlusion;
}

bool Clip(float alpha, uint materialId)
{
    return alpha <= materialBuffer.materials[materialId].scalars[2]; // alpha clip threshold
}

vec4 GetFinalColor(uint materialId, vec2 texCoord, vec3 pos, vec3 normal, vec3 tangent, vec3 bitangent)
{
    vec4 baseColor = GetAlbedo(materialId, texCoord);
    baseColor.rgb = pow(baseColor.rgb, vec3(2.2)); // compensate for gamma correction

    if (Clip(baseColor.a, materialId))
        return vec4(0.f);

    float roughness = GetRoughness(materialId, texCoord);
    float metalness = GetMetalness(materialId, texCoord);
    vec3 emissive = GetEmissive(materialId, texCoord);
    float occlusion = GetOcclusion(materialId, texCoord);

    vec3 V = normalize(gl_WorldRayOriginEXT - pos);

    mat3x3 tbn = mat3x3(tangent, bitangent, normal);
    vec3 N = normal;
    if (materialBuffer.materials[materialId].hasTextures[0] == 1.f) // has normal
        N = normalize(tbn * (SampleTexture(materialId, 2, texCoord).rgb * 2.0 - 1.0));

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 R = reflect(-V, N); 
    float nDotV = max(dot(N, V), 0.0);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor.rgb, metalness);

    float filteredRoughness = roughness;

    // contribution from all lights
    vec3 finalContribution = vec3(0.0);
    int lightCount = int(lightingBuffer.lights[0].position.x);
    for (int i = 1; i <= lightCount; i++)
    {
        float tMin = 0.001;
        float tMax;
        float attenuation = 1.f;
        if (lightingBuffer.lights[i].lightType == POINT)
        {
            float dist = length(lightingBuffer.lights[i].position.xyz - pos);
            attenuation = 1.f / (lightingBuffer.lights[i].constantAttenuation + lightingBuffer.lights[i].linearAttenuation * dist + lightingBuffer.lights[i].quadraticAttenuation * dist * dist);
            tMax = dist;
        }

        vec3 L = normalize(lightingBuffer.lights[i].position.xyz - pos);
        if (lightingBuffer.lights[i].lightType == DIRECTIONAL)
        {
            L = lightingBuffer.lights[i].direction.xyz;
            tMax = frameBuffer.data.clipPlanes.y;
        }

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL == 0)
            continue;

        // Trace shadow ray
        uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
        isShadowed = true;
        traceRayEXT(
            tlas,        // acceleration structure
            flags,       // rayFlags
            0xFF,        // cullMask
            0,           // sbtRecordOffset
            0,           // sbtRecordStride
            1,           // missIndex
            pos,         // ray origin
            tMin,        // ray min range
            L,           // ray direction
            tMax,        // ray max range
            1            // payload (location = 1)
        );

        if (isShadowed)
            continue;

        vec3 H = normalize(V + L);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 lightColor = lightingBuffer.lights[i].color.rgb;
        float intensity = lightingBuffer.lights[i].color.a;
        vec3 radiance = lightColor * attenuation * intensity;

        float NDF = DistributionGGX(N, H, filteredRoughness);
        float G = GeometrySmith(N, V, L, filteredRoughness);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = min(vec3(1.f), numerator / denominator); // limit specular intensity for bloom

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metalness;

        finalContribution += (kD * baseColor.rgb / PI + specular) * radiance * NdotL;
    }
    
    // ambient lighting
    vec3 F = FresnelSchlickRoughness(nDotV, F0, filteredRoughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metalness;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * baseColor.rgb;
    
    vec3 prefilteredColor = textureLod(prefilterMap, R, filteredRoughness * MAX_REFLECTION_LOD).rgb;   
    vec2 envBRDF = texture(brdfLUT, vec2(nDotV, filteredRoughness)).rg;
    vec3 specular = min(vec3(1.f), prefilteredColor * (F * envBRDF.x + envBRDF.y)); // limit specular intensity for bloom
    vec3 ambient = (kD * diffuse + specular) * occlusion;

    vec3 finalColor = ambient + finalContribution;

    return vec4(finalColor + emissive, baseColor.a);
}

