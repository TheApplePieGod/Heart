#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 texCoord;

layout(binding = 0) uniform FrameBuffer {
    mat4 viewProj;
} frameData;

struct ObjectData {
    mat4 model;
};

layout(binding = 1) readonly buffer ObjectBuffer {
    ObjectData object;
} objectBuffer;

void main() {
    gl_Position = frameData.viewProj * objectBuffer.object.model * vec4(inPosition, 1.0);
    //gl_Position = vec4(inPosition, 1.0);
    texCoord = inTexCoord;
}