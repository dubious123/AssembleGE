#include "age_pch.hpp"
#include "age.hpp"
#if defined(AGE_GRAPHICS_BACKEND_DX12)

// main
namespace age::graphics::render_pipeline::forward_plus
{
	void
	pipeline::init() noexcept
	{
		h_root_sig = binding_config_t::create_root_signature(
			D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		p_root_sig = graphics::g::root_signature_ptr_vec[h_root_sig];

		h_main_buffer_srv_desc	= graphics::g::cbv_srv_uav_desc_pool.pop();
		h_depth_buffer_srv_desc = graphics::g::cbv_srv_uav_desc_pool.pop();
		h_shadow_atlas_srv_desc = graphics::g::cbv_srv_uav_desc_pool.pop();

		{
			h_mapping_frame_data		 = resource::create_buffer_committed(sizeof(shared_type::frame_data) * graphics::g::frame_buffer_count);
			h_mapping_job_data_buffer	 = resource::create_buffer_committed(sizeof(shared_type::opaque_meshlet_render_data) * max_job_count_per_frame * graphics::g::frame_buffer_count);
			h_mapping_object_data_buffer = resource::create_buffer_committed(sizeof(shared_type::object_data) * max_object_data_count * graphics::g::frame_buffer_count);
			h_mapping_mesh_buffer		 = resource::create_buffer_committed(max_mesh_buffer_byte_size);

			h_mapping_directional_light_buffer = resource::create_buffer_committed(sizeof(shared_type::directional_light) * max_directional_light_count * graphics::g::frame_buffer_count);

			h_mapping_unified_light_buffer = resource::create_buffer_committed(sizeof(shared_type::unified_light) * g::max_light_count * graphics::g::frame_buffer_count);

			h_mapping_shadow_light_header_buffer = resource::create_buffer_committed(sizeof(shared_type::shadow_light_header) * g::max_shadow_light_count * graphics::g::frame_buffer_count);

			h_mapping_frame_data_rw_buffer = resource::create_buffer_committed(
				sizeof(shared_type::frame_data_rw) * graphics::g::frame_buffer_count,
				nullptr,
				resource::e::memory_kind::cpu_to_gpu_direct,
				D3D12_BARRIER_LAYOUT_UNDEFINED,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			h_mapping_debug_buffer_uav = resource::create_buffer_committed(
				sizeof(shared_type::debug_77) * graphics::g::frame_buffer_count,
				nullptr,
				resource::e::memory_kind::cpu_to_gpu_direct,
				D3D12_BARRIER_LAYOUT_UNDEFINED,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			h_light_sort_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(g::sort_buffer_total_byte_size),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_zbin_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(g::z_slice_count * sizeof(shared_type::zbin_entry)),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_unified_sorted_light_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(g::max_visible_light_count * sizeof(shared_type::unified_light)),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_shadow_atlas = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::texture_ds_2d(g::shadow_atlas_width, g::shadow_atlas_height),
				  .clear_value{
					  .Format		= DXGI_FORMAT_D32_FLOAT,
					  .DepthStencil = { .Depth = 0.f, .Stencil = 0 } },
				  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE,
				  .heap_memory_kind = resource::e::memory_kind::gpu_only,
				  .has_clear_value	= true });

			h_shadow_light_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(g::max_shadow_light_count * sizeof(shared_type::shadow_light)),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
				  .has_clear_value	   = false });
		}

