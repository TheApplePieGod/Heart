#version 450

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outColor2;

layout(set = 0, binding = 0) uniform frame_data {
    vec4 color;
} frameData;

layout(set = 0, binding = 1) uniform sampler2D samp;

void main() {
    outColor = frameData.color;
    outColor2 = texture(samp, texCoord);
}