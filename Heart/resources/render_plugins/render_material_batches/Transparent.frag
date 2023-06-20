#version 460

#include "PBR.glsl"
#include "../transparency_composite/Util.glsl"

layout(location = 0) out vec4 outHDRColor;
layout(location = 1) out vec4 outAccumColor;
layout(location = 2) out float outRevealColor;
layout(location = 3) out float outEntityId;

// https://learnopengl.com/Guest-Articles/2020/OIT/Weighted-Blended
void main() {
    vec4 color = GetFinalColor();
    outEntityId = float(entityId);

    outAccumColor = computeAccumColor(color, -viewPos.z);
    outRevealColor = computeRevealColor(color);
}
