#version 460

layout(location = 0) out vec4 outColor;

layout (input_attachment_index=0, binding=2) uniform subpassInput texColor;
layout (input_attachment_index=1, binding=3) uniform subpassInput texWeights;

void main() {
    vec4 accum = subpassLoad(texColor);
    float reveal = subpassLoad(texWeights).r;

    outColor = vec4(accum.rgb / max(accum.a, 1e-5), reveal);
}