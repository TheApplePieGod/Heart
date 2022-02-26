#version 460

layout(location = 0) in vec2 texCoord;

layout(binding = 1) uniform sampler2D image;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(texture(image, texCoord).rgb, 1.0);
}