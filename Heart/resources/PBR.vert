#version 460

#include "FrameBuffer.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out int entityId;
layout(location = 2) out float depth;
layout(location = 3) out vec3 worldPos;
layout(location = 4) out vec3 normal;
layout(location = 5) out vec3 tangent;
layout(location = 6) out vec3 bitangent;

struct ObjectData {
    mat4 model;
    int entityId;
    vec3 padding;
};

layout(binding = 1) readonly buffer ObjectBuffer {
    ObjectData object;
} objectBuffer;

void main() {
    worldPos = (objectBuffer.object.model * vec4(inPosition, 1.0)).xyz;
    vec4 viewPos = (frameData.view * vec4(worldPos, 1.0));
    depth = viewPos.z;
    gl_Position = frameData.proj * viewPos;
    
    texCoord = inTexCoord;
    entityId = objectBuffer.object.entityId;
    normal = inNormal;
    tangent = inTangent.xyz;
    bitangent = cross(inTangent.xyz, inNormal);
    bitangent *= inTangent.w;
}