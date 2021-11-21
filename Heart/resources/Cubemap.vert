#version 460

#include "FrameBuffer.glsl"
#include "VertexLayout.glsl"

layout(location = 0) out vec3 localPos;

void main() {
    localPos = inPosition;
    gl_Position = frameBuffer.data.proj * frameBuffer.data.view * vec4(localPos, 1.f);
}