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
#include "../../collect_materials/MaterialBuffer.glsl"

#include "Common.glsl"
#include "../../../VertexLayout.glsl"
#include "../../util/Lighting.glsl"
#include "../util/Barycentrics.glsl"

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    Vertex data[];
};

layout(buffer_reference, scalar) readonly buffer IndexBuffer {
    uvec3 data[];
};

layout(location = 0) rayPayloadInEXT HitPayload payload;
//layout(location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec3 hitAttributes;

void main()
{
    uint objectId = gl_InstanceID;
    /*
    VertexBuffer vertexBuffer = VertexBuffer(GET_OBJECT(0).vertexAddress);
    IndexBuffer indexBuffer = IndexBuffer(GET_OBJECT(0).indexAddress);
    //uint materialId = GET_OBJECT(objectId).materialId;
  
    // Hit triangle
    uvec3 ind = indexBuffer.data[0];
    Vertex v0 = vertexBuffer.data[ind.x];
    Vertex v1 = vertexBuffer.data[ind.y];
    Vertex v2 = vertexBuffer.data[ind.z];
    */

    //payload.hitValue = v0.position;
    payload.hitValue = vec3(float(GET_OBJECT(objectId).vertexAddress));

    /*
    vec3 bary = GetBarycentricCoordinates(hitAttributes);
    vec2 texCoord = GetTexCoord(v0, v1, v2, bary);
    vec3 P = GetWorldPosition(v0, v1, v2, bary);
    vec4 T = GetWorldTangent(v0, v1, v2, bary);
    vec3 N = GetWorldNormal(v0, v1, v2, bary);
    vec3 B = cross(T.xyz, N) * T.w;
    vec3 V = normalize(P - frameBuffer.data.cameraPos.xyz);

    vec4 albedo = GetAlbedo(materialId, texCoord);
    albedo.rgb = pow(albedo.rgb, vec3(2.2)); 
    N = GetNormal(T.xyz, B, N, materialId, texCoord);
    float metalness = GetMetalness(materialId, texCoord);
    float roughness = GetRoughness(materialId, texCoord);
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metalness);

    vec3 finalContribution = vec3(0.0);
    int lightCount = int(GET_LIGHT(0).position.x);
    for (int i = 1; i <= lightCount; i++)
    {
        if (GET_LIGHT(i).lightType == LIGHT_POINT)
            finalContribution += EvaluatePointLightBRDF(GET_LIGHT(i), P, N, V, F0, albedo.rgb, roughness);
        else if (GET_LIGHT(i).lightType == LIGHT_DIRECTIONAL)
            finalContribution += EvaluateDirectionalLightBRDF(GET_LIGHT(i), P, N, V, F0, albedo.rgb, roughness);
    }

    vec3 ambient = vec3(0.01);
    vec3 finalColor = finalContribution + ambient;

    payload.hitValue = finalColor.rgb;
    */
}
