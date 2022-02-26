struct ObjectData {
    mat4 model;
    vec4 data;
    vec4 boundingSphere;
};

layout(binding = 1) readonly buffer ObjectBuffer {
    ObjectData objects[];
} objectBuffer;