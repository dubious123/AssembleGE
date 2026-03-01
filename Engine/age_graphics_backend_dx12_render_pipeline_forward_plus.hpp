#pragma once
#include "age.hpp"

namespace age::graphics::render_pipeline::forward_plus
{
	struct depth_stage
	{
	};

	struct opaque_stage
	{
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
				pss_depth_stencil1{ .subobj = defaults::depth_stencil_desc1::depth_only_reversed },
				pss_blend{ .subobj = defaults::blend_desc::opaque },
				pss_sample_desc{ .subobj = DXGI_SAMPLE_DESC{ .Count = 1, .Quality = 0 } },
				pss_node_mask{ .subobj = 0 });

			p_pso = g::pso_ptr_vec[h_pso];

			h_main_buffer_rtv_desc	= g::rtv_desc_pool.pop();
			h_depth_buffer_dsv_desc = g::dsv_desc_pool.pop();
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
			auto render_pass_rt_desc = defaults::render_pass_rtv_desc::clear_preserve(h_main_buffer_rtv_desc, nullptr);
			auto render_pass_ds_desc = defaults::render_pass_ds_desc::depth_clear_preserve(h_depth_buffer_dsv_desc, 0.f);

			cmd_list.BeginRenderPass(
				1,
				&render_pass_rt_desc,
				&render_pass_ds_desc,
				D3D12_RENDER_PASS_FLAG_NONE);

			{
				cmd_list.SetPipelineState(p_pso);

				if (job_count > 0) [[likely]]
				{
					cmd_list.DispatchMesh((job_count + 31u) / 32u, 1, 1);
				}
			}

