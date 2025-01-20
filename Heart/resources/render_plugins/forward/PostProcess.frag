#version 460

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0, input_attachment_index = 0) uniform subpassInput inputColor;

layout(push_constant) uniform PushConstants
{
    uint tonemapEnable;
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

void main()
{
    vec3 finalColor = subpassLoad(inputColor).rgb;

    if (constants.tonemapEnable == 1)
    {
        // Tonemapping
        finalColor = ACESFilm(finalColor);

        // Gamma correction     
        finalColor = pow(finalColor, vec3(0.4545));
    }

    outColor = vec4(finalColor, 1.0);
}
