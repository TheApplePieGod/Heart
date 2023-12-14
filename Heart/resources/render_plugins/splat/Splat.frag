#version 460

layout(location = 0) in vec2 inLocalPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inConic;

layout(location = 0) out vec4 outColor;

void main() {
    float power = -0.5f * (inConic.x * inLocalPos.x * inLocalPos.x + inConic.z * inLocalPos.y * inLocalPos.y) - inConic.y * inLocalPos.x * inLocalPos.y;
    if (power > 0.f)
        discard;
    float opacity = min(0.99f, inColor.a * exp(power));
    if (opacity < 1.f / 255.f)
        discard;
    outColor = vec4(inColor.rgb, opacity);
}
