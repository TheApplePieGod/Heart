#version 450

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outColor2;

layout(set = 0, binding = 2) uniform sampler2D samp;

void main() {
    outColor = vec4(1.0, 1.0, 0.0, 0.5);
    outColor2 = texture(samp, texCoord);
}