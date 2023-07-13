#ifndef OBJECT_BUFFER
#define OBJECT_BUFFER

struct ObjectData {
    mat4 model;
    uint materialId;
    uint entityId;
};

layout(
    std430,
    binding = OBJECT_BUFFER_BINDING,
    set = OBJECT_BUFFER_SET
) readonly buffer ObjectBuffer {
    ObjectData data[];
} objectBuffer;

#define GET_OBJECT(objectId) objectBuffer.data[objectId]

#endif
