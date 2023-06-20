struct LightData {
    vec4 position;
    vec4 direction;
    vec4 color;
    uint lightType;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};

#define DIRECTIONAL 1
#define POINT 2

layout(binding = 3) readonly buffer LightingBuffer {
    LightData lights[];
} lightingBuffer;
