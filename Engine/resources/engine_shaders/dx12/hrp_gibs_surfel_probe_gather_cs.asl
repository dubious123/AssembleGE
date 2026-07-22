#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	gibs_data		   data			= gibs::load_data();
	gibs_lut_data	   lut_data		= gibs::load_lut_data();
	byte_array<uint32> alive_id_arr = gibs::probe::alive_id_arr_curr(data);

	rw_structured_buffer<gibs_surfel_probe>		 probe_buffer  = global_resource_buffer[data.h_surfel_probe_buffer_uav_id];
	rw_structured_buffer<gibs_surfel_probe_msme> msme_buffer   = global_resource_buffer[data.h_surfel_probe_msme_buffer_uav_id];
	structured_buffer<gibs_cell_surfel>			 surfel_buffer = global_resource_buffer[data.h_cell_surfel_buffer_srv_id];
	byte_array<uint32>							 surfel_id_arr = gibs::cell::cell_to_surfel_id_arr(data);

	if (thread_id >= alive_id_arr.size())
	{
		return;
	}

	const uint32 probe_id = alive_id_arr[thread_id];

	gibs_surfel_probe probe = probe_buffer[probe_id];

	const bool is_new_born = probe.is_new_born();

	const uint32				 cell_id	  = gibs::cell::calc_id(data, lut_data, probe.position);
	const gibs_cell_surfel_entry surfel_entry = gibs::cell::surfel_entry_arr(data)[cell_id];


	half depth_min[GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE];

	for (uint32 i = 0; i < GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE; ++i)
	{
		depth_min[i] = 1.h;
	}

	const float3 probe_normal = decode_oct_snorm16(probe.normal_oct_snorm16);
	const float	 probe_radius = gibs::calc_cell_size(data, lut_data, probe.position);

	float  max_contribution_near		   = 0.f;
	uint32 max_contribution_near_surfel_id = invalid_id_uint32;
	float  max_contribution_far			   = 0.f;
	uint32 max_contribution_far_surfel_id  = invalid_id_uint32;

	float4 irradiance_sum = zero<float4>();
	float4 radiance_sum	  = zero<float4>();

	float surfel_near_coverage = 0.f;
	float surfel_far_coverage  = 0.f;

	float4 sh_coverage_far = zero<float4>();

	for (uint32 i = 0; i < /*min(surfel_entry.surfel_count, 128),*/ surfel_entry.surfel_count; ++i)
	{
		const uint32	 surfel_id = surfel_id_arr[surfel_entry.offset + i];
		gibs_cell_surfel surfel	   = surfel_buffer[surfel_id];

		const float3 surfel_normal	  = decode_oct_snorm16(surfel.normal_oct_snorm16);
		const float	 surfel_far_range = gibs::calc_cell_size(data, lut_data, surfel.position);

		const float3 rel	 = surfel.position - probe.position;
		const float3 dir	 = normalize(rel);
		const float	 dist_sq = dot(rel, rel);
		const float	 dist	 = sqrt(dist_sq);

		const float sh_depth = is_new_born ? 1.f : max(0.f, sh1_eval_scalar(probe.depth_sh, -dir));

		float visibility = probe_radius * sh_depth > epsilon_1e4
							 ? 1.f - smoothstep(probe_radius * sh_depth, probe_radius * sh_depth * GIBS_PROBE_VIS_FADE_RATIO, dist)
							 : 1.f;

		const bool same_face = dot(probe_normal, surfel_normal) > 0.9f
						   and dot(probe_normal, dir) < 0.1f;

		if (dot(probe_normal, rel) < 0.f)
		{
			// surfel at back
			// no depth update
		}
		else if (dist >= probe_radius)
		{
			// to far
			// no depth update
		}
		else if (same_face)
		{
			// no depth update
		}
		else
		{
			float depth = dist;
			if (dist < surfel.radius and (dot(surfel_normal, -rel) >= 0.f))
			{
				float2		chebyshev;
				const float surfel_visibility = gibs::calc_surfel_visibility<false, gibs_cell_surfel>(data, surfel_id, surfel, probe.position, chebyshev);

				visibility = min(visibility, surfel_visibility);

				depth = dist - (1.f - surfel_visibility) * chebyshev.x * surfel.radius;
			}

			const float3 rel_local = mul(rel, gen_world_normal_transform(probe_normal));	// mul (rel, inv(gen_world_normal_transform_t) )

			float3 dir_local = normalize(rel_local);
			dir_local.y		 = max(dir_local.y, epsilon_1e4);
			dir_local		 = normalize(dir_local);

			const uint32 idx = gibs::calc_atlas_tile_local_idx(dir_local);

			depth_min[idx] = min(depth_min[idx], half(min(depth / probe_radius, 1.f)));
		}

		const float near_contribution = gibs::calc_near_contribution(dist, surfel.radius, surfel_normal, probe_normal)
									  * visibility;


		if (near_contribution > 0.f)
		{
			irradiance_sum += float4(decode_r11g11b10(surfel.irradiance_r11g11b10), 1.f)
							* near_contribution
							* smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.recycle_data.frame_since_born()));

			surfel_near_coverage += near_contribution;
		}

		const float far_contribution = gibs::calc_far_contribution(rel, dir, surfel_normal, surfel.radius, surfel_far_range, probe_normal)
									 * visibility;

		if (far_contribution > 0.f)
		{
			radiance_sum += float4(decode_r11g11b10(surfel.radiance_r11g11b10), 1.f)
						  * far_contribution
						  * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.recycle_data.frame_since_born()));

			surfel_far_coverage += far_contribution;

			sh1_project_add_scalar(sh_coverage_far, dir, far_contribution);
		}

		if (max_contribution_near < near_contribution)
		{
			max_contribution_near_surfel_id = surfel_id;
			max_contribution_near			= near_contribution;
		}

		if (max_contribution_far < far_contribution)
		{
			max_contribution_far_surfel_id = surfel_id;
			max_contribution_far		   = far_contribution;
		}

		if (near_contribution > 0.f or far_contribution > 0.f)
		{
			gibs::cell::set_surfel_ref(data, surfel_entry.offset + i);
		}
	}


	const float3 irradiance_near = irradiance_sum.w > 0.f ? irradiance_sum.xyz / irradiance_sum.w : zero<float3>();
	const float3 irradiance_far	 = radiance_sum.w > 0.f ? pi * radiance_sum.xyz / radiance_sum.w : zero<float3>();

	const float near_conf = irradiance_sum.w / 1.f * GIBS_NEAR_CONTRIBUTION_TRUST_BIAS;
	const float far_conf  = radiance_sum.w / 1.f;

	const float3 irradiance = lerp(irradiance_near, irradiance_far, far_conf / (near_conf + far_conf + epsilon_1e4));

	gibs_surfel_probe_msme msme = msme_buffer[probe_id];

	probe.irradiance_r11g11b10 = encode_r11g11b10(gibs::update_msme(irradiance, msme));

	probe.coverage_near	  = half(surfel_near_coverage);
	probe.coverage_far_sh = half4(sh_coverage_far);

	{
		float4			   depth_sh = zero<float4>();
		static const float d_omega	= pi_2 / float(GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE);

		const float3x3 world_to_local = gen_world_normal_transform_t(probe_normal);
		for (uint32 i = 0; i < GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE; ++i)
		{
			const float2 uv = saturate((float2(i % GIBS_ATLAS_TILE_SIZE, i / GIBS_ATLAS_TILE_SIZE) + 0.5f) / float(GIBS_ATLAS_TILE_SIZE));

			const float3 dir_local = decode_world_hemi_octahedral(uv * 2.f - 1.f);

			const float3 dir_world = mul(dir_local, world_to_local);	// mul(inv(world_to_local), dir_local) == mul(dir_local, world_to_local)
			sh1_project_add_scalar(depth_sh, dir_world, float(depth_min[i]) * d_omega);
		}

		// add 1.f for all hemi_lower

		// integral_hemi_lower(c)
		depth_sh.x += pi_2 * 0.28209479177387814347403972578039f;

		// integral_hemi_lower(c * dot(dir, basis))
		// c * dot(integral_hemi_lower(dir), basis)
		// c * dot(pi * (-normal), basis)
		depth_sh.y += pi * 0.48860251190291992158638462283835f * (-probe_normal.y);
		depth_sh.z += pi * 0.48860251190291992158638462283835f * (-probe_normal.z);
		depth_sh.w += pi * 0.48860251190291992158638462283835f * (-probe_normal.x);

		probe.depth_sh = half4(depth_sh);
	}

	probe_buffer[probe_id] = probe;
	msme_buffer[probe_id]  = msme;

	const float2 rng = random_pcg2d(uint32_2(probe_id, frame_index));

	attr_branch()

	if (gibs::debug::freeze_spawn_kill(data) is_false)
	{
		if (surfel_near_coverage > GIBS_CELL_SURFEL_KILL_COVERAGE_NEAR or surfel_far_coverage > GIBS_CELL_SURFEL_KILL_COVERAGE_FAR)
		{
			const float kill_prob_near = (surfel_near_coverage - GIBS_CELL_SURFEL_KILL_COVERAGE_NEAR) / float(GIBS_CELL_SURFEL_KILL_COVERAGE_NEAR);
			const float kill_prob_far  = (surfel_far_coverage - GIBS_CELL_SURFEL_KILL_COVERAGE_FAR) / float(GIBS_CELL_SURFEL_KILL_COVERAGE_FAR);
			const float kill_prob	   = max(kill_prob_near, kill_prob_far) * GIBS_KILL_PROB_FACTOR;

			const uint32 kill_surfel_id = kill_prob_near > kill_prob_far ? max_contribution_near_surfel_id : max_contribution_far_surfel_id;

			if (rng.x < kill_prob)
			{
				gibs::cell::set_surfel_kill(data, cell_id, kill_prob, kill_surfel_id);
			}
		}
	}
}