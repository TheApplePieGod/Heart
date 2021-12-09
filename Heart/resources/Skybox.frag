#version 460

layout(location = 0) in vec3 localPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform samplerCube environmentMap;

void main() {
    vec3 envColor = textureLod(environmentMap, localPos, 0.0).rgb;
    
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
  
    outColor = vec4(envColor, 1.0);
}