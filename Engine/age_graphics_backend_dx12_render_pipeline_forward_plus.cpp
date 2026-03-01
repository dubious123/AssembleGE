#include "age_pch.hpp"
#include "age.hpp"
#if defined(AGE_GRAPHICS_BACKEND_DX12)

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


		{
			h_mapping_frame_data		 = resource::create_buffer_committed(sizeof(shared_type::frame_data) * graphics::g::frame_buffer_count);
			h_mapping_job_data_buffer	 = resource::create_buffer_committed(sizeof(shared_type::job_data) * max_job_count_per_frame * graphics::g::frame_buffer_count);
			h_mapping_object_data_buffer = resource::create_buffer_committed(sizeof(shared_type::object_data) * max_object_data_count);
			h_mapping_mesh_buffer		 = resource::create_buffer_committed(max_mesh_buffer_byte_size);

			h_mapping_directional_light_buffer = resource::create_buffer_committed(sizeof(shared_type::directional_light) * max_directional_light_count);
			h_mapping_point_light_buffer	   = resource::create_buffer_committed(sizeof(shared_type::point_light) * max_point_light_count);
			h_mapping_spot_light_buffer		   = resource::create_buffer_committed(sizeof(shared_type::directional_light) * max_spot_light_count);

			h_mapping_global_counter_buffer = resource::create_buffer_committed(
				sizeof(uint32) * graphics::g::frame_buffer_count,
				nullptr,
				resource::e::memory_kind::cpu_to_gpu_direct,
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		}

		{
			frame_data_buffer.bind(h_mapping_frame_data->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data) * 0, 0);
			frame_data_buffer.bind(h_mapping_frame_data->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data) * 1, 1);
			frame_data_buffer.bind(h_mapping_frame_data->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data) * 2, 2);

			job_data_buffer.bind(h_mapping_job_data_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::job_data) * max_job_count_per_frame * 0, 0);
			job_data_buffer.bind(h_mapping_job_data_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::job_data) * max_job_count_per_frame * 1, 1);
			job_data_buffer.bind(h_mapping_job_data_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::job_data) * max_job_count_per_frame * 2, 2);

			object_data_buffer.bind(h_mapping_object_data_buffer->h_resource->p_resource->GetGPUVirtualAddress());

			mesh_data_buffer.bind(h_mapping_mesh_buffer->h_resource->p_resource->GetGPUVirtualAddress());

			directional_light_buffer.bind(h_mapping_directional_light_buffer->h_resource->p_resource->GetGPUVirtualAddress());
			point_light_buffer.bind(h_mapping_point_light_buffer->h_resource->p_resource->GetGPUVirtualAddress());
			spot_light_buffer.bind(h_mapping_spot_light_buffer->h_resource->p_resource->GetGPUVirtualAddress());

			global_counter_buffer.bind(h_mapping_global_counter_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(uint32) * 0, 0);
			global_counter_buffer.bind(h_mapping_global_counter_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(uint32) * 1, 1);
			global_counter_buffer.bind(h_mapping_global_counter_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(uint32) * 2, 2);
		}


		h_main_buffer_srv_desc = graphics::g::cbv_srv_uav_desc_pool.pop();

		stage_depth.init(h_root_sig);
		stage_light_culling.init(h_root_sig);
		stage_opaque.init(h_root_sig);
		stage_presentation.init(h_root_sig);

		create_resolution_dependent_buffers();
		create_static_buffers();

		{
			global_light_index_buffer_uav.bind(p_global_light_index_buffer->GetGPUVirtualAddress());
			cluster_light_info_buffer_uav.bind(p_cluster_light_data_buffer->GetGPUVirtualAddress());
			global_light_index_buffer_srv.bind(p_global_light_index_buffer->GetGPUVirtualAddress());
			cluster_light_info_buffer_srv.bind(p_cluster_light_data_buffer->GetGPUVirtualAddress());
		}
	}

	void
	pipeline::deinit() noexcept
	{
		stage_presentation.deinit();
		stage_light_culling.deinit();
		stage_opaque.deinit();
		stage_depth.deinit();

		root_signature::destroy(h_root_sig);

		graphics::g::cbv_srv_uav_desc_pool.push(h_main_buffer_srv_desc);

		resource::unmap_and_release(h_mapping_frame_data);
		resource::unmap_and_release(h_mapping_job_data_buffer);
		resource::unmap_and_release(h_mapping_object_data_buffer);
		resource::unmap_and_release(h_mapping_mesh_buffer);

		resource::unmap_and_release(h_mapping_directional_light_buffer);
		resource::unmap_and_release(h_mapping_point_light_buffer);
		resource::unmap_and_release(h_mapping_spot_light_buffer);
		resource::unmap_and_release(h_mapping_global_counter_buffer);

		resource::release_resource(h_main_buffer);
		resource::release_resource(h_depth_buffer);

		resource::release_resource(h_global_light_index_buffer);
		resource::release_resource(h_cluster_light_data_buffer);

		AGE_ASSERT(object_count == 0);
		AGE_ASSERT(mesh_data_vec.size() == 0);
		AGE_ASSERT(camera_data_vec.size() == 0);
	}

	bool
	pipeline::begin_render(render_surface_handle h_rs) noexcept
	{
		auto& rs = graphics::g::render_surface_vec[h_rs];

		if (rs.should_render is_false) [[unlikely]]
		{
			return false;
		}

		std::ranges::fill(job_count_array[graphics::g::frame_buffer_idx], 0);

		auto& cmd_list = *graphics::g::cmd_system_direct.cmd_list_pool[graphics::g::frame_buffer_idx][0];

		c_auto new_extent = age::extent_2d<uint16>{
			.width	= std::max(extent.width, static_cast<uint16>(age::platform::get_client_width(rs.h_window))),
			.height = std::max(extent.height, static_cast<uint16>(age::platform::get_client_height(rs.h_window)))
		};

		if (extent != new_extent) [[unlikely]]
		{
			resize_resolution_dependent_buffers(new_extent);
			cluster_light_info_buffer_uav.bind(p_cluster_light_data_buffer->GetGPUVirtualAddress());
			cluster_light_info_buffer_srv.bind(p_cluster_light_data_buffer->GetGPUVirtualAddress());
		}

		cmd_list.RSSetViewports(1, &rs.default_viewport);
		cmd_list.RSSetScissorRects(1, &rs.default_scissor_rect);

		cmd_list.SetDescriptorHeaps(1, &graphics::g::cbv_srv_uav_desc_pool.p_descriptor_heap);
		cmd_list.SetGraphicsRootSignature(p_root_sig);
		cmd_list.SetComputeRootSignature(p_root_sig);

		{
			frame_data_buffer.apply(cmd_list);
			job_data_buffer.apply(cmd_list);
			object_data_buffer.apply(cmd_list);
			mesh_data_buffer.apply(cmd_list);

			directional_light_buffer.apply(cmd_list);
			point_light_buffer.apply(cmd_list);
			spot_light_buffer.apply(cmd_list);

			global_light_index_buffer_srv.apply(cmd_list);
			cluster_light_info_buffer_srv.apply(cmd_list);

			global_counter_buffer.apply_compute(cmd_list);

			global_light_index_buffer_uav.apply_compute(cmd_list);
			cluster_light_info_buffer_uav.apply_compute(cmd_list);
		}

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

		return true;
	}

	void
	pipeline::render_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id) noexcept
	{
		c_auto job_idx		 = job_count_array[graphics::g::frame_buffer_idx][thread_id];
		c_auto mesh_offset	 = mesh_data_vec[mesh_id].offset;
		c_auto meshlet_count = mesh_data_vec[mesh_id].meshlet_count;

		for (auto meshlet_id = 0u;
			 auto i : std::views::iota(0) | std::views::take(meshlet_count))
		{
			job_array[graphics::g::frame_buffer_idx][thread_id][job_idx + i] = shared_type::job_data{
				.object_id		  = object_id,
				.mesh_byte_offset = mesh_offset,
				.meshlet_id		  = meshlet_id++
			};
		}

		job_count_array[graphics::g::frame_buffer_idx][thread_id] += meshlet_count;
	}

	void
	pipeline::end_render(render_surface_handle h_rs) noexcept
	{
		auto& rs	   = graphics::g::render_surface_vec[h_rs];
		auto& cmd_list = *graphics::g::cmd_system_direct.cmd_list_pool[graphics::g::frame_buffer_idx][0];

		auto total_job_count = 0u;

		{
			for (
				auto* p_dst = h_mapping_job_data_buffer->ptr + sizeof(shared_type::job_data) * max_job_count_per_frame * graphics::g::frame_buffer_idx;
				auto  thread_id : std::views::iota(0, graphics::g::thread_count))
			{
				c_auto& job_arr	  = job_array[graphics::g::frame_buffer_idx][thread_id];
				c_auto& job_count = job_count_array[graphics::g::frame_buffer_idx][thread_id];
				c_auto	byte_size = sizeof(shared_type::job_data) * job_count;

				std::memcpy(p_dst, &job_arr[0], byte_size);
				total_job_count += job_count;
				p_dst			+= byte_size;
			}

			{
				// todo multiple camera
				c_auto& cam_data = camera_data_vec[0];
				c_auto	dt_ns	 = age::global::get<runtime::interface>().delta_time_ns();
				c_auto	dt_ms	 = std::chrono::duration<float, std::milli>(dt_ns).count();

				auto frame_d = shared_type::frame_data{
					.view_proj				= cam_data.view_proj,
					.view_proj_inv			= cam_data.view_proj_inv,
					.camera_pos				= cam_data.pos,
					.time					= dt_ms,
					.frame_index			= age::global::get<runtime::interface>().frame_count(),
					.inv_backbuffer_size	= float2{ 1.f / extent.width, 1.f / extent.height },
					.main_buffer_texture_id = graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_main_buffer_srv_desc)
				};

				std::ranges::copy(cam_data.frustom_plane_arr, frame_d.frustum_planes);

				std::memcpy(h_mapping_frame_data->ptr + sizeof(shared_type::frame_data) * graphics::g::frame_buffer_idx, &frame_d, sizeof(shared_type::frame_data));
			}

			// root_constants.bind(total_job_count);
			// root_constants.apply(cmd_list);
			root_constants.bind(shared_type::root_constants{
				.job_count						   = total_job_count,
				.directional_light_count_and_extra = static_cast<t_directional_light_id>(directional_light_id_arr.size()),
				.point_light_count				   = static_cast<t_point_light_id>(point_light_id_arr.size()),
				.spot_light_count				   = static_cast<t_spot_light_id>(spot_light_id_arr.size()) });
			root_constants.apply(cmd_list);
		}

		stage_depth.execute(cmd_list, total_job_count);

		barrier.add_transition(*p_depth_buffer,
							   D3D12_RESOURCE_STATE_DEPTH_WRITE,
							   D3D12_RESOURCE_STATE_DEPTH_READ);

		barrier.apply_and_reset(cmd_list);


		{
			c_auto zero = 0u;
			std::memcpy(h_mapping_global_counter_buffer->ptr + sizeof(uint32) * graphics::g::frame_buffer_idx, &zero, sizeof(uint32));

			stage_light_culling.execute(cmd_list, light_culling_tile_count_x, light_culling_tile_count_y);

			barrier.add_uav(*p_global_light_index_buffer);
			barrier.add_uav(*p_cluster_light_data_buffer);
			barrier.apply_and_reset(cmd_list);
		}


		stage_opaque.execute(cmd_list, total_job_count);

		barrier.add_transition(*p_main_buffer,
							   D3D12_RESOURCE_STATE_RENDER_TARGET,
							   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);


		barrier.add_transition(rs.get_back_buffer(),
							   D3D12_RESOURCE_STATE_PRESENT,
							   D3D12_RESOURCE_STATE_RENDER_TARGET,
							   D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

		barrier.apply_and_reset(cmd_list);

		stage_presentation.execute(cmd_list, rs);

		barrier.add_transition(rs.get_back_buffer(),
							   D3D12_RESOURCE_STATE_RENDER_TARGET,
							   D3D12_RESOURCE_STATE_PRESENT);
		barrier.apply_and_reset(cmd_list);
	}

	void
	pipeline::create_resolution_dependent_buffers() noexcept
	{
		h_main_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_rt_2d(extent.width, extent.height),
			  .clear_value{
				  .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
				  .Color  = { 0.5f, 0.5f, 0.5f, 1.0f } },
			  .initial_state	= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			  .heap_memory_kind = resource::e::memory_kind::gpu_only,
			  .has_clear_value	= true });

		h_depth_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_ds_2d(extent.width, extent.height),
			  .clear_value{
				  .Format		= DXGI_FORMAT_D32_FLOAT,
				  .DepthStencil = { .Depth = 1.f, .Stencil = 0 } },
			  .initial_state	= D3D12_RESOURCE_STATE_DEPTH_READ,
			  .heap_memory_kind = resource::e::memory_kind::gpu_only,
			  .has_clear_value	= true });

		p_main_buffer  = h_main_buffer->p_resource;
		p_depth_buffer = h_depth_buffer->p_resource;

		resource::create_view(h_main_buffer,
							  h_main_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R16G16B16A16_FLOAT));

		stage_opaque.bind_rtv_dsv(h_main_buffer, h_depth_buffer);
		stage_depth.bind_dsv(h_depth_buffer);

		c_auto cluster_count		= light_culling_tile_count_x * light_culling_tile_count_y * g::light_culling_cluster_depth_slice_count;
		h_cluster_light_data_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(shared_type::cluster_light_info) * cluster_count),
			  .initial_state	   = D3D12_RESOURCE_STATE_COMMON,
			  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
			  .has_clear_value	   = false });

		p_cluster_light_data_buffer = h_cluster_light_data_buffer->p_resource;
	}

	void
	pipeline::create_static_buffers() noexcept
	{
		// global light index list - 1M × 4 bytes = 4MB
		h_global_light_index_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(t_global_light_index) * g::light_culling_max_global_light_index_count),
			  .initial_state	   = D3D12_RESOURCE_STATE_COMMON,
			  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
			  .has_clear_value	   = false });


		p_global_light_index_buffer = h_global_light_index_buffer->p_resource;
	}

	void
	pipeline::resize_resolution_dependent_buffers(const age::extent_2d<uint16>& new_extent) noexcept
	{
		extent = new_extent;

		light_culling_tile_count_x = (extent.width + g::light_culling_cluster_tile_size - 1) / g::light_culling_cluster_tile_size;
		light_culling_tile_count_y = (extent.height + g::light_culling_cluster_tile_size - 1) / g::light_culling_cluster_tile_size;

		resource::release_resource(h_main_buffer);
		resource::release_resource(h_depth_buffer);

		resource::release_resource(h_cluster_light_data_buffer);

		create_resolution_dependent_buffers();
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

#endif