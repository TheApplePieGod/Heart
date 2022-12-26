#version 460

#include "FrameBuffer.glsl"
#include "ObjectBuffer.glsl"
#include "VertexLayout.glsl"

layout(location = 0) out vec2 texCoord;
layout(location = 1) out int entityId;
layout(location = 2) out vec4 viewPos;
layout(location = 3) out vec3 worldPos;
layout(location = 4) out vec3 normal;
layout(location = 5) out vec3 tangent;
layout(location = 6) out vec3 bitangent;
layout(location = 7) out int instance;

void main() {
    #ifdef VULKAN
        int instanceIndex = gl_InstanceIndex;
    #else
        int instanceIndex = gl_BaseInstance + gl_InstanceID;
    #endif

    int objectId = instanceIndex;

    worldPos = (objectBuffer.objects[objectId].model * vec4(inPosition, 1.0)).xyz;
    viewPos = (frameBuffer.data.view * vec4(worldPos, 1.0));
    gl_Position = frameBuffer.data.proj * viewPos;
    
    texCoord = inTexCoord;
    entityId = int(objectBuffer.objects[objectId].data[0]); // entity id
    instance = objectId;

    // adjust to match object rotation (and technically scale but it gets normalized out later)
    normal = mat3(objectBuffer.objects[objectId].model) * inNormal;
    tangent = mat3(objectBuffer.objects[objectId].model) * inTangent.xyz;
    bitangent = cross(inTangent.xyz, inNormal);
    bitangent *= inTangent.w;
}