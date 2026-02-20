#pragma once
#include "age.hpp"

namespace age::graphics::render_pipeline::forward_plus
{
	struct frame_data
	{
		float4x4 view_proj;					// 64 bytes
		float4x4 view_proj_inv;				// 64 bytes
		float3	 camera_pos;				// 12
		float	 time;						// 4
		float4	 frustum_planes[6];			// 96
		uint32	 frame_index;				// 4
		float2	 inv_backbuffer_size;		// 8
		uint32	 main_buffer_texture_id;	// 4
											// total: 256 bytes
	};

	struct job_data
	{
		uint32 object_idx;
	};

	struct object_data
	{
		float3 pos;				// 12
		uint32 quaternion;		// 4 10 10 10 2
		half3  scale;			// 6

		uint16 instance_idx;	// 2
		uint16 asset_idx;		// 2
		uint16 extra;			// 2
	};	  // total: 28 bytes

	struct asset_data
	{
		uint32 meshlet_vertex_buffer_offset;
		uint32 meshlet_global_index_buffer_offset;
		uint32 meshlet_primitive_index_buffer_offset;
	};	  // total: 12 bytes

	using binding_config_t = binding_slot_config<
		binding_slot<
			"linear_clamp_sampler",
			D3D12_SAMPLER_FLAG_NONE,
			D3D12_SHADER_VISIBILITY_PIXEL,
			what::sampler<defaults::static_sampler_desc::linear_clamp>,
			how::static_sampler,
			where::s<0>>,

		binding_slot<
			"frame_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::constant_buffer_array<frame_data>,
			how::root_descriptor,
			where::b<0>>,

		binding_slot<
			"asset_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<job_data>,
			how::root_descriptor,
			where::t<0, 1>>,

		binding_slot<
			"job_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<asset_data>,
			how::root_descriptor,
			where::t<0>>,

		binding_slot<
			"object_data_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<object_data>,
			how::root_descriptor,
			where::t<1>>,

		binding_slot<
			"meshlet_header_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<asset::meshlet_header>,
			how::root_descriptor,
			where::t<2>>,

		binding_slot<
			"meshlet_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_ALL,
			what::structured_buffer<asset::meshlet>,
			how::root_descriptor,
			where::t<3>>,

		binding_slot<
			"vertex_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_MESH,
			what::structured_buffer<asset::vertex_p_uv1>,
			how::root_descriptor,
			where::t<4>>,

		binding_slot<
			"meshlet_global_index_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_MESH,
			what::structured_buffer<uint32>,
			how::root_descriptor,
			where::t<5>>,

		binding_slot<
			"meshlet_primitive_index_buffer",
			D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
			D3D12_SHADER_VISIBILITY_MESH,
			what::byte_address_buffer,
			how::root_descriptor,
			where::t<6>>>;

