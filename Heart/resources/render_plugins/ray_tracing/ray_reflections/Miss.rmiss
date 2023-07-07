#version 460
#extension GL_EXT_ray_tracing : require

#include "Common.glsl"

layout(location = 0) rayPayloadInEXT HitPayload prd;

layout(binding = 4, set = 1) uniform samplerCube environmentMap;

void main()
{
    prd.hitValue = textureLod(environmentMap, gl_WorldRayDirectionEXT, 0.0);
}
