float EdgeStoppingWeightNormal(vec3 centerNormal, vec3 sampleNormal, float sigma)
{
    return pow(clamp(dot(centerNormal, sampleNormal), 0.0f, 1.0f), sigma);
}

float EdgeStoppingWeightDepth(float centerDepth, float sampleDepth, float sigma)
{
    return -abs(centerDepth - sampleDepth) / sigma;
}

float EdgeStoppingWeightLuminance(float centerLuminance, float sampleLuminance, float phi)
{
    return -abs(centerLuminance - sampleLuminance) / phi;
}

float EdgeStoppingWeight(float wZ, float wN, float wL)
{
    return exp(wL + wZ) * wN;
}
