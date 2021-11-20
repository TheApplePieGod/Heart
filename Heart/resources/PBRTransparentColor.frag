#version 460

#include "PBR.glsl"

layout(location = 0) out float outEntityId;
layout(location = 1) out vec4 outColor;
layout(location = 2) out float outReveal;

void main() {
    vec4 color = GetFinalColor();
    color.rgb *= color.a;

    if (color.a < 0.001)
        discard;

    const float depthZ = -depth * 10.0f;

    const float distWeight = clamp(0.03 / (1e-5 + pow(depthZ / 200, 4.0)), 1e-2, 3e3);
    float alphaWeight = min(1.0, max(max(color.r, color.g), max(color.b, color.a)) * 40.0 + 0.01);
    alphaWeight *= alphaWeight;

    const float weight = alphaWeight * distWeight;

    outColor = color * weight;
    outEntityId = entityId;
    outReveal = color.a;
}