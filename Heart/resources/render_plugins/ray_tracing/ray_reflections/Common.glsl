#include "../../util/Halton.glsl"

layout(push_constant) uniform PushConstants
{
    HaltonData haltonData;
    float mipSpreadAngle;
} constants;

struct HitPayload
{
    vec4 hitValue;
    vec2 rayCone;
};
