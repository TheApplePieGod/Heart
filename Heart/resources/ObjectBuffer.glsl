struct ObjectData {
    mat4 model;
    vec4 data;
};

layout(binding = 1) readonly buffer ObjectBuffer {
    ObjectData objects[];
} objectBuffer;