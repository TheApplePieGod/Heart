#version 460

layout(location = 0) in vec2 texCoord;

#include "GaussianBlur.glsl"
#include "BloomBuffer.glsl"

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outUpsampleColor;

void main() {
    outColor = vec4(GaussianHorizontal(texCoord, bloomBuffer.data.mipLevel, bloomBuffer.data.blurScale, bloomBuffer.data.blurStrength), 1.0);
    outUpsampleColor = vec4(textureLod(image, texCoord, bloomBuffer.data.mipLevel + 1).rgb, 1.0);
}