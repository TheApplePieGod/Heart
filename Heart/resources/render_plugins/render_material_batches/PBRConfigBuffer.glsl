struct PBRConfigData {
    uint ssaoEnable;
    vec3 padding;
};

layout(binding = 4) readonly uniform PBRConfigBuffer {
    PBRConfigData data;
} pbrConfigBuffer;
