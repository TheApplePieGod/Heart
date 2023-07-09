#version 460

layout(location = 0) out vec4 outHDRColor;

layout(location = 0) in vec2 texCoord;

layout(binding = 0) uniform sampler2D bloomTex;

layout(push_constant) uniform PushConstants
{
    float strength;
} constants;

void main() {
    outHDRColor = textureLod(bloomTex, texCoord, 1) * constants.strength;
}
