#version 460

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outHDRColor;
layout(location = 1) out vec4 outBrightColor;

layout (input_attachment_index = 0, binding = 1) uniform subpassInput texColor;
layout (input_attachment_index = 1, binding = 2) uniform subpassInput texWeights;

void main() {
    vec4 accum = subpassLoad(texColor);
    float reveal = subpassLoad(texWeights).r;

    // reversed because of blend
    if (reveal > 0.9999) discard;

    outHDRColor = vec4(accum.rgb / max(accum.a, 1e-5), reveal);
}