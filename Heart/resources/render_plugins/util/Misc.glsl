#ifndef UTIL_MISC
#define UTIL_MISC

// https://wickedengine.net/2019/09/22/improved-normal-reconstruction-from-depth/
vec3 ComputePositionFromDepth(vec2 uv, float depth, mat4 invProj)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0 - uv.y) * 2.0f - 1.0f;
    vec4 positionS = vec4(x, y, depth, 1.0f);
    vec4 positionO = invProj * positionS;
    return positionO.xyz / positionO.w;
}

#endif
