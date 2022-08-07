#version 460

layout(location = 0) in vec3 localPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

vec3 Tonemap(vec3 c)
{
    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
    vec3 tc = c / (c + 1.f);
    return mix(c / (l + 1.f), tc, tc);
}

void main() {
    vec2 uv = SampleSphericalMap(normalize(localPos)); // make sure to normalize localPos
    vec3 color = texture(equirectangularMap, uv).rgb;

    // Tonemap to prevent color blowout
    color = Tonemap(color);
    outColor = vec4(color, 1.0);
}