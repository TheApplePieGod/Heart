#version 450

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outColor2;

void main() {
    outColor = fragColor;
    outColor2 = vec4(0.0, 0.0, 1.0, 1.0);
}