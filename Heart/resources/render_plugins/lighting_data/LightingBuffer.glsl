#ifndef LIGHT_BUFFER
#define LIGHT_BUFFER

struct Light {
    vec4 position;
    vec4 direction;
    vec4 color; // RGB: color, A: intensity
    uint lightType;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
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

#endif
