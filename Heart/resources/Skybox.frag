#version 460

layout(location = 0) in vec3 localPos;

layout(location = 0) out vec4 outHDRColor;
layout(location = 1) out vec4 outBrightColor;

layout(binding = 1) uniform samplerCube environmentMap;

void main() {
    outHDRColor = vec4(textureLod(environmentMap, localPos, 0.0).rgb, 1.f);
    
    // Output to the bright color attachment if it is above a certain threshold
    float brightness = dot(outHDRColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        outBrightColor = vec4(outHDRColor.rgb, 1.0);
    else
        outBrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}