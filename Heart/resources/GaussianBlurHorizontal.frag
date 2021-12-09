#version 460

layout(location = 0) in vec2 texCoord;

layout(binding = 1) uniform sampler2D image;

layout(location = 0) out vec4 outColor;

const float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

// adapted from https://learnopengl.com/Advanced-Lighting/Bloom
void main() {
    vec2 texOffset = 1.0 / textureSize(image, 0); // Size of single texel
    vec3 result = texture(image, texCoord).rgb * weights[0]; // Current fragment's contribution

    for(int i = 1; i < 5; i++)
    {
        result += texture(image, texCoord + vec2(texOffset.x * i, 0.0)).rgb * weights[i];
        result += texture(image, texCoord - vec2(texOffset.x * i, 0.0)).rgb * weights[i];
    }

    outColor = vec4(result, 1.0);
}