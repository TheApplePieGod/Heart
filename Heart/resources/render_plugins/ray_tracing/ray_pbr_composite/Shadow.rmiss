#version 460
#extension GL_EXT_ray_tracing : require

#include "Common.glsl"

layout(location = 0) rayPayloadInEXT HitPayload prd;

void main()
{
    prd.isShadowed = false;
}
