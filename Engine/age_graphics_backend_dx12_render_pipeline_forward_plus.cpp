#include "age_pch.hpp"
#include "age.hpp"
#if defined(AGE_GRAPHICS_BACKEND_DX12)

namespace age::graphics::render_pipeline::forward_plus
{
	void
	pipeline::init() noexcept
	{
		h_root_sig = create_root_signature();
		p_root_sig = g::root_signature_ptr_vec[h_root_sig];

		h_mapping_frame_data		 = resource::create_buffer_committed(sizeof(frame_data) * 3);
		h_mapping_job_data_buffer	 = resource::create_buffer_committed(sizeof(job_data) * max_job_data);
		h_mapping_object_data_buffer = resource::create_buffer_committed(sizeof(object_data) * max_object_data_count);
		h_mapping_mesh_buffer		 = resource::create_buffer_committed(max_mesh_buffer_byte_size);

		h_main_buffer_srv_desc = g::cbv_srv_uav_desc_pool.pop();

		{
			ID3D12Resource* p_resource = g::resource_vec[g::resource_mapping_vec[h_mapping_frame_data].h_resource].p_resource;
			frame_data_buffer.bind(p_resource->GetGPUVirtualAddress() + sizeof(frame_data) * 0, 0);
			frame_data_buffer.bind(p_resource->GetGPUVirtualAddress() + sizeof(frame_data) * 1, 1);
			frame_data_buffer.bind(p_resource->GetGPUVirtualAddress() + sizeof(frame_data) * 2, 2);
		}
		{
			ID3D12Resource* p_resource = g::resource_vec[g::resource_mapping_vec[h_mapping_job_data_buffer].h_resource].p_resource;
			job_data_buffer.bind(p_resource->GetGPUVirtualAddress());
		}
		{
			ID3D12Resource* p_resource = g::resource_vec[g::resource_mapping_vec[h_mapping_object_data_buffer].h_resource].p_resource;
			object_data_buffer.bind(p_resource->GetGPUVirtualAddress());
		}
		{
			ID3D12Resource* p_resource = g::resource_vec[g::resource_mapping_vec[h_mapping_mesh_buffer].h_resource].p_resource;
			mesh_data_buffer.bind(p_resource->GetGPUVirtualAddress());
		}

		stage_opaque.init(h_root_sig);
		stage_presentation.init(h_root_sig);

		create_buffers();
	}

	void
	pipeline::deinit() noexcept
	{
		stage_presentation.deinit();
		stage_opaque.deinit();
		root_signature::destroy(h_root_sig);

		g::cbv_srv_uav_desc_pool.push(h_main_buffer_srv_desc);

		resource::unmap_and_release(h_mapping_frame_data);
		resource::unmap_and_release(h_mapping_job_data_buffer);
		resource::unmap_and_release(h_mapping_object_data_buffer);
		resource::unmap_and_release(h_mapping_mesh_buffer);

		resource::release_resource(h_main_buffer);
		resource::release_resource(h_depth_buffer);

		AGE_ASSERT(object_count == 0);
		AGE_ASSERT(mesh_data_vec.size() == 0);
		AGE_ASSERT(camera_data_vec.size() == 0);
	}

