#version 460

layout(location = 0) in vec2 texCoord;
layout(location = 1) in flat int entityId;

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outEntityId;
layout(location = 2) out float outDepth;
layout(location = 3) out vec4 outAlbedo;

layout(binding = 2) uniform sampler2D samp;

// todo: buffer
const float NEAR_PLANE = 0.1f;
const float FAR_PLANE = 1000.f;
float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));	
}

void main() {
    //outColor = texture(samp, texCoord);
    //if (outColor.a < 0.01)
    //    discard;
    outColor = vec4(0.f);
    outAlbedo = texture(samp, texCoord);
    outDepth = linearDepth(gl_FragCoord.z);
    outEntityId = float(entityId);
}