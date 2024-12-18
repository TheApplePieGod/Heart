layout(push_constant) uniform PushConstants
{
    vec2 srcResolution;
    vec2 dstResolution;
    float threshold;
    float knee;
    float sampleScale;
    uint prefilter;
} constants;

// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
// https://github.com/Unity-Technologies/Graphics/blob/master/com.unity.postprocessing/PostProcessing/Shaders/Colors.hlsl
vec3 QuadraticThreshold(vec3 color)
{
    float threshold = constants.threshold;
    vec3 curve = vec3(constants.threshold - constants.knee, constants.knee * 2.0, 0.25 / constants.knee);

    // Pixel brightness
    float br = max(color.r, max(color.g, color.b));

    // Under-threshold part: quadratic curve
    float rq = clamp(br - curve.x, 0.0, curve.y);
    rq = curve.z * rq * rq;

    // Combine and apply the brightness response curve.
    color *= max(rq, br - threshold) / max(br, 1.0e-4);

    return color;
}
