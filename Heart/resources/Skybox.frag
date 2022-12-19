#version 460

layout(location = 0) in vec3 localPos;

#include "FrameBuffer.glsl"

layout(location = 0) out vec4 outHDRColor;

layout(binding = 1) uniform samplerCube environmentMap;

void main() {
    outHDRColor = vec4(textureLod(environmentMap, localPos, 0.0).rgb, 1.f);
}