#version 460

#define INCLUDE_VERTEX_LAYOUT_INPUT
#include "../../VertexLayout.glsl"

#define FRAME_BUFFER_BINDING 0
#define FRAME_BUFFER_SET 0
#include "../frame_data/FrameBuffer.glsl"

#define OBJECT_BUFFER_BINDING 1
#define OBJECT_BUFFER_SET 0
#include "../compute_mesh_batches/ObjectBuffer.glsl"

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec3 outBitangent;
layout(location = 4) out uint outMaterialId;
layout(location = 5) out vec3 outWorldPos;

void main() {
    int objectId = gl_InstanceIndex;

    vec4 worldPos = GET_OBJECT(objectId).model * vec4(inPosition, 1.0);
    outWorldPos = worldPos.xyz;

    vec4 viewPos = frameBuffer.data.view * worldPos;
    gl_Position = frameBuffer.data.proj * viewPos;
    
    outTexCoord = inTexCoord;
    outMaterialId = GET_OBJECT(objectId).materialId;

    // Convert to world space
    mat3 tangentToWorld = mat3(GET_OBJECT(objectId).model);
    outNormal = tangentToWorld * inNormal;
    outTangent = tangentToWorld * inTangent.xyz;
    outBitangent = cross(inTangent.xyz, inNormal);
    outBitangent *= inTangent.w;
}
