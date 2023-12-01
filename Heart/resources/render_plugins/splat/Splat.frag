#version 460

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = inColor;
    //outColor = vec4(1.f, 0.f, 0.f, 1.f);
}
