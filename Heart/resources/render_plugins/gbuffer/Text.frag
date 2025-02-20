#version 460

#include "Common.glsl"

layout(binding = 0, set = 2) uniform sampler2D atlasTex;

float ScreenPxRange() {
    vec2 unitRange = vec2(2.0f) / vec2(textureSize(atlasTex, 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(inTexCoord);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float Median(float r, float g, float b, float a) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec4 msd = texture(atlasTex, inTexCoord);
    float sd = Median(msd.r, msd.g, msd.b, msd.a);
    float screenPxDistance = ScreenPxRange() * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    if (opacity < 0.05)
        discard;

    WriteFinalColor();
}