	inline root_signature::handle
	create_root_signature() noexcept
	{
		return binding_config_t::create_root_signature(
			D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus
{
	struct opaque_stage
	{
		binding_config_t::reg_b<0>	  frame_data_buffer;
		binding_config_t::reg_t<0, 1> asset_data_buffer;
		binding_config_t::reg_t<0>	  job_data_buffer;
		binding_config_t::reg_t<1>	  object_data_buffer;
		binding_config_t::reg_t<2>	  meshlet_header_buffer;
		binding_config_t::reg_t<3>	  meshlet_buffer;
		binding_config_t::reg_t<4>	  vertex_buffer;
		binding_config_t::reg_t<5>	  meshlet_global_index_buffer;
		binding_config_t::reg_t<6>	  meshlet_primitive_index_buffer;

		rtv_desc_handle h_main_buffer_rtv_desc;
		dsv_desc_handle h_depth_buffer_dsv_desc;

		graphics::pso::handle h_pso = {};
		ID3D12PipelineState*  p_pso = nullptr;

		inline void
		init(graphics::root_signature::handle h_root_sig) noexcept
		{
			using namespace graphics::pso;

			auto as_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_opaque_as) });
			auto ms_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_opaque_ms) });
			auto ps_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_opaque_ps) });

			h_pso = graphics::pso::create(
				pss_root_signature{ .subobj = g::root_signature_ptr_vec[h_root_sig] },
				pss_as{ .subobj = as_byte_code },
				pss_ms{ .subobj = ms_byte_code },
				pss_ps{ .subobj = ps_byte_code },
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ DXGI_FORMAT_R16G16B16A16_FLOAT }, .NumRenderTargets = 1 } },
				pss_depth_stencil_format{ .subobj = DXGI_FORMAT_D32_FLOAT },
				pss_rasterizer{ .subobj = defaults::rasterizer_desc::backface_cull },
				pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_only },
				pss_blend{ .subobj = defaults::blend_desc::opaque },
				pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
				pss_node_mask{ .subobj = 0 });

			p_pso = g::pso_ptr_vec[h_pso];
		}

		inline void
		bind_rtv_dsv(graphics::resource_handle h_main_buffer,
					 graphics::resource_handle h_depth_buffer) noexcept
		{
			resource::create_view(h_main_buffer,
								  h_main_buffer_rtv_desc,
								  defaults::rtv_view_desc::hdr_rgba16_2d);
			resource::create_view(h_depth_buffer,
								  h_depth_buffer_dsv_desc,
								  defaults::dsv_view_desc::d32_float_2d);
		}

		inline void
		execute(t_cmd_list& cmd_list, uint32 job_count) noexcept
		{
			auto render_pass_rt_desc = defaults::render_pass_rtv_desc::overwrite_preserve(h_main_buffer_rtv_desc);
			auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_clear_preserve(h_depth_buffer_dsv_desc);

			cmd_list.BeginRenderPass(
				1,
				&render_pass_rt_desc,
				&render_pass_ds_desc,
				D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_STENCIL);

			{
				cmd_list.SetPipelineState(p_pso);

				cmd_list.DispatchMesh(job_count, 1, 1);
			}

			cmd_list.EndRenderPass();
		}

		inline void
		deinit() noexcept
		{
			pso::destroy(h_pso);
		}
	};

	struct presentation_stage
	{
		pso::handle			 h_pso = {};
		ID3D12PipelineState* p_pso = nullptr;

		srv_desc_handle h_main_buffer_srv_desc;

		inline void
		init(root_signature::handle h_root_sig) noexcept
		{
			using namespace graphics::pso;

			auto ms_byte_code = shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_presentation_ms) });

			auto&& [ps_byte_code, back_buffer_rt_format] = [&]() {
				auto space = global::get<graphics::interface>().display_color_space();
				switch (space)
				{
				case color_space::srgb:
					return std::tuple{
						shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_presentation_sdr_ps) }),
						DXGI_FORMAT_R8G8B8A8_UNORM_SRGB

					};
				case color_space::hdr:
					return std::tuple{
						shader::get_d3d12_bytecode(shader::shader_handle{ std::to_underlying(shader::e::engine_shader_kind::forward_plus_presentation_hdr10_ps) }),
						DXGI_FORMAT_R10G10B10A2_UNORM
					};
				default:
					AGE_UNREACHABLE("invalid color space");
				}
			}();

			h_pso = graphics::pso::create(
				pss_root_signature{ .subobj = g::root_signature_ptr_vec[h_root_sig] },
				pss_ms{ .subobj = ms_byte_code },
				pss_ps{ .subobj = ps_byte_code },
				pss_primitive_topology{ .subobj = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
				pss_render_target_formats{ .subobj = D3D12_RT_FORMAT_ARRAY{ .RTFormats{ back_buffer_rt_format }, .NumRenderTargets = 1 } },
				pss_depth_stencil_format{ .subobj = DXGI_FORMAT_UNKNOWN },
				pss_rasterizer{ .subobj = defaults::rasterizer_desc::no_cull },
				pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::disabled },
				pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
				pss_node_mask{ .subobj = 0 });

			p_pso = g::pso_ptr_vec[h_pso];
		}

		inline void
		bind_rtv(graphics::resource_handle h_main_buffer) noexcept
		{
			resource::create_view(h_main_buffer,
								  h_main_buffer_srv_desc,
								  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R16G16B16A16_FLOAT));
		}

		inline void
		deinit() noexcept
		{
			pso::destroy(h_pso);
		}

		inline void
		execute(t_cmd_list& cmd_list, render_surface& rs) noexcept
		{
			auto render_pass_rt_desc = defaults::render_pass_rtv_desc::overwrite_preserve(rs.h_rtv_desc());

			cmd_list.BeginRenderPass(
				1,
				&render_pass_rt_desc,
				nullptr,
				D3D12_RENDER_PASS_FLAG_NONE);

			{
				cmd_list.SetPipelineState(p_pso);
				cmd_list.DispatchMesh(1, 1, 1);
			}

			cmd_list.EndRenderPass();
		}
	};

	struct pipeline
	{
		extent_2d<uint16> extent{ .width = 100, .height = 100 };

		resource_handle h_main_buffer  = {};
		resource_handle h_depth_buffer = {};

		ID3D12Resource* p_main_buffer  = nullptr;
		ID3D12Resource* p_depth_buffer = nullptr;

		graphics::root_signature::handle h_root_sig = {};
		ID3D12RootSignature*			 p_root_sig = nullptr;

		opaque_stage	   stage_opaque{};
		presentation_stage stage_presentation{};

		inline void
		init() noexcept
		{
			h_root_sig = create_root_signature();
			p_root_sig = g::root_signature_ptr_vec[h_root_sig];

			create_buffers();
			stage_opaque.init(h_root_sig);
			stage_presentation.init(h_root_sig);
		}

		inline void
		deinit() noexcept
		{
			stage_presentation.deinit();
			stage_opaque.deinit();
			root_signature::destroy(h_root_sig);
		}

		inline void
		execute(t_cmd_list& cmd_list, render_surface& rs, uint32 job_count) noexcept
		{
			auto barrier = resource_barrier{};

			cmd_list.RSSetViewports(1, &rs.default_viewport);
			cmd_list.RSSetScissorRects(1, &rs.default_scissor_rect);
			cmd_list.SetGraphicsRootSignature(p_root_sig);
			cmd_list.SetDescriptorHeaps(1, &g::cbv_srv_uav_desc_pool.p_descriptor_heap);

			barrier.add_transition(rs.get_back_buffer(),
								   D3D12_RESOURCE_STATE_PRESENT,
								   D3D12_RESOURCE_STATE_RENDER_TARGET,
								   D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);

			barrier.add_transition(*p_main_buffer,
								   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
								   D3D12_RESOURCE_STATE_RENDER_TARGET);
			barrier.add_transition(*p_depth_buffer,
								   D3D12_RESOURCE_STATE_DEPTH_READ,
								   D3D12_RESOURCE_STATE_DEPTH_WRITE);

			barrier.apply_and_reset(cmd_list);

			stage_opaque.execute(cmd_list, job_count);

			barrier.add_transition(*p_main_buffer,
								   D3D12_RESOURCE_STATE_RENDER_TARGET,
								   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			barrier.add_transition(*p_depth_buffer,
								   D3D12_RESOURCE_STATE_DEPTH_WRITE,
								   D3D12_RESOURCE_STATE_DEPTH_READ);

			barrier.add_transition(rs.get_back_buffer(),
								   D3D12_RESOURCE_STATE_RENDER_TARGET,
								   D3D12_RESOURCE_STATE_PRESENT,
								   D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

			barrier.apply_and_reset(cmd_list);

			stage_presentation.execute(cmd_list, rs);
		}

	  private:
		void
		create_buffers() noexcept
		{
			h_main_buffer = resource::create_resource(
				{ .d3d12_desc{
					  .Dimension		= D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					  .Alignment		= 0,
					  .Width			= extent.width,
					  .Height			= extent.height,
					  .DepthOrArraySize = 1,
					  .MipLevels		= 1,
					  .Format			= DXGI_FORMAT_R16G16B16A16_FLOAT,
					  .SampleDesc		= { 1, 0 },
					  .Layout			= D3D12_TEXTURE_LAYOUT_UNKNOWN,
					  .Flags			= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET },
				  .clear_value{
					  .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
					  .Color  = { 0.5f, 0.5f, 0.5f, 1.0f } },
				  .initial_state	= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				  .heap_memory_kind = resource::memory_kind::gpu_only,
				  .has_clear_value	= true });

			h_depth_buffer = resource::create_resource(
				{ .d3d12_desc{
					  .Dimension		= D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					  .Alignment		= 0,
					  .Width			= extent.width,
					  .Height			= extent.height,
					  .DepthOrArraySize = 1,
					  .MipLevels		= 1,
					  .Format			= DXGI_FORMAT_D32_FLOAT,
					  .SampleDesc		= { 1, 0 },
					  .Layout			= D3D12_TEXTURE_LAYOUT_UNKNOWN,
					  .Flags			= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL },
				  .clear_value{
					  .Format		= DXGI_FORMAT_D32_FLOAT,
					  .DepthStencil = { .Depth = 1.f, .Stencil = 0 } },
				  .initial_state	= D3D12_RESOURCE_STATE_DEPTH_READ,
				  .heap_memory_kind = resource::memory_kind::gpu_only,
				  .has_clear_value	= true });

			p_main_buffer  = g::resource_vec[h_main_buffer].p_resource;
			p_depth_buffer = g::resource_vec[h_depth_buffer].p_resource;
		}

		void
		resize(const age::extent_2d<uint16>& new_extent) noexcept
		{
			extent = new_extent;

			resource::release_resource(h_main_buffer);
			resource::release_resource(h_depth_buffer);

			this->create_buffers();
		}
	};
}	 // namespace age::graphics::render_pipeline::forward_plus