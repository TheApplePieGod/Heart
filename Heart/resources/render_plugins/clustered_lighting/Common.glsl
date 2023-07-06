#define GROUP_X 16
#define GROUP_Y 8
#define GROUP_Z 4
#define GROUP_SIZE (GROUP_X * GROUP_Y * GROUP_Z)

layout(push_constant) uniform PushConstants
{
    vec4 clusterDims; // XYZ: cluster dim xyz
} constants;

