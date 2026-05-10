#pragma once
#include "age.hpp"

namespace age::graphics::bake
{
	using binding_config_t = binding_slot_config<
		binding_slot<
			"root_constants",
			D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::constant_buffer<shared_type::root_constants>,
			how::root_constant,
			where::b<1, 0>>,

		binding_slot<
			"env_light_radiance_luminance_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<float>,
			how::root_descriptor,
			where::t<0, 0>>,

		binding_slot<
			"env_light_radiance_luminance_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<float>,
			how::root_descriptor,
			where::u<0, 0>>,

		binding_slot<
			"env_light_margianl_cdf_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<float>,
			how::root_descriptor,
			where::t<1, 0>>,

		binding_slot<
			"env_light_margianl_cdf_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<float>,
			how::root_descriptor,
			where::u<1, 0>>,

		binding_slot<
			"env_light_conditional_cdf_srv",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<float>,
			how::root_descriptor,
			where::t<2, 0>>,

		binding_slot<
			"env_light_conditional_cdf_uav",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::rw_structured_buffer<float>,
			how::root_descriptor,
			where::u<2, 0>>,

		binding_slot<
			"equirect_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::equirect>,
			how::static_sampler,
			where::s<0>>,

		binding_slot<
			"linear_clamp_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::linear_clamp>,
			how::static_sampler,
			where::s<1>>,

		binding_slot<
			"linear_wrap_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_ALL,
			what::sampler<defaults::static_sampler_desc::linear_wrap>,
			how::static_sampler,
			where::s<2>>>;

	struct pipeline
	{
		root_signature::handle h_root_sig;

		binding_config_t::reg_b<1, 0> root_constants;

		binding_config_t::reg_t<0, 0> env_light_radiance_luminance_srv;
		binding_config_t::reg_u<0, 0> env_light_radiance_luminance_uav;
		binding_config_t::reg_t<1, 0> env_light_marginal_cdf_srv;
		binding_config_t::reg_u<1, 0> env_light_margianl_cdf_uav;
		binding_config_t::reg_t<2, 0> env_light_conditional_cdf_srv;
		binding_config_t::reg_u<2, 0> env_light_conditional_cdf_uav;

		srv_desc_handle h_env_light_input_srv_desc;
		srv_desc_handle h_env_light_radiance_srv_desc;
		uav_desc_handle h_env_light_irradiance_uav_desc;

		age::vector<uav_desc_handle> h_env_light_radiance_uav_desc_vec;
		age::vector<uav_desc_handle> h_env_light_prefilter_uav_desc_vec;

		resource_handle h_env_light_input_tex;
		resource_handle h_env_light_radiance_luminance_buffer;
		resource_handle h_env_light_marginal_cdf_buffer;
		resource_handle h_env_light_conditional_cdf_buffer;

		pso::handle h_pso_env_light_radiance;
		pso::handle h_pso_env_light_irradiance;
		pso::handle h_pso_env_light_prefilter;
		pso::handle h_pso_env_light_build_marginal_cdf;
		pso::handle h_pso_env_light_build_conditional_cdf;
		pso::handle h_pso_down_sample_cube;
	};

	void
	init() noexcept;

	void
	deinit() noexcept;
}	 // namespace age::graphics::bake