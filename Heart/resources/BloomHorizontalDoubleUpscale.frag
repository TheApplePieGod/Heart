#version 460

layout(location = 0) in vec2 texCoord;

#include "GaussianBlur.glsl"
#include "BloomBuffer.glsl"

layout(binding = 2) uniform sampler2D upsampleTex;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outUpsampleColor;

void main() {
    outColor = vec4(GaussianHorizontal(texCoord, bloomBuffer.data.mipLevel, bloomBuffer.data.blurScale, bloomBuffer.data.blurStrength), 1.0);

    vec3 finalUpsampleColor = textureLod(image, texCoord, bloomBuffer.data.mipLevel + 1).rgb + texture(upsampleTex, texCoord).rgb;
    outUpsampleColor = vec4(finalUpsampleColor, 1.0);
}