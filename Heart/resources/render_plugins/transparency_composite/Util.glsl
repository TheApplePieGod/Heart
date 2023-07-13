vec4 computeAccumColor(vec4 color, float depthZ) {
    float weight =
        max(min(1.0, max(max(color.r, color.g), color.b) * color.a), color.a) *
        clamp(0.03 / (1e-5 + pow(depthZ / 200, 4.0)), 1e-2, 3e3);

    return vec4(color.rgb * color.a, color.a) * weight;
}

float computeRevealColor(vec4 color) {
    return color.a;
}
