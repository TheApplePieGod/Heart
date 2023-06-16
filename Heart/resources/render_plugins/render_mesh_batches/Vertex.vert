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

    // Offset to fix depth test precision issues
    gl_Position.z += (1 - 2 * int(frameBuffer.data.reverseDepth)) * 0.00001;
    
    viewNormal = normalize((
        mat3(frameBuffer.data.view * objectBuffer.objects[objectId].model) * inNormal
    ).xyz);
    viewNormal.y *= -1; // Flip y to conform with view space
}
