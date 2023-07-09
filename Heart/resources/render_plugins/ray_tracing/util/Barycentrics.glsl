#ifndef UTIL_BARYCENTRICS
#define UTIL_BARYCENTRICS

vec3 GetBarycentricCoordinates(vec2 hitPoint)
{
    return vec3(1.0 - hitPoint.x - hitPoint.y, hitPoint.x, hitPoint.y);
}

vec2 GetTexCoord(inout Vertex v0, inout Vertex v1, inout Vertex v2, vec3 bary)
{
    return vec2(v0.texCoord * bary.x + v1.texCoord * bary.y + v2.texCoord * bary.z);
}

#ifndef BARYCENTRICS_TEXCOORD_ONLY
vec3 GetWorldPosition(inout Vertex v0, inout Vertex v1, inout Vertex v2, vec3 bary)
{
    vec4 pos = vec4(v0.position * bary.x + v1.position * bary.y + v2.position * bary.z, 1.0);
    return vec3(gl_ObjectToWorldEXT * pos);
}

vec3 GetWorldNormal(inout Vertex v0, inout Vertex v1, inout Vertex v2, vec3 bary)
{
    vec3 normal = v0.normal * bary.x + v1.normal * bary.y + v2.normal * bary.z;
    return normalize(vec3(normal * gl_WorldToObjectEXT));
}

vec4 GetWorldTangent(inout Vertex v0, inout Vertex v1, inout Vertex v2, vec3 bary)
{
    vec4 tangent = v0.tangent * bary.x + v1.tangent * bary.y + v2.tangent * bary.z;
    return vec4(normalize(vec3(tangent.xyz * gl_WorldToObjectEXT)), tangent.w);
}
#endif

#endif
