#version 460



void main()
{
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
    vec4 pos = vec4((float((gl_VertexIndex >> 1U) & 1U)) * 4.0 - 1.0, (float(gl_VertexIndex & 1U)) * 4.0 - 1.0, 0, 1.0);
    gl_Position = pos;
}