#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "../frame_data/FrameBuffer.glsl"
#include "../../VertexLayout.glsl"
#include "Common.glsl"

struct ObjectData {
    uint64_t vertexAddress;
    uint64_t indexAddress;
};

// TODO: need constants to define set and binding
struct MaterialData {
    vec4 baseColor;
    vec4 emissiveFactor;
    vec4 texCoordTransform; // [0-1]: scale, [2-3]: offset
    vec4 hasPBRTextures; // [0]: hasAlbedo, [1]: hasMetallicRoughness
    vec4 hasTextures; // [0]: hasNormal, [1]: hasEmissive, [2]: hasOcclusion
    vec4 scalars; // [0]: metalness, [1]: roughness, [2]: alphaClipThreshold
};

struct LightData {
    vec4 position;
    vec4 direction;
    vec4 color;
    uint lightType;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};

#define DIRECTIONAL 1
#define POINT 2

layout(binding = 1) uniform accelerationStructureEXT tlas;

layout(binding = 0, set = 1) readonly buffer ObjectBuffer {
    ObjectData data[];
} objectBuffer;
layout(binding = 1, set = 1) readonly buffer MaterialBuffer {
    MaterialData materials[];
} materialBuffer;
layout(binding = 6, set = 1) readonly buffer LightingBuffer {
    LightData lights[];
} lightingBuffer;

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    Vertex data[];
};
layout(buffer_reference, scalar) readonly buffer IndexBuffer {
    uvec3 data[];
};

layout(binding = 0, set = 2) uniform sampler2D textures[5000];

layout(location = 0) rayPayloadInEXT HitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;
hitAttributeEXT vec3 attribs;

#include "PBR.glsl"

vec4 UVDerivsFromRayCone(
    vec3 vRayDir, vec3 vWorldNormal, float vRayConeWidth, vec2 aUV[3], vec3 aPos[3], mat3x3 matWorld
)
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

void main()
{
    ObjectData objData = objectBuffer.data[gl_InstanceID];
    VertexBuffer vertexBuffer = VertexBuffer(objData.vertexAddress);
    IndexBuffer indexBuffer = IndexBuffer(objData.indexAddress);
  
    // Indices of the triangle
    uvec3 ind = indexBuffer.data[gl_PrimitiveID];
  
    // Vertex of the triangle
    Vertex v0 = vertexBuffer.data[ind.x];
    Vertex v1 = vertexBuffer.data[ind.y];
    Vertex v2 = vertexBuffer.data[ind.z];

    //prd.hitValue = vec3(1.0, 0.0, 0.0);

    // Computing hit position data
    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 pos = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
    vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));
    vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;
    vec3 inNormal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
    vec3 normal = normalize(vec3(inNormal * gl_WorldToObjectEXT));
    vec4 inTangent = v0.tangent * barycentrics.x + v1.tangent * barycentrics.y + v2.tangent * barycentrics.z;
    vec3 tangent = normalize(vec3(inTangent.xyz * gl_WorldToObjectEXT));
    vec3 bitangent = cross(tangent, normal) * inTangent.w;

    float initSpreadAngle = atan((2.0f*tan(70*0.5f))/frameBuffer.data.screenSize.y);
    vec2 rayConeAtOrigin = vec2(0, initSpreadAngle);
	vec2 rayConeAtHit = rayConeAtOrigin + vec2(
		//rayConeAtOrigin.y * length(worldPos - frameBuffer.data.cameraPos.xyz),
		2 * tan(rayConeAtOrigin.y * 0.5) * length(worldPos - frameBuffer.data.cameraPos.xyz),
		initSpreadAngle
    );

    //mat3x3 matWorld = mat3x3(vec3(1,0,0), vec3(0,1,0), vec3(0,0,1));
    vec4 mipDerivs = UVDerivsFromRayCone(
        gl_WorldRayDirectionEXT,
        normal,
        rayConeAtHit.x,
        vec2[3]( v0.texCoord, v1.texCoord, v2.texCoord ),
        vec3[3]( v0.position, v1.position, v2.position ),
        mat3x3(gl_ObjectToWorldEXT)
    );

    vec4 finalColor = GetFinalColor(
        gl_InstanceID,
        texCoord,
        worldPos,
        normal,
        tangent,
        bitangent,
        mipDerivs
    );
    
    prd.hitValue = finalColor.rgb;
}
