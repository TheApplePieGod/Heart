#version 460

#include "FrameBuffer.glsl"

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outHDRColor;
layout(location = 1) out vec4 outBrightColor;

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

    // reversed because of blend
    if (reveal > 0.9999) discard;

    outHDRColor = vec4(accum.rgb / max(accum.a, 1e-5), reveal);
    float brightness = dot(outHDRColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > frameBuffer.data.bloomThreshold)
        outBrightColor = vec4(outHDRColor.rgb, 1.0);
    else
        outBrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}