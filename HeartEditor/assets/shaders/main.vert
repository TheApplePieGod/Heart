#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out int entityId;
layout(location = 2) out float depth;

layout(binding = 0) uniform FrameBuffer {
    mat4 viewProj;
    mat4 view;
} frameData;

struct ObjectData {
    mat4 model;
    int entityId;
    vec3 padding;
};

layout(binding = 1) readonly buffer ObjectBuffer {
    ObjectData object;
} objectBuffer;

void main() {
    //gl_Position = vec4(inPosition, 1.0);
    gl_Position = frameData.viewProj * objectBuffer.object.model * vec4(inPosition, 1.0);
    depth = (frameData.view * objectBuffer.object.model * vec4(inPosition, 1.0)).z;
    texCoord = inTexCoord;
    entityId = objectBuffer.object.entityId;
}