#version 460

#define INCLUDE_VERTEX_LAYOUT_INPUT
#include "../../VertexLayout.glsl"

#define FRAME_BUFFER_BINDING 0
#define FRAME_BUFFER_SET 0
#include "../frame_data/FrameBuffer.glsl"

#define OBJECT_BUFFER_BINDING 1
#define OBJECT_BUFFER_SET 0
#include "ObjectBuffer.glsl"

layout (std430, set = 0, binding = 2) buffer IndexBuffer {
    uint data[];
} indexBuffer;

layout (std140, set = 0, binding = 3) buffer BuildData {
    uint keyCount;
} buildData;

layout(push_constant) uniform PushConstants
{
    mat4 model;
} constants;

layout(location = 0) out vec2 outLocalPos;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec3 outConic;

// https://www.cs.umd.edu/~zwicker/publications/EWASplatting-TVCG02.pdf
void main() {
    //uint objectId = indexBuffer.data[buildData.keyCount - 1 - gl_InstanceIndex];
    uint objectId = indexBuffer.data[gl_InstanceIndex];
    ObjectData objectData = GET_OBJECT(objectId);

    vec4 worldPos = constants.model * objectData.position;
    vec4 viewPos = frameBuffer.data.view * worldPos;
    vec4 fragPos = frameBuffer.data.proj * viewPos;
    fragPos /= fragPos.w;

    // Ignore out of bounds/behind the camera
    float clip = 1.2 * fragPos.w;
    if (fragPos.z < -clip || fragPos.x < -clip || fragPos.x > clip || fragPos.y < -clip || fragPos.y > clip)
        return;
    
    float z2 = viewPos.z * viewPos.z;
    mat3 sigma = mat3(objectData.sigma);

    // Compute J matrix (34)
    float aspect = frameBuffer.data.screenSize.x / frameBuffer.data.screenSize.y;
    //float focalLen = -0.5f * frameBuffer.data.proj[1][1] * frameBuffer.data.screenSize.y;
    vec2 focalLen = -frameBuffer.data.proj[1][1] * frameBuffer.data.screenSize;
    //vec2 focalLen = vec2(-frameBuffer.data.proj[1][1]);
    //float focalLen = -500.f * frameBuffer.data.proj[1][1];
    mat3 J = mat3(
       focalLen.x / viewPos.z, 0.f, -focalLen.x * viewPos.x / z2,
        0.f, -focalLen.y / viewPos.z, focalLen.y * viewPos.y / z2,
        0.f, 0.f, 0.f
    );

    // Compute the 2d screen space covariance matrix
    mat3 W = transpose(mat3(frameBuffer.data.view * constants.model));
    mat3 WJ = W * J;
    mat3 camCov = transpose(WJ) * sigma * WJ;
    vec3 cov2d = vec3(camCov[0][0] + 0.3f, camCov[0][1], camCov[1][1] + 0.3f);

    float det = (cov2d.x * cov2d.z - cov2d.y * cov2d.y);
	if (det == 0.0f)
        return;
    
    vec2 wh = frameBuffer.data.screenSize;
    vec2 quadwh_scr = vec2(3.f * sqrt(cov2d.x), 3.f * sqrt(cov2d.z));  // screen space half quad height and width
    vec2 quadwh_ndc = quadwh_scr / wh;  // in ndc space

    // Purge extremely large points
    if (abs(quadwh_ndc.x) > 2 || abs(quadwh_ndc.y) > 2)
        return;

    // Compute final position
    vec2 scaledPosition = inPosition.xy * 2.f;
    vec2 ndcAdd = vec2(scaledPosition.x * quadwh_ndc.x / aspect, scaledPosition.y * quadwh_ndc.y);
    outLocalPos = scaledPosition * quadwh_scr;
    gl_Position = vec4(fragPos.xy + ndcAdd, fragPos.z, 1.f);

    float det_inv = 1.f / det;
	outConic = vec3(cov2d.z * det_inv, -cov2d.y * det_inv, cov2d.x * det_inv);
    outColor = objectData.color;
}
