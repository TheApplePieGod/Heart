#version 460

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform FrameBuffer {
    mat4 viewProj;
    mat4 view;
    vec2 screenSize;
    bool reverseDepth;
    bool padding;
} frameData;

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
    vec4 accum = texture(texColor, gl_FragCoord.xy / frameData.screenSize);
    float reveal = texture(texWeights, gl_FragCoord.xy / frameData.screenSize).r;
    #endif

    outColor = vec4(accum.rgb / max(accum.a, 1e-5), reveal);
}