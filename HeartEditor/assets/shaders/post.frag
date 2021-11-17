#version 460

layout(location = 0) in vec2 texCoord;
layout(location = 1) in flat int entityId;

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outEntityId;

layout(binding = 2) uniform sampler2D samp;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));	
}

void main() {
    outColor = texture(samp, texCoord);
    //if (outColor.a < 0.01)
    //    discard;
    outEntityId = float(entityId);
}