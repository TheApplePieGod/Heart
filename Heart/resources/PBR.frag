#version 460

#include "PBR.glsl"

layout(location = 0) out vec4 outHDRColor;
layout(location = 1) out vec4 outBrightColor;
layout(location = 2) out float outEntityId;

void main() {
    outHDRColor = GetFinalColor();

    // Output to the bright color attachment if it is above a certain threshold
    float brightness = dot(outHDRColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > frameBuffer.data.bloomThreshold)
        outBrightColor = vec4(outHDRColor.rgb, 1.0);
    else
        outBrightColor = vec4(0.0, 0.0, 0.0, 1.0);

    outEntityId = float(entityId);
}