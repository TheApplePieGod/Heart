#version 460

#define INCLUDE_VERTEX_LAYOUT_INPUT
#include "../../VertexLayout.glsl"

#define FRAME_BUFFER_BINDING 0
#define FRAME_BUFFER_SET 0
#include "../frame_data/FrameBuffer.glsl"

struct ObjectData {
    mat4 sigma;
    vec4 position;
    vec4 color;
};

layout(
    std430,
    binding = 1,
    set = 0
) readonly buffer ObjectBuffer {
    ObjectData data[];
} objectBuffer;

layout(push_constant) uniform PushConstants
{
    mat4 model;
} constants;

layout(location = 0) out vec2 outLocalPos;
layout(location = 1) out vec4 outColor;

// https://www.cs.umd.edu/~zwicker/publications/EWASplatting-TVCG02.pdf
void main() {
    int objectId = gl_InstanceIndex;
    ObjectData objectData = objectBuffer.data[objectId];

    vec4 worldPos = constants.model * objectData.position;
    vec4 viewPos = frameBuffer.data.view * worldPos;
    vec4 fragPos = frameBuffer.data.proj * viewPos;

    // Ignore out of bounds/behind the camera
    float clip = 1.2 * fragPos.w;
    if (fragPos.z < -clip || fragPos.x < -clip || fragPos.x > clip || fragPos.y < -clip || fragPos.y > clip)
    {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }
    
    float z2 = viewPos.z * viewPos.z;
    mat3 sigma = mat3(objectData.sigma);

    // Compute J matrix (34)
    vec2 focalLen = -frameBuffer.data.proj[1][1] * frameBuffer.data.screenSize * 2.f;
    mat3 J = mat3(
        focalLen.x / viewPos.z, 0.f, -focalLen.x * viewPos.x / z2,
        0.f, -focalLen.y / viewPos.z, focalLen.y * viewPos.y / z2,
        0.f, 0.f, 0.f
    );

    // Compute the 2d screen space covariance matrix
    mat3 WJ = transpose(mat3(frameBuffer.data.view)) * J;
    mat3 camCov = transpose(WJ) * sigma * WJ;
    vec3 cov2d = vec3(camCov[0][0], camCov[0][1], camCov[1][1]);
    
    // Compute the eigenvalues of cov2d in order to determine the extent of the gaussian
    float a = (cov2d.x + cov2d.z) * 0.5f;
    float b = length(vec2((cov2d.x - cov2d.z) * 0.5f, cov2d.y));
    vec2 lambda = vec2(a + b, a - b);
    if (lambda.y < 0.f)
        return;
    vec2 eigenvec1 = normalize(vec2(cov2d.y, lambda.x - cov2d.x));
    vec2 eigenvec2 = vec2(eigenvec1.y, -eigenvec1.x);
    
    // Define the axes that span the final plane
    vec2 axis1 = min(sqrt(2 * lambda.x), 1024) * eigenvec1;
    vec2 axis2 = min(sqrt(2 * lambda.y), 1024) * eigenvec2;

    // Compute final position
    vec2 scaledPosition = inPosition.xy * 4.f;
    gl_Position = vec4(
        fragPos.xy / fragPos.w // Pre-divide by homogenous coordinate for computation
            + scaledPosition.x * axis1 / frameBuffer.data.screenSize // Coords are in [-1, 1] so this will extrude properly
            + scaledPosition.y * axis2 / frameBuffer.data.screenSize,
        0.f, 1.f
    );

    outLocalPos = scaledPosition;
    outColor = objectData.color;
}
