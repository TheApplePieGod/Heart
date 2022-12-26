struct FrameData {
    mat4 proj;
    mat4 view;
    mat4 invViewProj;
    vec4 cameraPos;
    vec2 screenSize;
    bool reverseDepth;
    bool cullEnable;
    bool bloomEnable;
    bool ssaoEnable;
    vec2 padding2;
};

layout(binding = 0) readonly uniform FrameBuffer {
    FrameData data;
} frameBuffer;