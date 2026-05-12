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

		graphics::pop_descriptor(h_main_buffer_srv_desc);
		graphics::pop_descriptor(h_post_buffer_srv_desc);
		graphics::pop_descriptor(h_depth_buffer_srv_desc);
		graphics::pop_descriptor(h_rt_tlas_buffer_srv_desc);
		graphics::pop_descriptor(h_rt_transparent_tex_buffer_srv_desc);
		graphics::pop_descriptor(h_rt_transparent_tex_buffer_uav_desc);
		graphics::pop_descriptor(h_env_light_brdf_lut);

		graphics::resource::create_view(graphics::g::h_brdf_lut, h_env_light_brdf_lut, defaults::srv_view_desc::tex2d(DXGI_FORMAT_R16G16_FLOAT));

		{
			h_mapping_static_ring_buffer_arr = resource::create_buffer_committed<graphics::g::frame_buffer_count>(g::static_buffer_size);

			h_mapping_frame_data = resource::create_buffer_committed(sizeof(shared_type::frame_data) * graphics::g::frame_buffer_count);

			h_mapping_mesh_buffer			   = resource::create_buffer_committed(1024);
			h_mapping_rt_index_buffer		   = resource::create_buffer_committed(1024);
			h_mapping_rt_vertex_scratch_buffer = resource::create_buffer_committed(1024);

			h_mapping_material_buffer = resource::create_buffer_committed(sizeof(shared_type::material) * 5);

			h_mapping_env_light_buffer_arr = resource::create_buffer_committed<graphics::g::frame_buffer_count>(sizeof(shared_type::env_light) * 1);

			h_mapping_ui_data_buffer_arr = resource::create_buffer_committed<graphics::g::frame_buffer_count>(1024);

			h_scratch_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(g::scratch_buffer_total_size),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_light_cull_stage_sorted_light_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(shared_type::unified_light) * g::max_visible_light_count),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

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

			mesh_data_buffer.bind(h_mapping_mesh_buffer);

			scratch_buffer_uav.bind(h_scratch_buffer);

			light_cull_stage_sorted_light_buffer_srv.bind(h_light_cull_stage_sorted_light_buffer);
			light_cull_stage_sorted_light_buffer_uav.bind(h_light_cull_stage_sorted_light_buffer);

			rt_instance_render_data_buffer_srv.bind_array(h_mapping_rt_instance_render_data_buffer_arr);

			rt_index_buffer_srv.bind(h_mapping_rt_index_buffer);

			ui_data_buffer.bind_array(h_mapping_ui_data_buffer_arr);

			material_buffer.bind(h_mapping_material_buffer);

			env_light_buffer.bind_array(h_mapping_env_light_buffer_arr);
		}

		resource::create_view(h_rt_tlas_buffer_srv_desc, defaults::srv_view_desc::rt_acceleration_structure(h_rt_tlas_buffer));

		create_resolution_dependent_buffers();

		{
			stage_depth.init(h_root_sig);
			stage_depth.bind_dsv(h_depth_buffer);

			stage_skybox.init(h_root_sig);
			stage_skybox.bind_rtv_dsv(h_main_buffer, h_depth_buffer);

			stage_light_culling.init(h_root_sig);

			stage_opaque.init(h_root_sig);
			stage_opaque.bind_rtv_dsv(h_main_buffer, h_depth_buffer);

			stage_transparent.init(h_root_sig);
			stage_transparent.bind_rtv(h_main_buffer);

			stage_post_process.init(h_root_sig);
			stage_post_process.bind_rtv(h_post_buffer);

			stage_ui.init(h_root_sig);
			stage_ui.bind_rtv(h_post_buffer);

			stage_presentation.init(h_root_sig);
		}

		resource::set_name(h_mapping_static_ring_buffer_arr, L"static_ring_buffer[{}]");


		h_main_buffer->set_name(L"main_buffer");
		h_post_buffer->set_name(L"post_buffer");
		h_depth_buffer->set_name(L"depth_buffer");

		h_mapping_frame_data->h_resource->set_name(L"mapping_frame_data");
		h_mapping_mesh_buffer->h_resource->set_name(L"mapping_mesh_buffer");

		h_scratch_buffer->set_name(L"scratch_buffer");

		h_light_cull_stage_sorted_light_buffer->set_name(L"light_cull_stage_sorted_light_buffer");

		h_rt_tlas_buffer->set_name(L"rt_tlas_buffer");
		h_rt_tlas_scratch_buffer->set_name(L"rt_tlas_scratch_buffer");
		h_mapping_rt_index_buffer->h_resource->set_name(L"rt_index_buffer");
		h_mapping_material_buffer->h_resource->set_name(L"mapping_material_buffer");

		resource::set_name(h_mapping_env_light_buffer_arr, L"mapping_env_light_buffer[{}]");
		resource::set_name(h_mapping_rt_instance_buffer_arr, L"mapping_rt_instance_buffer_arr[{}]");
		resource::set_name(h_mapping_rt_instance_render_data_buffer_arr, L"mapping_rt_instance_render_data_buffer_arr[{}]");
		resource::set_name(h_mapping_ui_data_buffer_arr, L"mapping_ui_data_buffer_arr[{}]");

		// default camera
		main_camera_id = add_camera(age::graphics::render_pipeline::forward_plus::camera_desc{
			.kind		= age::graphics::e::camera_kind::perspective,
			.pos		= float3::zero(),
			.quaternion = age::math::g::quaternion_identity,
			.near_z		= 0.1f,
			.far_z		= 1000.f,
			.perspective{
				.fov_y		  = age::cvt_to_radian(75.f),
				.aspect_ratio = 16.f / 9.f } });

		AGE_ASSERT(main_camera_id == 0);
	}

	void
	pipeline::deinit() noexcept
	{
		mesh_persistant_buffer_offset_pool.clear();
		mesh_rt_index_buffer_offset_pool.clear();

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

		ui_render_data_vec.clear();
		ui_render_data_z_range_vec.clear();

		stage_presentation.deinit();
		stage_ui.deinit();
		stage_transparent.deinit();
		stage_post_process.deinit();
		stage_opaque.deinit();
		stage_light_culling.deinit();
		stage_skybox.deinit();
		stage_depth.deinit();

		root_signature::destroy(h_root_sig);

		graphics::push_descriptor(h_main_buffer_srv_desc);
		graphics::push_descriptor(h_post_buffer_srv_desc);
		graphics::push_descriptor(h_depth_buffer_srv_desc);
		graphics::push_descriptor(h_rt_tlas_buffer_srv_desc);
		graphics::push_descriptor(h_rt_transparent_tex_buffer_srv_desc);
		graphics::push_descriptor(h_rt_transparent_tex_buffer_uav_desc);
		graphics::push_descriptor(h_env_light_brdf_lut);

		// static
		resource::unmap_and_release(h_mapping_static_ring_buffer_arr);
		resource::unmap_and_release(h_mapping_frame_data);
		resource::unmap_and_release(h_mapping_mesh_buffer);
		resource::unmap_and_release(h_mapping_rt_index_buffer);
		resource::unmap_and_release(h_mapping_rt_vertex_scratch_buffer);
		resource::unmap_and_release(h_mapping_ui_data_buffer_arr);
		resource::unmap_and_release(h_mapping_material_buffer);
		resource::unmap_and_release(h_mapping_env_light_buffer_arr);


		resource::release(h_main_buffer);
		resource::release(h_post_buffer);
		resource::release(h_depth_buffer);
		resource::release(h_scratch_buffer);
		resource::release(h_light_cull_stage_buffer);
		resource::release(h_light_cull_stage_sorted_light_buffer);

		// rt
		resource::release(h_rt_tlas_buffer);
		resource::release(h_rt_tlas_scratch_buffer);
		resource::release(h_rt_transparent_texture_buffer);
		resource::unmap_and_release(h_mapping_rt_instance_buffer_arr);
		resource::unmap_and_release(h_mapping_rt_instance_render_data_buffer_arr);

		AGE_ASSERT(camera_data_vec.size() == 1);
		AGE_ASSERT(camera_desc_vec.size() == 1);

		AGE_ASSERT(main_camera_id == 0);
		remove_camera(main_camera_id);

		AGE_ASSERT(object_data_vec.size() == 0);
		AGE_ASSERT(object_transform_data_vec.size() == 0);
		AGE_ASSERT(mesh_data_vec.size() == 0);
		AGE_ASSERT(camera_data_vec.size() == 0);
		AGE_ASSERT(camera_desc_vec.size() == 0);
		AGE_ASSERT(directional_light_vec.size() == 0);
		AGE_ASSERT(unified_light_vec.size() == 0);
		AGE_ASSERT(texture_map.size() == 0);
		AGE_ASSERT(env_light_cpu_data_vec.size() == 0);

		texture_map.clear();
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

		ui_render_data_vec.clear();
		ui_render_data_z_range_vec.clear();

		c_auto new_extent = age::extent_2d<uint16>{
			.width	= static_cast<uint16>(age::platform::get_client_width(rs.h_window)),
			.height = static_cast<uint16>(age::platform::get_client_height(rs.h_window))
		};

		if (extent != new_extent) [[unlikely]]
		{
			resize_resolution_dependent_buffers(new_extent);

			stage_depth.bind_dsv(h_depth_buffer);
			stage_skybox.bind_rtv_dsv(h_main_buffer, h_depth_buffer);
			stage_opaque.bind_rtv_dsv(h_main_buffer, h_depth_buffer);
			stage_transparent.bind_rtv(h_main_buffer);
			stage_post_process.bind_rtv(h_post_buffer);
			stage_ui.bind_rtv(h_post_buffer);
		}

		command::begin_frame(e::queue_kind::direct);

		command::set_descriptor_heaps(1, &graphics::g::cbv_srv_uav_desc_pool.p_descriptor_heap);
		command::set_graphics_root_sig(p_root_sig);
		command::set_compute_root_sig(p_root_sig);

		// command::apply_barriers(barrier::undefined_to_rtv(h_main_buffer->p_resource, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
		//						barrier::undefined_to_rtv(h_post_buffer->p_resource, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
		//						barrier::undefined_to_dsv_write(h_depth_buffer->p_resource, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
		//						barrier::undefined_to_rtv(&rs.get_back_buffer(), D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
		//						barrier::undefined_to_uav(h_rt_transparent_texture_buffer->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_TEXTURE_BARRIER_FLAG_DISCARD));

		command::apply_barriers(barrier::undefined_to_rtv(h_main_buffer->p_resource, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
								barrier::undefined_to_rtv(h_post_buffer->p_resource, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
								barrier::undefined_to_dsv_write(h_depth_buffer->p_resource, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
								barrier::undefined_to_rtv(&rs.get_back_buffer(), D3D12_TEXTURE_BARRIER_FLAG_DISCARD));
		return true;
	}

	void
	pipeline::render_mesh(uint8 thread_id, t_object_id object_id, asset::handle h_mesh, asset::handle h_mat) noexcept
	{
		c_auto& mesh_entry = h_mesh.get_entry<asset::e::kind::mesh_baked>();
		c_auto& mat_entry  = h_mat.get_entry<asset::e::kind::material>();

		// todo
		if (mat_entry.alpha_mode == asset::e::alpha_mode_kind::opaque)
		{
			render_mesh(thread_id, object_id, mesh_entry.render_id, mat_entry.render_id);
		}
		else
		{
			render_transparent_mesh(thread_id, object_id, mesh_entry.render_id, mat_entry.render_id);
		}
	}

	void
	pipeline::render_mesh(uint8 thread_id, t_object_id object_id, asset::handle h_mesh, t_material_id mat_id) noexcept
	{
		c_auto& entry = h_mesh.get_entry<asset::e::kind::mesh_baked>();
		render_mesh(thread_id, object_id, entry.render_id, mat_id);
	}

	void
	pipeline::render_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id, t_material_id mat_id) noexcept
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
					.object_id		   = object_data_vec.get_pos(object_id),
					.mesh_byte_offset  = msh_data.offset,
					.mesh_chunk_srv_id = msh_data.chunk_srv_id,
					.meshlet_id		   = meshlet_id++,
					.material_id	   = mat_id });
		}

		rt_inst_render_data_vec.emplace_back(
			shared_type::rt_instance_render_data{
				.object_id				= object_data_vec.get_pos(object_id),
				.mesh_byte_offset		= msh_data.offset,
				.mesh_chunk_srv_id		= msh_data.chunk_srv_id,
				.rt_index_buffer_offset = msh_data.rt_idx_offset / 4,
				.material_id			= mat_id });

		c_auto& transform = object_transform_data_vec[object_id];

		auto desc = D3D12_RAYTRACING_INSTANCE_DESC{
			.InstanceID							 = rt_instance_id_temp,
			.InstanceMask						 = to_idx(e::rt_mask::opaque),
			.InstanceContributionToHitGroupIndex = 0,
			.Flags								 = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE,
			.AccelerationStructure				 = mesh_data_vec[mesh_id].h_blas->get_va(),
		};

		std::memcpy(desc.Transform, &transform, sizeof(float3x4));

		rt_instance_vec.emplace_back(desc);
	}

	void
	pipeline::render_transparent_mesh(uint8 thread_id, t_object_id object_id, asset::handle h_mesh, t_material_id mat_id) noexcept
	{
		c_auto& entry = h_mesh.get_entry<asset::e::kind::mesh_baked>();
		render_transparent_mesh(thread_id, object_id, entry.render_id, mat_id);
	}

	void
	pipeline::render_transparent_mesh(uint8 thread_id, t_object_id object_id, t_mesh_id mesh_id, t_material_id mat_id) noexcept
	{
		c_auto& msh_data = mesh_data_vec[mesh_id];

		auto& rt_inst_render_data_vec = rt_instance_render_data_vec[graphics::g::frame_buffer_idx][thread_id];
		auto& rt_instance_vec		  = rt_instance_data_vec[graphics::g::frame_buffer_idx][thread_id];

		c_auto rt_instance_id_temp = rt_inst_render_data_vec.size<uint32>();

		rt_inst_render_data_vec.emplace_back(
			shared_type::rt_instance_render_data{
				.object_id				= object_data_vec.get_pos(object_id),
				.mesh_byte_offset		= msh_data.offset,
				.mesh_chunk_srv_id		= msh_data.chunk_srv_id,
				.rt_index_buffer_offset = msh_data.rt_idx_offset / 4,
				.material_id			= mat_id });

		c_auto& transform = object_transform_data_vec[object_id];

		auto desc = D3D12_RAYTRACING_INSTANCE_DESC{
			.InstanceID							 = rt_instance_id_temp,
			.InstanceMask						 = to_idx(e::rt_mask::transparent),
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

		{
			frame_data_buffer.apply();
			frame_data_buffer.apply_compute();

			static_ring_buffer.apply();
			static_ring_buffer.apply_compute();

			mesh_data_buffer.apply();
			mesh_data_buffer.apply_compute();

			scratch_buffer_uav.apply_compute();

			light_cull_stage_sorted_light_buffer_uav.apply_compute();

			rt_instance_render_data_buffer_srv.apply_compute();
			rt_instance_render_data_buffer_srv.apply();
			rt_index_buffer_srv.apply_compute();

			ui_data_buffer.apply();

			material_buffer.apply();
			material_buffer.apply_compute();

			env_light_buffer.apply();
			env_light_buffer.apply_compute();
		}

		command::set_view_ports(1, &rs.default_viewport);
		command::set_scissor_rects(1, &rs.default_scissor_rect);

		root_constants.apply();
		root_constants.apply_compute();
		light_cull_stage_buffer_uav.apply_compute();

		stage_light_culling.execute(light_tile_count_x,
									light_tile_count_y,
									h_scratch_buffer);
		command::apply_barriers(
			barrier::buf_uav_to_srv(h_light_cull_stage_sorted_light_buffer->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING | D3D12_BARRIER_SYNC_COMPUTE_SHADING),
			barrier::buf_uav_to_srv(h_light_cull_stage_buffer->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING | D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		light_cull_stage_sorted_light_buffer_srv.apply();
		light_cull_stage_buffer_srv.apply();

		stage_depth.execute(opaque_meshlet_render_data_count);

		command::apply_barriers(barrier::dsv_write_to_generic_read(h_depth_buffer->p_resource,
																   D3D12_BARRIER_SYNC_DEPTH_STENCIL | D3D12_BARRIER_SYNC_PIXEL_SHADING,
																   D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ | D3D12_BARRIER_ACCESS_SHADER_RESOURCE));

		stage_skybox.execute(env_light_gpu_data_vec.size<uint32>());

		stage_opaque.execute(opaque_meshlet_render_data_count);

		command::apply_barriers(barrier::dsv_generic_read_to_srv(h_depth_buffer->p_resource,
																 D3D12_BARRIER_SYNC_DEPTH_STENCIL | D3D12_BARRIER_SYNC_PIXEL_SHADING,
																 D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ | D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
																 D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		// command::apply_barriers(
		//	barrier::dsv_read_to_srv(h_depth_buffer->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		command::apply_barriers(barrier::tex_srv_to_uav(h_rt_transparent_texture_buffer->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));

		light_cull_stage_sorted_light_buffer_srv.apply_compute();
		light_cull_stage_buffer_srv.apply_compute();
		stage_transparent.execute(h_rt_transparent_texture_buffer, extent);

		command::apply_barriers(barrier::rtv_to_srv(h_main_buffer->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));

		stage_post_process.execute();

		stage_ui.execute(ui_render_data_z_range_vec);

		command::apply_barriers(barrier::rtv_to_srv(h_post_buffer->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));

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

		h_post_buffer = resource::create_committed(
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

		resource::create_view(h_post_buffer,
							  h_post_buffer_srv_desc,
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
			{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(g::light_cull_tile_mask_offset + sizeof(uint32) * light_tile_count_x * light_tile_count_y * g::light_bitmask_uint32_count),
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
		resource::release(h_post_buffer);
		resource::release(h_depth_buffer);
		resource::release(h_rt_transparent_texture_buffer);

		resource::release(h_light_cull_stage_buffer);

		create_resolution_dependent_buffers();
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// texture
namespace age::graphics::render_pipeline::forward_plus
{
	t_texture_id
	pipeline::upload_texture(asset::handle h_tex) noexcept
	{
		c_auto& entry		= h_tex.get_entry<asset::e::kind::texture>();
		c_auto& header		= entry.get_header();
		c_auto	dx12_format = graphics::dx12_format(header.format);
		c_auto	h_resource	= resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d_array(
				  header.extent.width, header.extent.height,
				  dx12_format,
				  header.tex_depth_or_array_size,
				  D3D12_RESOURCE_FLAG_NONE,
				  header.mip_count),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		c_auto h_srv_desc = graphics::g::cbv_srv_uav_desc_pool.pop();

		if (entry.is_cube_map())
		{
			if (header.tex_depth_or_array_size == 1)
			{
				resource::create_view(h_resource,
									  h_srv_desc,
									  defaults::srv_view_desc::tex_cube(dx12_format, header.mip_count));
			}
			else
			{
				resource::create_view(h_resource,
									  h_srv_desc,
									  defaults::srv_view_desc::tex_cube_array(dx12_format, header.tex_depth_or_array_size, header.mip_count));
			}
		}
		else if (entry.is_tex3d())
		{
			AGE_ASSERT(header.tex_depth_or_array_size == 1);
			resource::create_view(h_resource,
								  h_srv_desc,
								  defaults::srv_view_desc::tex3d(dx12_format, header.mip_count));
		}
		else
		{
			if (header.tex_depth_or_array_size == 1)
			{
				resource::create_view(h_resource,
									  h_srv_desc,
									  defaults::srv_view_desc::tex2d(dx12_format, header.mip_count));
			}
			else
			{
				resource::create_view(h_resource,
									  h_srv_desc,
									  defaults::srv_view_desc::tex2d_array(dx12_format, header.tex_depth_or_array_size, header.mip_count));
			}
		}


		c_auto id = graphics::calc_desc_idx(h_srv_desc);

		texture_map[id] = {
			.h_srv_desc = h_srv_desc,
			.h_resource = h_resource,
		};

		command::begin();
		resource::upload_texture(h_resource, entry.get_texture_buffer());
		command::apply_barriers(barrier::tex_copy_dest_to_srv(h_resource->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));
		command::execute_and_wait();

		return t_texture_id{ id };
	}

	t_texture_id
	pipeline::upload_texture(const void* p_src, age::extent_2d<uint32> tex_extent, graphics::e::texture_format format) noexcept
	{
		c_auto dx12_format = graphics::dx12_format(format);
		c_auto h_resource  = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d(
				  tex_extent.width, tex_extent.height,
				  dx12_format,
				  D3D12_RESOURCE_FLAG_NONE),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		c_auto h_srv_desc = graphics::g::cbv_srv_uav_desc_pool.pop();

		resource::create_view(h_resource,
							  h_srv_desc,
							  defaults::srv_view_desc::tex2d(dx12_format));

		c_auto id = graphics::calc_desc_idx(h_srv_desc);

		texture_map[id] = {
			.h_srv_desc = h_srv_desc,
			.h_resource = h_resource,
		};
		command::begin();
		resource::upload_texture(h_resource, p_src, tex_extent, dx12_format);
		command::apply_barriers(barrier::tex_copy_dest_to_srv(h_resource->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));
		command::execute_and_wait();

		return t_texture_id{ id };
	}

	void
	pipeline::release_texture(t_texture_id& id) noexcept
	{
		AGE_ASSERT(texture_map.contains(id));

		auto& value = texture_map.find(id)->second;
		graphics::push_descriptor(texture_map[id].h_srv_desc);
		graphics::resource::release_deferred(texture_map[id].h_resource);

		texture_map.erase(id);

		AGE_SET_INVALID_ID(id);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// material
namespace age::graphics::render_pipeline::forward_plus
{
	namespace detail
	{
		void
		update_material_impl(pipeline& renderer, t_material_id id, asset::handle h_mat) noexcept
		{
			c_auto& entry = h_mat.get_entry<asset::e::kind::material>();

			auto mat = shared_type::material{
				.base_color_factor			   = entry.base_color_factor,
				.metallic_factor			   = entry.metallic_factor,
				.roughness_factor			   = entry.roughness_factor,
				.emissive_factor			   = entry.emissive_factor,
				.normal_scale				   = entry.normal_scale,
				.occlusion_strength			   = entry.occlusion_strength,
				.alpha_cutoff				   = entry.alpha_cutoff,
				.alpha_mode_and_extra		   = to_idx(entry.alpha_mode),
				.base_color_texture_id		   = age::get_invalid_id<uint32>(),
				.metallic_roughness_texture_id = age::get_invalid_id<uint32>(),
				.normal_texture_id			   = age::get_invalid_id<uint32>(),
				.occlusion_texture_id		   = age::get_invalid_id<uint32>(),
				.emissive_texture_id		   = age::get_invalid_id<uint32>(),
			};

			if (age::runtime::is_handle_invalid(entry.h_tex_base_color) is_false)
			{
				mat.base_color_texture_id = entry.h_tex_base_color.get_entry<asset::e::kind::texture>().render_id;
			}
			if (age::runtime::is_handle_invalid(entry.h_tex_metallic_roughness) is_false)
			{
				mat.metallic_roughness_texture_id = entry.h_tex_metallic_roughness.get_entry<asset::e::kind::texture>().render_id;
			}
			if (age::runtime::is_handle_invalid(entry.h_tex_normal) is_false)
			{
				mat.normal_texture_id = entry.h_tex_normal.get_entry<asset::e::kind::texture>().render_id;
			}
			if (age::runtime::is_handle_invalid(entry.h_tex_occlusion) is_false)
			{
				mat.occlusion_texture_id = entry.h_tex_occlusion.get_entry<asset::e::kind::texture>().render_id;
			}
			if (age::runtime::is_handle_invalid(entry.h_tex_emissive) is_false)
			{
				mat.emissive_texture_id = entry.h_tex_emissive.get_entry<asset::e::kind::texture>().render_id;
			}

			if (resource::resize_buffer_preserve(renderer.h_mapping_material_buffer, sizeof(mat) * (id + 1)))
			{
				renderer.material_buffer.bind(renderer.h_mapping_material_buffer);
			}

			renderer.h_mapping_material_buffer->upload(&mat, sizeof(mat), sizeof(mat) * id);
		}
	}	 // namespace detail

	t_material_id
	pipeline::upload_material(asset::handle h_mat) noexcept
	{
		auto id = static_cast<t_material_id>(material_vec.emplace_back(h_mat));

		detail::update_material_impl(*this, id, h_mat);

		return id;
	}

	void
	pipeline::update_material(asset::handle h_mat) noexcept
	{
		detail::update_material_impl(*this, h_mat.get_entry<asset::e::kind::material>().render_id, h_mat);
	}

	void
	pipeline::release_material(t_material_id& id) noexcept
	{
		material_vec.remove(id);

		AGE_SET_INVALID_ID(id);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// mesh
namespace age::graphics::render_pipeline::forward_plus
{
	t_mesh_id
	pipeline::upload_mesh(asset::handle h_mesh) noexcept
	{
		using enum asset::e::kind;
		c_auto& entry  = h_mesh.get_entry<mesh_baked>();
		c_auto& header = entry.get_header();

		AGE_ASSERT(mesh_persistant_buffer_offset_pool.capacity() < std::numeric_limits<uint32>::max() - header.meshlet_buffer_byte_size);
		AGE_ASSERT(header.meshlet_buffer_byte_size % 4 == 0);

		c_auto flat_index_arr = std::span<const uint32>{ reinterpret_cast<const uint32*>(entry.index_buffer_data()), header.index_count };
		c_auto vertex_pos_arr = std::span<const float3>{ reinterpret_cast<const float3*>(entry.pos_buffer_data()), header.pos_count };

		auto   mesh_byte_offset = mesh_persistant_buffer_offset_pool.allocate(header.meshlet_buffer_byte_size);
		c_auto mesh_need_resize = AGE_IS_INVALID_IDX(mesh_byte_offset);
		c_auto mesh_new_size	= std::max<uint32>(mesh_persistant_buffer_offset_pool.capacity() + static_cast<uint32>(header.meshlet_buffer_byte_size), mesh_persistant_buffer_offset_pool.capacity() * 2);

		auto   index_byte_offset = mesh_rt_index_buffer_offset_pool.allocate(flat_index_arr.size_bytes());
		c_auto index_need_resize = AGE_IS_INVALID_IDX(index_byte_offset);
		c_auto index_new_size	 = std::max<uint32>(mesh_rt_index_buffer_offset_pool.capacity() + static_cast<uint32>(flat_index_arr.size_bytes()), mesh_rt_index_buffer_offset_pool.capacity() * 2);

		c_auto vertex_required_size = 0 + vertex_pos_arr.size_bytes();
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
			auto offset		  = uint32{ 0 };
			for (auto& persistant_mesh_data : mesh_data_vec)
			{
				command::copy_buffer(e::queue_kind::copy, 0,
									 h_new_mesh_buffer->h_resource->p_resource, offset,
									 h_mapping_mesh_buffer->h_resource->p_resource, persistant_mesh_data.offset,
									 persistant_mesh_data.byte_size);
				persistant_mesh_data.offset	 = offset;
				offset						+= persistant_mesh_data.byte_size;
			}

			mesh_byte_offset = offset;
			mesh_persistant_buffer_offset_pool.clear();
			mesh_persistant_buffer_offset_pool.set_capacity(mesh_new_size);
			mesh_persistant_buffer_offset_pool.set_size(static_cast<uint32>(offset + header.meshlet_buffer_byte_size));
		}
		if (index_need_resize)
		{
			h_new_index_buffer = resource::create_buffer_committed(index_new_size);

			auto offset = uint32{ 0 };
			for (auto& persistant_mesh_data : mesh_data_vec)
			{
				command::copy_buffer(e::queue_kind::copy, 0,
									 h_new_index_buffer->h_resource->p_resource, offset,
									 h_mapping_rt_index_buffer->h_resource->p_resource, persistant_mesh_data.rt_idx_offset,
									 persistant_mesh_data.rt_idx_size);
				persistant_mesh_data.rt_idx_offset	= offset;
				offset							   += persistant_mesh_data.rt_idx_size;
			}

			index_byte_offset = offset;
			mesh_rt_index_buffer_offset_pool.clear();
			mesh_rt_index_buffer_offset_pool.set_capacity(index_new_size);
			mesh_rt_index_buffer_offset_pool.set_size(static_cast<uint32>(offset + flat_index_arr.size_bytes()));
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
				resource::unmap_and_release_deferred(h_mapping_mesh_buffer);
				h_mapping_mesh_buffer = h_new_mesh_buffer;

				mesh_data_buffer.bind(h_mapping_mesh_buffer);
			}

			if (index_need_resize)
			{
				resource::unmap_and_release_deferred(h_mapping_rt_index_buffer);
				h_mapping_rt_index_buffer = h_new_index_buffer;

				h_mapping_rt_index_buffer->h_resource->set_name(L"rt_index_buffer");

				rt_index_buffer_srv.bind(h_mapping_rt_index_buffer);
			}
		}

		h_mapping_rt_index_buffer->upload(flat_index_arr.data(), flat_index_arr.size_bytes(), index_byte_offset);
		h_mapping_rt_vertex_scratch_buffer->upload(vertex_pos_arr.data(), vertex_pos_arr.size_bytes());
		h_mapping_mesh_buffer->upload(entry.meshlet_buffer_data(), header.meshlet_buffer_byte_size, mesh_byte_offset);

		auto h_blas = graphics::rt::build_blas(
			defaults::rt::geo_desc::triangles(
				static_cast<uint32>(flat_index_arr.size()),
				static_cast<uint32>(vertex_pos_arr.size()),
				h_mapping_rt_index_buffer->h_resource->get_va() + index_byte_offset,
				h_mapping_rt_vertex_scratch_buffer->h_resource->get_va()));

		auto id = static_cast<t_mesh_id>(mesh_data_vec.emplace_back(
			mesh_data{
				.id					 = static_cast<t_mesh_id>(mesh_data_vec.size()),
				.offset				 = mesh_byte_offset,
				.chunk_srv_id		 = 0,
				.byte_size			 = static_cast<uint32>(header.meshlet_buffer_byte_size),
				.meshlet_count		 = entry.get_mesh_header().meshlet_count,
				.rt_idx_chunk_srv_id = 0,
				.rt_idx_offset		 = index_byte_offset,
				.rt_idx_size		 = static_cast<uint32>(flat_index_arr.size_bytes()),
				.h_blas				 = h_blas }));

		return id;
	}

	void
	pipeline::release_mesh(t_mesh_id& id) noexcept
	{
		auto& mesh_data = mesh_data_vec[id];
		if (mesh_data.chunk_srv_id == 0)
		{
			mesh_persistant_buffer_offset_pool.free(mesh_data.offset, mesh_data.byte_size);
		}
		else
		{
			AGE_UNREACHABLE("not implemented yet");
		}

		if (mesh_data.rt_idx_chunk_srv_id == 0)
		{
			mesh_rt_index_buffer_offset_pool.free(mesh_data.rt_idx_offset, mesh_data.rt_idx_size);
		}
		else
		{
			AGE_UNREACHABLE("not implemented yet");
		}

		resource::release_deferred(mesh_data.h_blas);

		mesh_data_vec.remove(id);

		AGE_SET_INVALID_ID(id);
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
		c_auto quat_encode	= math::quaternion_encode(quat);
		c_auto scale_encode = age::cvt_to<half3>(scale);
		object_data_vec[id] = shared_type::object_data{
			.pos		= pos,
			.quaternion = quat_encode,
			.scale		= scale_encode,
			//			 .scale = scale,
			// .quaternion_debug = quat,
		};

		// mimic encode/decode error
		object_transform_data_vec[id] = simd::transformation(simd::load(cvt_to<float3>(scale_encode)), simd::g::xm_zero_f4, simd::load(quaternion_decode(quat_encode)), simd::load(pos))
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
		// init
		c_auto desc_id = camera_desc_vec.emplace_back(desc);
		c_auto data_id = camera_data_vec.emplace_back(detail::calc_camera_data<true>(desc));

		AGE_ASSERT(desc_id == data_id);

		return static_cast<t_camera_id>(data_id);
	}

	void
	pipeline::set_main_camera(t_camera_id id) noexcept
	{
		main_camera_id = id;
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
		if (id == main_camera_id)
		{
			main_camera_id = 0;
		}

		camera_desc_vec.remove(id);
		camera_data_vec.remove(id);
		id = invalid_id_uint32;
	}

}	 // namespace age::graphics::render_pipeline::forward_plus

// light
namespace age::graphics::render_pipeline::forward_plus
{
	void
	pipeline::update_directional_light(t_directional_light_id id, const directional_light_desc& desc) noexcept
	{
		auto& light					= directional_light_vec[id];
		light.direction				= age::normalize(desc.direction);
		light.intensity				= desc.intensity;
		light.color					= desc.color;
		light.cast_shadow_and_extra = util::set_bit(light.cast_shadow_and_extra, 0, desc.cast_shadow);
	}

	t_directional_light_id
	pipeline::add_directional_light(const directional_light_desc& desc) noexcept
	{
		t_directional_light_id id = static_cast<t_directional_light_id>(directional_light_vec.emplace_back());

		update_directional_light(id, desc);

		return id;
	}

	void
	pipeline::remove_directional_light(t_directional_light_id& id) noexcept
	{
		c_auto& light = directional_light_vec[id];

		directional_light_vec.remove(id);

		id = age::get_invalid_id<t_directional_light_id>();
	}

	void
	pipeline::update_point_light(t_unified_light_id id, const point_light_desc& desc) noexcept
	{
		auto& light = unified_light_vec[id];

		light.position				= desc.position;
		light.range					= desc.range;
		light.color					= math::cvt_to<half3>(desc.color);
		light.intensity				= math::cvt_to<half>(desc.intensity);
		light.direction				= math::cvt_to<half3>(float3{ 1.f, 0.f, 0.f });
		light.cos_inner				= math::cvt_to<half>(-1.f);
		light.cos_outer				= math::cvt_to<half>(-2.f);
		light.cast_shadow_and_extra = util::set_bit(light.cast_shadow_and_extra, 0, desc.cast_shadow);
	}

	t_unified_light_id
	pipeline::add_point_light(const point_light_desc& desc) noexcept
	{
		t_unified_light_id id = static_cast<t_unified_light_id>(unified_light_vec.emplace_back());

		update_point_light(id, desc);

		return id;
	}

	t_unified_light_id
	pipeline::add_spot_light(const spot_light_desc& desc) noexcept
	{
		t_unified_light_id id = static_cast<t_unified_light_id>(unified_light_vec.emplace_back());

		update_spot_light(id, desc);

		return id;
	}

	void
	pipeline::update_spot_light(t_unified_light_id id, const spot_light_desc& desc) noexcept
	{
		auto& light = unified_light_vec[id];

		light.position				= desc.position;
		light.range					= desc.range;
		light.color					= math::cvt_to<half3>(desc.color);
		light.intensity				= math::cvt_to<half>(desc.intensity);
		light.direction				= math::cvt_to<half3>(age::normalize(desc.direction));
		light.cos_inner				= math::cvt_to<half>(desc.cos_inner);
		light.cos_outer				= math::cvt_to<half>(desc.cos_outer);
		light.cast_shadow_and_extra = util::set_bit(light.cast_shadow_and_extra, 0, desc.cast_shadow);
	}

	void
	pipeline::remove_point_light(t_unified_light_id& id) noexcept
	{
		unified_light_vec.remove(id);

		id = age::get_invalid_id<t_unified_light_id>();
	}

	void
	pipeline::remove_spot_light(t_unified_light_id& id) noexcept
	{
		unified_light_vec.remove(id);

		id = age::get_invalid_id<t_unified_light_id>();
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus
{
	t_env_light_id
	pipeline::upload_env_light(asset::handle h_env_light) noexcept
	{
		c_auto& entry		 = h_env_light.get_entry<asset::e::kind::env_light>();
		c_auto& header		 = entry.get_header();
		c_auto& runtime_info = header.runtime_info;
		c_auto& bake_info	 = header.bake_info;
		c_auto	dx12_format	 = graphics::dx12_format(bake_info.format);

		c_auto h_radiance = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d_array(
				  bake_info.cubemap_size, bake_info.cubemap_size,
				  dx12_format,
				  6,
				  D3D12_RESOURCE_FLAG_NONE,
				  std::bit_width(bake_info.cubemap_size)),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		c_auto h_prefilter = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d_array(
				  bake_info.prefilter_size, bake_info.prefilter_size,
				  dx12_format,
				  6,
				  D3D12_RESOURCE_FLAG_NONE,
				  bake_info.prefilter_mip_count),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		c_auto h_irradiance = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d_array(
				  bake_info.irradiance_size, bake_info.irradiance_size,
				  dx12_format,
				  6,
				  D3D12_RESOURCE_FLAG_NONE,
				  1),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		c_auto h_radiance_srv_desc	 = graphics::g::cbv_srv_uav_desc_pool.pop();
		c_auto h_prefilter_srv_desc	 = graphics::g::cbv_srv_uav_desc_pool.pop();
		c_auto h_irradiance_srv_desc = graphics::g::cbv_srv_uav_desc_pool.pop();

		resource::create_view(h_radiance,
							  h_radiance_srv_desc,
							  defaults::srv_view_desc::tex_cube(dx12_format, std::bit_width(bake_info.cubemap_size)));
		resource::create_view(h_prefilter,
							  h_prefilter_srv_desc,
							  defaults::srv_view_desc::tex_cube(dx12_format, bake_info.prefilter_mip_count));
		resource::create_view(h_irradiance,
							  h_irradiance_srv_desc,
							  defaults::srv_view_desc::tex_cube(dx12_format));

		command::begin();
		resource::upload_texture(h_radiance, entry.get_radiance_texture_buffer());
		command::execute_and_wait();
		command::begin();
		resource::upload_texture(h_prefilter, entry.get_prefilter_texture_buffer());
		command::execute_and_wait();
		command::begin();
		resource::upload_texture(h_irradiance, entry.get_irradiance_texture_buffer());
		command::apply_barriers(barrier::tex_copy_dest_to_srv(h_radiance->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING),
								barrier::tex_copy_dest_to_srv(h_prefilter->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING),
								barrier::tex_copy_dest_to_srv(h_irradiance->p_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));
		command::execute_and_wait();
		c_auto gpu_id = env_light_gpu_data_vec.emplace_back(
			shared_type::env_light{
				.radiance_tex_id	 = graphics::calc_desc_idx(h_radiance_srv_desc),
				.prefilter_tex_id	 = graphics::calc_desc_idx(h_prefilter_srv_desc),
				.irradiance_tex_id	 = graphics::calc_desc_idx(h_irradiance_srv_desc),
				.prefilter_mip_count = bake_info.prefilter_mip_count,
				.intensity			 = cvt_to<half>(runtime_info.intensity),
				.tint				 = runtime_info.tint,
				.rotation_mat		 = math::euler_deg_to_mat3x3(runtime_info.euler_deg),
			});

		auto id = static_cast<t_env_light_id>(env_light_cpu_data_vec.emplace_back(
			env_light_data{
				.gpu_id				   = gpu_id,
				.h_radiance			   = h_radiance,
				.h_prefilter		   = h_prefilter,
				.h_irradiance		   = h_irradiance,
				.h_radiance_srv_desc   = h_radiance_srv_desc,
				.h_prefilter_srv_desc  = h_prefilter_srv_desc,
				.h_irradiance_srv_desc = h_irradiance_srv_desc }));

		return id;
	}

	void
	pipeline::update_env_light_runtime(asset::handle h_env_light) noexcept
	{
		c_auto& entry		 = h_env_light.get_entry<asset::e::kind::env_light>();
		c_auto& runtime_info = entry.get_runtime_info();

		c_auto gpu_id = env_light_cpu_data_vec[entry.render_id].gpu_id;

		auto& gpu_data = env_light_gpu_data_vec[gpu_id];

		gpu_data.intensity	  = cvt_to<half>(runtime_info.intensity);
		gpu_data.tint		  = runtime_info.tint;
		gpu_data.rotation_mat = math::euler_deg_to_mat3x3(runtime_info.euler_deg);
	}

	void
	pipeline::release_env_light(t_env_light_id& id) noexcept
	{
		auto& data = env_light_cpu_data_vec[id];

		env_light_gpu_data_vec.remove(data.gpu_id);

		resource::release_deferred(data.h_radiance, data.h_radiance_srv_desc);
		resource::release_deferred(data.h_prefilter, data.h_prefilter_srv_desc);
		resource::release_deferred(data.h_irradiance, data.h_irradiance_srv_desc);

		env_light_cpu_data_vec.remove(id);

		AGE_SET_INVALID_ID(id);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// ui
namespace age::graphics::render_pipeline::forward_plus
{
	age::vector<ui::render_data>&
	pipeline::get_ui_render_data_vec() noexcept
	{
		return ui_render_data_vec;
	}

	age::vector<util::range>&
	pipeline::get_ui_render_data_z_range_vec() noexcept
	{
		return ui_render_data_z_range_vec;
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus
{
	uint32
	pipeline::upload_data() noexcept
	{
		auto  h_mapping_static_buffer				   = h_mapping_static_ring_buffer_arr[graphics::g::frame_buffer_idx];
		auto& h_mapping_rt_instance_buffer			   = h_mapping_rt_instance_buffer_arr[graphics::g::frame_buffer_idx];
		auto& h_mapping_rt_instance_render_data_buffer = h_mapping_rt_instance_render_data_buffer_arr[graphics::g::frame_buffer_idx];
		auto& h_mapping_env_light_buffer			   = h_mapping_env_light_buffer_arr[graphics::g::frame_buffer_idx];

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
				rt_instance_render_data_buffer_srv.bind(h_mapping_rt_instance_render_data_buffer, graphics::g::frame_buffer_idx);
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

			command::apply_barriers(barrier::rt_write_to_rt_read(h_rt_tlas_buffer->p_resource, D3D12_BARRIER_SYNC_COMPUTE_SHADING));
		}

		// env_light
		{
			if (resource::resize_buffer(h_mapping_env_light_buffer, env_light_gpu_data_vec.byte_size()))
			{
				env_light_buffer.bind(h_mapping_env_light_buffer, graphics::g::frame_buffer_idx);
			}

			h_mapping_env_light_buffer->upload(env_light_gpu_data_vec.data(), env_light_gpu_data_vec.byte_size());
		}

		h_mapping_static_buffer->upload(object_data_vec.data(), object_data_vec.byte_size<uint32>(), g::object_data_offset);
		h_mapping_static_buffer->upload(directional_light_vec.data(), directional_light_vec.byte_size<uint32>(), g::directional_light_offset);
		h_mapping_static_buffer->upload(unified_light_vec.data(), unified_light_vec.byte_size<uint32>(), g::unified_light_offset);


		// ui
		{
			auto& h_mapping_ui_data_buffer = h_mapping_ui_data_buffer_arr[graphics::g::frame_buffer_idx];

			if (resource::resize_buffer(h_mapping_ui_data_buffer, ui_render_data_vec.byte_size<uint32>()))
			{
				ui_data_buffer.bind(h_mapping_ui_data_buffer, graphics::g::frame_buffer_idx);
				ui_data_buffer.apply();
			}
			h_mapping_ui_data_buffer->upload(ui_render_data_vec.data(), ui_render_data_vec.byte_size<uint32>());
		}

		c_auto& main_cam_desc = camera_desc_vec[main_camera_id];
		root_constants.bind(shared_type::root_constants{
			.opaque_meshlet_render_data_count  = static_cast<uint32>(opaque_mshlt_object_data_count),
			.directional_light_count_and_extra = static_cast<t_directional_light_id>(directional_light_vec.size()),
			.unified_light_count			   = static_cast<t_unified_light_id>(unified_light_vec.size()),
			.light_tile_count_x				   = light_tile_count_x,
			.light_tile_count_y				   = light_tile_count_y,
			//.cluster_near_z					   = cam_desc.near_z,
			//.cluster_far_z					   = cam_desc.far_z,
			.cam_near_z				= main_cam_desc.near_z,
			.cam_far_z				= main_cam_desc.far_z,
			.cam_log_far_near_ratio = std::log2(main_cam_desc.far_z / main_cam_desc.near_z),
			.env_light_brdf_lut_id	= graphics::calc_desc_idx(h_env_light_brdf_lut),
			.env_light_count		= env_light_gpu_data_vec.size<uint32>(),
			//.shadow_atlas_id		= graphics::calc_desc_idx(h_shadow_atlas_srv_desc),
		});

		// todo multiple camera
		c_auto& main_cam_data = camera_data_vec[main_camera_id];
		c_auto	dt_ns		  = runtime::i_time.get_delta_time_ns();
		c_auto	dt_ms		  = std::chrono::duration<float, std::milli>(dt_ns).count();

		auto frame_d = shared_type::frame_data{
			.view_proj							  = main_cam_data.view_proj,
			.view_proj_inv						  = main_cam_data.view_proj_inv,
			.camera_pos							  = main_cam_data.pos,
			.time								  = dt_ms,
			.inv_backbuffer_size				  = float2{ 1.f / extent.width, 1.f / extent.height },
			.backbuffer_size					  = float2{ static_cast<float>(extent.width), static_cast<float>(extent.height) },
			.camera_forward						  = main_cam_data.forward,
			.frame_index						  = runtime::i_time.get_frame_count(),
			.camera_right						  = main_cam_data.right,
			.main_buffer_texture_id				  = graphics::calc_desc_idx(h_main_buffer_srv_desc),
			.post_buffer_texture_id				  = graphics::calc_desc_idx(h_post_buffer_srv_desc),
			.depth_buffer_texture_id			  = graphics::calc_desc_idx(h_depth_buffer_srv_desc),
			.rt_tlas_buffer_id					  = graphics::calc_desc_idx(h_rt_tlas_buffer_srv_desc),
			.rt_transparent_buffer_srv_texture_id = graphics::calc_desc_idx(h_rt_transparent_tex_buffer_srv_desc),
			.rt_transparent_buffer_uav_texture_id = graphics::calc_desc_idx(h_rt_transparent_tex_buffer_uav_desc),
		};

		std::ranges::copy(main_cam_data.frustum_plane_arr, frame_d.frustum_planes);

		h_mapping_frame_data->upload(&frame_d, sizeof(shared_type::frame_data), sizeof(shared_type::frame_data) * graphics::g::frame_buffer_idx);

		return opaque_mshlt_object_data_count;
	}
}	 // namespace age::graphics::render_pipeline::forward_plus
#endif