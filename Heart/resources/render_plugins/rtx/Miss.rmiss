#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;

layout(binding = 2, set = 1) uniform samplerCube environmentMap;

void main()
{
    hitValue = textureLod(environmentMap, gl_WorldRayDirectionEXT, 0.0).rgb;
}
