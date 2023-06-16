#version 460

#include "../frame_data/FrameBuffer.glsl"

layout(location = 0) out float outColor;

layout(location = 0) in vec2 texCoord;

struct SSAOData {
    vec4 samples[64];
    uint kernelSize;
    float radius;
    float bias;
    float padding;
    vec2 renderSize;
};

layout(binding = 1) readonly uniform SSAODataBuffer {
    SSAOData data;
} ssaoDataBuffer;

layout(binding = 2) uniform sampler2D depthTex;
layout(binding = 3) uniform sampler2D noiseTex;
layout(binding = 4) uniform sampler2D normalTex;

// https://wickedengine.net/2019/09/22/improved-normal-reconstruction-from-depth/
vec3 ComputePosition(vec2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0 - uv.y) * 2.0f - 1.0f;
    vec4 positionS = vec4(x, y, depth, 1.0f);
    vec4 positionV = frameBuffer.data.invProj * positionS;
    return positionV.xyz / positionV.w;
}

// https://learnopengl.com/Advanced-Lighting/SSAO
void main()
{
    float depth = texture(depthTex, texCoord).r;
    vec3 position = ComputePosition(texCoord, depth);
    vec3 normal = texture(normalTex, texCoord).rgb;
    
    vec2 noiseScale = ssaoDataBuffer.data.renderSize * 0.25;
    vec3 randomVec = normalize(texture(noiseTex, texCoord * noiseScale).xyz);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // Compute total occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < ssaoDataBuffer.data.kernelSize; i++)
    {
        // Get sample position
        vec3 samplePos = TBN * ssaoDataBuffer.data.samples[i].xyz; // from tangent to view-space
        samplePos = position + samplePos * ssaoDataBuffer.data.radius; 
        
        // Project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = frameBuffer.data.proj * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
        offset.y = 1 - offset.y; // Invert y tex coordinate
        
        // Get sample depth
        float sampleDepth = texture(depthTex, offset.xy).r;
        sampleDepth = ComputePosition(offset.xy, sampleDepth).z;
        
        // Range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, ssaoDataBuffer.data.radius / abs(position.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + ssaoDataBuffer.data.bias ? 1.0 : 0.0) * rangeCheck;           
    }
    occlusion = 1.0 - (occlusion / ssaoDataBuffer.data.kernelSize);

    outColor = occlusion;
}
