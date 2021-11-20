#version 460

#include "FrameBuffer.glsl"

// 0---^-----------2
// |   |   |     /
// <---.---|---/---> x+
// |   |   | /
// |-------/
// |   | /
// |   /
// | / |
// 1   V
//     y+

void main() {
    #ifdef VULKAN
    uint vertexIndex = gl_VertexIndex;
    #else
    uint vertexIndex = gl_VertexID;
    #endif

    vec4 pos = vec4((float((vertexIndex >> 1U) & 1U)) * 4.0 - 1.0, (float(vertexIndex & 1U)) * 4.0 - 1.0, frameData.reverseDepth, 1.0);
    gl_Position = pos;
}