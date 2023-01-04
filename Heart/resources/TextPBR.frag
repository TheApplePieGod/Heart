#version 460

#include "PBR.glsl"

layout(binding = 16) uniform sampler2D atlasTex;

layout(location = 0) out vec4 outHDRColor;
layout(location = 1) out float outEntityId;

float screenPxRange() {
    vec2 unitRange = vec2(2.0f)/vec2(textureSize(atlasTex, 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(texCoord);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float median(float r, float g, float b, float a) {
    return max(min(r, g), min(max(r, g), b));
}

// https://github.com/Chlumsky/msdfgen
void main() {
    vec4 finalColor = GetFinalColor();
    outEntityId = float(entityId);

    vec4 msd = texture(atlasTex, texCoord).rgba;
    float sd = median(msd.r, msd.g, msd.b, msd.a);
    float screenPxDistance = screenPxRange()*(sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    if (opacity < 0.05) discard;

    outHDRColor = mix(vec4(0.f), finalColor, opacity);
}