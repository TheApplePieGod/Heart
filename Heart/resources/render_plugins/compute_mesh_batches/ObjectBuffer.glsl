struct ObjectData {
    mat4 model;
};

layout(binding = 1) readonly buffer ObjectBuffer {
    ObjectData objects[];
} objectBuffer;