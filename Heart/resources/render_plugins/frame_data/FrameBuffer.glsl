struct FrameData {
    mat4 proj;
    mat4 view;
    mat4 invProj;
    mat4 invViewProj;
    vec4 cameraPos;
    vec2 clipPlanes;
    vec2 screenSize;
    bool reverseDepth;
    float padding;
    vec2 padding2;
};

layout(binding = 0) readonly uniform FrameBuffer {
    FrameData data;
} frameBuffer;
