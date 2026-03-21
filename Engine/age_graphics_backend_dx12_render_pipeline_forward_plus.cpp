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

		h_main_buffer_srv_desc				 = graphics::g::cbv_srv_uav_desc_pool.pop();
		h_depth_buffer_srv_desc				 = graphics::g::cbv_srv_uav_desc_pool.pop();
		h_shadow_atlas_srv_desc				 = graphics::g::cbv_srv_uav_desc_pool.pop();
		h_rt_tlas_buffer_srv_desc			 = graphics::g::cbv_srv_uav_desc_pool.pop();
		h_rt_transparent_tex_buffer_srv_desc = graphics::g::cbv_srv_uav_desc_pool.pop();
		h_rt_transparent_tex_buffer_uav_desc = graphics::g::cbv_srv_uav_desc_pool.pop();
		{
			h_mapping_static_ring_buffer_arr = resource::create_buffer_committed<graphics::g::frame_buffer_count>(g::static_buffer_size);

			h_mapping_frame_data = resource::create_buffer_committed(sizeof(shared_type::frame_data) * graphics::g::frame_buffer_count);

			h_mapping_mesh_buffer			   = resource::create_buffer_committed(1024);
			h_mapping_rt_index_buffer		   = resource::create_buffer_committed(1024);
			h_mapping_rt_vertex_scratch_buffer = resource::create_buffer_committed(1024);

			h_mapping_rt_index_buffer->h_resource->set_name(L"rt_index_buffer");

			h_shadow_atlas = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::texture_ds_2d(g::shadow_atlas_width, g::shadow_atlas_height),
				  .clear_value{
					  .Format		= DXGI_FORMAT_D32_FLOAT,
					  .DepthStencil = { .Depth = 0.f, .Stencil = 0 } },
				  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE,
				  .heap_memory_kind = e::memory_kind::gpu_only,
				  .has_clear_value	= true });

			h_scratch_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(SCRATCH_BUFFER_TOTAL_SIZE),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_shadow_stage_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(SHADOW_STAGE_BUFFER_SIZE),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_shadow_stage_shadow_light_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(shared_type::shadow_light) * g::max_shadow_light_count),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_light_cull_stage_sorted_light_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(shared_type::unified_light) * g::max_visible_light_count),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_rt_blas_buffer = rt::create_blas_buffer(1024);

			h_rt_tlas_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_rt(1024),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_rt_tlas_scratch_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(1024),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_mapping_rt_instance_buffer_arr			 = resource::create_buffer_committed<graphics::g::frame_buffer_count>(sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
			h_mapping_rt_instance_render_data_buffer_arr = resource::create_buffer_committed<graphics::g::frame_buffer_count>(sizeof(shared_type::rt_instance_render_data));
		}

		{
			static_ring_buffer.bind_array(h_mapping_static_ring_buffer_arr);

			frame_data_buffer.bind_array(h_mapping_frame_data, sizeof(shared_type::frame_data));

			scratch_buffer_uav.bind(h_scratch_buffer);

			shadow_stage_buffer_srv.bind(h_shadow_stage_buffer);
			shadow_stage_buffer_uav.bind(h_shadow_stage_buffer);

			shadow_stage_shadow_light_buffer_srv.bind(h_shadow_stage_shadow_light_buffer);
			shadow_stage_shadow_light_buffer_uav.bind(h_shadow_stage_shadow_light_buffer);

			light_cull_stage_sorted_light_buffer_srv.bind(h_light_cull_stage_sorted_light_buffer);
			light_cull_stage_sorted_light_buffer_uav.bind(h_light_cull_stage_sorted_light_buffer);

			rt_instance_render_data_buffer_srv.bind_array(h_mapping_rt_instance_render_data_buffer_arr);

			rt_index_buffer_srv.bind(h_mapping_rt_index_buffer);
		}

		{
			resource::create_view(h_shadow_atlas,
								  h_shadow_atlas_srv_desc,
								  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R32_FLOAT));

			resource::create_view(h_rt_tlas_buffer_srv_desc, defaults::srv_view_desc::rt_acceleration_structure(h_rt_tlas_buffer));
		}

		create_resolution_dependent_buffers();

		{
			stage_depth.init(h_root_sig);
			stage_depth.bind_dsv(h_depth_buffer);

			stage_shadow.init(h_root_sig);
			stage_shadow.bind_dsv(h_shadow_atlas);

			stage_light_culling.init(h_root_sig);

			stage_opaque.init(h_root_sig);
			stage_opaque.bind_rtv_dsv(h_main_buffer, h_depth_buffer);

			stage_transparent.init(h_root_sig);
			stage_transparent.bind_rtv(h_main_buffer);

			stage_presentation.init(h_root_sig);
		}

		resource::set_name(h_mapping_static_ring_buffer_arr, L"static_ring_buffer[{}]");


		h_main_buffer->set_name(L"main_buffer");
		h_depth_buffer->set_name(L"depth_buffer");
		h_shadow_atlas->set_name(L"shadow_atlas");

		h_mapping_frame_data->h_resource->set_name(L"mapping_frame_data");
		h_mapping_mesh_buffer->h_resource->set_name(L"mapping_mesh_buffer");

		h_scratch_buffer->set_name(L"scratch_buffer");
		h_shadow_stage_buffer->set_name(L"shadow_stage_buffer");
		h_shadow_stage_shadow_light_buffer->set_name(L"shadow_stage_shadow_light_buffer");

		h_light_cull_stage_sorted_light_buffer->set_name(L"light_cull_stage_sorted_light_buffer");

		h_rt_blas_buffer->set_name(L"rt_blas_buffer");

		h_rt_tlas_buffer->set_name(L"rt_tlas_buffer");
		h_rt_tlas_scratch_buffer->set_name(L"rt_tlas_scratch_buffer");

		resource::set_name(h_mapping_rt_instance_buffer_arr, L"mapping_rt_instance_buffer_arr[{}]");
		resource::set_name(h_mapping_rt_instance_render_data_buffer_arr, L"mapping_rt_instance_render_data_buffer_arr[{}]");
	}

	void
	pipeline::deinit() noexcept
	{
		mesh_byte_offset			= 0;
		rt_index_buffer_byte_offset = 0;

		for (auto& vec : opaque_meshlet_render_data_vec | std::views::join)
		{
			vec.clear();
		}
		for (auto& vec : rt_instance_render_data_vec | std::views::join)
		{
			vec.clear();
		}
		for (auto& vec : rt_instance_data_vec | std::views::join)
		{
			vec.clear();
		}

		stage_presentation.deinit();
		stage_transparent.deinit();
		stage_opaque.deinit();
		stage_light_culling.deinit();
		stage_shadow.deinit();
		stage_depth.deinit();

		root_signature::destroy(h_root_sig);

		graphics::g::cbv_srv_uav_desc_pool.push(h_main_buffer_srv_desc);
		graphics::g::cbv_srv_uav_desc_pool.push(h_depth_buffer_srv_desc);
		graphics::g::cbv_srv_uav_desc_pool.push(h_shadow_atlas_srv_desc);
		graphics::g::cbv_srv_uav_desc_pool.push(h_rt_tlas_buffer_srv_desc);
		graphics::g::cbv_srv_uav_desc_pool.push(h_rt_transparent_tex_buffer_srv_desc);
		graphics::g::cbv_srv_uav_desc_pool.push(h_rt_transparent_tex_buffer_uav_desc);

		// static
		resource::unmap_and_release(h_mapping_static_ring_buffer_arr);

		resource::unmap_and_release(h_mapping_frame_data);

		resource::unmap_and_release(h_mapping_mesh_buffer);
		resource::unmap_and_release(h_mapping_rt_index_buffer);
		resource::unmap_and_release(h_mapping_rt_vertex_scratch_buffer);


		resource::release(h_main_buffer);
		resource::release(h_depth_buffer);

		resource::release(h_scratch_buffer);

		resource::release(h_light_cull_stage_buffer);


		// shadow
		resource::release(h_shadow_atlas);
		resource::release(h_shadow_stage_buffer);
		resource::release(h_shadow_stage_shadow_light_buffer);
		resource::release(h_light_cull_stage_sorted_light_buffer);

		// rt
		rt::release_blas_buffer(h_rt_blas_buffer);
		resource::release(h_rt_tlas_buffer);
		resource::release(h_rt_tlas_scratch_buffer);
		resource::release(h_rt_transparent_texture_buffer);
		resource::unmap_and_release(h_mapping_rt_instance_buffer_arr);
		resource::unmap_and_release(h_mapping_rt_instance_render_data_buffer_arr);


		AGE_ASSERT(object_data_vec.size() == 0);
		AGE_ASSERT(object_transform_data_vec.size() == 0);
		AGE_ASSERT(mesh_data_vec.size() == 0);
		AGE_ASSERT(camera_data_vec.size() == 0);
		AGE_ASSERT(directional_light_vec.size() == 0);
		AGE_ASSERT(unified_light_vec.size() == 0);

		AGE_ASSERT(shadow_light_header_count == 0);
		AGE_ASSERT(next_shadow_light_id == 0);
		AGE_ASSERT(directional_shadow_light_count == 0);

		object_data_vec.clear();
		object_transform_data_vec.clear();
		directional_light_vec.clear();
		unified_light_vec.clear();
	}

	bool
	pipeline::begin_render(render_surface_handle h_rs) noexcept
	{
		auto& rs = graphics::g::render_surface_vec[h_rs];

		if (rs.should_render is_false) [[unlikely]]
		{
			return false;
		}

		for (auto& vec : opaque_meshlet_render_data_vec | std::views::join)
		{
			vec.clear();
		}
		for (auto& vec : rt_instance_render_data_vec | std::views::join)
		{
			vec.clear();
		}
		for (auto& vec : rt_instance_data_vec | std::views::join)
		{
			vec.clear();
		}

		c_auto new_extent = age::extent_2d<uint16>{
			.width	= static_cast<uint16>(age::platform::get_client_width(rs.h_window)),
			.height = static_cast<uint16>(age::platform::get_client_height(rs.h_window))
		};

		if (extent != new_extent) [[unlikely]]
		{
			resize_resolution_dependent_buffers(new_extent);

			stage_depth.bind_dsv(h_depth_buffer);
			stage_opaque.bind_rtv_dsv(h_main_buffer, h_depth_buffer);
			stage_transparent.bind_rtv(h_main_buffer);
		}

		command::begin_frame(e::queue_kind::direct);

		command::set_descriptor_heaps(1, &graphics::g::cbv_srv_uav_desc_pool.p_descriptor_heap);
		command::set_graphics_root_sig(p_root_sig);
		command::set_compute_root_sig(p_root_sig);

		{
			frame_data_buffer.apply();
			frame_data_buffer.apply_compute();

			static_ring_buffer.apply();
			static_ring_buffer.apply_compute();

			mesh_data_buffer.apply();
			mesh_data_buffer.apply_compute();

			scratch_buffer_uav.apply_compute();

			shadow_stage_shadow_light_buffer_uav.apply_compute();
			light_cull_stage_sorted_light_buffer_uav.apply_compute();
		}

		command::apply_barriers(barrier::undefined_to_rtv(h_main_buffer->p_resource, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
								barrier::undefined_to_dsv_write(h_depth_buffer->p_resource, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
								barrier::undefined_to_dsv_write(h_shadow_atlas->p_resource, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
								barrier::undefined_to_rtv(&rs.get_back_buffer(), D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
								barrier::undefined_to_uav(h_rt_transparent_texture_buffer->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_TEXTURE_BARRIER_FLAG_DISCARD));
		return true;
	}

	void
	pipeline::render_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id) noexcept
	{
		auto& render_data_vec		  = opaque_meshlet_render_data_vec[graphics::g::frame_buffer_idx][thread_id];
		auto& rt_instance_vec		  = rt_instance_data_vec[graphics::g::frame_buffer_idx][thread_id];
		auto& rt_inst_render_data_vec = rt_instance_render_data_vec[graphics::g::frame_buffer_idx][thread_id];

		c_auto rt_instance_id_temp = rt_inst_render_data_vec.size<uint32>();

		c_auto& msh_data = mesh_data_vec[mesh_id];

		for (auto meshlet_id = 0u;
			 auto i : std::views::iota(0) | std::views::take(msh_data.meshlet_count))
		{
			render_data_vec.emplace_back(
				shared_type::opaque_meshlet_render_data{
					.object_id		  = object_data_vec.get_pos(object_id),
					.mesh_byte_offset = msh_data.offset,
					.meshlet_id		  = meshlet_id++ });
		}

		rt_inst_render_data_vec.emplace_back(
			shared_type::rt_instance_render_data{
				.object_id				= object_data_vec.get_pos(object_id),
				.mesh_byte_offset		= msh_data.offset,
				.rt_index_buffer_offset = msh_data.rt_index_buffer_elem_offset });

		c_auto& transform = object_transform_data_vec[object_id];

		auto desc = D3D12_RAYTRACING_INSTANCE_DESC{
			.InstanceID							 = rt_instance_id_temp,
			.InstanceMask						 = RT_MASK_OPAQUE,
			.InstanceContributionToHitGroupIndex = 0,
			.Flags								 = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE,
			.AccelerationStructure				 = mesh_data_vec[mesh_id].h_blas->get_va(),
		};

		std::memcpy(desc.Transform, &transform, sizeof(float3x4));

		rt_instance_vec.emplace_back(desc);
	}

	void
	pipeline::render_transparent_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id) noexcept
	{
		c_auto& msh_data = mesh_data_vec[mesh_id];

		auto& rt_inst_render_data_vec = rt_instance_render_data_vec[graphics::g::frame_buffer_idx][thread_id];
		auto& rt_instance_vec		  = rt_instance_data_vec[graphics::g::frame_buffer_idx][thread_id];

		c_auto rt_instance_id_temp = rt_inst_render_data_vec.size<uint32>();

		rt_inst_render_data_vec.emplace_back(
			shared_type::rt_instance_render_data{
				.object_id				= object_data_vec.get_pos(object_id),
				.mesh_byte_offset		= msh_data.offset,
				.rt_index_buffer_offset = msh_data.rt_index_buffer_elem_offset });

		c_auto& transform = object_transform_data_vec[object_id];

		auto desc = D3D12_RAYTRACING_INSTANCE_DESC{
			.InstanceID							 = rt_instance_id_temp,
			.InstanceMask						 = RT_MASK_TRANSPARENT,
			.InstanceContributionToHitGroupIndex = 0,
			.Flags								 = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE,
			.AccelerationStructure				 = mesh_data_vec[mesh_id].h_blas->get_va(),
		};

		std::memcpy(desc.Transform, &transform, sizeof(float3x4));

		rt_instance_vec.emplace_back(desc);
	}

	void
	pipeline::end_render(render_surface_handle h_rs) noexcept
	{
		auto& rs = graphics::g::render_surface_vec[h_rs];

		c_auto opaque_meshlet_render_data_count = upload_data();


		root_constants.apply();
		root_constants.apply_compute();
		light_cull_stage_buffer_uav.apply_compute();
		shadow_stage_buffer_uav.apply_compute();

		stage_light_culling.execute(light_tile_count_x,
									light_tile_count_y,
									h_scratch_buffer);
		command::apply_barriers(
			barrier::buf_uav_to_srv(h_light_cull_stage_sorted_light_buffer->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING),
			barrier::buf_uav_to_srv(h_light_cull_stage_buffer->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));

		light_cull_stage_sorted_light_buffer_srv.apply();
		light_cull_stage_buffer_srv.apply();

		command::set_view_ports(1, &rs.default_viewport);
		command::set_scissor_rects(1, &rs.default_scissor_rect);

		stage_depth.execute(opaque_meshlet_render_data_count);


		command::apply_barriers(barrier::dsv_write_to_srv(h_depth_buffer->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		stage_shadow.execute(
			extent.width,
			extent.height,
			next_shadow_light_id,
			shadow_light_header_count,
			opaque_meshlet_render_data_count,
			h_shadow_stage_buffer,
			shadow_stage_buffer_srv,
			h_shadow_stage_shadow_light_buffer,
			shadow_stage_shadow_light_buffer_srv);

		command::set_view_ports(1, &rs.default_viewport);
		command::set_scissor_rects(1, &rs.default_scissor_rect);

		command::apply_barriers(
			barrier::srv_to_dsv_read(h_depth_buffer->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING),
			barrier::dsv_write_to_srv(h_shadow_atlas->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));


		stage_opaque.execute(opaque_meshlet_render_data_count);

		command::apply_barriers(
			barrier::dsv_read_to_srv(h_depth_buffer->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING));


		stage_transparent.execute(h_rt_transparent_texture_buffer, extent);


		command::apply_barriers(barrier::rtv_to_srv(h_main_buffer->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));

		stage_presentation.execute(rs);

		command::apply_barriers(barrier::rtv_to_present(&rs.get_back_buffer()));

		command::end_frame(e::queue_kind::direct);
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
			  .heap_memory_kind = e::memory_kind::gpu_only,
			  .has_clear_value	= true });

		h_depth_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_ds_2d(extent.width, extent.height),
			  .clear_value{
				  .Format		= DXGI_FORMAT_D32_FLOAT,
				  .DepthStencil = { .Depth = 0.f, .Stencil = 0 } },
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,
			  .heap_memory_kind = e::memory_kind::gpu_only,
			  .has_clear_value	= true });

		h_rt_transparent_texture_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d(
				  extent.width, extent.height,
				  DXGI_FORMAT_R16G16B16A16_FLOAT,
				  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		h_main_buffer->set_name(L"main_buffer");
		h_depth_buffer->set_name(L"depth_buffer");
		h_rt_transparent_texture_buffer->set_name(L"rt_transparent_buffer");

		resource::create_view(h_main_buffer,
							  h_main_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R16G16B16A16_FLOAT));

		resource::create_view(h_depth_buffer,
							  h_depth_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R32_FLOAT));

		resource::create_view(h_rt_transparent_texture_buffer,
							  h_rt_transparent_tex_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R16G16B16A16_FLOAT));

		resource::create_view(h_rt_transparent_texture_buffer,
							  h_rt_transparent_tex_buffer_uav_desc,
							  defaults::uav_view_desc::tex2d(DXGI_FORMAT_R16G16B16A16_FLOAT));

		h_light_cull_stage_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(LIGHT_CULL_TILE_MASK_OFFSET + sizeof(uint32) * light_tile_count_x * light_tile_count_y * g::light_bitmask_uint32_count),
			  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
			  .heap_memory_kind	   = e::memory_kind::gpu_only,
			  .has_clear_value	   = false });

		light_cull_stage_buffer_srv.bind(h_light_cull_stage_buffer);
		light_cull_stage_buffer_uav.bind(h_light_cull_stage_buffer);

		h_light_cull_stage_buffer->set_name(L"light_cull_stage_buffer");
	}

	void
	pipeline::resize_resolution_dependent_buffers(const age::extent_2d<uint16>& new_extent) noexcept
	{
		extent = new_extent;

		light_tile_count_x = (extent.width + g::light_tile_size - 1) / g::light_tile_size;
		light_tile_count_y = (extent.height + g::light_tile_size - 1) / g::light_tile_size;

		AGE_ASSERT(light_tile_count_x <= 0xff);
		AGE_ASSERT(light_tile_count_y <= 0xff);

		resource::release(h_main_buffer);
		resource::release(h_depth_buffer);
		resource::release(h_rt_transparent_texture_buffer);

		resource::release(h_light_cull_stage_buffer);

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

		c_auto flat_index_arr = baked.gen_flat_index_arr();
		c_auto vertex_pos_arr = baked.gen_vertex_pos_arr<asset::vertex_pnt_uv1>();

		c_auto mesh_required_size = mesh_byte_offset + baked.buffer.byte_size();
		c_auto mesh_need_resize	  = h_mapping_mesh_buffer->h_resource->buffer_size() < mesh_required_size;
		c_auto mesh_new_size	  = std::max(mesh_required_size, h_mapping_mesh_buffer->h_resource->buffer_size() * 2);

		c_auto index_required_size = rt_index_buffer_byte_offset + flat_index_arr.byte_size();
		c_auto index_need_resize   = h_mapping_rt_index_buffer->h_resource->buffer_size() < index_required_size;
		c_auto index_new_size	   = std::max(index_required_size, h_mapping_rt_index_buffer->h_resource->buffer_size() * 2);

		c_auto vertex_required_size = 0 + vertex_pos_arr.byte_size();
		c_auto vertex_need_resize	= h_mapping_rt_vertex_scratch_buffer->h_resource->buffer_size() < vertex_required_size;
		c_auto vertex_new_size		= std::max(vertex_required_size, h_mapping_rt_vertex_scratch_buffer->h_resource->buffer_size() * 2);

		c_auto need_copy = mesh_need_resize or index_need_resize;

		auto h_new_mesh_buffer	= mapping_handle{};
		auto h_new_index_buffer = mapping_handle{};

		if (need_copy)
		{
			command::begin(e::queue_kind::copy);
		}

		if (mesh_need_resize)
		{
			h_new_mesh_buffer = resource::create_buffer_committed(static_cast<uint32>(mesh_new_size));

			command::copy_buffer(e::queue_kind::copy, 0,
								 h_new_mesh_buffer->h_resource->p_resource, 0,
								 h_mapping_mesh_buffer->h_resource->p_resource, 0,
								 mesh_byte_offset);
		}
		if (index_need_resize)
		{
			h_new_index_buffer = resource::create_buffer_committed(static_cast<uint32>(index_new_size));

			command::copy_buffer(e::queue_kind::copy, 0,
								 h_new_index_buffer->h_resource->p_resource, 0,
								 h_mapping_rt_index_buffer->h_resource->p_resource, 0,
								 rt_index_buffer_byte_offset);
		}

		if (vertex_need_resize)
		{
			resource::unmap_and_release(h_mapping_rt_vertex_scratch_buffer);
			h_mapping_rt_vertex_scratch_buffer = resource::create_buffer_committed(static_cast<uint32>(vertex_new_size));
		}

		if (need_copy)
		{
			command::execute_and_wait(e::queue_kind::copy);

			if (mesh_need_resize)
			{
				resource::unmap_and_release(h_mapping_mesh_buffer);
				h_mapping_mesh_buffer = h_new_mesh_buffer;

				mesh_data_buffer.bind(h_mapping_mesh_buffer);
			}

			if (index_need_resize)
			{
				resource::unmap_and_release(h_mapping_rt_index_buffer);
				h_mapping_rt_index_buffer = h_new_index_buffer;

				h_mapping_rt_index_buffer->h_resource->set_name(L"rt_index_buffer");

				rt_index_buffer_srv.bind(h_mapping_rt_index_buffer);
			}
		}

		h_mapping_rt_index_buffer->upload(flat_index_arr.data(), flat_index_arr.byte_size(), rt_index_buffer_byte_offset);
		h_mapping_rt_vertex_scratch_buffer->upload(vertex_pos_arr.data(), vertex_pos_arr.byte_size());
		h_mapping_mesh_buffer->upload(baked.buffer.data(), baked.buffer.byte_size(), mesh_byte_offset);

		auto h_blas = graphics::rt::build_blas(
			h_rt_blas_buffer,
			defaults::rt::geo_desc::triangles(
				static_cast<uint32>(flat_index_arr.size()),
				static_cast<uint32>(vertex_pos_arr.size()),
				h_mapping_rt_index_buffer->h_resource->get_va() + rt_index_buffer_byte_offset,
				h_mapping_rt_vertex_scratch_buffer->h_resource->get_va()));

		auto id = static_cast<t_mesh_id>(mesh_data_vec.emplace_back(
			mesh_data{
				.id							 = static_cast<t_mesh_id>(mesh_data_vec.size()),
				.offset						 = mesh_byte_offset,
				.byte_size					 = baked.buffer.byte_size<uint32>(),
				.meshlet_count				 = baked.get_header().meshlet_count,
				.rt_index_buffer_elem_offset = rt_index_buffer_byte_offset / sizeof(uint32),
				.h_blas						 = h_blas }));

		mesh_byte_offset			+= baked.buffer.byte_size<uint32>();
		rt_index_buffer_byte_offset += flat_index_arr.byte_size<uint32>();
		return id;
	}

	void
	pipeline::release_mesh(t_mesh_id id) noexcept
	{
		rt::release_blas(mesh_data_vec[id].h_blas);

		mesh_data_vec.remove(id);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// object
namespace age::graphics::render_pipeline::forward_plus
{
	t_object_id
	pipeline::add_object(const float3 pos, const float4 quat, const float3 scale) noexcept
	{
		c_auto id = static_cast<t_object_id>(object_data_vec.emplace_back());
		object_transform_data_vec.emplace_back();

		pipeline::update_object(id, pos, quat, scale);

		return id;
	}

	void
	pipeline::update_object(t_object_id id, const float3 pos, const float4 quat, const float3 scale) noexcept
	{
		object_data_vec[id] = shared_type::object_data{
			.pos		= pos,
			.quaternion = math::quaternion_encode(quat),
			.scale		= age::cvt_to<half3>(scale)
		};

		object_transform_data_vec[id] = simd::transformation(simd::load(scale), simd::g::xm_zero_f4, simd::load(quat), simd::load(pos))
									  | simd::mat_transpose()
									  | simd::to<float3x4>();
	}

	void
	pipeline::remove_object(t_object_id& id) noexcept
	{
		object_data_vec.remove(id);
		object_transform_data_vec.remove(id);
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
	pipeline::update_directional_light(t_directional_light_id id, const directional_light_desc& desc) noexcept
	{
		auto& light		= directional_light_vec[id];
		light.direction = desc.direction;
		light.intensity = desc.intensity;
		light.color		= desc.color;
	}

	t_directional_light_id
	pipeline::add_directional_light(const directional_light_desc& desc, bool cast_shadow) noexcept
	{
		t_directional_light_id id = static_cast<t_directional_light_id>(directional_light_vec.emplace_back());

		detail::handle_shadow_light_add(*this, graphics::e::light_kind::directional, id, cast_shadow);

		update_directional_light(id, desc);

		return id;
	}

	void
	pipeline::remove_directional_light(t_directional_light_id& id) noexcept
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
	pipeline::update_point_light(t_unified_light_id id, const point_light_desc& desc) noexcept
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
	pipeline::add_point_light(const point_light_desc& desc, bool cast_shadow) noexcept
	{
		t_unified_light_id id = static_cast<t_unified_light_id>(unified_light_vec.emplace_back());

		detail::handle_shadow_light_add(*this, graphics::e::light_kind::point, id, cast_shadow);

		update_point_light(id, desc);

		return id;
	}

	void
	pipeline::update_spot_light(t_unified_light_id id, const spot_light_desc& desc) noexcept
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
	pipeline::add_spot_light(const spot_light_desc& desc, bool cast_shadow) noexcept
	{
		t_unified_light_id id = static_cast<t_unified_light_id>(unified_light_vec.emplace_back());

		detail::handle_shadow_light_add(*this, graphics::e::light_kind::spot, id, cast_shadow);

		update_spot_light(id, desc);

		return id;
	}

	void
	pipeline::remove_point_light(t_unified_light_id& id) noexcept
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
	pipeline::remove_spot_light(t_unified_light_id& id) noexcept
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

	uint32
	pipeline::upload_data() noexcept
	{
		auto  h_mapping_static_buffer				   = h_mapping_static_ring_buffer_arr[graphics::g::frame_buffer_idx];
		auto& h_mapping_rt_instance_buffer			   = h_mapping_rt_instance_buffer_arr[graphics::g::frame_buffer_idx];
		auto& h_mapping_rt_instance_render_data_buffer = h_mapping_rt_instance_render_data_buffer_arr[graphics::g::frame_buffer_idx];

		auto opaque_mshlt_object_data_count = 0u;
		{
			auto opaque_offset = g::opaque_mshlt_object_data_offset;

			auto rt_instance_render_data_offset = 0ull;

			auto rt_instance_count	= 0ul;
			auto rt_instance_offset = 0ull;
			for (auto i : std::views::iota(0u) | std::views::take(graphics::g::thread_count))
			{
				auto& opaque_mshlt_render_data_vec = opaque_meshlet_render_data_vec[graphics::g::frame_buffer_idx][i];
				auto& rt_inst_render_data_vec	   = rt_instance_render_data_vec[graphics::g::frame_buffer_idx][i];
				auto& rt_instance_vec			   = rt_instance_data_vec[graphics::g::frame_buffer_idx][i];

				h_mapping_static_buffer->upload(opaque_mshlt_render_data_vec.data(), opaque_mshlt_render_data_vec.byte_size<uint32>(), opaque_offset);

				opaque_offset				   += opaque_mshlt_render_data_vec.byte_size<uint32>();
				rt_instance_render_data_offset += rt_inst_render_data_vec.byte_size<uint32>();
				rt_instance_offset			   += rt_instance_vec.byte_size<uint32>();

				// prefix
				for (auto& desc : rt_instance_vec)
				{
					desc.InstanceID += rt_instance_count;
				}

				opaque_mshlt_object_data_count += opaque_mshlt_render_data_vec.size<uint32>();
				rt_instance_count			   += rt_instance_vec.size<uint32>();
			}

			AGE_ASSERT(opaque_offset < g::object_data_offset);

			resource::resize_buffer(h_mapping_rt_instance_buffer, rt_instance_offset);

			if (resource::resize_buffer(h_mapping_rt_instance_render_data_buffer, rt_instance_render_data_offset))
			{
				rt_instance_render_data_buffer_srv.bind_array(h_mapping_rt_instance_render_data_buffer_arr);
			}

			rt_instance_offset			   = 0;
			rt_instance_render_data_offset = 0;
			for (auto i : std::views::iota(0u) | std::views::take(graphics::g::thread_count))
			{
				auto& rt_instance_vec		  = rt_instance_data_vec[graphics::g::frame_buffer_idx][i];
				auto& rt_inst_render_data_vec = rt_instance_render_data_vec[graphics::g::frame_buffer_idx][i];

				h_mapping_rt_instance_buffer->upload(rt_instance_vec.data(), rt_instance_vec.byte_size<uint32>(), rt_instance_offset);
				h_mapping_rt_instance_render_data_buffer->upload(rt_inst_render_data_vec.data(), rt_inst_render_data_vec.byte_size<uint32>(), rt_instance_render_data_offset);

				rt_instance_offset			   += rt_instance_vec.byte_size<uint32>();
				rt_instance_render_data_offset += rt_inst_render_data_vec.byte_size<uint32>();
			}

			auto&& [tlas_buffer_size, tlas_scratch_buffer_size] = graphics::rt::query_tlas_size(rt_instance_count);

			if (resource::resize_buffer(h_rt_tlas_buffer, tlas_buffer_size))
			{
				resource::create_view(h_rt_tlas_buffer_srv_desc, defaults::srv_view_desc::rt_acceleration_structure(h_rt_tlas_buffer));
			}

			resource::resize_buffer(h_rt_tlas_scratch_buffer, tlas_scratch_buffer_size);

			graphics::rt::build_tlas(h_rt_tlas_buffer, h_rt_tlas_scratch_buffer, h_mapping_rt_instance_buffer->h_resource, rt_instance_count);

			command::apply_barriers(barrier::rt_write_to_rt_read(h_rt_tlas_buffer->p_resource, D3D12_BARRIER_SYNC_RAYTRACING));
		}

		h_mapping_static_buffer->upload(object_data_vec.data(), object_data_vec.byte_size<uint32>(), g::object_data_offset);
		h_mapping_static_buffer->upload(directional_light_vec.data(), directional_light_vec.byte_size<uint32>(), g::directional_light_offset);
		h_mapping_static_buffer->upload(unified_light_vec.data(), unified_light_vec.byte_size<uint32>(), g::unified_light_offset);
		{
			auto src_arr = std::array<shared_type::shadow_light_header, g::max_shadow_light_count>{};

			for (auto&& [i, header] : shadow_light_header_arr | std::views::take(shadow_light_header_count) | std::views::enumerate)
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

			h_mapping_static_buffer->upload(src_arr.data(), sizeof(shared_type::shadow_light_header) * shadow_light_header_count, g::shadow_light_header_offset);
		}

		c_auto& cam_desc = camera_desc_vec[0];
		root_constants.bind(shared_type::root_constants{
			.opaque_meshlet_render_data_count  = static_cast<uint32>(opaque_mshlt_object_data_count),
			.directional_light_count_and_extra = static_cast<t_directional_light_id>(directional_light_vec.size()),
			.unified_light_count			   = static_cast<t_unified_light_id>(unified_light_vec.size()),
			.light_tile_count_x				   = light_tile_count_x,
			.light_tile_count_y				   = light_tile_count_y,
			//.cluster_near_z					   = cam_desc.near_z,
			//.cluster_far_z					   = cam_desc.far_z,
			.cam_near_z				= cam_desc.near_z,
			.cam_far_z				= cam_desc.far_z,
			.cam_log_far_near_ratio = std::log2(cam_desc.far_z / cam_desc.near_z),
			.shadow_atlas_id		= graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_shadow_atlas_srv_desc),
		});

		// todo multiple camera
		c_auto& cam_data = camera_data_vec[0];
		c_auto	dt_ns	 = runtime::i_time.get_delta_time_ns();
		c_auto	dt_ms	 = std::chrono::duration<float, std::milli>(dt_ns).count();

		auto frame_d = shared_type::frame_data{
			.view_proj							  = cam_data.view_proj,
			.view_proj_inv						  = cam_data.view_proj_inv,
			.camera_pos							  = cam_data.pos,
			.time								  = dt_ms,
			.inv_backbuffer_size				  = float2{ 1.f / extent.width, 1.f / extent.height },
			.backbuffer_size					  = float2{ static_cast<float>(extent.width), static_cast<float>(extent.height) },
			.camera_forward						  = cam_data.forward,
			.frame_index						  = runtime::i_time.get_frame_count(),
			.camera_right						  = cam_data.right,
			.main_buffer_texture_id				  = graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_main_buffer_srv_desc),
			.depth_buffer_texture_id			  = graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_depth_buffer_srv_desc),
			.rt_tlas_buffer_id					  = graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_rt_tlas_buffer_srv_desc),
			.rt_transparent_buffer_srv_texture_id = graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_rt_transparent_tex_buffer_srv_desc),
			.rt_transparent_buffer_uav_texture_id = graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_rt_transparent_tex_buffer_uav_desc),
		};


		std::ranges::copy(cam_data.frustum_plane_arr, frame_d.frustum_planes);

		h_mapping_frame_data->upload(&frame_d, sizeof(shared_type::frame_data), sizeof(shared_type::frame_data) * graphics::g::frame_buffer_idx);

		return opaque_mshlt_object_data_count;
	}
}	 // namespace age::graphics::render_pipeline::forward_plus
#endif