#include "forward_plus_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	// float2 uv = (pos.xy + 0.5) * inv_backbuffer_size;
	float2 uv = pos.xy * inv_backbuffer_size;

	texture_2d<float4> main_buffer_tex = global_resource_buffer[main_buffer_texture_id];

	float3 color  = sample(main_buffer_tex, linear_clamp_sampler, uv).rgb;
	color		  = max(color, 0.0);
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;
	color		  = saturate((color * (a * color + b)) / (color * (c * color + d) + e));

	float2 debug_uv = pos.xy * inv_backbuffer_size;
	if (debug_uv.x > 0.75 && debug_uv.y < 0.25)
	{
		float2 tile_uv = (debug_uv - float2(0.75, 0.0)) * 4.0;
		uint32 tile_x  = min(uint32(tile_uv.x * backbuffer_size.x) / LIGHT_TILE_SIZE, light_tile_count_x - 1);
		uint32 tile_y  = min(uint32(tile_uv.y * backbuffer_size.y) / LIGHT_TILE_SIZE, light_tile_count_y - 1);
		uint32 tile_id = tile_x + tile_y * light_tile_count_x;

		uint32 count = 0;
		for (uint32 w = 0; w < LIGHT_BITMASK_UINT32_COUNT; ++w)
		{
			count += countbits(load_tile_mask(tile_id, w));
		}

		float  t = saturate(count / 1250.f);
		float3 c = float3(t, 1.0 - abs(t * 2.0 - 1.0), 1.0 - t);
		return float4(c, 1.0);
	}

	texture_2d<float> shadow_atlas = global_resource_buffer[shadow_atlas_id];
	/*float2*/ debug_uv			   = pos.xy * inv_backbuffer_size;
	if (debug_uv.x < 0.25 && debug_uv.y < 0.25)
	{
		float2 shadow_uv = debug_uv * 4.0;
		float  depth	 = sample(shadow_atlas, linear_clamp_sampler, shadow_uv).r;

		return float4(depth, depth, depth, 1.0) * float4(10.f, 0, 0, 1);
	}

	return float4(color, 1.f);
}