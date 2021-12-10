#version 460

layout(location = 0) in vec2 texCoord;

#include "GaussianBlur.glsl"
#include "BloomBuffer.glsl"

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(GaussianHorizontal(texCoord, bloomBuffer.data.mipLevel, bloomBuffer.data.blurScale, bloomBuffer.data.blurStrength), 1.0);
}