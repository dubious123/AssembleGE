#include "hrp_common.asli"

[numthreads(1, 1, 1)] void
main_cs()

{
	const gist_data data = gist::load_data();

	const uint32_3 ray_count_ideal = gist::adaptive::load_ray_count_ideal<true>(data);

	const uint32 new_born_ray_count = min(ray_count_ideal[GIST_ADAPTIVE_RAY_TYPE_NEW_BORN], data.adaptive_ray_budget);
	const uint32 specular_ray_count = min(ray_count_ideal[GIST_ADAPTIVE_RAY_TYPE_SPECULAR], data.adaptive_ray_budget - new_born_ray_count);
	const uint32 variance_ray_count = min(ray_count_ideal[GIST_ADAPTIVE_RAY_TYPE_VARIANCE], data.adaptive_ray_budget - new_born_ray_count - specular_ray_count);
	gist::adaptive::set_ray_count_ideal(data, zero<uint32_3>());
	gist::adaptive::set_ray_entry_alloc_counter(data, zero<uint32_2>());

	gist::adaptive::set_ray_entry_cap(data, uint32_2(new_born_ray_count + variance_ray_count, specular_ray_count));

	gist::adaptive::set_ray_entry_prob(data,
									   float(new_born_ray_count) / float(max(ray_count_ideal.x, 1u)),
									   float(specular_ray_count) / float(max(ray_count_ideal.y, 1u)),
									   float(variance_ray_count) / float(max(ray_count_ideal.z, 1u)));
}