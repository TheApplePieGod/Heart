#ifndef UTIL_RAYQUERY
#define UTIL_RAYQUERY

#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable

bool QueryVisibility(accelerationStructureEXT tlas, vec3 P, vec3 dir, float maxDist, uint rayFlags, uint mask)
{
    float tMin = 0.01f;
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(
        rayQuery,
        tlas,
        rayFlags,
        mask,
        P,
        tMin,
        dir,
        maxDist
    );

    // Start traversal: return false if traversal is complete
    while (rayQueryProceedEXT(rayQuery)) {}

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT)
        return false;
    return true;
}

#endif
