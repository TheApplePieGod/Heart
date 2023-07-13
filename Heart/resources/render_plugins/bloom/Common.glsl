layout(push_constant) uniform PushConstants
{
    vec2 srcResolution;
    vec2 dstResolution;
    float threshold;
    float knee;
    float sampleScale;
    uint prefilter;
} constants;
