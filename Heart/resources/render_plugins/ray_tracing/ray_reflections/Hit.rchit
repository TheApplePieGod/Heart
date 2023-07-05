#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_buffer_reference2 : require

#define FRAME_BUFFER_BINDING 0
#define FRAME_BUFFER_SET 0
#include "../../frame_data/FrameBuffer.glsl"

#define LIGHT_BUFFER_BINDING 0
#define LIGHT_BUFFER_SET 1
#include "../../lighting_data/LightingBuffer.glsl"

#define OBJECT_BUFFER_BINDING 1
#define OBJECT_BUFFER_SET 1
#include "../tlas/ObjectBuffer.glsl"

#define MATERIAL_BUFFER_BINDING 2
#define MATERIAL_BUFFER_SET 1
#define MATERIAL_TEXTURES_BINDING 0
#define MATERIAL_TEXTURES_SET 2
#define MATERIAL_EXPLICIT_LOD
#include "../../collect_materials/MaterialBuffer.glsl"

#include "Common.glsl"
#include "../../../VertexLayout.glsl"
#include "../../util/Lighting.glsl"
#include "../util/Barycentrics.glsl"
#include "../util/RayCone.glsl"

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    Vertex data[];
};

layout(buffer_reference, scalar) readonly buffer IndexBuffer {
    uvec3 data[];
};

layout(binding = 1) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT HitPayload payload;
layout(location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec3 hitAttributes;

bool CheckShadowed(inout LightEvalData lightData, vec3 P, vec3 N)
{
    float tMin = 0.01;
    uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    vec3 shadowOrigin = P + N * 0.01;
    isShadowed = true;
    traceRayEXT(tlas, flags, 0xFF, 0, 0, 1, shadowOrigin, tMin, lightData.l, lightData.dist, 1);
    return isShadowed;
}

void main()
{
    uint objectId = gl_InstanceID;
    VertexBuffer vertexBuffer = VertexBuffer(GET_OBJECT(objectId).vertexAddress);
    IndexBuffer indexBuffer = IndexBuffer(GET_OBJECT(objectId).indexAddress);
    uint materialId = uint(GET_OBJECT(objectId).data.r);
  
    // Hit triangle
    uvec3 ind = indexBuffer.data[gl_PrimitiveID];
    Vertex v0 = vertexBuffer.data[ind.x];
    Vertex v1 = vertexBuffer.data[ind.y];
    Vertex v2 = vertexBuffer.data[ind.z];

    vec3 bary = GetBarycentricCoordinates(hitAttributes);
    vec2 texCoord = GetTexCoord(v0, v1, v2, bary);
    vec3 P = GetWorldPosition(v0, v1, v2, bary);
    vec4 T = GetWorldTangent(v0, v1, v2, bary);
    vec3 N = GetWorldNormal(v0, v1, v2, bary);
    vec3 B = cross(T.xyz, N) * T.w;
    vec3 V = normalize(gl_WorldRayOriginEXT - P);

    // Update raycone width with hit. NOTE: subsequent rays would require an update
    // to the spread angle (rayCone.y)
    payload.rayCone.x += payload.rayCone.y * length(P - gl_WorldRayOriginEXT);
    vec4 mip = UVDerivsFromRayCone(
        gl_WorldRayDirectionEXT,
        N,
        payload.rayCone.x,
        vec2[3](v0.texCoord, v1.texCoord, v2.texCoord),
        vec3[3](v0.position, v1.position, v2.position),
        mat3x3(gl_ObjectToWorldEXT)
    );

    vec4 albedo = GetAlbedo(materialId, texCoord, mip);
    albedo.rgb = pow(albedo.rgb, vec3(2.2)); 
    N = GetNormal(T.xyz, B, N, materialId, texCoord, mip);
    float metalness = GetMetalness(materialId, texCoord, mip);
    float roughness = GetRoughness(materialId, texCoord, mip);
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metalness);
    vec3 diffuse = mix(albedo.rgb * (vec3(1.0) - F0), vec3(0.0), metalness);

    vec3 finalContribution = vec3(0.0);
    int lightCount = int(GET_LIGHT(0).position.x);
    for (int i = 1; i <= lightCount; i++)
    {
        LightEvalData data;
        if (GET_LIGHT(i).lightType == LIGHT_POINT)
            GetPointLightEvalData(data, GET_LIGHT(i), P, N);
        else if (GET_LIGHT(i).lightType == LIGHT_DIRECTIONAL)
            GetDirectionalLightEvalData(data, GET_LIGHT(i), N);

        if (data.nDotL <= 0)
            continue;

        if (CheckShadowed(data, P, N))
            continue;

        finalContribution += EvaluateLightBRDF(data, N, V, F0, diffuse, roughness);
    }

    // Indirect
    // TODO: generalize
    {
        vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metalness;

        vec3 irradiance = vec3(0.015);
        vec3 diffuse = irradiance * albedo.rgb;
        
        finalContribution += (kD * diffuse);
    }

    payload.hitValue.rgb = finalContribution.rgb;
    payload.hitValue.a = gl_RayTminEXT + gl_HitTEXT;
}
