#ifndef UTIL_RAYCONE
#define UTIL_RAYCONE

// https://github.com/Apress/Ray-Tracing-Gems-II/blob/main/Chapter_07/Raytracing.hlsl
vec4 UVDerivsFromRayCone(vec3 vRayDir, vec3 vWorldNormal, float vRayConeWidth, vec2 aUV[3], vec3 aPos[3], mat3x3 matWorld)
{
    vec2 vUV10 = aUV[1]-aUV [0];
    vec2 vUV20 = aUV[2]-aUV [0];
    float fQuadUVArea = abs(vUV10.x*vUV20.y - vUV20.x*vUV10.y);

    vec3 vEdge10 = (aPos[1]-aPos[0]) * matWorld;
    vec3 vEdge20 = (aPos[2]-aPos[0]) * matWorld;
    vec3 vFaceNrm = cross(vEdge10 , vEdge20);
    float fQuadArea = length(vFaceNrm);

    float fNormalTerm = abs(dot(vRayDir ,vWorldNormal));
    float fPrjConeWidth = vRayConeWidth/fNormalTerm;
    float fVisibleAreaRatio = (fPrjConeWidth*fPrjConeWidth)/fQuadArea;

    float fVisibleUVArea = fQuadUVArea*fVisibleAreaRatio;
    float fULength = sqrt(fVisibleUVArea);
    return vec4(fULength ,0,0, fULength);
}

vec2 UVAreaFromRayCone(vec3 vRayDir, vec3 vWorldNormal, float vRayConeWidth, vec2 aUV[3],vec3 aPos[3], mat3x3 matWorld)
{
	vec2 vUV10 = aUV[1]-aUV[0];
	vec2 vUV20 = aUV[2]-aUV[0];
	float fTriUVArea = abs(vUV10.x*vUV20.y - vUV20.x*vUV10.y);

	vec3 vEdge10 = (aPos[1] - aPos[0]) * matWorld;
	vec3 vEdge20 = (aPos[2] - aPos[0]) * matWorld;

	vec3 vFaceNrm = cross(vEdge10, vEdge20);
	float fTriLODOffset = 0.5f * log2(fTriUVArea/length(vFaceNrm));
	float fDistTerm = vRayConeWidth * vRayConeWidth;
	float fNormalTerm = dot(vRayDir, vWorldNormal);

	return vec2(fTriLODOffset, fDistTerm/(fNormalTerm*fNormalTerm));
}

float UVAreaToTexLOD(ivec2 vTexSize, vec3 vUVAreaInfo)
{
	return vUVAreaInfo.x + 0.5f*log2(vTexSize.x * vTexSize.y * vUVAreaInfo.y);
}

#endif
