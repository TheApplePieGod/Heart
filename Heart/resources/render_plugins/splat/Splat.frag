#version 460

layout(location = 0) in vec2 inLocalPos;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    float coeff = dot(inLocalPos, inLocalPos);
    if (coeff > 4.0)
        discard;
    float factor = exp(-coeff) * inColor.a;
    outColor = vec4(inColor.rgb * factor, factor);
    //outColor = vec4(0.f);
    //outColor = vec4(1.f, 0.f, 0.f, 1.f);
}
