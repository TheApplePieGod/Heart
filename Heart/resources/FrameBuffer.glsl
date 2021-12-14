struct FrameData {
    mat4 proj;
    mat4 view;
    vec4 cameraPos;
    vec2 screenSize;
    bool reverseDepth;
    float padding;
};

layout(binding = 0) readonly uniform FrameBuffer {
    FrameData data;
} frameBuffer;