			cmd_list.EndRenderPass();
		}

		inline void
		deinit() noexcept
		{
			g::rtv_desc_pool.push(h_main_buffer_rtv_desc);
			g::dsv_desc_pool.push(h_depth_buffer_dsv_desc);

			pso::destroy(h_pso);
		}
	};

	struct presentation_stage
	{
		pso::handle			 h_pso = {};
		ID3D12PipelineState* p_pso = nullptr;

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
		struct mesh_data
		{
			t_mesh_id id;
			uint32	  offset;
			uint32	  byte_size;
			uint32	  meshlet_count;
		};

		struct camera_data
		{
			float3				  pos;
			float4x4			  view_proj;
			float4x4			  view_proj_inv;
			std::array<float4, 6> frustom_plane_arr;
		};

		extent_2d<uint16> extent{ .width = 100, .height = 100 };

		resource_handle h_main_buffer  = {};
		resource_handle h_depth_buffer = {};

		ID3D12Resource* p_main_buffer  = nullptr;
		ID3D12Resource* p_depth_buffer = nullptr;

		graphics::root_signature::handle h_root_sig = {};
		ID3D12RootSignature*			 p_root_sig = nullptr;

		opaque_stage	   stage_opaque{};
		presentation_stage stage_presentation{};

		binding_config_t::reg_b<0> frame_data_buffer;
		binding_config_t::reg_b<1> root_constants;
		binding_config_t::reg_t<0> job_data_buffer;
		binding_config_t::reg_t<1> object_data_buffer;
		binding_config_t::reg_t<2> mesh_data_buffer;

		binding_config_t::reg_t<3> directional_light_buffer;
		binding_config_t::reg_t<4> point_light_buffer;
		binding_config_t::reg_t<5> spot_light_buffer;

		resource::mapping_handle h_mapping_frame_data		  = {};
		resource::mapping_handle h_mapping_job_data_buffer	  = {};
		resource::mapping_handle h_mapping_object_data_buffer = {};
		resource::mapping_handle h_mapping_mesh_buffer		  = {};

		resource::mapping_handle h_mapping_directional_light_buffer = {};
		resource::mapping_handle h_mapping_point_light_buffer		= {};
		resource::mapping_handle h_mapping_spot_light_buffer		= {};

		// bindless texture
		srv_desc_handle h_main_buffer_srv_desc;

		resource_barrier barrier;

		std::array<t_object_id, max_object_data_count> object_id_stack = age::util::iota_array<0u, max_object_data_count>();

		uint32 object_count = 0;

		// for contents updating camera
		data_structure::sparse_vector<camera_desc> camera_desc_vec;
		data_structure::sparse_vector<camera_data> camera_data_vec;

		data_structure::sparse_vector<mesh_data> mesh_data_vec;
		uint32									 mesh_byte_offset = 0;

		shared_type::job_data job_array[g::frame_buffer_count][g::thread_count][max_job_count_per_thread];
		uint32				  job_count_array[g::frame_buffer_count][g::thread_count];

		data_structure::sparse_vector<t_directional_light_id, max_directional_light_count> directional_light_id_arr;
		data_structure::sparse_vector<t_point_light_id, max_point_light_count>			   point_light_id_arr;
		data_structure::sparse_vector<t_spot_light_id, max_spot_light_count>			   spot_light_id_arr;

		t_mesh_id
		upload_mesh(const asset::mesh_baked& baked) noexcept
		{
			AGE_ASSERT(baked.buffer.byte_size() < std::numeric_limits<uint32>::max() - mesh_byte_offset);
			AGE_ASSERT(baked.buffer.byte_size() % 4 == 0);

			auto id = static_cast<t_mesh_id>(mesh_data_vec.emplace_back(
				mesh_data{
					.id			   = static_cast<t_mesh_id>(mesh_data_vec.size()),
					.offset		   = mesh_byte_offset,
					.byte_size	   = baked.buffer.byte_size<uint32>(),
					.meshlet_count = baked.get_header().meshlet_count }));

			std::memcpy(h_mapping_mesh_buffer->ptr + mesh_byte_offset, baked.buffer.data(), baked.buffer.byte_size());

			mesh_byte_offset += baked.buffer.byte_size<uint32>();

			return id;
		}

		void
		release_mesh(t_mesh_id id) noexcept
		{
			mesh_data_vec.remove(id);
		}

	  private:
		template <bool reversed = false>
		static FORCE_INLINE camera_data
		calc_camera_data(const camera_desc& desc) noexcept
		{
			auto&& [xm_pos, xm_quat] = simd::load(desc.pos, desc.quaternion);
			c_auto xm_forward		 = simd::g::xm_forward_f4 | simd::rotate3(xm_quat);
			c_auto xm_up			 = simd::g::xm_up_f4 | simd::rotate3(xm_quat);

			c_auto xm_view = simd::view_look_to(xm_pos, xm_forward, xm_up);

			auto xm_proj = xm_mat{};

			if constexpr (reversed)
			{
				xm_proj = (desc.kind == e::camera_kind::perspective)
							? simd::proj_perspective_fov_reversed(desc.perspective.fov_y, desc.perspective.aspect_ratio, desc.perspective.near_z, desc.perspective.far_z)
							: simd::proj_orthographic_reversed(desc.orthographic.view_width, desc.orthographic.view_height, desc.orthographic.near_z, desc.orthographic.far_z);
			}
			else
			{
				xm_proj = (desc.kind == e::camera_kind::perspective)
							? simd::proj_perspective_fov(desc.perspective.fov_y, desc.perspective.aspect_ratio, desc.perspective.near_z, desc.perspective.far_z)
							: simd::proj_orthographic(desc.orthographic.view_width, desc.orthographic.view_height, desc.orthographic.near_z, desc.orthographic.far_z);
			}

			c_auto xm_view_proj		= xm_proj * xm_view;
			c_auto xm_view_proj_inv = xm_view_proj | simd::mat_inv();

			c_auto frustom_plane_arr = std::array{
				xm_view_proj.r[3] | simd::add(xm_view_proj.r[0]) | simd::plane_normalize() | simd::to<float4>(),	// left
				xm_view_proj.r[3] | simd::sub(xm_view_proj.r[0]) | simd::plane_normalize() | simd::to<float4>(),	// right
				xm_view_proj.r[3] | simd::sub(xm_view_proj.r[1]) | simd::plane_normalize() | simd::to<float4>(),	// top
				xm_view_proj.r[3] | simd::add(xm_view_proj.r[1]) | simd::plane_normalize() | simd::to<float4>(),	// bottom
				xm_view_proj.r[2] | simd::plane_normalize() | simd::to<float4>(),									// near
				xm_view_proj.r[3] | simd::sub(xm_view_proj.r[2]) | simd::plane_normalize() | simd::to<float4>(),	// far
			};

			if (desc.kind == e::camera_kind::perspective)
			{
				return camera_data{
					.pos			   = desc.pos,
					.view_proj		   = xm_view_proj | simd::to<float4x4>(),
					.view_proj_inv	   = xm_view_proj_inv | simd::to<float4x4>(),
					.frustom_plane_arr = frustom_plane_arr
				};
			}
			else
			{
				AGE_ASSERT(false, "orthographic camera is not supported yet");
				return {};
			}
		}

	  public:
		t_camera_id
		add_camera(const camera_desc& desc) noexcept
		{
			c_auto desc_id = camera_desc_vec.emplace_back(desc);
			c_auto data_id = camera_data_vec.emplace_back(calc_camera_data<true>(desc));

			AGE_ASSERT(desc_id == data_id);

			return static_cast<t_camera_id>(data_id);
		}

		camera_desc
		get_camera_desc(t_camera_id id) noexcept
		{
			return camera_desc_vec[id];
		}

		void
		update_camera(t_camera_id id, const camera_desc& desc) noexcept
		{
			camera_data_vec[id] = calc_camera_data<true>(desc);
			camera_desc_vec[id] = desc;
		}

		void
		remove_camera(t_camera_id& id) noexcept
		{
			camera_desc_vec.remove(id);
			camera_data_vec.remove(id);
			id = invalid_id_uint32;
		}

		void
		update_directional_light(t_directional_light_id id, const shared_type::directional_light& data) noexcept
		{
			std::memcpy(h_mapping_directional_light_buffer->ptr + sizeof(shared_type::directional_light) * id,
						&data,
						sizeof(shared_type::directional_light));
		}

		t_directional_light_id
		add_directional_light(const shared_type::directional_light& data) noexcept
		{
			c_auto id = static_cast<t_directional_light_id>(
				directional_light_id_arr.emplace_back(
					static_cast<t_directional_light_id>(directional_light_id_arr.size())));

			update_directional_light(id, data);

			return static_cast<t_directional_light_id>(id);
		}

		void
		remove_directional_light(t_directional_light_id& id) noexcept
		{
			directional_light_id_arr.remove(id);

			id = age::get_invalid_id<t_directional_light_id>();
		}

		void
		update_point_light(t_point_light_id id, const shared_type::point_light& data) noexcept
		{
			std::memcpy(h_mapping_point_light_buffer->ptr + sizeof(shared_type::point_light) * id,
						&data,
						sizeof(shared_type::point_light));
		}

		t_point_light_id
		add_point_light(const shared_type::point_light& data) noexcept
		{
			c_auto id = static_cast<t_point_light_id>(
				point_light_id_arr.emplace_back(
					static_cast<t_point_light_id>(point_light_id_arr.size())));
			update_point_light(id, data);
			return static_cast<t_point_light_id>(id);
		}

		void
		remove_point_light(t_point_light_id& id) noexcept
		{
			point_light_id_arr.remove(id);
			id = age::get_invalid_id<t_point_light_id>();
		}

		void
		update_spot_light(t_spot_light_id id, const shared_type::spot_light& data) noexcept
		{
			std::memcpy(h_mapping_spot_light_buffer->ptr + sizeof(shared_type::spot_light) * id,
						&data,
						sizeof(shared_type::spot_light));
		}

		t_spot_light_id
		add_spot_light(const shared_type::spot_light& data) noexcept
		{
			c_auto id = static_cast<t_spot_light_id>(
				spot_light_id_arr.emplace_back(
					static_cast<t_spot_light_id>(spot_light_id_arr.size())));
			update_spot_light(id, data);
			return static_cast<t_spot_light_id>(id);
		}

		void
		remove_spot_light(t_spot_light_id& id) noexcept
		{
			spot_light_id_arr.remove(id);
			id = age::get_invalid_id<t_spot_light_id>();
		}

		t_object_id
		add_object(shared_type::object_data data = {}) noexcept
		{
			auto object_id = object_id_stack[object_count++];

			std::memcpy(h_mapping_object_data_buffer->ptr + sizeof(shared_type::object_data) * object_id, &data, sizeof(shared_type::object_data));

			return object_id;
		}

		void
		update_object(t_object_id id, shared_type::object_data data = {}) noexcept
		{
			AGE_ASSERT(id < object_id_stack.size());

			std::memcpy(h_mapping_object_data_buffer->ptr + sizeof(shared_type::object_data) * id, &data, sizeof(shared_type::object_data));
		}

		void
		remove_object(t_object_id id) noexcept
		{
			object_id_stack[--object_count] = id;
		}

		void
		init() noexcept;

		void
		bind() noexcept;

		void
		deinit() noexcept;

		bool
		begin_render(render_surface_handle h_rs) noexcept;

		void
		render_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id) noexcept;

		void
		end_render(render_surface_handle h_rs) noexcept;

	  private:
		void
		create_buffers() noexcept;

		void
		resize(const age::extent_2d<uint16>& new_extent) noexcept;
	};
}	 // namespace age::graphics::render_pipeline::forward_plus