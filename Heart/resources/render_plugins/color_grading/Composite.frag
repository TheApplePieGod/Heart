#version 460

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 texCoord;

layout(binding = 0) uniform sampler2D hdrTex;

layout(push_constant) uniform PushConstants
{
    bool tonemapEnable;
} constants;

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

// https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom
void main()
{
    vec3 finalColor = texture(hdrTex, texCoord).rgb;

    if (constants.tonemapEnable) {
        // Tonemapping
        finalColor = ACESFilm(finalColor);

        // Gamma correction     
        finalColor = pow(finalColor, vec3(0.4545));
    }

    outColor = vec4(finalColor, 1.f);
}
