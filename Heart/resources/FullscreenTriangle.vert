#version 460

layout(location = 0) out vec2 texCoord;

// Counterclockwise winding order

// 0---^-----------2
// |   |   |     /
// <---.---|---/---> x+
// |   |   | /
// |-------/
// |   | /
// |   /
// | / |
// 1   V
//     y+

void main() {
    uint vertexIndex = gl_VertexIndex;

    vec4 pos = vec4((float((vertexIndex >> 1U) & 1U)) * 4.0 - 1.0, (float(vertexIndex & 1U)) * 4.0 - 1.0, 0, 1.0);
    gl_Position = pos;
    texCoord = pos.xy * 0.5 + 0.5;
}
