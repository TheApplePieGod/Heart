#version 460

#define FRAME_BUFFER_BINDING 0
#define FRAME_BUFFER_SET 0
#include "../frame_data/FrameBuffer.glsl"

// http://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;

vec4 GRID_PLANE[6] = vec4[](
    vec4(1, 1, 0, 1), vec4(-1, -1, 0, 1), vec4(-1, 1, 0, 1),
    vec4(-1, -1, 0, 1), vec4(1, 1, 0, 1), vec4(1, -1, 0, 1)
);

// Getting world space coord of the point as if it was infinitely close / far away
vec3 UnprojectPoint(float x, float y, float z)
{
    vec4 unprojected = frameBuffer.data.invViewProj * vec4(x, y, z, 1.0);
    return unprojected.xyz / unprojected.w;
}

void main() {
    vec4 clipPoint = GRID_PLANE[gl_VertexIndex];
    nearPoint = UnprojectPoint(clipPoint.x, clipPoint.y, float(frameBuffer.data.reverseDepth));
    farPoint = UnprojectPoint(clipPoint.x, clipPoint.y, 1.0 - float(frameBuffer.data.reverseDepth));
    gl_Position = clipPoint;
}
