#version 460

layout(location = 0) in vec2 texCoord;

layout(binding = 1) uniform sampler2D preBloom;
layout(binding = 2) uniform sampler2D postBloom;

layout(location = 0) out vec4 outColor;

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), vec3(0.f), vec3(1.f));
}

// adapted from https://learnopengl.com/Advanced-Lighting/Bloom
void main() {
    vec3 hdrColor = texture(preBloom, texCoord).rgb;      
    vec3 bloomColor = texture(postBloom, texCoord).rgb;
    hdrColor += bloomColor; // additive blending

    // tone mapping
    vec3 result = ACESFilm(hdrColor);

    // also gamma correct while we're at it       
    result = pow(result, vec3(0.4545));
    outColor = vec4(result, 1.0);
}