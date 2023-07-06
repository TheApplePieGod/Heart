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

#endif
