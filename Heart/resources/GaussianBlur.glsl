layout(binding = 1) uniform sampler2D image;

const float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

// adapted from https://learnopengl.com/Advanced-Lighting/Bloom
vec3 GaussianHorizontal(vec2 texCoords, uint mip, float blurScale, float blurStrength) {
    vec2 texOffset = 1.0 / textureSize(image, int(mip)) * blurScale; // Size of single texel
    vec3 originialResult = textureLod(image, texCoords, mip).rgb * weights[0]; // Current fragment's contribution
    vec3 result = originialResult;

    for (int i = 1; i < 5; i++)
    {
        result += textureLod(image, texCoords + vec2(texOffset.x * i, 0.0), mip).rgb * weights[i] * blurStrength;
        result += textureLod(image, texCoords - vec2(texOffset.x * i, 0.0), mip).rgb * weights[i] * blurStrength;
    }

    return result;
}

vec3 GaussianVertical(vec2 texCoords, uint mip, float blurScale, float blurStrength) {
    vec2 texOffset = 1.0 / textureSize(image, int(mip)) * blurScale; // Size of single texel
    vec3 result = textureLod(image, texCoords, mip).rgb * weights[0]; // Current fragment's contribution

    for (int i = 1; i < 5; i++)
    {
        result += textureLod(image, texCoords + vec2(0.0, texOffset.y * i), mip).rgb * weights[i] * blurStrength;
        result += textureLod(image, texCoords - vec2(0.0, texOffset.y * i), mip).rgb * weights[i] * blurStrength;
    }
    
    return result;
}