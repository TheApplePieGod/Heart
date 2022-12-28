#version 460

struct CameraData {
    mat4 proj;
    mat4 view;
};

layout(binding = 0) readonly uniform CameraDataBuffer {
    CameraData data;
} cameraData;

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = cameraData.data.proj * cameraData.data.view * vec4(inPosition, 1.f);
}