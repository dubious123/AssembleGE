#include "forward_plus_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	float2 uv = pos.xy * inv_backbuffer_size;

	texture_2d<float4> main_buffer_tex = global_resource_buffer[post_buffer_texture_id];

	float3 color = sample(main_buffer_tex, get_linear_clamp_sampler(), uv).rgb;
	color		 = max(color, 0.0);
	color		 = color * (100.0 / 10000.0);	 // (sdr_peak / pq_peak)

	color = rec709_to_rec2020(color);
	color = linear_to_pq(color);

	if (ddgi_enabled())
	{
		const ddgi_data data = load_ddgi_data();

		{
			texture_2d<float3> irradiance_atlas = global_resource_buffer[data.irradiance_atlas_srv_id];
			float2			   debug_uv			= pos.xy * inv_backbuffer_size;
			if (debug_uv.x > 0.75 and debug_uv.y < 0.25)
			{
				float2 uv = (debug_uv - float2(0.75f, 0.f)) * 4;
				// font_uv.y	   = 1.f - font_uv.y;
				float3 rgb = sample_level(irradiance_atlas, get_linear_clamp_sampler(), uv, 0);
				return float4(rgb, 1.f);
			}
		}

		{
			texture_2d<float2> visibility_atlas = global_resource_buffer[data.visibility_atlas_srv_id];
			float2			   debug_uv			= pos.xy * inv_backbuffer_size;
			if (debug_uv.x > 0.5 and debug_uv.x < 0.75 and debug_uv.y < 0.25)
			{
				float2 uv = (debug_uv - float2(0.5f, 0.f)) * 4;
				// font_uv.y	   = 1.f - font_uv.y;
				float2 rg = sample_level(visibility_atlas, get_linear_clamp_sampler(), uv, 0);
				return float4(rg, 0.f, 1.f);
			}
		}
	}


	return float4(color, 1.0);
}

// float4
// main_ps(float4 pos sv_position) sv_target_0
//{
//	// float2 uv = (pos.xy + 0.5) * inv_backbuffer_size;
//	float2 uv = pos.xy * inv_backbuffer_size;
//
//	texture_2d<float4> main_buffer_tex = global_resource_buffer[post_buffer_texture_id];
//
//	float3 color  = sample(main_buffer_tex, linear_clamp_sampler, uv).rgb;
//	color		  = max(color, 0.0);
//	const float a = 2.51f;
//	const float b = 0.03f;
//	const float c = 2.43f;
//	const float d = 0.59f;
//	const float e = 0.14f;
//	color		  = saturate((color * (a * color + b)) / (color * (c * color + d) + e));
//
//
//	// texture_2d<float> shadow_atlas = global_resource_buffer[shadow_atlas_id];
//	// float2			  debug_uv	   = pos.xy * inv_backbuffer_size;
//	// if (debug_uv.x > 0.75 && debug_uv.y > 0.75)
//	//{
//	//	float2 shadow_uv = (debug_uv - 0.75) * 4.0;
//	//	float  depth	 = sample(shadow_atlas, linear_clamp_sampler, shadow_uv).r;
//
//	//	return float4(depth, depth, depth, 1.0) * float4(10.f, 0, 0, 1);
//	//}
//
//
//	return float4(color, 1.f);
// }