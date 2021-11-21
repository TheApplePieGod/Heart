#version 460

#include "FrameBuffer.glsl"

layout(location = 0) out vec4 outColor;

#ifdef VULKAN
layout (input_attachment_index = 0, binding = 1) uniform subpassInput texColor;
layout (input_attachment_index = 1, binding = 2) uniform subpassInput texWeights;
#else
layout(binding = 1) uniform sampler2D texColor;
layout(binding = 2) uniform sampler2D texWeights;
#endif

void main() {
    #ifdef VULKAN
    vec4 accum = subpassLoad(texColor);
    float reveal = subpassLoad(texWeights).r;
    #else
    vec4 accum = texture(texColor, gl_FragCoord.xy / frameBuffer.data.screenSize);
    float reveal = texture(texWeights, gl_FragCoord.xy / frameBuffer.data.screenSize).r;
    #endif

    outColor = vec4(accum.rgb / max(accum.a, 1e-5), reveal);
}