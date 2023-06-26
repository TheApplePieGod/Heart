#ifndef UTIL_MISC
#define UTIL_MISC

vec3 ComputeWorldPositionFromDepth(vec2 uv, float depth, mat4 invViewProj)
{
    uv = uv * 2.0 - 1.0;
    vec4 positionS = vec4(uv, depth, 1.0f);
    vec4 positionW = invViewProj * positionS;
    return positionW.xyz / positionW.w;
}

// https://wickedengine.net/2019/09/22/improved-normal-reconstruction-from-depth/
vec3 ComputeViewPositionFromDepth(vec2 uv, float depth, mat4 invProj)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0 - uv.y) * 2.0f - 1.0f;
    vec4 positionS = vec4(x, y, depth, 1.0f);
    vec4 positionV = invProj * positionS;
    return positionV.xyz / positionV.w;
}

#endif
