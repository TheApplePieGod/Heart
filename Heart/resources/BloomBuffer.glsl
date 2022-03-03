struct BloomData {
    uint mipLevel;
    bool reverseDepth;
    float blurScale;
    float blurStrength;
};

layout(binding = 0) readonly buffer BloomBuffer {
    BloomData data;
} bloomBuffer;