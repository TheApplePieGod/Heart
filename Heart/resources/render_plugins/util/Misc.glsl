#ifndef UTIL_MISC
#define UTIL_MISC

vec3 ComputeWorldPositionFromDepth(vec2 uv, float depth, mat4 invViewProj)
{
    uv = uv * 2.0 - 1.0;
    vec4 positionS = vec4(uv, depth, 1.0f);
    vec4 positionW = invViewProj * positionS;
    return positionW.xyz / positionW.w;
}

// https://wickedengine.net/2019/09/22/improved-normal-reconstruction-from-depth/
vec3 ComputeViewPositionFromDepth(vec2 uv, float depth, mat4 invProj)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0 - uv.y) * 2.0f - 1.0f;
    vec4 positionS = vec4(x, y, depth, 1.0f);
    vec4 positionV = invProj * positionS;
    return positionV.xyz / positionV.w;
}

float Luminance(vec3 rgb)
{
    return max(dot(rgb, vec3(0.299, 0.587, 0.114)), 0.0001);
}

// https://github.com/LouisBavoil/ThreadGroupIDSwizzling/blob/master/ThreadGroupTilingX.hlsl
// https://developer.nvidia.com/blog/optimizing-compute-shaders-for-l2-locality-using-thread-group-id-swizzling/
uvec2 ThreadGroupTilingX(
	const uvec2 dipatchGridDim,		// Arguments of the Dispatch call (typically from a ConstantBuffer)
	const uvec2 ctaDim,			// Already known in HLSL, eg:[numthreads(8, 8, 1)] -> uint2(8, 8)
	const uint maxTileWidth,		// User parameter (N). Recommended values: 8, 16 or 32.
	const uvec2 groupThreadID,		// SV_GroupThreadID
	const uvec2 groupId			// SV_GroupID
)
{
	// A perfect tile is one with dimensions = [maxTileWidth, dipatchGridDim.y]
	const uint Number_of_CTAs_in_a_perfect_tile = maxTileWidth * dipatchGridDim.y;

	// Possible number of perfect tiles
	const uint Number_of_perfect_tiles = dipatchGridDim.x / maxTileWidth;

	// Total number of CTAs present in the perfect tiles
	const uint Total_CTAs_in_all_perfect_tiles = Number_of_perfect_tiles * maxTileWidth * dipatchGridDim.y;
	const uint vThreadGroupIDFlattened = dipatchGridDim.x * groupId.y + groupId.x;

	// Tile_ID_of_current_CTA : current CTA to TILE-ID mapping.
	const uint Tile_ID_of_current_CTA = vThreadGroupIDFlattened / Number_of_CTAs_in_a_perfect_tile;
	const uint Local_CTA_ID_within_current_tile = vThreadGroupIDFlattened % Number_of_CTAs_in_a_perfect_tile;
	uint Local_CTA_ID_y_within_current_tile;
	uint Local_CTA_ID_x_within_current_tile;

	if (Total_CTAs_in_all_perfect_tiles <= vThreadGroupIDFlattened)
	{
		// Path taken only if the last tile has imperfect dimensions and CTAs from the last tile are launched. 
		uint X_dimension_of_last_tile = dipatchGridDim.x % maxTileWidth;
		Local_CTA_ID_y_within_current_tile = Local_CTA_ID_within_current_tile / X_dimension_of_last_tile;
		Local_CTA_ID_x_within_current_tile = Local_CTA_ID_within_current_tile % X_dimension_of_last_tile;
	}
	else
	{
		Local_CTA_ID_y_within_current_tile = Local_CTA_ID_within_current_tile / maxTileWidth;
		Local_CTA_ID_x_within_current_tile = Local_CTA_ID_within_current_tile % maxTileWidth;
	}

	const uint Swizzled_vThreadGroupIDFlattened =
		Tile_ID_of_current_CTA * maxTileWidth +
		Local_CTA_ID_y_within_current_tile * dipatchGridDim.x +
		Local_CTA_ID_x_within_current_tile;

	uvec2 SwizzledvThreadGroupID;
	SwizzledvThreadGroupID.y = Swizzled_vThreadGroupIDFlattened / dipatchGridDim.x;
	SwizzledvThreadGroupID.x = Swizzled_vThreadGroupIDFlattened % dipatchGridDim.x;

	uvec2 SwizzledvThreadID;
	SwizzledvThreadID.x = ctaDim.x * SwizzledvThreadGroupID.x + groupThreadID.x;
	SwizzledvThreadID.y = ctaDim.y * SwizzledvThreadGroupID.y + groupThreadID.y;

	return SwizzledvThreadID.xy;
}

#endif
