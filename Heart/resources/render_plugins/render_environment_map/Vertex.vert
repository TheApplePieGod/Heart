#version 460

#define FRAME_BUFFER_BINDING 0
#define FRAME_BUFFER_SET 0
#include "../frame_data/FrameBuffer.glsl"

#define INCLUDE_VERTEX_LAYOUT_INPUT
#include "../../VertexLayout.glsl"

layout(location = 0) out vec3 localPos;

void main() {
    localPos = inPosition;

    mat4 rotView = mat4(mat3(frameBuffer.data.view)); // Remove translation
    vec4 clipPos = frameBuffer.data.proj * rotView * vec4(localPos, 1.f);

    gl_Position = clipPos.xyww; // Farthest depth
}
