struct BloomData
{
    vec2 SrcResolution;
    vec2 DstResolution;
    float Threshold;
    float Knee;
    float SampleScale;
    uint Prefilter;
    vec4 Padding1;
    vec4 Padding2;
};

layout(binding = 0) readonly buffer BloomDataBuffer {
    BloomData data;
} dataBuffer;
