#version 450

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outColor2;

layout(set = 0, binding = 0) uniform frame_data {
    vec4 color;
} frameData;

void main() {
    outColor = fragColor;
    outColor2 = frameData.color;
}