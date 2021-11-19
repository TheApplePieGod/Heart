#version 460

#include "PBR.glsl"

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outEntityId;

void main() {
    outColor = GetAlbedo();
    outEntityId = float(entityId);
}