#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 texCoord;

layout(set = 0, binding = 0) uniform frame_data {
    mat4 viewProj;
} frameData;

layout(set = 0, binding = 1) uniform object_data {
    mat4 model;
} objectData;

void main() {
    gl_Position = frameData.viewProj * objectData.model * vec4(inPosition, 1.0);
    texCoord = inTexCoord;
}