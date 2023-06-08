#version 460

#include "../../VertexLayout.glsl"
#include "../frame_data/FrameBuffer.glsl"
#include "../compute_material_batches/ObjectBuffer.glsl"

layout(location = 0) out vec2 texCoord;
layout(location = 1) out int entityId;
layout(location = 2) out vec4 viewPos;
layout(location = 3) out vec3 worldPos;
layout(location = 4) out vec3 normal;
layout(location = 5) out vec3 tangent;
layout(location = 6) out vec3 bitangent;
layout(location = 7) out int materialId;

void main() {
    int objectId = gl_InstanceIndex;

    worldPos = (objectBuffer.objects[objectId].model * vec4(inPosition, 1.0)).xyz;
    viewPos = (frameBuffer.data.view * vec4(worldPos, 1.0));
    gl_Position = frameBuffer.data.proj * viewPos;
    
    texCoord = inTexCoord;
    entityId = int(objectBuffer.objects[objectId].data[0]);
    materialId = int(objectBuffer.objects[objectId].data[1]);

    // Adjust to match object rotation (and technically scale but it gets normalized out later)
    normal = mat3(objectBuffer.objects[objectId].model) * inNormal;
    tangent = mat3(objectBuffer.objects[objectId].model) * inTangent.xyz;
    bitangent = cross(inTangent.xyz, inNormal);
    bitangent *= inTangent.w;
}
