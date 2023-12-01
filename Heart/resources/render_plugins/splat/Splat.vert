#version 460

#define INCLUDE_VERTEX_LAYOUT_INPUT
#include "../../VertexLayout.glsl"

#define FRAME_BUFFER_BINDING 0
#define FRAME_BUFFER_SET 0
#include "../frame_data/FrameBuffer.glsl"

layout(
    std430,
    binding = 1,
    set = 0
) readonly buffer ObjectBuffer {
    mat4 data[];
} objectBuffer;

layout(
    std430,
    binding = 2,
    set = 0
) readonly buffer ColorBuffer {
    vec4 data[];
} colorBuffer;

layout(push_constant) uniform PushConstants
{
    mat4 model;
} constants;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec4 outColor;

void main() {
    int objectId = gl_InstanceIndex;

    vec4 worldPos = constants.model * objectBuffer.data[objectId] * vec4(inPosition, 1.f);
    vec4 viewPos = frameBuffer.data.view * worldPos;
    gl_Position = frameBuffer.data.proj * viewPos;

    outTexCoord = inTexCoord;
    outColor = colorBuffer.data[objectId];
}
