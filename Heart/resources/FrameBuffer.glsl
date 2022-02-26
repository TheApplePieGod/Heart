struct FrameData {
    mat4 proj;
    mat4 view;
    vec4 cameraPos;
    vec2 screenSize;
    bool reverseDepth;
    float bloomThreshold;
    bool cullEnable;
    bool padding1;
    vec2 padding2;
};

layout(binding = 0) readonly uniform FrameBuffer {
    FrameData data;
} frameBuffer;