#version 460

#include "FrameBuffer.glsl"

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = frameBuffer.data.proj * frameBuffer.data.view * vec4(inPosition, 1.f);
}