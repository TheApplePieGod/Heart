#version 460

#include "PBR.glsl"

layout(location = 0) out vec4 outHDRColor;
layout(location = 3) out float outEntityId;

void main() {
    outHDRColor = GetFinalColor();
    outEntityId = float(entityId);
}
