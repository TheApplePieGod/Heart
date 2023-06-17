#version 460

#include "../frame_data/FrameBuffer.glsl"
#include "../transparency_composite/Util.glsl"

layout(location = 0) out vec4 outAccumColor;
layout(location = 1) out float outRevealColor;

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;

const float SCALE = 10;
const float SCALE_LOG = log(SCALE);
const float VIEW_CELLS = 15;

vec4 grid(vec3 fragPos, float offset, int falloff) {
    // Compute grid scale level based on camera y pos
    float level = log(max(1, abs(frameBuffer.data.cameraPos.y)))/SCALE_LOG + offset;
    float levelf = fract(level);
    float leveli = ceil(level);

    // Compute cell scae
    float scaledi = pow(SCALE, leveli);
    float scaledf = pow(SCALE, level);
    float scaledno = scaledf * pow(SCALE, -offset); // Without offset

    // Scale coordinate based on cell count
    vec2 coord = fragPos.xz / scaledi;

    // Draw the line on cell boundaries
    vec2 derivative = fwidth(coord) * 1.5;
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float lineAlpha = (1.0 - min(line, 1.0));

    // As we approach the next level,  fade out this level
    float falloffFactor = (1.0 - pow(levelf, 5) * falloff);

    // Fade based on cell distance (ignoring offset)
    float dist = length(frameBuffer.data.cameraPos.xz - fragPos.xz);
    float fade = 1.0 - min(dist / (scaledno * VIEW_CELLS), 1);

    // Final color
    float finalAlpha = lineAlpha * falloffFactor * fade;
    vec4 color = vec4(0.6, 0.6, 0.6, finalAlpha);

    float minimumz = 0.005;// min(derivative.y, 1);
    float minimumx = 0.005; //min(derivative.x, 1);
    
    // z axis
    if(fragPos.x > -scaledno * minimumx && fragPos.x < scaledno * minimumx)
        color.xy = vec2(0.0);
    // x axis
    if(fragPos.z > -scaledno * minimumz && fragPos.z < scaledno * minimumz)
        color.yz = vec2(0.0);

    return color;
}

float computeDepth(vec3 fragPos) {
    vec4 clipPos = frameBuffer.data.proj * frameBuffer.data.view * vec4(fragPos, 1.0);
    return (clipPos.z / clipPos.w);
}

void main() {
    // Render when line is on y = 0
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    vec3 pos = nearPoint + t * (farPoint - nearPoint);

    // Output depth manually
    float depth = computeDepth(pos);
    gl_FragDepth = depth;

    // Render two subdivisions of the grid
    vec4 finalColor = grid(pos, 0, 0);
    finalColor += grid(pos, -1, 0);
    finalColor += grid(pos, -2, 1);
    finalColor *= float(t > 0) * 0.333;
    finalColor.a *= 0.7;

    outAccumColor = computeAccumColor(finalColor, depth);
    outRevealColor = computeRevealColor(finalColor);
}
