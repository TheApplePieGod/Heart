#version 460

layout(location = 0) in vec2 texCoord;
layout(location = 1) in flat int entityId;
layout(location = 2) in float depth;

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outEntityId;

layout(binding = 2) uniform sampler2D samp;

void main() {
    //outColor = texture(samp, texCoord);
    //if (outColor.a < 0.01)
    //    discard;
    outColor = texture(samp, texCoord);
    outEntityId = float(entityId);
}