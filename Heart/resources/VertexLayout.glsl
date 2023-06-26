#ifndef VERTEX_LAYOUT
#define VERTEX_LAYOUT

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;

struct Vertex {
    vec3 position;
    vec2 texCoord;
    vec3 normal;
    vec4 tangent;
};

#endif
