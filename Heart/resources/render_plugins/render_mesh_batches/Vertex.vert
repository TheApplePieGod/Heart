#version 460

#include "../../VertexLayout.glsl"
#include "../frame_data/FrameBuffer.glsl"
#include "../compute_mesh_batches/ObjectBuffer.glsl"

layout(location = 0) out vec3 viewNormal;

void main() {
    int objectId = gl_InstanceIndex;

    vec4 worldPos = objectBuffer.objects[objectId].model * vec4(inPosition, 1.0);
    vec4 viewPos = frameBuffer.data.view * worldPos;
    gl_Position = frameBuffer.data.proj * viewPos;
    
    viewNormal = normalize((
        frameBuffer.data.proj * frameBuffer.data.view * vec4(mat3(objectBuffer.objects[objectId].model) * inNormal, 1.0)
    ).xyz);
}
