#ifndef FRAME_BUFFER
#define FRAME_BUFFER

struct FrameData {
    mat4 proj;
    mat4 view;
    mat4 viewProj;
    mat4 prevViewProj;
    mat4 invProj;
    mat4 invView;
    mat4 invViewProj;
    vec4 cameraPos;
    vec2 clipPlanes;
    vec2 screenSize;
    bool reverseDepth;
    uint frameCount;
};

layout(
    binding = FRAME_BUFFER_BINDING,
    set = FRAME_BUFFER_SET
) readonly uniform FrameBuffer {
    FrameData data;
} frameBuffer;

#endif
