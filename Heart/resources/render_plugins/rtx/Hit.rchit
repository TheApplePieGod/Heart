#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "../../VertexLayout.glsl"
#include "Common.glsl"

struct ObjectData {
    uint64_t vertexAddress;
    uint64_t indexAddress;
};

// TODO: need constants to define set and binding
struct MaterialData {
    vec4 baseColor;
    vec4 emissiveFactor;
    vec4 texCoordTransform; // [0-1]: scale, [2-3]: offset
    vec4 hasPBRTextures; // [0]: hasAlbedo, [1]: hasMetallicRoughness
    vec4 hasTextures; // [0]: hasNormal, [1]: hasEmissive, [2]: hasOcclusion
    vec4 scalars; // [0]: metalness, [1]: roughness, [2]: alphaClipThreshold
};


layout(binding = 0, set = 1) readonly buffer ObjectBuffer {
    ObjectData data[];
} objectBuffer;
layout(binding = 1, set = 1) readonly buffer MaterialBuffer {
    MaterialData materials[];
} materialBuffer;

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    Vertex data[];
};
layout(buffer_reference, scalar) readonly buffer IndexBuffer {
    uvec3 data[];
};

layout(binding = 0, set = 2) uniform sampler2D textures[5000];

layout(location = 0) rayPayloadInEXT HitPayload prd;
hitAttributeEXT vec3 attribs;

void main()
{
    ObjectData objData = objectBuffer.data[gl_InstanceID];
    VertexBuffer vertexBuffer = VertexBuffer(objData.vertexAddress);
    IndexBuffer indexBuffer = IndexBuffer(objData.indexAddress);
  
    // Indices of the triangle
    uvec3 ind = indexBuffer.data[gl_PrimitiveID];
  
    // Vertex of the triangle
    Vertex v0 = vertexBuffer.data[ind.x];
    Vertex v1 = vertexBuffer.data[ind.y];
    Vertex v2 = vertexBuffer.data[ind.z];

    //prd.hitValue = vec3(1.0, 0.0, 0.0);

    // Computing hit position data
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    const vec3 pos = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
    const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));
    const vec3 nrm = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
    const vec3 worldNrm = normalize(vec3(nrm * gl_WorldToObjectEXT));
    vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;

    MaterialData material = materialBuffer.materials[gl_InstanceID];

    vec3 L = normalize(vec3(0.5, 0.5, 0.5));
    float intensity = clamp(dot(worldNrm, L), 0.05, 1.0);
    vec3 diffuse = texture(textures[gl_InstanceID * 5], texCoord).rgb;
    //vec3 diffuse = vec3(1.0, 0.0, 0.0);
    
    prd.hitValue = diffuse * intensity * material.baseColor.rgb;
}
