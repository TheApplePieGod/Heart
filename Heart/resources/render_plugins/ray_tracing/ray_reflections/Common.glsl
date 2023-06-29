#include "../../util/Halton.glsl"

layout(push_constant) uniform PushConstants
{
    HaltonData haltonData;
    float mipSpreadAngle;
} constants;

struct HitPayload
{
    vec3 hitValue;
    vec2 rayCone;
};
