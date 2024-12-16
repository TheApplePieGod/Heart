#ifndef UTIL_GBUFFER
#define UTIL_GBUFFER

uvec4 PackColorData(vec3 albedo, float metalness, float roughness)
{
    uvec4 colorOutput = uvec4(0);

    // Compensate for gamma correction
    // TODO: fix? this is probably because textures are unorm
    colorOutput.rgb = uvec3(clamp(pow(albedo, vec3(2.2f)), 0.f, 1.f) * 255.f) & 0xFC;

    // Pack metalness/roughness
    uint qMetal = clamp(uint(metalness * 127.f), 0, 127);
    uint qRough = clamp(uint(roughness * 127.f), 0, 127);
    colorOutput.rgb |= uvec3(qMetal & 0x3, (qMetal >> 2) & 0x3, (qMetal >> 4) & 0x3);
    colorOutput.a = qRough | ((qMetal & 0x40) << 1);

    return colorOutput;
}

vec3 UnpackAlbedo(uvec4 data)
{
    return vec3(data.rgb & 0xFC) / 255.f;
}

float UnpackMetalness(uvec4 data)
{
    uint qMetal = (data.r & 0x3) | ((data.g & 0x3) << 2) | ((data.b & 0x3) << 4) | ((data.a & 0x80) >> 1);
    return float(qMetal) / 127.f;
}

float UnpackRoughness(uvec4 data)
{
    uint qRough = data.a & 0x7F;
    return float(qRough) / 127.f;
}

void UnpackColorData(uvec4 data, out vec3 albedo, out float metalness, out float roughness)
{
    albedo = UnpackAlbedo(data);
    metalness = UnpackMetalness(data);
    roughness = UnpackRoughness(data);
}

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/

vec2 OctWrap(vec2 v)
{
    return (1.0 - abs(vec2(v.y, v.x))) * vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}
 
vec2 OctahedronEncode(vec3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : OctWrap(n.xy);
    n.xy = n.xy * 0.5 + 0.5;
    return n.xy;
}
 
vec3 OctahedronDecode(vec2 f)
{
    f = f * 2.0 - 1.0;
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = clamp(-n.z, 0.0, 1.0);
    n.xy += vec2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);
    return normalize(n);
}

#endif
