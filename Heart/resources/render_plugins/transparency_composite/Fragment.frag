#version 460

layout(location = 0) out vec4 outHDRColor;

layout(location = 0) in vec2 texCoord;

layout(binding = 0) uniform sampler2D accumTex;
layout(binding = 1) uniform sampler2D revealTex;

void main() {
    vec4 accum = texelFetch(accumTex, ivec2(gl_FragCoord.xy), 0);
    float reveal = texelFetch(revealTex, ivec2(gl_FragCoord.xy), 0).r;

    outHDRColor = vec4(accum.rgb / max(accum.a, 1e-5), reveal);
}
