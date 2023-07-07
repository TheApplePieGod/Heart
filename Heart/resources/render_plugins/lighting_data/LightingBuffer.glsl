#ifndef LIGHT_BUFFER
#define LIGHT_BUFFER

struct Light {
    vec4 position;
    vec4 direction;
    vec4 color; // RGB: color, A: intensity
    uint lightType;
    float radius;
    vec2 padding;
};

#define LIGHT_DIRECTIONAL 1
#define LIGHT_POINT 2

layout(
    std430,
    binding = LIGHT_BUFFER_BINDING,
    set = LIGHT_BUFFER_SET
) readonly buffer LightingBuffer {
    Light data[];
} lightingBuffer;

#define GET_LIGHT(index) lightingBuffer.data[index]
#define GET_LIGHT_COUNT() uint(GET_LIGHT(0).position.x)

#ifdef DIRECTIONAL_LIGHT_BUFFER_BINDING
layout(
    std430,
    binding = DIRECTIONAL_LIGHT_BUFFER_BINDING,
    set = LIGHT_BUFFER_SET
) readonly buffer DirLightingBuffer {
    uvec4 data[];
} dirLightingBuffer;

#define GET_DIRECTIONAL_LIGHT(index) dirLightingBuffer.data[(index) / 4][(index) % 4]
#define GET_DIRECTIONAL_LIGHT_COUNT() GET_DIRECTIONAL_LIGHT(0)
#endif

#endif
