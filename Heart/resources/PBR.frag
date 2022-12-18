#version 460

#include "PBR.glsl"

layout(location = 0) out vec4 outHDRColor;
layout(location = 1) out vec4 outBrightColor;
layout(location = 2) out float outEntityId;

void main() {
    outHDRColor = GetFinalColor();
    outEntityId = float(entityId);
}