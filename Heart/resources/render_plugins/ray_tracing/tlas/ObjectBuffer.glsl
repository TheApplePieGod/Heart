#ifndef OBJECT_BUFFER
#define OBJECT_BUFFER

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct ObjectData {
    uint64_t vertexAddress;
    uint64_t indexAddress;
    vec4 data; // R: materialId
};

layout(
    binding = OBJECT_BUFFER_BINDING,
    set = OBJECT_BUFFER_SET
) readonly buffer ObjectBuffer {
    ObjectData data[];
} objectBuffer;

#define GET_OBJECT(objectId) objectBuffer.data[objectId]

#endif
