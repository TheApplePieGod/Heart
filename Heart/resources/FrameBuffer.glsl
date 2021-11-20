layout(binding = 0) uniform FrameBuffer {
    mat4 proj;
    mat4 view;
    vec4 cameraPos;
    vec2 screenSize;
    bool reverseDepth;
    bool padding;
} frameData;