struct CubemapData {
    mat4 proj;
    mat4 view;
    vec4 parameters;
};

layout(binding = 0) readonly buffer CubemapBuffer {
    CubemapData data;
} cubemapBuffer;