#version 460

layout(location = 0) in vec3 inViewNormal;

layout(location = 0) out vec4 outNormalData;

void main() {
    outNormalData = vec4(inViewNormal, 1.f);
}