		{
			frame_data_buffer.bind(h_mapping_frame_data->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data) * 0, 0);
			frame_data_buffer.bind(h_mapping_frame_data->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data) * 1, 1);
			frame_data_buffer.bind(h_mapping_frame_data->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data) * 2, 2);

			job_data_buffer.bind(h_mapping_job_data_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::opaque_meshlet_render_data) * max_job_count_per_frame * 0, 0);
			job_data_buffer.bind(h_mapping_job_data_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::opaque_meshlet_render_data) * max_job_count_per_frame * 1, 1);
			job_data_buffer.bind(h_mapping_job_data_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::opaque_meshlet_render_data) * max_job_count_per_frame * 2, 2);

			object_data_buffer.bind(h_mapping_object_data_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::object_data) * max_object_data_count * 0, 0);
			object_data_buffer.bind(h_mapping_object_data_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::object_data) * max_object_data_count * 1, 1);
			object_data_buffer.bind(h_mapping_object_data_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::object_data) * max_object_data_count * 2, 2);

			mesh_data_buffer.bind(h_mapping_mesh_buffer->h_resource->p_resource->GetGPUVirtualAddress());

			directional_light_buffer.bind(h_mapping_directional_light_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::directional_light) * max_directional_light_count * 0, 0);
			directional_light_buffer.bind(h_mapping_directional_light_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::directional_light) * max_directional_light_count * 1, 1);
			directional_light_buffer.bind(h_mapping_directional_light_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::directional_light) * max_directional_light_count * 2, 2);

			unified_light_buffer.bind(h_mapping_unified_light_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::unified_light) * g::max_light_count * 0, 0);
			unified_light_buffer.bind(h_mapping_unified_light_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::unified_light) * g::max_light_count * 1, 1);
			unified_light_buffer.bind(h_mapping_unified_light_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::unified_light) * g::max_light_count * 2, 2);

			shadow_light_header_buffer.bind(h_mapping_shadow_light_header_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::shadow_light_header) * g::max_shadow_light_count * 0, 0);
			shadow_light_header_buffer.bind(h_mapping_shadow_light_header_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::shadow_light_header) * g::max_shadow_light_count * 1, 1);
			shadow_light_header_buffer.bind(h_mapping_shadow_light_header_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::shadow_light_header) * g::max_shadow_light_count * 2, 2);

			frame_data_rw_buffer_uav.bind(h_mapping_frame_data_rw_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data_rw) * 0, 0);
			frame_data_rw_buffer_uav.bind(h_mapping_frame_data_rw_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data_rw) * 1, 1);
			frame_data_rw_buffer_uav.bind(h_mapping_frame_data_rw_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data_rw) * 2, 2);

			frame_data_rw_buffer_srv.bind(h_mapping_frame_data_rw_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data_rw) * 0, 0);
			frame_data_rw_buffer_srv.bind(h_mapping_frame_data_rw_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data_rw) * 1, 1);
			frame_data_rw_buffer_srv.bind(h_mapping_frame_data_rw_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data_rw) * 2, 2);

			debug_buffer_uav.bind(h_mapping_debug_buffer_uav->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::debug_77) * 0, 0);
			debug_buffer_uav.bind(h_mapping_debug_buffer_uav->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::debug_77) * 1, 1);
			debug_buffer_uav.bind(h_mapping_debug_buffer_uav->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::debug_77) * 2, 2);

			light_sort_buffer_uav.bind(h_light_sort_buffer->p_resource->GetGPUVirtualAddress());
			light_sort_buffer_srv.bind(h_light_sort_buffer->p_resource->GetGPUVirtualAddress());

			zbin_buffer_uav.bind(h_zbin_buffer->p_resource->GetGPUVirtualAddress());
			zbin_buffer_srv.bind(h_zbin_buffer->p_resource->GetGPUVirtualAddress());

			unified_sorted_light_buffer_uav.bind(h_unified_sorted_light_buffer->p_resource->GetGPUVirtualAddress());
			unified_sorted_light_buffer_srv.bind(h_unified_sorted_light_buffer->p_resource->GetGPUVirtualAddress());

			shadow_light_buffer_srv.bind(h_shadow_light_buffer->p_resource->GetGPUVirtualAddress());
			shadow_light_buffer_uav.bind(h_shadow_light_buffer->p_resource->GetGPUVirtualAddress());
		}

		{
			resource::create_view(h_shadow_atlas,
								  h_shadow_atlas_srv_desc,
								  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R32_FLOAT));
		}

		create_resolution_dependent_buffers();

		{
			stage_init.init(h_root_sig);

			stage_depth.init(h_root_sig);
			stage_depth.bind_dsv(h_depth_buffer);

			stage_shadow.init(h_root_sig);
			stage_shadow.bind_dsv(h_shadow_atlas);

			stage_light_culling.init(h_root_sig);

			stage_opaque.init(h_root_sig);
			stage_opaque.bind_rtv_dsv(h_main_buffer, h_depth_buffer);

			stage_presentation.init(h_root_sig);
		}


		h_main_buffer->p_resource->SetName(L"main_buffer");
		h_depth_buffer->p_resource->SetName(L"depth_buffer");
		h_shadow_atlas->p_resource->SetName(L"shadow_atlas");
		h_light_sort_buffer->p_resource->SetName(L"light_sort_buffer");
		h_zbin_buffer->p_resource->SetName(L"zbin_buffer");
		h_tile_mask_buffer->p_resource->SetName(L"tile_mask_buffer");
		h_unified_sorted_light_buffer->p_resource->SetName(L"unified_sorted_light_buffer");
		h_shadow_light_buffer->p_resource->SetName(L"shadow_light_buffer");


		h_mapping_frame_data->h_resource->p_resource->SetName(L"mapping_frame_data");
		h_mapping_job_data_buffer->h_resource->p_resource->SetName(L"mapping_job_data_buffer");
		h_mapping_object_data_buffer->h_resource->p_resource->SetName(L"mapping_object_data_buffer");
		h_mapping_mesh_buffer->h_resource->p_resource->SetName(L"mapping_mesh_buffer");
		h_mapping_directional_light_buffer->h_resource->p_resource->SetName(L"mapping_directional_light_buffer");
		h_mapping_unified_light_buffer->h_resource->p_resource->SetName(L"mapping_unified_light_buffer");
		h_mapping_frame_data_rw_buffer->h_resource->p_resource->SetName(L"mapping_frame_data_rw_buffer");
		h_mapping_shadow_light_header_buffer->h_resource->p_resource->SetName(L"mapping_shadow_light_header_buffer");
		h_mapping_debug_buffer_uav->h_resource->p_resource->SetName(L"mapping_debug_buffer_uav");
	}

	void
	pipeline::deinit() noexcept
	{
		for (auto& i : opaque_meshlet_count_arr | std::views::join)
		{
			i = 0;
		}

		stage_init.deinit();
		stage_presentation.deinit();
		stage_opaque.deinit();
		stage_light_culling.deinit();
		stage_shadow.deinit();
		stage_depth.deinit();

		root_signature::destroy(h_root_sig);

		graphics::g::cbv_srv_uav_desc_pool.push(h_main_buffer_srv_desc);
		graphics::g::cbv_srv_uav_desc_pool.push(h_depth_buffer_srv_desc);
		graphics::g::cbv_srv_uav_desc_pool.push(h_shadow_atlas_srv_desc);

		resource::unmap_and_release(h_mapping_frame_data);
		resource::unmap_and_release(h_mapping_job_data_buffer);
		resource::unmap_and_release(h_mapping_object_data_buffer);
		resource::unmap_and_release(h_mapping_mesh_buffer);

		resource::unmap_and_release(h_mapping_directional_light_buffer);
		resource::unmap_and_release(h_mapping_unified_light_buffer);
		resource::unmap_and_release(h_mapping_frame_data_rw_buffer);

		resource::unmap_and_release(h_mapping_shadow_light_header_buffer);

		resource::unmap_and_release(h_mapping_debug_buffer_uav);


		resource::release_resource(h_main_buffer);
		resource::release_resource(h_depth_buffer);

		resource::release_resource(h_light_sort_buffer);

		resource::release_resource(h_zbin_buffer);
		resource::release_resource(h_tile_mask_buffer);

		resource::release_resource(h_unified_sorted_light_buffer);

		resource::release_resource(h_shadow_atlas);
		resource::release_resource(h_shadow_light_buffer);


		AGE_ASSERT(object_data_vec.count() == 0);
		AGE_ASSERT(mesh_data_vec.size() == 0);
		AGE_ASSERT(camera_data_vec.size() == 0);
		AGE_ASSERT(directional_light_vec.count() == 0);
		AGE_ASSERT(unified_light_vec.count() == 0);

		AGE_ASSERT(shadow_light_header_count == 0);
		AGE_ASSERT(next_shadow_light_id == 0);
		AGE_ASSERT(directional_shadow_light_count == 0);
	}

	bool
	pipeline::begin_render(render_surface_handle h_rs) noexcept
	{
		auto& rs	   = graphics::g::render_surface_vec[h_rs];
		auto& cmd_list = *graphics::g::cmd_system_direct.cmd_list_pool[graphics::g::frame_buffer_idx][0];

		if (rs.should_render is_false) [[unlikely]]
		{
			return false;
		}

		std::ranges::fill(opaque_meshlet_count_arr[graphics::g::frame_buffer_idx], 0);

		c_auto new_extent = age::extent_2d<uint16>{
			.width	= std::max(extent.width, static_cast<uint16>(age::platform::get_client_width(rs.h_window))),
			.height = std::max(extent.height, static_cast<uint16>(age::platform::get_client_height(rs.h_window)))
		};

		if (extent != new_extent) [[unlikely]]
		{
			resize_resolution_dependent_buffers(new_extent);

			stage_opaque.bind_rtv_dsv(h_main_buffer, h_depth_buffer);
			stage_depth.bind_dsv(h_depth_buffer);
		}

		cmd_list.SetDescriptorHeaps(1, &graphics::g::cbv_srv_uav_desc_pool.p_descriptor_heap);
		cmd_list.SetGraphicsRootSignature(p_root_sig);
		cmd_list.SetComputeRootSignature(p_root_sig);

		{
			frame_data_buffer.apply(cmd_list);
			frame_data_buffer.apply_compute(cmd_list);

			job_data_buffer.apply(cmd_list);
			job_data_buffer.apply_compute(cmd_list);

			object_data_buffer.apply(cmd_list);
			object_data_buffer.apply_compute(cmd_list);

			mesh_data_buffer.apply(cmd_list);
			mesh_data_buffer.apply_compute(cmd_list);

			directional_light_buffer.apply(cmd_list);
			directional_light_buffer.apply_compute(cmd_list);

			shadow_light_header_buffer.apply_compute(cmd_list);

			unified_light_buffer.apply_compute(cmd_list);

			frame_data_rw_buffer_uav.apply_compute(cmd_list);
			// frame_data_rw_buffer_srv.apply(cmd_list);

			// light_sort_buffer_srv.apply(cmd_list);
			// light_sort_buffer_srv.apply_compute(cmd_list);
			light_sort_buffer_uav.apply_compute(cmd_list);

			// zbin_buffer_srv.apply(cmd_list);
			zbin_buffer_uav.apply_compute(cmd_list);

			shadow_light_buffer_uav.apply_compute(cmd_list);
			// shadow_light_buffer_srv.apply(cmd_list);

			// unified_sorted_light_buffer_srv.apply(cmd_list);
			unified_sorted_light_buffer_uav.apply_compute(cmd_list);

			// tile_mask_buffer_srv.apply(cmd_list);

			tile_mask_buffer_uav.apply(cmd_list);
			tile_mask_buffer_uav.apply_compute(cmd_list);

			debug_buffer_uav.apply(cmd_list);
			debug_buffer_uav.apply_compute(cmd_list);
		}


		apply_barriers(cmd_list,
					   barrier::srv_to_rtv(
						   h_main_buffer->p_resource,
						   D3D12_BARRIER_SYNC_PIXEL_SHADING,
						   D3D12_TEXTURE_BARRIER_FLAG_DISCARD)
						   | barrier::as_split_begin,

					   barrier::dsv_read_to_dsv_write(
						   h_depth_buffer->p_resource,
						   D3D12_TEXTURE_BARRIER_FLAG_DISCARD)
						   | barrier::as_split_begin,

					   barrier::srv_to_dsv_write(
						   h_shadow_atlas->p_resource,
						   D3D12_BARRIER_SYNC_PIXEL_SHADING,
						   D3D12_TEXTURE_BARRIER_FLAG_DISCARD)
						   | barrier::as_split_begin);
		return true;
	}

	void
	pipeline::render_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id) noexcept
	{
		c_auto meshlet_idx	 = opaque_meshlet_count_arr[graphics::g::frame_buffer_idx][thread_id];
		c_auto mesh_offset	 = mesh_data_vec[mesh_id].offset;
		c_auto meshlet_count = mesh_data_vec[mesh_id].meshlet_count;

		for (auto meshlet_id = 0u;
			 auto i : std::views::iota(0) | std::views::take(meshlet_count))
		{
			opaque_meshlet_render_arr[graphics::g::frame_buffer_idx][thread_id][meshlet_idx + i] = shared_type::opaque_meshlet_render_data{
				.object_id		  = object_id,
				.mesh_byte_offset = mesh_offset,
				.meshlet_id		  = meshlet_id++
			};
		}

		opaque_meshlet_count_arr[graphics::g::frame_buffer_idx][thread_id] += meshlet_count;
	}

	void
	pipeline::render_transparent_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id) noexcept
	{
	}

	void
	pipeline::end_render(render_surface_handle h_rs) noexcept
	{
		auto& rs	   = graphics::g::render_surface_vec[h_rs];
		auto& cmd_list = *graphics::g::cmd_system_direct.cmd_list_pool[graphics::g::frame_buffer_idx][0];

		auto total_opaque_meshlet_count = 0u;

		{
			auto src_arr = std::array<shared_type::shadow_light_header, g::max_shadow_light_count>{};

			for (auto&& [i, header] : shadow_light_header_arr | std::views::enumerate)
			{
				if (header.light_kind == graphics::e::light_kind::directional)
				{
					src_arr[i].light_id = directional_light_vec.get_pos(header.light_id);
				}
				else
				{
					src_arr[i].light_id = unified_light_vec.get_pos(header.light_id);
				}
				src_arr[i].light_kind = std::to_underlying(header.light_kind);
				src_arr[i].shadow_id  = header.shadow_id;
			}

			std::memcpy(
				h_mapping_shadow_light_header_buffer->ptr + sizeof(shared_type::shadow_light_header) * g::max_shadow_light_count * graphics::g::frame_buffer_idx,
				src_arr.data(),
				sizeof(shared_type::shadow_light_header) * shadow_light_header_count);


			std::memcpy(
				h_mapping_object_data_buffer->ptr + sizeof(shared_type::object_data) * max_object_data_count * graphics::g::frame_buffer_idx,
				object_data_vec.data(),
				sizeof(shared_type::object_data) * object_data_vec.count());

			std::memcpy(
				h_mapping_directional_light_buffer->ptr + sizeof(shared_type::directional_light) * max_directional_light_count * graphics::g::frame_buffer_idx,
				directional_light_vec.data(),
				sizeof(shared_type::directional_light) * max_directional_light_count);

			std::memcpy(
				h_mapping_unified_light_buffer->ptr + sizeof(shared_type::unified_light) * g::max_light_count * graphics::g::frame_buffer_idx,
				unified_light_vec.data(),
				sizeof(shared_type::unified_light) * unified_light_vec.count());

			for (
				auto* p_dst = h_mapping_job_data_buffer->ptr + sizeof(shared_type::opaque_meshlet_render_data) * max_job_count_per_frame * graphics::g::frame_buffer_idx;
				auto  thread_id : std::views::iota(0, graphics::g::thread_count))
			{
				c_auto& render_arr			 = opaque_meshlet_render_arr[graphics::g::frame_buffer_idx][thread_id];
				c_auto& opaque_meshlet_count = opaque_meshlet_count_arr[graphics::g::frame_buffer_idx][thread_id];
				c_auto	byte_size			 = sizeof(shared_type::opaque_meshlet_render_data) * opaque_meshlet_count;

				std::memcpy(p_dst, &render_arr[0], byte_size);
				total_opaque_meshlet_count += opaque_meshlet_count;
				p_dst					   += byte_size;
			}

			{
				// todo multiple camera
				c_auto& cam_data = camera_data_vec[0];
				c_auto	dt_ns	 = age::global::get<runtime::interface>().delta_time_ns();
				c_auto	dt_ms	 = std::chrono::duration<float, std::milli>(dt_ns).count();

				auto frame_d = shared_type::frame_data{
					.view_proj				 = cam_data.view_proj,
					.view_proj_inv			 = cam_data.view_proj_inv,
					.camera_pos				 = cam_data.pos,
					.time					 = dt_ms,
					.inv_backbuffer_size	 = float2{ 1.f / extent.width, 1.f / extent.height },
					.backbuffer_size		 = float2{ static_cast<float>(extent.width), static_cast<float>(extent.height) },
					.camera_forward			 = cam_data.forward,
					.frame_index			 = age::global::get<runtime::interface>().frame_count(),
					.camera_right			 = cam_data.right,
					.main_buffer_texture_id	 = graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_main_buffer_srv_desc),
					.depth_buffer_texture_id = graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_depth_buffer_srv_desc),
				};


				std::ranges::copy(cam_data.frustum_plane_arr, frame_d.frustum_planes);

				std::memcpy(h_mapping_frame_data->ptr + sizeof(shared_type::frame_data) * graphics::g::frame_buffer_idx, &frame_d, sizeof(shared_type::frame_data));
			}

			{
				c_auto& cam_desc = camera_desc_vec[0];
				root_constants.bind(shared_type::root_constants{
					.job_count						   = total_opaque_meshlet_count,
					.directional_light_count_and_extra = static_cast<t_directional_light_id>(directional_light_vec.count()),
					.unified_light_count			   = static_cast<t_unified_light_id>(unified_light_vec.count()),
					.light_tile_count_x				   = light_tile_count_x,
					.light_tile_count_y				   = light_tile_count_y,
					//.cluster_near_z					   = cam_desc.near_z,
					//.cluster_far_z					   = cam_desc.far_z,
					.cam_near_z				= cam_desc.near_z,
					.cam_far_z				= cam_desc.far_z,
					.cam_log_far_near_ratio = std::log2(cam_desc.far_z / cam_desc.near_z),
					.shadow_atlas_id		= graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_shadow_atlas_srv_desc),
				});

				root_constants.apply(cmd_list);
				root_constants.apply_compute(cmd_list);
			}
		}
		apply_barriers(cmd_list,
					   barrier::dsv_read_to_dsv_write(h_depth_buffer->p_resource)
						   | barrier::as_split_end);

		stage_init.execute(cmd_list, light_tile_count_x * light_tile_count_y * g::light_bitmask_uint32_count);

		apply_barriers(cmd_list,
					   barrier::buf_uav_to_uav(h_mapping_frame_data_rw_buffer->h_resource->p_resource),
					   barrier::buf_uav_to_uav(h_tile_mask_buffer->p_resource),
					   barrier::buf_uav_to_uav(h_zbin_buffer->p_resource));

		cmd_list.RSSetViewports(1, &rs.default_viewport);
		cmd_list.RSSetScissorRects(1, &rs.default_scissor_rect);
		stage_depth.execute(cmd_list, total_opaque_meshlet_count);

		apply_barriers(cmd_list,
					   barrier::srv_to_dsv_write(h_shadow_atlas->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING)
						   | barrier::as_split_end,
					   barrier::dsv_write_to_srv(h_depth_buffer->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		stage_shadow.execute(
			cmd_list,
			extent.width,
			extent.height,
			next_shadow_light_id,
			shadow_light_header_count,
			*h_mapping_frame_data_rw_buffer->h_resource->p_resource,
			*h_shadow_light_buffer->p_resource,
			shadow_light_buffer_srv,
			total_opaque_meshlet_count);

		cmd_list.RSSetViewports(1, &rs.default_viewport);
		cmd_list.RSSetScissorRects(1, &rs.default_scissor_rect);

		apply_barriers(cmd_list,
					   // used by opaque
					   barrier::srv_to_dsv_read(h_depth_buffer->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING),
					   barrier::dsv_write_to_srv(h_shadow_atlas->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));

		stage_light_culling.execute(cmd_list,
									light_tile_count_x,
									light_tile_count_y,
									*h_unified_sorted_light_buffer->p_resource,
									*h_mapping_frame_data_rw_buffer->h_resource->p_resource,
									frame_data_rw_buffer_srv,
									*h_light_sort_buffer->p_resource,
									light_sort_buffer_srv,
									*h_zbin_buffer->p_resource,
									zbin_buffer_srv);

		apply_barriers(
			cmd_list,
			barrier::buf_uav_to_srv(h_unified_sorted_light_buffer->p_resource,
									D3D12_BARRIER_SYNC_PIXEL_SHADING),

			barrier::buf_uav_to_srv(h_tile_mask_buffer->p_resource,
									D3D12_BARRIER_SYNC_PIXEL_SHADING),

			// handled by light_culling_stage
			// barrier::buf_uav_to_srv(h_mapping_frame_data_rw_buffer->h_resource->p_resource,
			//								  D3D12_BARRIER_SYNC_PIXEL_SHADING),

			barrier::srv_to_rtv(h_main_buffer->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING)
				| barrier::as_split_end);

		unified_sorted_light_buffer_srv.apply(cmd_list);
		tile_mask_buffer_srv.apply(cmd_list);

		stage_opaque.execute(cmd_list, total_opaque_meshlet_count);

		apply_barriers(
			cmd_list,
			barrier::rtv_to_srv(h_main_buffer->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING),
			barrier::present_to_rtv(&rs.get_back_buffer(), D3D12_TEXTURE_BARRIER_FLAG_DISCARD)

		);


		stage_presentation.execute(cmd_list, rs);

		apply_barriers(
			cmd_list,
			barrier::rtv_to_present(&rs.get_back_buffer()));


		{
			// auto* ptr = (shared_type::debug_77*)h_mapping_debug_buffer_uav->ptr + graphics::g::frame_buffer_idx;

			// std::println("[0] id : {}, key : {}"
			//			 "[1] id : {}, key : {}"
			//			 "[2] id : {}, key : {}",
			//			 ptr->light_id_0, ptr->key_0, ptr->light_id_1, ptr->key_1, ptr->light_id_2, ptr->key_2);

			// if (ptr->light_id_0 == 0 and ptr->light_id_1 == 0)
			//{
			//	std::println("============ frame_idx : {} =======================", graphics::g::frame_buffer_idx);
			// }

			// std::println("{}", std::span{ ptr->tile_bit_mask_arr });

			// auto* ptr2 = (shared_type::frame_data_rw*)h_mapping_frame_data_rw_buffer->ptr + graphics::g::frame_buffer_idx;
			// std::println("ptr2->z_min : {}, ptr2->z_max: {}", std::bit_cast<float>(ptr2->z_min), std::bit_cast<float>(ptr2->z_max));
			// std::println("ptr2->cascade_splits : {}", ptr2->cascade_splits[0]);
		}
	}

	void
	pipeline::create_resolution_dependent_buffers() noexcept
	{
		h_main_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_rt_2d(extent.width, extent.height),
			  .clear_value{
				  .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
				  .Color  = { 0.5f, 0.5f, 0.5f, 1.0f } },
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE,
			  .heap_memory_kind = resource::e::memory_kind::gpu_only,
			  .has_clear_value	= true });

		h_depth_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_ds_2d(extent.width, extent.height),
			  .clear_value{
				  .Format		= DXGI_FORMAT_D32_FLOAT,
				  .DepthStencil = { .Depth = 0.f, .Stencil = 0 } },
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,
			  .heap_memory_kind = resource::e::memory_kind::gpu_only,
			  .has_clear_value	= true });

		h_main_buffer->p_resource->SetName(L"main_buffer");
		h_depth_buffer->p_resource->SetName(L"depth_buffer");

		resource::create_view(h_main_buffer,
							  h_main_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R16G16B16A16_FLOAT));

		resource::create_view(h_depth_buffer,
							  h_depth_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R32_FLOAT));

		h_tile_mask_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(uint32) * light_tile_count_x * light_tile_count_y * g::light_bitmask_uint32_count),
			  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
			  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
			  .has_clear_value	   = false });

		tile_mask_buffer_uav.bind(h_tile_mask_buffer->p_resource->GetGPUVirtualAddress());
		tile_mask_buffer_srv.bind(h_tile_mask_buffer->p_resource->GetGPUVirtualAddress());
	}

	void
	pipeline::resize_resolution_dependent_buffers(const age::extent_2d<uint16>& new_extent) noexcept
	{
		extent = new_extent;

		light_tile_count_x = (extent.width + g::light_tile_size - 1) / g::light_tile_size;
		light_tile_count_y = (extent.height + g::light_tile_size - 1) / g::light_tile_size;

		AGE_ASSERT(light_tile_count_x <= 0xff);
		AGE_ASSERT(light_tile_count_y <= 0xff);

		resource::release_resource(h_main_buffer);
		resource::release_resource(h_depth_buffer);

		resource::release_resource(h_tile_mask_buffer);

		create_resolution_dependent_buffers();
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// mesh
namespace age::graphics::render_pipeline::forward_plus
{
	t_mesh_id
	pipeline::upload_mesh(const asset::mesh_baked& baked) noexcept
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
	pipeline::release_mesh(t_mesh_id id) noexcept
	{
		mesh_data_vec.remove(id);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// object
namespace age::graphics::render_pipeline::forward_plus
{
	t_object_id
	pipeline::add_object(const shared_type::object_data& data) noexcept
	{
		c_auto id = static_cast<t_object_id>(object_data_vec.count());

		object_data_vec.emplace_back(data);

		return id;
	}

	void
	pipeline::update_object(t_object_id id, const shared_type::object_data& data) noexcept
	{
		object_data_vec[id] = data;
	}

	void
	pipeline::remove_object(t_object_id& id) noexcept
	{
		object_data_vec.remove(id);
		id = age::get_invalid_id<t_object_id>();
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// camera
namespace age::graphics::render_pipeline::forward_plus
{
	namespace detail
	{
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
				xm_proj = (desc.kind == graphics::e::camera_kind::perspective)
							? simd::proj_perspective_fov_reversed(desc.perspective.fov_y, desc.perspective.aspect_ratio, desc.near_z, desc.far_z)
							: simd::proj_orthographic_reversed(desc.orthographic.view_width, desc.orthographic.view_height, desc.near_z, desc.far_z);
			}
			else
			{
				xm_proj = (desc.kind == graphics::e::camera_kind::perspective)
							? simd::proj_perspective_fov(desc.perspective.fov_y, desc.perspective.aspect_ratio, desc.near_z, desc.far_z)
							: simd::proj_orthographic(desc.orthographic.view_width, desc.orthographic.view_height, desc.near_z, desc.far_z);
			}

			c_auto xm_view_proj		= xm_proj * xm_view;
			c_auto xm_view_proj_inv = xm_view_proj | simd::mat_inv();

			c_auto frustum_plane_arr = std::array{
				xm_view_proj.r[3] | simd::add(xm_view_proj.r[0]) | simd::plane_normalize() | simd::to<float4>(),	// left
				xm_view_proj.r[3] | simd::sub(xm_view_proj.r[0]) | simd::plane_normalize() | simd::to<float4>(),	// right
				xm_view_proj.r[3] | simd::sub(xm_view_proj.r[1]) | simd::plane_normalize() | simd::to<float4>(),	// top
				xm_view_proj.r[3] | simd::add(xm_view_proj.r[1]) | simd::plane_normalize() | simd::to<float4>(),	// bottom
				xm_view_proj.r[2] | simd::plane_normalize() | simd::to<float4>(),									// near
				xm_view_proj.r[3] | simd::sub(xm_view_proj.r[2]) | simd::plane_normalize() | simd::to<float4>(),	// far
			};

			if (desc.kind == graphics::e::camera_kind::perspective)
			{
				return camera_data{
					.pos			   = desc.pos,
					.forward		   = xm_forward | simd::to<float3>(),
					.right			   = simd::g::xm_right_f4 | simd::rotate3(xm_quat) | simd::to<float3>(),
					.view_proj		   = xm_view_proj | simd::to<float4x4>(),
					.view_proj_inv	   = xm_view_proj_inv | simd::to<float4x4>(),
					.frustum_plane_arr = frustum_plane_arr
				};
			}
			else
			{
				AGE_ASSERT(false, "orthographic camera is not supported yet");
				return {};
			}
		}
	}	 // namespace detail

	t_camera_id
	pipeline::add_camera(const camera_desc& desc) noexcept
	{
		c_auto desc_id = camera_desc_vec.emplace_back(desc);
		c_auto data_id = camera_data_vec.emplace_back(detail::calc_camera_data<true>(desc));

		AGE_ASSERT(desc_id == data_id);

		return static_cast<t_camera_id>(data_id);
	}

	camera_desc
	pipeline::get_camera_desc(t_camera_id id) noexcept
	{
		return camera_desc_vec[id];
	}

	void
	pipeline::update_camera(t_camera_id id, const camera_desc& desc) noexcept
	{
		camera_data_vec[id] = detail::calc_camera_data<true>(desc);
		camera_desc_vec[id] = desc;
	}

	void
	pipeline::remove_camera(t_camera_id& id) noexcept
	{
		camera_desc_vec.remove(id);
		camera_data_vec.remove(id);
		id = invalid_id_uint32;
	}

}	 // namespace age::graphics::render_pipeline::forward_plus

// light
namespace age::graphics::render_pipeline::forward_plus
{
	namespace detail
	{
		constexpr FORCE_INLINE t_shadow_light_id
		get_shadow_light_count(graphics::e::light_kind kind) noexcept
		{
			switch (kind)
			{
			case graphics::e::light_kind::directional:
				return static_cast<t_shadow_light_id>(g::directional_shadow_cascade_count);
			case graphics::e::light_kind::point:
				return static_cast<t_shadow_light_id>(6);
			case graphics::e::light_kind::spot:
				return static_cast<t_shadow_light_id>(1);
			default:
				AGE_UNREACHABLE();
			}
		}

		constexpr FORCE_INLINE void
		handle_shadow_light_add(pipeline& ctx, graphics::e::light_kind kind, uint32 id, bool cast_shadow) noexcept
		{
			if (kind == graphics::e::light_kind::directional)
			{
				if (cast_shadow)
				{
					std::move_backward(ctx.shadow_light_header_arr.begin(),
									   ctx.shadow_light_header_arr.begin() + ctx.shadow_light_header_count,
									   ctx.shadow_light_header_arr.begin() + ctx.shadow_light_header_count + 1);
					for (auto& header : ctx.shadow_light_header_arr | std::views::drop(1) | std::views::take(ctx.shadow_light_header_count))
					{
						header.shadow_id += g::directional_shadow_cascade_count;
						if (header.light_kind == graphics::e::light_kind::directional)
						{
							ctx.directional_light_vec[header.light_id].shadow_id_and_extra = header.shadow_id;
						}
						else
						{
							ctx.unified_light_vec[header.light_id].shadow_id_and_extra = header.shadow_id;
						}

						AGE_ASSERT(header.shadow_id < g::max_shadow_light_count);
					}

					ctx.shadow_light_header_arr[0] = shadow_light_header{
						.light_id	= id,
						.light_kind = kind,
						.shadow_id	= 0
					};

					ctx.next_shadow_light_id += get_shadow_light_count(kind);

					++ctx.shadow_light_header_count;
					++ctx.directional_shadow_light_count;
				}
				else
				{
					ctx.directional_light_vec[id].shadow_id_and_extra = age::get_invalid_id<t_shadow_light_id>();
				}
			}
			else
			{
				if (cast_shadow)
				{
					ctx.shadow_light_header_arr[ctx.shadow_light_header_count++] = shadow_light_header{
						.light_id	= id,
						.light_kind = kind,
						.shadow_id	= ctx.next_shadow_light_id,
					};

					ctx.unified_light_vec[id].shadow_id_and_extra = ctx.next_shadow_light_id;

					ctx.next_shadow_light_id += get_shadow_light_count(kind);
				}
				else
				{
					ctx.unified_light_vec[id].shadow_id_and_extra = age::get_invalid_id<t_shadow_light_id>();
				}
			}

			AGE_ASSERT(ctx.shadow_light_header_count <= g::max_shadow_light_count);
		}

		constexpr FORCE_INLINE void
		handle_shadow_light_remove(pipeline& ctx, graphics::e::light_kind kind, t_shadow_light_id shadow_id) noexcept
		{
			auto it = std::ranges::find_if(
				ctx.shadow_light_header_arr,
				[shadow_id](const auto& h) { return h.shadow_id == shadow_id; });

			AGE_ASSERT(it->light_kind == kind);

			for (auto idx = std::distance(ctx.shadow_light_header_arr.begin(), it), idx_next = idx + 1;
				 idx_next < ctx.shadow_light_header_count;
				 ++idx, ++idx_next)
			{
				auto& header	  = ctx.shadow_light_header_arr[idx];
				header			  = ctx.shadow_light_header_arr[idx_next];
				header.shadow_id -= get_shadow_light_count(kind);
				if (header.light_kind == graphics::e::light_kind::directional)
				{
					ctx.directional_light_vec[header.light_id].shadow_id_and_extra = header.shadow_id;
				}
				else
				{
					ctx.unified_light_vec[header.light_id].shadow_id_and_extra = header.shadow_id;
				}

				AGE_ASSERT(ctx.shadow_light_header_arr[idx].shadow_id < g::max_shadow_light_count);
			}

			if (kind == graphics::e::light_kind::directional)
			{
				--ctx.directional_shadow_light_count;
			}
			--ctx.shadow_light_header_count;
			ctx.next_shadow_light_id -= get_shadow_light_count(kind);
		}
	}	 // namespace detail

	void
	pipeline::pipeline::update_directional_light(t_directional_light_id id, const directional_light_desc& desc) noexcept
	{
		auto& light		= directional_light_vec[id];
		light.direction = desc.direction;
		light.intensity = desc.intensity;
		light.color		= desc.color;
	}

	t_directional_light_id
	pipeline::pipeline::add_directional_light(const directional_light_desc& desc, bool cast_shadow) noexcept
	{
		t_directional_light_id id = static_cast<t_directional_light_id>(directional_light_vec.emplace_back());

		detail::handle_shadow_light_add(*this, graphics::e::light_kind::directional, id, cast_shadow);

		update_directional_light(id, desc);

		return id;
	}

	void
	pipeline::pipeline::remove_directional_light(t_directional_light_id& id) noexcept
	{
		c_auto&			  light		= directional_light_vec[id];
		t_shadow_light_id shadow_id = light.shadow_id_and_extra & std::numeric_limits<t_shadow_light_id>::max();
		if (shadow_id != age::get_invalid_id<t_shadow_light_id>())
		{
			detail::handle_shadow_light_remove(*this, graphics::e::light_kind::directional, shadow_id);
		}

		directional_light_vec.remove(id);

		id = age::get_invalid_id<t_directional_light_id>();
	}

	void
	pipeline::pipeline::update_point_light(t_unified_light_id id, const point_light_desc& desc) noexcept
	{
		auto& light = unified_light_vec[id];


		light.position	= desc.position;
		light.range		= desc.range;
		light.color		= math::cvt_to<half3>(desc.color);
		light.intensity = math::cvt_to<half>(desc.intensity);
		light.direction = math::cvt_to<half3>(float3{ 1.f, 0.f, 0.f });
		light.cos_inner = math::cvt_to<half>(-1.f);
		light.cos_outer = math::cvt_to<half>(-2.f);
	}

	t_unified_light_id
	pipeline::pipeline::add_point_light(const point_light_desc& desc, bool cast_shadow) noexcept
	{
		t_unified_light_id id = static_cast<t_unified_light_id>(unified_light_vec.emplace_back());

		detail::handle_shadow_light_add(*this, graphics::e::light_kind::point, id, cast_shadow);

		update_point_light(id, desc);

		return id;
	}

	void
	pipeline::pipeline::update_spot_light(t_unified_light_id id, const spot_light_desc& desc) noexcept
	{
		auto& light = unified_light_vec[id];

		light.position	= desc.position;
		light.range		= desc.range;
		light.color		= math::cvt_to<half3>(desc.color);
		light.intensity = math::cvt_to<half>(desc.intensity);
		light.direction = math::cvt_to<half3>(desc.direction);
		light.cos_inner = math::cvt_to<half>(desc.cos_inner);
		light.cos_outer = math::cvt_to<half>(desc.cos_outer);
	}

	t_unified_light_id
	pipeline::pipeline::add_spot_light(const spot_light_desc& desc, bool cast_shadow) noexcept
	{
		t_unified_light_id id = static_cast<t_unified_light_id>(unified_light_vec.emplace_back());

		detail::handle_shadow_light_add(*this, graphics::e::light_kind::spot, id, cast_shadow);

		update_spot_light(id, desc);

		return id;
	}

	void
	pipeline::pipeline::remove_point_light(t_unified_light_id& id) noexcept
	{
		c_auto&			  light		= unified_light_vec[id];
		t_shadow_light_id shadow_id = light.shadow_id_and_extra & std::numeric_limits<t_shadow_light_id>::max();
		if (shadow_id != age::get_invalid_id<t_shadow_light_id>())
		{
			detail::handle_shadow_light_remove(*this, graphics::e::light_kind::point, shadow_id);
		}

		unified_light_vec.remove(id);

		id = age::get_invalid_id<t_unified_light_id>();
	}

	void
	pipeline::pipeline::remove_spot_light(t_unified_light_id& id) noexcept
	{
		c_auto&			  light		= unified_light_vec[id];
		t_shadow_light_id shadow_id = light.shadow_id_and_extra & std::numeric_limits<t_shadow_light_id>::max();
		if (shadow_id != age::get_invalid_id<t_shadow_light_id>())
		{
			detail::handle_shadow_light_remove(*this, graphics::e::light_kind::spot, shadow_id);
		}

		unified_light_vec.remove(id);

		id = age::get_invalid_id<t_unified_light_id>();
	}
}	 // namespace age::graphics::render_pipeline::forward_plus
#endif