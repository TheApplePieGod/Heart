struct CubemapData {
    mat4 proj;
    mat4 view;
    vec4 parameters;
};

//#use_dynamic_offsets 0
layout(binding = 0) readonly buffer CubemapBuffer {
    CubemapData data;
} cubemapBuffer;