	bool
	pipeline::begin_render(render_surface_handle h_rs) noexcept
	{
		auto& rs = g::render_surface_vec[h_rs];

		if (rs.should_render is_false) [[unlikely]]
		{
			return false;
		}

		std::ranges::fill(job_count_array[g::frame_buffer_idx], 0);

		auto& cmd_list = *g::cmd_system_direct.cmd_list_pool[g::frame_buffer_idx][0];

		c_auto new_extent = age::extent_2d<uint16>{
			.width	= std::max(extent.width, static_cast<uint16>(age::platform::get_client_width(rs.h_window))),
			.height = std::max(extent.height, static_cast<uint16>(age::platform::get_client_height(rs.h_window)))
		};

		if (extent != new_extent) [[unlikely]]
		{
			resize(new_extent);
		}

		cmd_list.RSSetViewports(1, &rs.default_viewport);
		cmd_list.RSSetScissorRects(1, &rs.default_scissor_rect);

		cmd_list.SetDescriptorHeaps(1, &g::cbv_srv_uav_desc_pool.p_descriptor_heap);
		cmd_list.SetGraphicsRootSignature(p_root_sig);

		{
			frame_data_buffer.apply(cmd_list);
			job_data_buffer.apply(cmd_list);
			object_data_buffer.apply(cmd_list);
			mesh_data_buffer.apply(cmd_list);
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
		c_auto job_idx		 = job_count_array[g::frame_buffer_idx][thread_id];
		c_auto mesh_offset	 = mesh_data_vec[mesh_id].offset;
		c_auto meshlet_count = mesh_data_vec[mesh_id].meshlet_count;

		for (auto meshlet_idx = 0u;
			 auto i : std::views::iota(0) | std::views::take(meshlet_count))
		{
			job_array[g::frame_buffer_idx][thread_id][job_idx + i] = job_data{
				.object_id		  = object_id,
				.mesh_byte_offset = mesh_offset,
				.meshlet_idx	  = meshlet_idx++
			};
		}

		job_count_array[g::frame_buffer_idx][thread_id] += meshlet_count;
	}

	void
	pipeline::end_render(render_surface_handle h_rs) noexcept
	{
		auto& rs	   = g::render_surface_vec[h_rs];
		auto& cmd_list = *g::cmd_system_direct.cmd_list_pool[g::frame_buffer_idx][0];

		{
			auto total_job_count = 0;

			for (auto* p_dst = h_mapping_job_data_buffer->ptr;
				 auto  thread_id : std::views::iota(0, g::thread_count))
			{
				c_auto& job_arr	  = job_array[g::frame_buffer_idx][thread_id];
				c_auto& job_count = job_count_array[g::frame_buffer_idx][thread_id];
				c_auto	byte_size = sizeof(job_data) * job_count;

				if (g::frame_buffer_idx % g::frame_buffer_count == 0)
				{
					std::memcpy(p_dst, &job_arr[0], byte_size);
				}
				// std::memcpy(p_dst, &job_arr[0], byte_size);
				total_job_count += job_count;
				p_dst			+= byte_size;
			}

			{
				// todo
				static uint32 frame_index = 0;

				auto cam_data = camera_data_vec[0];
				auto dt_ns	  = age::global::get<runtime::interface>().delta_time_ns();
				auto dt_ms	  = std::chrono::duration<float, std::milli>(dt_ns).count();

				auto frame_d = frame_data{
					.view_proj				= cam_data.view_proj,
					.view_proj_inv			= cam_data.view_proj_inv,
					.camera_pos				= cam_data.pos,
					.time					= dt_ms,
					.frustum_planes			= cam_data.frustom_plane_arr,
					.frame_index			= frame_index++,
					.inv_backbuffer_size	= float2{ 1.f / extent.width, 1.f / extent.height },
					.main_buffer_texture_id = g::cbv_srv_uav_desc_pool.calc_idx(h_main_buffer_srv_desc)
				};

				std::memcpy(h_mapping_frame_data->ptr + sizeof(frame_data) * g::frame_buffer_idx, &frame_d, sizeof(frame_data));
			}

			auto* ptr1 = h_mapping_frame_data->ptr;
			auto* ptr2 = h_mapping_job_data_buffer->ptr;
			auto* ptr3 = h_mapping_mesh_buffer->ptr;
			auto* ptr4 = h_mapping_object_data_buffer->ptr;

			for (auto j_id : std::views::iota(0) | std::views::take(total_job_count))
			{
				auto& job = reinterpret_cast<job_data*>(h_mapping_job_data_buffer->ptr)[j_id];
				auto& obj = reinterpret_cast<object_data*>(h_mapping_object_data_buffer->ptr)[job.object_id];

				auto p = obj.pos;

				asset::mesh_baked_header header;

				{
					uint32_4 raw4 = *(uint32_4*)(reinterpret_cast<std::byte*>(h_mapping_mesh_buffer->ptr) + job.mesh_byte_offset);

					// res.vertex_buffer_offset = 20 + mesh_byte_offset;

					header.global_vertex_index_buffer_offset = job.mesh_byte_offset + raw4.x;
					header.local_vertex_index_buffer_offset	 = job.mesh_byte_offset + raw4.y;
					header.meshlet_header_buffer_offset		 = job.mesh_byte_offset + raw4.z;
					header.meshlet_buffer_offset			 = job.mesh_byte_offset + raw4.w;

					header.meshlet_count = *(uint32*)(reinterpret_cast<std::byte*>(h_mapping_mesh_buffer->ptr) + (job.mesh_byte_offset + sizeof(uint32_4)));

					int a = 1;
				}

				asset::meshlet mshlt;
				{
					uint32_4 raw1 = *(uint32_4*)(reinterpret_cast<std::byte*>(h_mapping_mesh_buffer->ptr) + (header.meshlet_buffer_offset + (j_id % 32) * sizeof(asset::meshlet)));
					uint32_2 raw2 = *(uint32_2*)(reinterpret_cast<std::byte*>(h_mapping_mesh_buffer->ptr) + (header.meshlet_buffer_offset + (j_id % 32) * sizeof(asset::meshlet) + sizeof(uint32_4)));

					mshlt.global_index_offset = raw1.x;
					mshlt.primitive_offset	  = raw1.y;
					mshlt.vertex_count		  = raw1.z & 0xffu;
					mshlt.primitive_count	  = (raw1.z >> 8) & 0xffu;

					int a = 1;
				}
			}

			stage_opaque.execute(cmd_list, total_job_count);
		}

		barrier.add_transition(*p_main_buffer,
							   D3D12_RESOURCE_STATE_RENDER_TARGET,
							   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		barrier.add_transition(*p_depth_buffer,
							   D3D12_RESOURCE_STATE_DEPTH_WRITE,
							   D3D12_RESOURCE_STATE_DEPTH_READ);

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
	pipeline::create_buffers() noexcept
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

		p_main_buffer  = g::resource_vec[h_main_buffer].p_resource;
		p_depth_buffer = g::resource_vec[h_depth_buffer].p_resource;

		resource::create_view(h_main_buffer,
							  h_main_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R16G16B16A16_FLOAT));

		stage_opaque.bind_rtv_dsv(h_main_buffer, h_depth_buffer);
	}

	void
	pipeline::resize(const age::extent_2d<uint16>& new_extent) noexcept
	{
		extent = new_extent;

		resource::release_resource(h_main_buffer);
		resource::release_resource(h_depth_buffer);

		create_buffers();
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

#endif