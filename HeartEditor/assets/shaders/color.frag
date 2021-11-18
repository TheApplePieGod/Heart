#version 460

layout(location = 0) in vec2 texCoord;
layout(location = 1) in flat int entityId;
layout(location = 2) in float depth;

layout(location = 0) out float outEntityId;
layout(location = 1) out vec4 outColor;
layout(location = 2) out float outReveal;

layout(binding = 2) uniform sampler2D samp;

void main() {
    vec4 color = texture(samp, texCoord);
    color.rgb *= color.a;

    if (color.a < 0.001)
        discard;

    // The depth functions in the paper want a camera-space depth of 0.1 < z < 500,
    // but the scene at the moment uses a range of about 0.01 to 50, so multiply
    // by 10 to get an adjusted depth:
    const float depthZ = -depth * 10.0f;

    const float distWeight = clamp(0.03 / (1e-5 + pow(depthZ / 200, 4.0)), 1e-2, 3e3);
    float alphaWeight = min(1.0, max(max(color.r, color.g), max(color.b, color.a)) * 40.0 + 0.01);
    alphaWeight *= alphaWeight;

    const float weight = alphaWeight * distWeight;

    outColor = color * weight;
    outEntityId = entityId;
    outReveal = color.a;
}