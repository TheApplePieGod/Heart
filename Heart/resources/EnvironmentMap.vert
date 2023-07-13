#version 460

#define INCLUDE_VERTEX_LAYOUT_INPUT
#include "VertexLayout.glsl"

#include "CubemapBuffer.glsl"

layout(location = 0) out vec3 localPos;

void main() {
    localPos = inPosition;

    mat4 rotView = mat4(mat3(cubemapBuffer.data.view)); // remove translation
    vec4 clipPos = cubemapBuffer.data.proj * rotView * vec4(localPos, 1.f);

    gl_Position = clipPos.xyww; // farthest depth
}
