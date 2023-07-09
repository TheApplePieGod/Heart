#ifndef CLUSTER_BUFFER
#define CLUSTER_BUFFER

#include "../util/Misc.glsl"

struct Cluster {
    vec4 minBounds;
    vec4 maxBounds;
};

struct LightGrid {
    uint offset;
    uint lightCount;
};

struct ClusterData {
    vec4 clusterDims;
    float clusterScale;
    float clusterBias;
};

#ifdef CLUSTER_BUFFER_BINDING
layout(
    std430,
    binding = CLUSTER_BUFFER_BINDING,
    set = CLUSTER_BUFFER_SET
)
#ifndef CLUSTER_BUFFER_WRITE
readonly
#endif
buffer ClusterBuffer {
    Cluster data[];
} clusterBuffer;

#define GET_CLUSTER(index) clusterBuffer.data[index]
#endif

#ifdef CLUSTER_LIGHT_INDICES_BINDING
layout(
    binding = CLUSTER_LIGHT_INDICES_BINDING,
    set = CLUSTER_BUFFER_SET
)
#ifndef CLUSTER_BUFFER_WRITE
readonly
#endif
buffer ClusterLightIndices {
    uvec4 data[];
} clusterLightIndices;

#define GET_CLUSTER_LIGHT_INDEX(index) clusterLightIndices.data[(index) / 4][(index) % 4]
#endif

#ifdef CLUSTER_LIGHT_GRID_BINDING
layout(
    std430,
    binding = CLUSTER_LIGHT_GRID_BINDING,
    set = CLUSTER_BUFFER_SET
)
#ifndef CLUSTER_BUFFER_WRITE
readonly
#endif
buffer ClusterLightGrid {
    vec4 data[];
} clusterLightGrid;

#define GET_CLUSTER_LIGHT_GRID(index) clusterLightGrid.data[index]
#endif

#ifdef CLUSTER_DATA_BINDING
layout(
    binding = CLUSTER_DATA_BINDING,
    set = CLUSTER_BUFFER_SET
) readonly uniform ClusterDataBuffer {
    ClusterData data;
} clusterData;

#define GET_CLUSTER_DATA() clusterData.data
#endif

uint GetClusterZIndex(float screenDepth, float nearPlane, float farPlane, float scale, float bias)
{
    float eyeDepth = nearPlane * farPlane / (farPlane + nearPlane - screenDepth * (farPlane - nearPlane));
    uint zIndex = uint(max(log(eyeDepth) * scale + bias, 0.0));
    return zIndex;
}

uint GetClusterIndex(vec2 coord, vec2 screenSize, vec4 clusterDims, uint zIndex)
{
    vec2 clusterSize = screenSize / clusterDims.xy;
    uvec3 indices = uvec3(uvec2(coord.xy / clusterSize), zIndex);
    uint idx = (uint(clusterDims.x) * uint(clusterDims.y)) * indices.z +
                uint(clusterDims.x) * indices.y +
                indices.x;
    return idx;
}

#if defined(CLUSTER_LIGHT_GRID_BINDING) && defined(CLUSTER_LIGHT_INDICES_BINDING)
LightGrid GetLightGrid(uint clusterIdx)
{
    vec4 gridVec = GET_CLUSTER_LIGHT_GRID(clusterIdx);
    LightGrid grid;
    grid.offset = uint(gridVec.x);
    grid.lightCount = uint(gridVec.y);
    return grid;
}

uint GetGridLightIndex(uint start, uint offset)
{
    return GET_CLUSTER_LIGHT_INDEX(start + offset);
}
#endif

#endif
