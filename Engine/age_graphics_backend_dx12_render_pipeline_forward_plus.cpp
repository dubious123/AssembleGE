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

		h_mapping_frame_data					 = resource::create_buffer_committed(sizeof(frame_data) * 3);
		h_mappint_asset_data_buffer				 = resource::create_buffer_committed(sizeof(asset_data) * max_asset_data_count);
		h_mapping_job_data_buffer				 = resource::create_buffer_committed(sizeof(job_data) * max_job_data);
		h_mapping_object_data_buffer			 = resource::create_buffer_committed(sizeof(object_data) * max_object_data_count);
		h_mapping_meshlet_header_buffer			 = resource::create_buffer_committed(sizeof(asset::meshlet_header) * max_meshlet_count);
		h_mapping_meshlet_buffer				 = resource::create_buffer_committed(sizeof(asset::meshlet) * max_meshlet_count);
		h_mapping_vertex_buffer					 = resource::create_buffer_committed(sizeof(asset::vertex_p_uv1) * max_vertex_count);
		h_mapping_meshlet_global_index_buffer	 = resource::create_buffer_committed(sizeof(uint32) * max_global_index_buffer_count);
		h_mapping_meshlet_primitive_index_buffer = resource::create_buffer_committed(sizeof(uint8) * 3 * max_primitive_index_buffer_count);

		{
			ID3D12Resource* p_resource = g::resource_vec[g::resource_mapping_vec[h_mapping_frame_data].h_resource].p_resource;
			frame_data_buffer.bind(p_resource->GetGPUVirtualAddress() + sizeof(frame_data) * 0, 0);
			frame_data_buffer.bind(p_resource->GetGPUVirtualAddress() + sizeof(frame_data) * 1, 1);
			frame_data_buffer.bind(p_resource->GetGPUVirtualAddress() + sizeof(frame_data) * 2, 2);
		}
		{
			ID3D12Resource* p_resource = g::resource_vec[g::resource_mapping_vec[h_mappint_asset_data_buffer].h_resource].p_resource;
			asset_data_buffer.bind(p_resource->GetGPUVirtualAddress());
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
			ID3D12Resource* p_resource = g::resource_vec[g::resource_mapping_vec[h_mapping_meshlet_header_buffer].h_resource].p_resource;
			meshlet_header_buffer.bind(p_resource->GetGPUVirtualAddress());
		}
		{
			ID3D12Resource* p_resource = g::resource_vec[g::resource_mapping_vec[h_mapping_meshlet_buffer].h_resource].p_resource;
			meshlet_buffer.bind(p_resource->GetGPUVirtualAddress());
		}
		{
			ID3D12Resource* p_resource = g::resource_vec[g::resource_mapping_vec[h_mapping_vertex_buffer].h_resource].p_resource;
			vertex_buffer.bind(p_resource->GetGPUVirtualAddress());
		}
		{
			ID3D12Resource* p_resource = g::resource_vec[g::resource_mapping_vec[h_mapping_meshlet_global_index_buffer].h_resource].p_resource;
			meshlet_global_index_buffer.bind(p_resource->GetGPUVirtualAddress());
		}
		{
			ID3D12Resource* p_resource = g::resource_vec[g::resource_mapping_vec[h_mapping_meshlet_primitive_index_buffer].h_resource].p_resource;
			meshlet_primitive_index_buffer.bind(p_resource->GetGPUVirtualAddress());
		}

		h_main_buffer_srv_desc = g::cbv_srv_uav_desc_pool.pop();

		create_buffers();

		stage_opaque.init(h_root_sig);
		stage_presentation.init(h_root_sig);
	}

	void
	pipeline::deinit() noexcept
	{
		stage_presentation.deinit();
		stage_opaque.deinit();
		root_signature::destroy(h_root_sig);

		g::cbv_srv_uav_desc_pool.push(h_main_buffer_srv_desc);

		resource::unmap_and_release(h_mapping_frame_data);
		resource::unmap_and_release(h_mappint_asset_data_buffer);
		resource::unmap_and_release(h_mapping_job_data_buffer);
		resource::unmap_and_release(h_mapping_object_data_buffer);
		resource::unmap_and_release(h_mapping_meshlet_header_buffer);
		resource::unmap_and_release(h_mapping_meshlet_buffer);
		resource::unmap_and_release(h_mapping_vertex_buffer);
		resource::unmap_and_release(h_mapping_meshlet_global_index_buffer);
		resource::unmap_and_release(h_mapping_meshlet_primitive_index_buffer);

		resource::release_resource(h_main_buffer);
		resource::release_resource(h_depth_buffer);
	}

	void
	pipeline::begin_render(render_surface& rs) noexcept
	{
		auto& cmd_list = *g::cmd_system_direct.cmd_list_pool[g::frame_buffer_idx][0];

		auto new_extent = age::extent_2d<uint16>{
			.width	= std::max(extent.width, static_cast<uint16>(age::platform::get_client_width(rs.h_window))),
			.height = std::max(extent.height, static_cast<uint16>(age::platform::get_client_height(rs.h_window)))
		};

		if (extent != new_extent) [[unlikely]]
		{
			resize(new_extent);
		}

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

		frame_data_buffer.apply(cmd_list);
	}

	void
	pipeline::end_render(render_surface& rs) noexcept
	{
		auto& cmd_list = *g::cmd_system_direct.cmd_list_pool[g::frame_buffer_idx][0];

		auto job_count = 1000;

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