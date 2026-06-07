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

		pop_descriptor(h_main_buffer_rtv_desc);
		pop_descriptor(h_post_buffer_rtv_desc);
		pop_descriptor(h_selection_outline_mask_buffer_rtv_desc);
		pop_descriptor(h_depth_buffer_dsv_readonly_desc);
		pop_descriptor(h_depth_buffer_dsv_desc);
		pop_descriptor(h_debug_depth_buffer_dsv_desc);

		pop_descriptor(h_main_buffer_srv_desc);
		pop_descriptor(h_post_buffer_srv_desc);
		pop_descriptor(h_selection_outline_mask_buffer_srv_desc);
		pop_descriptor(h_depth_buffer_srv_desc);
		pop_descriptor(h_rt_tlas_buffer_srv_desc);
		pop_descriptor(h_rt_transparent_tex_buffer_srv_desc);
		pop_descriptor(h_rt_transparent_tex_buffer_uav_desc);
		pop_descriptor(h_env_light_brdf_lut);

		pop_descriptor(h_bloom_srv_desc);
		for (auto& h : h_bloom_mip_uav_desc_arr)
		{
			pop_descriptor(h);
		}

		AGE_ASSERT(ddgi_data_cpu.enabled is_false);

		graphics::resource::create_view(graphics::g::h_brdf_lut, h_env_light_brdf_lut, defaults::srv_view_desc::tex2d(DXGI_FORMAT_R16G16_FLOAT));

		{
			// todo : cleanup
			h_mapping_static_ring_buffer_arr = resource::create_buffer_committed_arr(g::static_buffer_size);

			h_mapping_frame_data = resource::create_buffer_committed(sizeof(shared_type::frame_data) * global::frame_buffer_count);

			h_mapping_mesh_buffer			   = resource::create_buffer_committed(1024);
			h_mapping_rt_index_buffer		   = resource::create_buffer_committed(1024);
			h_mapping_rt_vertex_scratch_buffer = resource::create_buffer_committed(1024);

			h_mapping_material_buffer = resource::create_buffer_committed(sizeof(shared_type::material) * 5);

			h_mapping_env_light_buffer_arr = resource::create_buffer_committed_arr(sizeof(shared_type::env_light) * 1);

			h_mapping_ui_data_buffer_arr	  = resource::create_buffer_committed_arr(sizeof(shared_type::ui_data));
			h_mapping_ui_root_data_buffer_arr = resource::create_buffer_committed_arr(sizeof(shared_type::ui_root_data));

			h_mapping_selection_outline_meshlet_render_data_buffer_arr = resource::create_buffer_committed_arr(sizeof(shared_type::selection_outline_meshlet_render_data));
			h_mapping_selection_outline_data_buffer_arr				   = resource::create_buffer_committed_arr(sizeof(shared_type::selection_outline_data));

			h_mapping_debug_meshlet_render_data_buffer_arr = resource::create_buffer_committed_arr(sizeof(shared_type::debug_meshlet_render_data));
			h_mapping_debug_object_data_buffer_arr		   = resource::create_buffer_committed_arr(sizeof(shared_type::debug_object_data));

			h_scratch_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(g::scratch_buffer_total_size),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_sorted_light_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(shared_type::unified_light) * g::max_light_count),
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

			h_mapping_rt_instance_buffer_arr			 = resource::create_buffer_committed_arr(sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
			h_mapping_rt_instance_render_data_buffer_arr = resource::create_buffer_committed_arr(sizeof(shared_type::rt_instance_render_data));

			h_mapping_rt_raycast_request_buffer_arr = resource::create_buffer_committed_arr(sizeof(shared_type::raycast_request));
			h_rt_raycast_result_buffer				= resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(shared_type::raycast_result)),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });
			h_readback_rt_raycast_result_buffer_arr = resource::create_buffer_committed_arr(sizeof(shared_type::raycast_request), nullptr, e::memory_kind::gpu_to_cpu);

			h_light_bin_stage_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(g::light_bin_stage_buffer_size),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_light_bin_stage_buffer->set_name(L"light_bin_stage_buffer");


			h_indirect_arg_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(shared_type::indirect_arg)),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_indirect_arg_buffer->set_name(L"indirect_arg_buffer");
		}

		{
			static_ring_buffer.bind_array(h_mapping_static_ring_buffer_arr);

			frame_data_buffer.bind_array(h_mapping_frame_data, sizeof(shared_type::frame_data));

			mesh_data_buffer.bind(h_mapping_mesh_buffer);

			scratch_buffer_uav.bind(h_scratch_buffer);

			sorted_light_buffer_srv.bind(h_sorted_light_buffer);
			sorted_light_buffer_uav.bind(h_sorted_light_buffer);

			rt_instance_render_data_buffer_srv.bind_array(h_mapping_rt_instance_render_data_buffer_arr);
			rt_index_buffer_srv.bind(h_mapping_rt_index_buffer);
			rt_raycast_request_buffer_srv.bind_array(h_mapping_rt_raycast_request_buffer_arr);
			rt_raycast_result_buffer_uav.bind(h_rt_raycast_result_buffer);

			ui_data_buffer.bind_array(h_mapping_ui_data_buffer_arr);
			ui_root_data_buffer.bind_array(h_mapping_ui_root_data_buffer_arr);
			selection_outline_meshlet_render_data_buffer.bind_array(h_mapping_selection_outline_meshlet_render_data_buffer_arr);
			selection_outline_data_buffer.bind_array(h_mapping_selection_outline_data_buffer_arr);

			material_buffer.bind(h_mapping_material_buffer);

			env_light_buffer.bind_array(h_mapping_env_light_buffer_arr);

			debug_meshlet_render_data_buffer.bind_array(h_mapping_debug_meshlet_render_data_buffer_arr);
			debug_object_data_buffer.bind_array(h_mapping_debug_object_data_buffer_arr);

			light_bin_stage_buffer_srv.bind(h_light_bin_stage_buffer);
			light_bin_stage_buffer_uav.bind(h_light_bin_stage_buffer);

			indirect_arg_buffer_uav.bind(h_indirect_arg_buffer);
		}

		resource::create_view(h_rt_tlas_buffer_srv_desc, defaults::srv_view_desc::rt_acceleration_structure(h_rt_tlas_buffer));

		create_resolution_dependent_buffers();

		stage_depth.init(h_root_sig);
		stage_skybox.init(h_root_sig);
		stage_light_bin.init(h_root_sig);
		stage_ddgi.init(h_root_sig);
		stage_gibs.init(h_root_sig);
		stage_opaque.init(h_root_sig);
		stage_transparent.init(h_root_sig);
		stage_raycast.init(h_root_sig);
		stage_bloom.init(h_root_sig);
		stage_post_process.init(h_root_sig);
		stage_selection_outline.init(h_root_sig);
		stage_ui.init(h_root_sig);
		stage_presentation.init(h_root_sig);
		stage_debug.init(h_root_sig);

		resource::set_name(h_mapping_static_ring_buffer_arr, L"static_ring_buffer[{}]");

		h_mapping_frame_data->h_resource->set_name(L"mapping_frame_data");
		h_mapping_mesh_buffer->h_resource->set_name(L"mapping_mesh_buffer");

		h_scratch_buffer->set_name(L"scratch_buffer");

		h_sorted_light_buffer->set_name(L"sorted_light_buffer");

		h_rt_tlas_buffer->set_name(L"rt_tlas_buffer");
		h_rt_tlas_scratch_buffer->set_name(L"rt_tlas_scratch_buffer");
		h_mapping_rt_index_buffer->h_resource->set_name(L"rt_index_buffer");
		h_mapping_material_buffer->h_resource->set_name(L"mapping_material_buffer");

		resource::set_name(h_mapping_env_light_buffer_arr, L"mapping_env_light_buffer[{}]");

		resource::set_name(h_mapping_rt_instance_buffer_arr, L"mapping_rt_instance_buffer_arr[{}]");
		resource::set_name(h_mapping_rt_instance_render_data_buffer_arr, L"mapping_rt_instance_render_data_buffer_arr[{}]");
		resource::set_name(h_mapping_rt_raycast_request_buffer_arr, L"mapping_rt_raycast_request_buffer_arr[{}]");
		h_rt_raycast_result_buffer->set_name(L"rt_raycast_result_buffer_uav");
		resource::set_name(h_readback_rt_raycast_result_buffer_arr, L"readback_rt_raycast_result_buffer_arr[{}]");

		resource::set_name(h_mapping_ui_data_buffer_arr, L"mapping_ui_data_buffer_arr[{}]");
		resource::set_name(h_mapping_ui_root_data_buffer_arr, L"mapping_ui_root_data_buffer_arr[{}]");
		resource::set_name(h_mapping_selection_outline_meshlet_render_data_buffer_arr, L"mapping_selection_outline_meshlet_render_data_buffer_arr[{}]");
		resource::set_name(h_mapping_selection_outline_data_buffer_arr, L"mapping_selection_outline_data_buffer_arr[{}]");

		resource::set_name(h_mapping_debug_meshlet_render_data_buffer_arr, L"mapping_debug_meshlet_render_data_buffer_arr[{}]");
		resource::set_name(h_mapping_debug_object_data_buffer_arr, L"mapping_debug_object_data_buffer_arr[{}]");
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

		active_bloom_id			 = get_invalid_id<t_bloom_id>();
		bloom_gpu.srv_texture_id = get_invalid_id<uint32>();
	}

	void
	pipeline::deinit() noexcept
	{
		mesh_persistant_buffer_offset_pool.clear();
		mesh_rt_index_buffer_offset_pool.clear();

		for (auto& vec : opaque_meshlet_render_data_vec)
		{
			vec.clear();
		}
		for (auto& vec : rt_instance_render_data_vec)
		{
			vec.clear();
		}
		for (auto& vec : rt_instance_data_vec)
		{
			vec.clear();
		}

		ui_render_data_vec.clear();
		ui_render_data_z_range_vec.clear();
		ui_render_data_z_range_of_range_vec.clear();

		for (auto& v : ui_root_data_vec_arr)
		{
			v.clear();
		}

		for (auto& id : debug_object_id_vec)
		{
			remove_object(id);
		}

		debug_object_id_vec.clear();
		debug_object_data_vec.clear();
		debug_meshlet_render_data_vec.clear();
		debug_aot_meshlet_render_data_vec.clear();

		stage_debug.deinit();
		stage_presentation.deinit();
		stage_ui.deinit();
		stage_selection_outline.deinit();
		stage_post_process.deinit();
		stage_bloom.deinit();
		stage_raycast.deinit();
		stage_transparent.deinit();
		stage_opaque.deinit();
		stage_gibs.deinit();
		stage_ddgi.deinit();
		stage_light_bin.deinit();
		stage_skybox.deinit();
		stage_depth.deinit();

		root_signature::destroy(h_root_sig);

		push_descriptor(h_main_buffer_rtv_desc);
		push_descriptor(h_post_buffer_rtv_desc);
		push_descriptor(h_selection_outline_mask_buffer_rtv_desc);
		push_descriptor(h_depth_buffer_dsv_readonly_desc);
		push_descriptor(h_depth_buffer_dsv_desc);
		push_descriptor(h_debug_depth_buffer_dsv_desc);

		push_descriptor(h_main_buffer_srv_desc);
		push_descriptor(h_post_buffer_srv_desc);
		push_descriptor(h_selection_outline_mask_buffer_srv_desc);
		push_descriptor(h_depth_buffer_srv_desc);
		push_descriptor(h_rt_tlas_buffer_srv_desc);
		push_descriptor(h_rt_transparent_tex_buffer_srv_desc);
		push_descriptor(h_rt_transparent_tex_buffer_uav_desc);
		push_descriptor(h_env_light_brdf_lut);

		push_descriptor(h_bloom_srv_desc);
		for (auto h : h_bloom_mip_uav_desc_arr)
		{
			push_descriptor(h);
		}

		// static
		resource::unmap_and_release(h_mapping_static_ring_buffer_arr);
		resource::unmap_and_release(h_mapping_frame_data);
		resource::unmap_and_release(h_mapping_mesh_buffer);
		resource::unmap_and_release(h_mapping_rt_index_buffer);
		resource::unmap_and_release(h_mapping_rt_vertex_scratch_buffer);
		resource::unmap_and_release(h_mapping_ui_data_buffer_arr);
		resource::unmap_and_release(h_mapping_ui_root_data_buffer_arr);
		resource::unmap_and_release(h_mapping_selection_outline_meshlet_render_data_buffer_arr);
		resource::unmap_and_release(h_mapping_selection_outline_data_buffer_arr);
		resource::unmap_and_release(h_mapping_material_buffer);
		resource::unmap_and_release(h_mapping_env_light_buffer_arr);
		resource::unmap_and_release(h_mapping_debug_meshlet_render_data_buffer_arr);
		resource::unmap_and_release(h_mapping_debug_object_data_buffer_arr);

		resource::release(h_main_buffer);
		resource::release(h_post_buffer);
		resource::release(h_selection_outline_mask_buffer);
		resource::release(h_depth_buffer);
		resource::release(h_debug_depth_buffer);
		resource::release(h_scratch_buffer);
		resource::release(h_indirect_arg_buffer);
		resource::release(h_light_bin_stage_buffer);
		resource::release(h_sorted_light_buffer);
		resource::release(h_bloom_chain);
		if (ddgi_data_cpu.enabled)
		{
			disable_ddgi();
		}

		if (gibs_data_cpu.enabled)
		{
			disable_gibs();
		}

		// rt
		resource::release(h_rt_tlas_buffer);
		resource::release(h_rt_tlas_scratch_buffer);
		resource::release(h_rt_transparent_texture_buffer);
		resource::unmap_and_release(h_mapping_rt_instance_buffer_arr);
		resource::unmap_and_release(h_mapping_rt_instance_render_data_buffer_arr);
		resource::unmap_and_release(h_mapping_rt_raycast_request_buffer_arr);
		resource::release(h_rt_raycast_result_buffer);
		resource::unmap_and_release(h_readback_rt_raycast_result_buffer_arr);

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
		AGE_ASSERT(bloom_desc_vec.size() == 0);

		texture_map.clear();
		object_data_vec.clear();
		object_transform_data_vec.clear();
		object_generation_vec.clear();
		directional_light_vec.clear();
		unified_light_vec.clear();
		bloom_desc_vec.clear();
	}

	void
	pipeline::begin_frame() noexcept
	{
		command::wait_current_frame(e::queue_kind::direct);

		raycast_request_vec.clear();

		h_readback_rt_raycast_result_buffer_arr[global::i_graphics.get_frame_buffer_idx]->readback(raycast_result_vec[global::i_graphics.get_frame_buffer_idx].data(), raycast_result_vec[global::i_graphics.get_frame_buffer_idx].byte_size());

		for (auto& res : raycast_result_vec[global::i_graphics.get_frame_buffer_idx])
		{
			if (AGE_IS_INVALID_IDX(res.object_id) is_false)
			{
				res.object_id = object_pos_to_id_arr[global::i_graphics.get_frame_buffer_idx][res.object_id & 0x0fffffff];
				//  res.object_id = object_data_vec.nth_id(res.object_id);
			}
		}

		selection_outline_data_vec.clear();
		selection_outline_meshlet_render_data_vec.clear();

		ui_render_data_vec.clear();
		ui_render_data_z_range_vec.clear();
		ui_render_data_z_range_of_range_vec.clear();

		for (auto& v : ui_root_data_vec_arr)
		{
			v.clear();
		}

		for (auto& id : debug_object_id_vec)
		{
			remove_object(id);
		}

		for (auto& vec : opaque_meshlet_render_data_vec)
		{
			vec.clear();
		}
		for (auto& vec : rt_instance_render_data_vec)
		{
			vec.clear();
		}
		for (auto& vec : rt_instance_data_vec)
		{
			vec.clear();
		}

		debug_object_id_vec.clear();
		debug_object_data_vec.clear();

		debug_meshlet_render_data_vec.clear();
		debug_aot_meshlet_render_data_vec.clear();
	}

	bool
	pipeline::begin_render(render_surface_handle h_rs) noexcept
	{
		auto& rs = graphics::g::render_surface_vec[h_rs];

		if (rs.should_render is_false) [[unlikely]]
		{
			return false;
		}


		ui_render_data_vec.clear();
		ui_render_data_z_range_vec.clear();
		for (auto& v : ui_root_data_vec_arr)
		{
			v.clear();
		}

		c_auto new_extent = age::extent_2d<uint16>{
			.width	= static_cast<uint16>(age::platform::get_client_width(rs.h_window)),
			.height = static_cast<uint16>(age::platform::get_client_height(rs.h_window))
		};

		if (extent != new_extent) [[unlikely]]
		{
			resize_resolution_dependent_buffers(new_extent);
		}

		command::begin_frame(e::queue_kind::direct);

		command::set_descriptor_heaps(1, &graphics::g::cbv_srv_uav_desc_heap.p_heap);
		command::set_graphics_root_sig(p_root_sig);
		command::set_compute_root_sig(p_root_sig);

		// command::apply_barriers(barrier::undefined_to_rtv(h_main_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
		//						barrier::undefined_to_rtv(h_post_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
		//						barrier::undefined_to_dsv_write(h_depth_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
		//						barrier::undefined_to_rtv(&rs.get_back_buffer(), D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
		//						barrier::undefined_to_uav(h_rt_transparent_texture_buffer, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_TEXTURE_BARRIER_FLAG_DISCARD));

		if (gibs_enabled() is_false)
		{
			command::apply_barriers(barrier::undefined_to_rtv(h_main_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
									barrier::undefined_to_rtv(h_post_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
									barrier::undefined_to_rtv(h_selection_outline_mask_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
									barrier::undefined_to_dsv_write(h_depth_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
									barrier::undefined_to_dsv_write(h_debug_depth_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
									barrier::undefined_to_rtv(&rs.get_back_buffer(), D3D12_TEXTURE_BARRIER_FLAG_DISCARD));
		}
		else
		{
			command::apply_barriers(barrier::undefined_to_rtv(h_main_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
									barrier::undefined_to_rtv(h_post_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
									barrier::undefined_to_rtv(h_selection_outline_mask_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
									barrier::undefined_to_dsv_write(h_depth_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
									barrier::undefined_to_dsv_write(h_debug_depth_buffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
									barrier::undefined_to_rtv(gibs_data_cpu.h_gbuffer, D3D12_TEXTURE_BARRIER_FLAG_DISCARD),
									barrier::undefined_to_rtv(&rs.get_back_buffer(), D3D12_TEXTURE_BARRIER_FLAG_DISCARD));
		}


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
		auto& render_data_vec		  = opaque_meshlet_render_data_vec[thread_id];
		auto& rt_instance_vec		  = rt_instance_data_vec[thread_id];
		auto& rt_inst_render_data_vec = rt_instance_render_data_vec[thread_id];

		c_auto rt_instance_id_temp = rt_inst_render_data_vec.size<uint32>();

		c_auto& msh_data = mesh_data_vec[mesh_id];

		for (auto meshlet_id = 0u;
			 auto _ : views::loop(msh_data.meshlet_count))
		{
			render_data_vec.emplace_back(
				shared_type::opaque_meshlet_render_data{
					.object_id		   = object_data_vec.get_pos(object_id) | (object_generation_vec[object_id] << 28u),
					.mesh_byte_offset  = msh_data.offset,
					.mesh_chunk_srv_id = msh_data.chunk_srv_id,
					.meshlet_id		   = meshlet_id++,
					.material_id	   = mat_id });
		}

		rt_inst_render_data_vec.emplace_back(
			shared_type::rt_instance_render_data{
				.object_id				= object_data_vec.get_pos(object_id) | (object_generation_vec[object_id] << 28u),
				.mesh_byte_offset		= msh_data.offset,
				.mesh_chunk_srv_id		= msh_data.chunk_srv_id,
				.rt_index_buffer_offset = msh_data.rt_idx_offset / 4,
				.material_id			= mat_id,
				.rt_mask_and_extra		= to_idx(e::rt_mask_kind::opaque) });

		c_auto& transform = object_transform_data_vec[object_id];

		auto desc = D3D12_RAYTRACING_INSTANCE_DESC{
			.InstanceID							 = rt_instance_id_temp,
			.InstanceMask						 = to_idx(e::rt_mask_kind::opaque),
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

		auto& rt_inst_render_data_vec = rt_instance_render_data_vec[thread_id];
		auto& rt_instance_vec		  = rt_instance_data_vec[thread_id];

		c_auto rt_instance_id_temp = rt_inst_render_data_vec.size<uint32>();

		rt_inst_render_data_vec.emplace_back(
			shared_type::rt_instance_render_data{
				.object_id				= object_data_vec.get_pos(object_id) | (object_generation_vec[object_id] << 28u),
				.mesh_byte_offset		= msh_data.offset,
				.mesh_chunk_srv_id		= msh_data.chunk_srv_id,
				.rt_index_buffer_offset = msh_data.rt_idx_offset / 4,
				.material_id			= mat_id,
				.rt_mask_and_extra		= to_idx(e::rt_mask_kind::transparent) });

		c_auto& transform = object_transform_data_vec[object_id];

		auto desc = D3D12_RAYTRACING_INSTANCE_DESC{
			.InstanceID							 = rt_instance_id_temp,
			.InstanceMask						 = to_idx(e::rt_mask_kind::transparent),
			.InstanceContributionToHitGroupIndex = 0,
			.Flags								 = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE,
			.AccelerationStructure				 = mesh_data_vec[mesh_id].h_blas->get_va(),
		};

		std::memcpy(desc.Transform, &transform, sizeof(float3x4));

		rt_instance_vec.emplace_back(desc);
	}

	void
	pipeline::render_selection_outline(t_object_id object_id, asset::handle h_mesh, const float4& rgba, float thickness, float softness) noexcept
	{
		AGE_ASSERT(thickness <= 2.f and thickness >= 0.f);
		AGE_ASSERT(softness <= 1.f and softness >= 0.f);

		auto& outline_data_vec		  = selection_outline_data_vec;
		auto& outline_render_data_vec = selection_outline_meshlet_render_data_vec;

		c_auto& mesh_entry = h_mesh.get_entry<asset::e::kind::mesh_baked>();
		c_auto& msh_data   = mesh_data_vec[mesh_entry.render_id];

		for (auto meshlet_id = 0u;
			 auto _ : views::loop(msh_data.meshlet_count))
		{
			outline_render_data_vec.emplace_back(
				shared_type::selection_outline_meshlet_render_data{
					.object_id						= object_data_vec.get_pos(object_id) | (object_generation_vec[object_id] << 28u),
					.mesh_byte_offset				= msh_data.offset,
					.mesh_chunk_srv_id				= msh_data.chunk_srv_id,
					.meshlet_id						= meshlet_id++,
					.selection_outline_id_and_extra = outline_data_vec.size<uint8>() });
		}

		c_auto thickness_2o6  = static_cast<uint8>(std::round(thickness * float(1 << 6)));	  // 2.6 fixed point encoding;
		c_auto softness_unorm = cvt_to<uint8>(softness, cvt_unorm_tag{});

		outline_data_vec.emplace_back(shared_type::selection_outline_data{
			.rgba					= rgba,
			.thickness_and_softness = static_cast<uint16>(thickness_2o6 | (static_cast<uint32>(softness_unorm) << 8u)),
		});

		AGE_ASSERT(outline_data_vec.size() < 0xff);
	}

	namespace detail
	{
		t_object_id
		render_debug_mesh_impl(pipeline& self, const float3& pos, const float4& quat, const float3& scale, asset::handle h_mesh, const float3& color, bool is_aot, bool draw, bool enable_raycast) noexcept
		{
			AGE_ASSERT(draw or enable_raycast);
			auto& render_data_vec		  = is_aot ? self.debug_aot_meshlet_render_data_vec : self.debug_meshlet_render_data_vec;
			auto& rt_instance_vec		  = self.rt_instance_data_vec[0];
			auto& rt_inst_render_data_vec = self.rt_instance_render_data_vec[0];

			c_auto rt_instance_id_temp = rt_inst_render_data_vec.size<uint32>();

			c_auto& msh_entry = h_mesh.get_entry<asset::e::kind::mesh_baked>();
			c_auto& msh_data  = self.mesh_data_vec[msh_entry.render_id];

			c_auto object_id = self.add_object(pos, quat, scale);

			if (draw)
			{
				for (auto meshlet_id = 0u;
					 auto _ : views::loop(msh_data.meshlet_count))
				{
					render_data_vec.emplace_back(
						shared_type::debug_meshlet_render_data{
							.debug_object_id   = self.debug_object_data_vec.size<uint32>(),
							.mesh_byte_offset  = msh_data.offset,
							.mesh_chunk_srv_id = msh_data.chunk_srv_id,
							.meshlet_id		   = meshlet_id++ });
				}

				self.debug_object_data_vec.emplace_back(shared_type::debug_object_data{ self.object_data_vec.get_pos(object_id), color });
				self.debug_object_id_vec.emplace_back(object_id);
			}

			if (enable_raycast)
			{
				c_auto rt_mask = is_aot ? e::rt_mask_kind::debug | e::rt_mask_kind::always_on_top : e::rt_mask_kind::debug;
				rt_inst_render_data_vec.emplace_back(
					shared_type::rt_instance_render_data{
						.object_id				= self.object_data_vec.get_pos(object_id) | (self.object_generation_vec[object_id] << 28u),
						.mesh_byte_offset		= msh_data.offset,
						.mesh_chunk_srv_id		= msh_data.chunk_srv_id,
						.rt_index_buffer_offset = msh_data.rt_idx_offset / 4,
						.material_id			= get_invalid_id<uint32>(),
						.rt_mask_and_extra		= to_idx(rt_mask) });

				c_auto& transform = self.object_transform_data_vec[object_id];

				auto desc = D3D12_RAYTRACING_INSTANCE_DESC{
					.InstanceID							 = rt_instance_id_temp,
					.InstanceMask						 = to_idx(rt_mask),
					.InstanceContributionToHitGroupIndex = 0,
					.Flags								 = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE,
					.AccelerationStructure				 = self.mesh_data_vec[msh_entry.render_id].h_blas->get_va(),
				};

				std::memcpy(desc.Transform, &transform, sizeof(float3x4));

				rt_instance_vec.emplace_back(desc);
			}

			return object_id;
		}
	}	 // namespace detail

	t_object_id
	pipeline::render_debug_mesh(const float3& pos, const float4& quat, const float3& scale, asset::handle h_mesh, const float3& color, bool draw, bool enable_raycast) noexcept
	{
		return detail::render_debug_mesh_impl(*this, pos, quat, scale, h_mesh, color, false, draw, enable_raycast);
	}

	t_object_id
	pipeline::render_debug_mesh_aot(const float3& pos, const float4& quat, const float3& scale, asset::handle h_mesh, const float3& color, bool draw, bool enable_raycast) noexcept
	{
		return detail::render_debug_mesh_impl(*this, pos, quat, scale, h_mesh, color, true, draw, enable_raycast);
	}

	void
	pipeline::end_render(render_surface_handle h_rs) noexcept
	{
		AGE_ASSERT((ddgi_enabled() and gibs_enabled()) is_false);

		auto& rs = graphics::g::render_surface_vec[h_rs];

		c_auto opaque_meshlet_render_data_count = upload_data();

		object_pos_to_id_arr[global::i_graphics.get_frame_buffer_idx].resize(object_data_vec.size());
		std::memcpy(object_pos_to_id_arr[global::i_graphics.get_frame_buffer_idx].data(), object_data_vec.pos_to_idx_arr(), object_data_vec.size() * sizeof(uint32));


		{
			frame_data_buffer.apply();
			frame_data_buffer.apply_compute();

			static_ring_buffer.apply();
			static_ring_buffer.apply_compute();

			mesh_data_buffer.apply();
			mesh_data_buffer.apply_compute();

			scratch_buffer_uav.apply_compute();

			sorted_light_buffer_uav.apply_compute();

			rt_instance_render_data_buffer_srv.apply_compute();
			rt_instance_render_data_buffer_srv.apply();
			rt_index_buffer_srv.apply_compute();

			rt_raycast_request_buffer_srv.apply_compute();
			rt_raycast_result_buffer_uav.apply_compute();

			ui_data_buffer.apply();
			ui_root_data_buffer.apply();
			selection_outline_meshlet_render_data_buffer.apply();
			selection_outline_data_buffer.apply();

			material_buffer.apply();
			material_buffer.apply_compute();

			env_light_buffer.apply();
			env_light_buffer.apply_compute();

			debug_meshlet_render_data_buffer.apply();
			debug_object_data_buffer.apply();

			indirect_arg_buffer_uav.apply_compute();
		}

		command::set_view_ports(1, &rs.default_viewport);
		command::set_scissor_rects(1, &rs.default_scissor_rect);

		root_constants.apply();
		root_constants.apply_compute();
		light_bin_stage_buffer_uav.apply_compute();

		stage_light_bin.execute(unified_light_vec.size<uint32>(),
								h_light_bin_stage_buffer,
								h_scratch_buffer);
		command::apply_barriers(
			barrier::buf_uav_to_srv(h_sorted_light_buffer, D3D12_BARRIER_SYNC_PIXEL_SHADING | D3D12_BARRIER_SYNC_COMPUTE_SHADING),
			barrier::buf_uav_to_srv(h_light_bin_stage_buffer, D3D12_BARRIER_SYNC_PIXEL_SHADING | D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		sorted_light_buffer_srv.apply();
		light_bin_stage_buffer_srv.apply();

		if (gibs_enabled() is_false)
		{
			stage_depth.execute(h_depth_buffer_dsv_desc, opaque_meshlet_render_data_count);
			command::apply_barriers(barrier::dsv_write_to_generic_read(h_depth_buffer,
																	   D3D12_BARRIER_SYNC_DEPTH_STENCIL | D3D12_BARRIER_SYNC_PIXEL_SHADING,
																	   D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ | D3D12_BARRIER_ACCESS_SHADER_RESOURCE));
		}
		else
		{
			stage_gibs.execute_depth_prepass(gibs_data_cpu, h_depth_buffer_dsv_desc, opaque_meshlet_render_data_count);

			command::apply_barriers(barrier::dsv_write_to_generic_read(h_depth_buffer,
																	   D3D12_BARRIER_SYNC_DEPTH_STENCIL | D3D12_BARRIER_SYNC_PIXEL_SHADING,
																	   D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ | D3D12_BARRIER_ACCESS_SHADER_RESOURCE),
									barrier::rtv_to_srv(gibs_data_cpu.h_gbuffer, D3D12_BARRIER_SYNC_COMPUTE_SHADING));
		}


		stage_skybox.execute(h_main_buffer_rtv_desc, h_depth_buffer_dsv_readonly_desc, env_light_gpu_data_vec.size<uint32>());

		if (ddgi_enabled())
		{
			ddgi_probe_buffer_uav.apply_compute();
			ddgi_scratch_buffer.apply_compute();
			ddgi_scratch_buffer.apply();
			sorted_light_buffer_srv.apply_compute();
			light_bin_stage_buffer_srv.apply_compute();

			stage_ddgi.execute(ddgi_data_cpu, ddgi_probe_buffer_srv, h_indirect_arg_buffer);

			command::apply_barriers(
				barrier::tex_uav_to_srv(ddgi_data_cpu.h_irradiance_atlas, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING | D3D12_BARRIER_SYNC_PIXEL_SHADING),
				barrier::tex_uav_to_srv(ddgi_data_cpu.h_visibility_atlas, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING | D3D12_BARRIER_SYNC_PIXEL_SHADING),
				barrier::buf_uav_to_srv(ddgi_data_cpu.h_probe_buffer, D3D12_BARRIER_SYNC_COMPUTE_SHADING | D3D12_BARRIER_SYNC_PIXEL_SHADING));

			// std::swap(ddgi_data_cpu.idx_prev, ddgi_data_cpu.idx_curr);
		}
		else if (gibs_enabled())
		{
			stage_gibs.execute(gibs_data_cpu, extent, h_indirect_arg_buffer);

			gibs_data_cpu.need_cleanup = false;
		}

		stage_opaque.execute(h_main_buffer_rtv_desc, h_depth_buffer_dsv_readonly_desc, opaque_meshlet_render_data_count);

		if (ddgi_enabled())
		{
			command::apply_barriers(barrier::buf_uav_to_uav(ddgi_data_cpu.h_ddgi_scratch_buffer));
		}

		// debug mesh
		stage_debug.execute(root_constants, h_main_buffer_rtv_desc, h_debug_depth_buffer_dsv_desc, false, debug_meshlet_render_data_vec.size<uint32>(), 0);

		stage_ui.execute(root_constants,
						 h_main_buffer_rtv_desc,
						 h_depth_buffer_dsv_readonly_desc,
						 ui::e::space_mode_kind::world,
						 ui_root_data_idx_arr[to_idx(ui::e::space_mode_kind::world)],
						 ui_root_data_vec_arr[to_idx(ui::e::space_mode_kind::world)].size<uint32>(),
						 ui_render_data_z_range_vec,
						 ui_render_data_z_range_of_range_vec);

		command::apply_barriers(barrier::dsv_generic_read_to_srv(h_depth_buffer,
																 D3D12_BARRIER_SYNC_DEPTH_STENCIL | D3D12_BARRIER_SYNC_PIXEL_SHADING,
																 D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ | D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
																 D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		// command::apply_barriers(
		//	barrier::dsv_read_to_srv(h_depth_buffer, D3D12_BARRIER_SYNC_COMPUTE_SHADING));

		command::apply_barriers(barrier::tex_srv_to_uav(h_rt_transparent_texture_buffer, D3D12_BARRIER_SYNC_PIXEL_SHADING));

		if (ddgi_enabled() is_false)
		{
			sorted_light_buffer_srv.apply_compute();
			light_bin_stage_buffer_srv.apply_compute();
		}

		stage_transparent.execute(h_main_buffer_rtv_desc, h_rt_transparent_texture_buffer, extent);


		if (ddgi_enabled() and ddgi_data_cpu.render_probes)
		{
			ddgi_probe_buffer_srv.apply();
			command::apply_barriers(barrier::buf_uav_to_uav(ddgi_data_cpu.h_ddgi_scratch_buffer));

			stage_ddgi.execute_render_probes(h_main_buffer_rtv_desc,
											 h_debug_depth_buffer_dsv_desc,
											 ddgi_data_cpu);
		}

		if (AGE_IS_INVALID_ID(active_bloom_id))
		{
			command::apply_barriers(barrier::rtv_to_srv(h_main_buffer, D3D12_BARRIER_SYNC_PIXEL_SHADING));
		}
		else
		{
			command::apply_barriers(barrier::rtv_to_srv(h_main_buffer, D3D12_BARRIER_SYNC_PIXEL_SHADING | D3D12_BARRIER_SYNC_COMPUTE_SHADING));
		}

		if (raycast_request_vec.is_empty() is_false)
		{
			stage_raycast.execute(raycast_request_vec.size<uint32>());

			command::apply_barriers(barrier::buf_uav_to_copy_src(h_rt_raycast_result_buffer));

			command::copy_buffer(h_readback_rt_raycast_result_buffer_arr[global::i_graphics.get_frame_buffer_idx]->h_resource->p_resource, 0,
								 h_rt_raycast_result_buffer->p_resource, 0,
								 raycast_request_vec.size() * sizeof(shared_type::raycast_result));
		}

		if (AGE_IS_INVALID_ID(active_bloom_id) is_false)
		{
			stage_bloom.execute(root_constants, h_bloom_chain, bloom_mip_count, bloom_gpu);
		}

		stage_post_process.execute(h_post_buffer_rtv_desc);

		stage_selection_outline.execute(selection_outline_meshlet_render_data_vec.size<uint32>(),
										h_selection_outline_mask_buffer_rtv_desc,
										h_post_buffer_rtv_desc,
										h_selection_outline_mask_buffer);

		// debug aot mesh
		stage_debug.execute(root_constants, h_post_buffer_rtv_desc, h_debug_depth_buffer_dsv_desc, true, debug_aot_meshlet_render_data_vec.size<uint32>(), debug_meshlet_render_data_vec.size<uint32>());

		stage_ui.execute(root_constants,
						 h_post_buffer_rtv_desc,
						 h_depth_buffer_dsv_readonly_desc,	  // not used
						 ui::e::space_mode_kind::world_always_on_top,
						 ui_root_data_idx_arr[to_idx(ui::e::space_mode_kind::world_always_on_top)],
						 ui_root_data_vec_arr[to_idx(ui::e::space_mode_kind::world_always_on_top)].size<uint32>(),
						 ui_render_data_z_range_vec,
						 ui_render_data_z_range_of_range_vec);

		stage_ui.execute(root_constants,
						 h_post_buffer_rtv_desc,
						 h_depth_buffer_dsv_readonly_desc,	  // not used
						 ui::e::space_mode_kind::screen,
						 ui_root_data_idx_arr[to_idx(ui::e::space_mode_kind::screen)],
						 ui_root_data_vec_arr[to_idx(ui::e::space_mode_kind::screen)].size<uint32>(),
						 ui_render_data_z_range_vec,
						 ui_render_data_z_range_of_range_vec);

		command::apply_barriers(barrier::rtv_to_srv(h_post_buffer, D3D12_BARRIER_SYNC_PIXEL_SHADING));

		stage_presentation.execute(rs);

		command::apply_barriers(barrier::rtv_to_present(&rs.get_back_buffer()));

		command::end_frame(e::queue_kind::direct);
	}

	void
	pipeline::create_resolution_dependent_buffers() noexcept
	{
		h_main_buffer = resource::create_committed_tex2d_rtv(extent,
															 graphics::e::texture_format::r16g16b16a16_float,
															 float4{ 0.5f });

		h_post_buffer = resource::create_committed_tex2d_rtv(extent,
															 graphics::e::texture_format::r16g16b16a16_float,
															 float4{ 0.5f });

		h_selection_outline_mask_buffer = resource::create_committed_tex2d_rtv(extent,
																			   graphics::e::texture_format::r8_uint,
																			   float4{ 255.f });

		h_depth_buffer = resource::create_committed_tex2d_dsv(extent,
															  graphics::e::texture_format::d32_float,
															  0.f, 0, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ);

		h_debug_depth_buffer = resource::create_committed_tex2d_dsv(extent,
																	graphics::e::texture_format::d16_unorm,
																	0.f, 0, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ);

		h_rt_transparent_texture_buffer = resource::create_committed_tex2d_uav(extent,
																			   graphics::e::texture_format::r16g16b16a16_float,
																			   D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE);

		h_bloom_chain = resource::create_committed_tex2d_uav(extent_2d<uint32>{ extent.width / 2u, extent.height / 2u },
															 graphics::e::texture_format::r11g11b10_float,
															 D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE,
															 bloom_mip_count);

		h_main_buffer->set_name(L"main_buffer");
		h_depth_buffer->set_name(L"depth_buffer");
		h_post_buffer->set_name(L"post_buffer");
		h_selection_outline_mask_buffer->set_name(L"selection_outline_mask_buffer");
		h_debug_depth_buffer->set_name(L"debug_depth_buffer");
		h_rt_transparent_texture_buffer->set_name(L"rt_transparent_buffer");
		h_bloom_chain->set_name(L"bloom_chain");

		resource::create_view(h_main_buffer,
							  h_main_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(graphics::e::texture_format::r16g16b16a16_float));

		resource::create_view(h_main_buffer,
							  h_main_buffer_rtv_desc,
							  defaults::rtv_view_desc::hdr_rgba16_2d);

		resource::create_view(h_post_buffer,
							  h_post_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(graphics::e::texture_format::r16g16b16a16_float));

		resource::create_view(h_post_buffer,
							  h_post_buffer_rtv_desc,
							  defaults::rtv_view_desc::hdr_rgba16_2d);

		resource::create_view(h_selection_outline_mask_buffer,
							  h_selection_outline_mask_buffer_rtv_desc,
							  defaults::rtv_view_desc::r8_uint_2d);

		resource::create_view(h_selection_outline_mask_buffer,
							  h_selection_outline_mask_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(graphics::e::texture_format::r8_uint));

		resource::create_view(h_depth_buffer,
							  h_depth_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(graphics::e::texture_format::r32_float));

		resource::create_view(h_depth_buffer,
							  h_depth_buffer_dsv_desc,
							  defaults::dsv_view_desc::d32_float_2d);

		resource::create_view(h_depth_buffer,
							  h_depth_buffer_dsv_readonly_desc,
							  defaults::dsv_view_desc::d32_float_2d_readonly);

		resource::create_view(h_debug_depth_buffer,
							  h_debug_depth_buffer_dsv_desc,
							  defaults::dsv_view_desc::d16_unorm_2d);

		resource::create_view(h_rt_transparent_texture_buffer,
							  h_rt_transparent_tex_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(graphics::e::texture_format::r16g16b16a16_float));

		resource::create_view(h_rt_transparent_texture_buffer,
							  h_rt_transparent_tex_buffer_uav_desc,
							  defaults::uav_view_desc::tex2d(graphics::e::texture_format::r16g16b16a16_float));

		resource::create_view(h_bloom_chain,
							  h_bloom_srv_desc,
							  defaults::srv_view_desc::tex2d(graphics::e::texture_format::r11g11b10_float, bloom_mip_count));

		bloom_gpu.srv_texture_id = calc_desc_idx(h_bloom_srv_desc);
		for (auto mip : views::loop(bloom_mip_count))
		{
			resource::create_view(h_bloom_chain,
								  h_bloom_mip_uav_desc_arr[mip],
								  defaults::uav_view_desc::tex2d(graphics::e::texture_format::r11g11b10_float, mip));

			bloom_gpu.uav_texture_id_arr[mip] = calc_desc_idx(h_bloom_mip_uav_desc_arr[mip]);
		}

		if (gibs_enabled())
		{
			resource::release_deferred(gibs_data_cpu.h_gbuffer);

			gibs_data_cpu.h_gbuffer = resource::create_committed_tex2d_rtv(extent, graphics::e::texture_format::r32g32_uint);
			gibs_data_cpu.h_gbuffer->set_name(L"gibs_gbuffer");

			resource::create_view(gibs_data_cpu.h_gbuffer,
								  gibs_data_cpu.h_gbuffer_srv_desc,
								  defaults::srv_view_desc::tex2d(graphics::e::texture_format::r32g32_uint));
			resource::create_view(gibs_data_cpu.h_gbuffer,
								  gibs_data_cpu.h_gbuffer_rtv_desc,
								  defaults::rtv_view_desc::tex2d(graphics::e::texture_format::r32g32_uint));
		}
	}

	void
	pipeline::resize_resolution_dependent_buffers(const age::extent_2d<uint16>& new_extent) noexcept
	{
		extent				= new_extent;
		c_auto bloom_extent = extent_2d{ static_cast<uint16>(extent.width / 2u), static_cast<uint16>(extent.height / 2u) };
		bloom_mip_count		= max(min(calc_mip_count<uint16>(bloom_extent.width, bloom_extent.height), g::max_bloom_mip_count), uint16{ 1 });
		{
			auto mip = uint16{ 1 };
			for (auto w = bloom_extent.width, h = bloom_extent.height; mip < g::max_bloom_mip_count and min<uint16>(w >> 1u, h >> 1u) >= g::min_bloom_mip_pixel; ++mip)
			{
				w >>= 1u;
				h >>= 1u;
			}
			bloom_mip_count = mip;
		}

		bloom_gpu.width	 = bloom_extent.width;
		bloom_gpu.height = bloom_extent.height;

		resource::release(h_main_buffer);
		resource::release(h_post_buffer);
		resource::release(h_selection_outline_mask_buffer);
		resource::release(h_depth_buffer);
		resource::release(h_debug_depth_buffer);
		resource::release(h_rt_transparent_texture_buffer);
		resource::release(h_bloom_chain);

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

		c_auto h_srv_desc = pop_descriptor<srv_desc_handle>();

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
		command::apply_barriers(barrier::tex_copy_dest_to_srv(h_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));
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

		c_auto h_srv_desc = pop_descriptor<srv_desc_handle>();

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
		command::apply_barriers(barrier::tex_copy_dest_to_srv(h_resource, D3D12_BARRIER_SYNC_PIXEL_SHADING));
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
	pipeline::add_object(const float3& pos, const float4& quat, const float3& scale) noexcept
	{
		AGE_ASSERT(object_data_vec.size() < g::max_object_count);
		c_auto id = static_cast<t_object_id>(object_data_vec.emplace_back());
		object_transform_data_vec.emplace_back();

		object_generation_vec.resize(id + 1);
		object_generation_vec[id] = (object_generation_vec[id] + 1) & 0xf;

		update_object(id, pos, quat, scale);

		return id;
	}

	void
	pipeline::update_object(t_object_id id, const float3& pos, const float4& quat, const float3& scale) noexcept
	{
		// c_auto quat_encode	= math::quaternion_encode(quat);
		// c_auto scale_encode = age::cvt_to<half3>(scale);
		object_data_vec[id] = shared_type::object_data{
			.pos = pos,
			// .quaternion = quat_encode,
			.quaternion = quat,
			//.scale		= scale_encode,
			.scale = scale,
			// .quaternion_debug = quat,

			.gen_and_extra = object_generation_vec[id],
		};

		// mimic encode/decode error
		// object_transform_data_vec[id] = simd::transformation(simd::load(cvt_to<float3>(scale_encode)), simd::g::xm_zero_f4, simd::load(quaternion_decode(quat_encode)), simd::load(pos))
		//							  | simd::mat_transpose()
		//							  | simd::to<float3x4>();

		object_transform_data_vec[id] = simd::transformation_mat3x4(simd::load(scale), simd::g::xm_zero_f4, simd::load(quat), simd::load(pos));
		// object_transform_data_vec[id] = simd::transformation_mat3x4(simd::load(scale), simd::g::xm_zero_f4, simd::load(quaternion_decode(quat_encode)), simd::load(pos));
		//  object_transform_data_vec[id] = simd::transformation_mat3x4(simd::load(cvt_to<float3>(scale_encode)), simd::g::xm_zero_f4, simd::load(quaternion_decode(quat_encode)), simd::load(pos));
		//   object_transform_data_vec[id] = simd::transformation_mat3x4(simd::load(scale), simd::g::xm_zero_f4, simd::load(quat), simd::load(pos));
	}

	shared_type::object_data
	pipeline::get_object_data(t_object_id id) noexcept
	{
		return object_data_vec[id];
	}

	float4x4
	pipeline::get_object_transform_matrix(t_object_id id) const noexcept
	{
		return float4x4{ object_transform_data_vec[id], float4{ 0, 0, 0, 1 } };
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
			c_auto xm_forward		 = xm_quat | simd::rotate3(simd::g::xm_forward_f4);
			c_auto xm_up			 = xm_quat | simd::rotate3(simd::g::xm_up_f4);

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
					.right			   = xm_quat | simd::rotate3(simd::g::xm_right_f4) | simd::to<float3>(),
					.up				   = xm_up | simd::to<float3>(),
					.view			   = xm_view | simd::to<float4x4>(),
					.proj			   = xm_proj | simd::to<float4x4>(),
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

	camera_data
	pipeline::get_camera_data(t_camera_id id) noexcept
	{
		return camera_data_vec[id];
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

		c_auto h_radiance_srv_desc	 = pop_descriptor<srv_desc_handle>();
		c_auto h_prefilter_srv_desc	 = pop_descriptor<srv_desc_handle>();
		c_auto h_irradiance_srv_desc = pop_descriptor<srv_desc_handle>();

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
		command::apply_barriers(barrier::tex_copy_dest_to_srv(h_radiance, D3D12_BARRIER_SYNC_PIXEL_SHADING),
								barrier::tex_copy_dest_to_srv(h_prefilter, D3D12_BARRIER_SYNC_PIXEL_SHADING),
								barrier::tex_copy_dest_to_srv(h_irradiance, D3D12_BARRIER_SYNC_PIXEL_SHADING));
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
	std::tuple<
		age::vector<ui::render_data>&,
		age::vector<util::range>&,
		age::vector<util::range>&,
		age::array<age::vector<ui::root_graphics_data>, ui::e::space_mode_kind_size>&>
	pipeline::get_ui_sink() noexcept
	{
		return std::tie(ui_render_data_vec, ui_render_data_z_range_vec, ui_render_data_z_range_of_range_vec, ui_root_data_vec_arr);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus
{
	t_raycast_id
	pipeline::request_raycast(const float3& origin, const float3& dir, float t_max, e::rt_mask_kind mask) noexcept
	{
		AGE_ASSERT(raycast_request_vec.size() < 0x00ff'ffff);
		static_assert(global::frame_buffer_count < 0xff);

		auto id = t_raycast_id{ ((global::i_graphics.get_frame_buffer_idx & 0xff) << 24) | (raycast_request_vec.size<uint32>() & 0x00ff'ffff) };

		raycast_request_vec.emplace_back(shared_type::raycast_request{
			.origin			= origin,
			.dir			= dir,
			.t_max			= t_max,
			.mask_and_extra = to_idx(mask) });

		return id;
	}

	shared_type::raycast_result
	pipeline::get_raycast_result(t_raycast_id id) noexcept
	{
		// AGE_ASSERT((id >> 24) % global::frame_buffer_count == global::i_graphics.get_frame_buffer_idx);

		c_auto idx		 = id & 0x00ff'ffff;
		c_auto frame_idx = (id & 0xff00'0000) >> 24u;

		if (frame_idx < global::frame_buffer_count and idx < raycast_result_vec[frame_idx].size<uint32>())
		{
			return raycast_result_vec[frame_idx][idx];
		}
		else
		{
			return shared_type::raycast_result{
				.object_id = get_invalid_idx<uint32>()
			};
		}
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

// bloom
namespace age::graphics::render_pipeline::forward_plus
{
	// bloom
	t_bloom_id
	pipeline::add_bloom(const bloom_desc& desc, bool set_active) noexcept
	{
		AGE_ASSERT(bloom_desc_vec.size() < std::numeric_limits<t_bloom_id>::max());
		c_auto id = static_cast<t_bloom_id>(bloom_desc_vec.emplace_back());

		update_bloom(id, desc);
		set_bloom_active(id, set_active);

		return id;
	}

	void
	pipeline::update_bloom(t_bloom_id id, const bloom_desc& desc) noexcept
	{
		bloom_desc_vec[id] = desc;
	}

	void
	pipeline::set_bloom_active(t_bloom_id id, bool active) noexcept
	{
		if (active)
		{
			bloom_gpu.srv_texture_id = calc_desc_idx(h_bloom_srv_desc);
			active_bloom_id			 = id;
		}
		else
		{
			AGE_SET_INVALID_ID(active_bloom_id);
			AGE_SET_INVALID_ID(bloom_gpu.srv_texture_id);
		}
	}

	void
	pipeline::remove_bloom(t_bloom_id& id) noexcept
	{
		bloom_desc_vec.remove(id);
		if (active_bloom_id == id)
		{
			AGE_SET_INVALID_ID(active_bloom_id);
			AGE_SET_INVALID_ID(bloom_gpu.srv_texture_id);
		}
		AGE_SET_INVALID_ID(id);
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus
{
	// ddgi
	void
	pipeline::enable_ddgi(const ddgi_desc& desc) noexcept
	{
		AGE_ASSERT(ddgi_data_cpu.enabled is_false);

		auto& ddgi_data_gpu = ddgi_data_cpu.ddgi_data_gpu;

		c_auto ppl_log2_x	= log2_pow2<uint8>(desc.probe_per_level_axis.x);
		c_auto ppl_log2_y	= log2_pow2<uint8>(desc.probe_per_level_axis.y);
		c_auto ppl_log2_z	= log2_pow2<uint8>(desc.probe_per_level_axis.z);
		c_auto ppl_bitwidth = cast_to<uint8>(ppl_log2_x + ppl_log2_y + ppl_log2_z);

		AGE_ASSERT(ppl_bitwidth <= age::g::uint8_max);

		c_auto probe_count		  = (1u << ppl_bitwidth) * desc.level_count;
		c_auto prefix_group_count = util::ceil(probe_count, g::ddgi_prefix_element_per_group);

		uint32 tile_count_w_log2 = 0u;
		while (true)
		{
			c_auto w = 1u << tile_count_w_log2;
			c_auto h = util::ceil(probe_count, w);
			if (w >= h) { break; }
			++tile_count_w_log2;
		}

		c_auto tile_count_w = 1u << tile_count_w_log2;
		c_auto tile_count_h = util::ceil(probe_count, tile_count_w);

		ddgi_data_cpu.irradiance_atlas_extent = extent_2d<uint32>{ .width = tile_count_w * g::ddgi_irradiance_tile_size, .height = tile_count_h * g::ddgi_irradiance_tile_size };
		ddgi_data_cpu.visibility_atlas_extent = extent_2d<uint32>{ .width = tile_count_w * g::ddgi_visibility_tile_size, .height = tile_count_h * g::ddgi_visibility_tile_size };


		c_auto offset_weight_sum	   = uint32{ 0u };
		c_auto ray_count_offset		   = offset_weight_sum + probe_count * sizeof(uint32);
		c_auto ray_prefix_offset	   = ray_count_offset + probe_count * sizeof(uint32);
		c_auto probe_state_offset	   = ray_prefix_offset + probe_count * sizeof(uint32);
		c_auto ray_group_count_offset  = probe_state_offset + probe_count * sizeof(uint16);
		c_auto ray_group_prefix_offset = ray_group_count_offset + prefix_group_count * sizeof(uint32);
		c_auto ray_result_offset	   = ray_group_prefix_offset + (1u + prefix_group_count) * sizeof(uint32);
		c_auto buffer_byte_size		   = ray_result_offset + g::ddgi_ray_budget * sizeof(shared_type::ddgi_ray_result);

		AGE_LOG(buffer_byte_size);

		ddgi_data_gpu = shared_type::ddgi_data{
			.ppl_log_2_and_ppl_bitwidth		= cast_to<uint32>((ppl_log2_x << 0u) | (ppl_log2_y << 8u) | (ppl_log2_z << 16u) | (ppl_bitwidth << 24u)),
			.base_probe_spacing				= desc.base_probe_spacing,
			.level_count__tile_count_w_log2 = (desc.level_count & 0xff) | (tile_count_w_log2 << 8u),
			.tile_count_h_float				= cast_to<float>(tile_count_h),
			.debug_flags					= to_idx(desc.debug_flags),

			.ray_count_offset		 = cast_to<uint32>(ray_count_offset),
			.ray_prefix_offset		 = cast_to<uint32>(ray_prefix_offset),
			.ray_group_count_offset	 = cast_to<uint32>(ray_group_count_offset),
			.ray_group_prefix_offset = cast_to<uint32>(ray_group_prefix_offset),
			.ray_result_offset		 = cast_to<uint32>(ray_result_offset),
		};


		pop_descriptor(ddgi_data_cpu.h_irradiance_srv_desc);
		pop_descriptor(ddgi_data_cpu.h_irradiance_uav_desc);
		pop_descriptor(ddgi_data_cpu.h_visibility_srv_desc);
		pop_descriptor(ddgi_data_cpu.h_visibility_uav_desc);

		pop_descriptor(ddgi_data_cpu.h_irradiance_clear_uav_desc);
		pop_descriptor(ddgi_data_cpu.h_visibility_clear_uav_desc);
		pop_descriptor(ddgi_data_cpu.h_probe_buffer_clear_uav_desc);
		ddgi_data_cpu.h_irradiance_atlas = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d(
				  ddgi_data_cpu.irradiance_atlas_extent,
				  DXGI_FORMAT_R11G11B10_FLOAT,
				  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		ddgi_data_cpu.h_visibility_atlas = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::texture_2d(
				  ddgi_data_cpu.visibility_atlas_extent,
				  DXGI_FORMAT_R16G16_FLOAT,
				  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			  .initial_layout	= D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,
			  .heap_memory_kind = e::memory_kind::gpu_only });

		ddgi_data_cpu.h_probe_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(shared_type::ddgi_probe) * cast_to<uint64>(1u << ppl_bitwidth) * desc.level_count),
			  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
			  .heap_memory_kind	   = e::memory_kind::gpu_only,
			  .has_clear_value	   = false });

		ddgi_data_cpu.h_ddgi_scratch_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(buffer_byte_size),
			  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
			  .heap_memory_kind	   = e::memory_kind::gpu_only,
			  .has_clear_value	   = false });

		resource::create_view(ddgi_data_cpu.h_irradiance_atlas, ddgi_data_cpu.h_irradiance_srv_desc, defaults::srv_view_desc::tex2d(DXGI_FORMAT_R11G11B10_FLOAT));
		resource::create_view(ddgi_data_cpu.h_irradiance_atlas, ddgi_data_cpu.h_irradiance_uav_desc, defaults::uav_view_desc::tex2d(DXGI_FORMAT_R11G11B10_FLOAT));
		resource::create_view(ddgi_data_cpu.h_visibility_atlas, ddgi_data_cpu.h_visibility_srv_desc, defaults::srv_view_desc::tex2d(DXGI_FORMAT_R16G16_FLOAT));
		resource::create_view(ddgi_data_cpu.h_visibility_atlas, ddgi_data_cpu.h_visibility_uav_desc, defaults::uav_view_desc::tex2d(DXGI_FORMAT_R16G16_FLOAT));

		resource::create_view(ddgi_data_cpu.h_irradiance_atlas, ddgi_data_cpu.h_irradiance_clear_uav_desc, defaults::uav_view_desc::tex2d(DXGI_FORMAT_R11G11B10_FLOAT));
		resource::create_view(ddgi_data_cpu.h_visibility_atlas, ddgi_data_cpu.h_visibility_clear_uav_desc, defaults::uav_view_desc::tex2d(DXGI_FORMAT_R16G16_FLOAT));
		resource::create_view(ddgi_data_cpu.h_probe_buffer, ddgi_data_cpu.h_probe_buffer_clear_uav_desc, defaults::uav_view_desc::byte_address_buffer(probe_count * sizeof(shared_type::ddgi_probe)));

		ddgi_data_cpu.h_irradiance_atlas->set_name(L"ddgi_irradiance_atlas[0]");
		ddgi_data_cpu.h_visibility_atlas->set_name(L"ddgi_visibility_atlas[0]");
		ddgi_data_cpu.h_probe_buffer->set_name(L"ddgi_probe_buffer");
		ddgi_data_cpu.h_ddgi_scratch_buffer->set_name(L"ddgi_scratch_buffer");
		ddgi_probe_buffer_srv.bind(ddgi_data_cpu.h_probe_buffer);
		ddgi_probe_buffer_uav.bind(ddgi_data_cpu.h_probe_buffer);
		ddgi_scratch_buffer.bind(ddgi_data_cpu.h_ddgi_scratch_buffer);

		ddgi_data_cpu.enabled	   = true;
		ddgi_data_cpu.need_cleanup = true;
		if (desc.lock_origin and (ddgi_data_cpu.lock_origin is_false))
		{
			ddgi_data_cpu.origin = camera_data_vec[main_camera_id].pos;
		}
		ddgi_data_cpu.lock_origin	= desc.lock_origin;
		ddgi_data_cpu.render_probes = has_all(desc.debug_flags, graphics::e::ddgi_debug_flags::render_probe);

		ddgi_data_gpu.irradiance_atlas_srv_id = calc_desc_idx(ddgi_data_cpu.h_irradiance_srv_desc);
		ddgi_data_gpu.irradiance_atlas_uav_id = calc_desc_idx(ddgi_data_cpu.h_irradiance_uav_desc);
		ddgi_data_gpu.visibility_atlas_srv_id = calc_desc_idx(ddgi_data_cpu.h_visibility_srv_desc);
		ddgi_data_gpu.visibility_atlas_uav_id = calc_desc_idx(ddgi_data_cpu.h_visibility_uav_desc);
	}

	void
	pipeline::disable_ddgi() noexcept
	{
		AGE_ASSERT(ddgi_data_cpu.enabled is_true);

		resource::release_deferred(ddgi_data_cpu.h_irradiance_atlas);
		resource::release_deferred(ddgi_data_cpu.h_visibility_atlas);
		resource::release_deferred(ddgi_data_cpu.h_probe_buffer);
		resource::release_deferred(ddgi_data_cpu.h_ddgi_scratch_buffer);

		push_descriptor(ddgi_data_cpu.h_irradiance_srv_desc);
		push_descriptor(ddgi_data_cpu.h_irradiance_uav_desc);
		push_descriptor(ddgi_data_cpu.h_visibility_srv_desc);
		push_descriptor(ddgi_data_cpu.h_visibility_uav_desc);

		push_descriptor(ddgi_data_cpu.h_irradiance_clear_uav_desc);
		push_descriptor(ddgi_data_cpu.h_visibility_clear_uav_desc);
		push_descriptor(ddgi_data_cpu.h_probe_buffer_clear_uav_desc);

		ddgi_data_cpu.enabled = false;
	}

	void
	pipeline::update_ddgi(const ddgi_desc& desc) noexcept
	{
		// todo
		// not going to call this function often at runtime anyway
		AGE_ASSERT(ddgi_data_cpu.enabled);
		disable_ddgi();
		enable_ddgi(desc);
	}

	bool
	pipeline::ddgi_enabled() const noexcept
	{
		return ddgi_data_cpu.enabled;
	}

	// for editor
	void
	pipeline::clear_ddgi() noexcept
	{
		// todo
	}

	float
	pipeline::get_ddgi_convergence() noexcept
	{
		// todo
		return 0.f;
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus
{
	void
	pipeline::enable_gibs(const gibs_desc& desc) noexcept
	{
		AGE_ASSERT(gibs_data_cpu.enabled is_false);

		AGE_ASSERT(desc.max_surfel_count <= g::gibs_max_surfel_count);
		AGE_ASSERT(desc.outer_layer_count <= g::gibs_max_outer_layer_count);
		AGE_ASSERT(desc.outer_cell_size_factor > 1.f);


		auto& gibs_data_gpu = gibs_data_cpu.gibs_data_gpu;

		{
			gibs_data_cpu.h_surfel_buffer = resource::create_committed_buf_uav(desc.max_surfel_count * sizeof(shared_type::surfel));
			gibs_data_cpu.h_surfel_buffer->set_name(L"gibs_surfel_buffer");
			gibs_data_cpu.h_surfel_buffer_srv_desc = resource::create_view(gibs_data_cpu.h_surfel_buffer,
																		   defaults::srv_view_desc::structured_buffer(desc.max_surfel_count, sizeof(shared_type::surfel)));

			gibs_data_cpu.h_surfel_buffer_uav_desc = resource::create_view(gibs_data_cpu.h_surfel_buffer,
																		   defaults::uav_view_desc::structured_buffer(desc.max_surfel_count, sizeof(shared_type::surfel)));
		}
		c_auto cell_count_total = desc.cell_count * desc.cell_count * desc.cell_count
								+ 6 * (desc.cell_count * desc.cell_count) * desc.outer_layer_count;

		{
			c_auto cell_info_buffer_size = (sizeof(shared_type::gibs_cell_entry)) * cell_count_total + sizeof(uint32) * desc.max_surfel_count;
			AGE_ASSERT(cell_info_buffer_size < math::g::uint32_max);
			gibs_data_cpu.h_cell_info_buffer = resource::create_committed_buf_uav(cast_to<uint32>(cell_info_buffer_size));
			gibs_data_cpu.h_cell_info_buffer->set_name(L"gibs_cell_info_buffer");
			gibs_data_cpu.h_cell_info_srv_desc = resource::create_view(gibs_data_cpu.h_cell_info_buffer,
																	   defaults::srv_view_desc::byte_address_buffer(cast_to<uint32>(cell_info_buffer_size)));

			gibs_data_cpu.h_cell_info_uav_desc = resource::create_view(gibs_data_cpu.h_cell_info_buffer,
																	   defaults::uav_view_desc::byte_address_buffer(cast_to<uint32>(cell_info_buffer_size)));
		}

		{
			gibs_data_cpu.h_irradiance_atlas = resource::create_committed_tex2d_uav(g::gibs_atlas_extent, graphics::e::texture_format::r16_float);
			gibs_data_cpu.h_irradiance_atlas->set_name(L"gibs_irradiance_atlas");

			gibs_data_cpu.h_irradiance_atlas_srv_desc = resource::create_view(gibs_data_cpu.h_irradiance_atlas,
																			  defaults::srv_view_desc::tex2d(graphics::e::texture_format::r16_float));
			gibs_data_cpu.h_irradiance_atlas_uav_desc = resource::create_view(gibs_data_cpu.h_irradiance_atlas,
																			  defaults::uav_view_desc::tex2d(graphics::e::texture_format::r16_float));
		}

		{
			gibs_data_cpu.h_visibility_atlas = resource::create_committed_tex2d_uav(g::gibs_atlas_extent, graphics::e::texture_format::r16g16_float);
			gibs_data_cpu.h_visibility_atlas->set_name(L"gibs_visibility_atlas");

			gibs_data_cpu.h_visibility_atlas_srv_desc = resource::create_view(gibs_data_cpu.h_visibility_atlas,
																			  defaults::srv_view_desc::tex2d(graphics::e::texture_format::r16g16_float));
			gibs_data_cpu.h_visibility_atlas_uav_desc = resource::create_view(gibs_data_cpu.h_visibility_atlas,
																			  defaults::uav_view_desc::tex2d(graphics::e::texture_format::r16g16_float));
		}

		{
			gibs_data_cpu.h_gbuffer = resource::create_committed_tex2d_rtv(extent, graphics::e::texture_format::r32g32_uint);
			gibs_data_cpu.h_gbuffer->set_name(L"gibs_gbuffer");

			gibs_data_cpu.h_gbuffer_srv_desc = resource::create_view(gibs_data_cpu.h_gbuffer,
																	 defaults::srv_view_desc::tex2d(graphics::e::texture_format::r32g32_uint));
			gibs_data_cpu.h_gbuffer_rtv_desc = resource::create_view(gibs_data_cpu.h_gbuffer,
																	 defaults::rtv_view_desc::tex2d(graphics::e::texture_format::r32g32_uint));
		}

		{
			c_auto ray_count_reduce_group_count = util::ceil(desc.max_surfel_count, g::gibs_ray_count_reduce_epg);

			auto offset_calculator = util::offset_calculator{};

			c_auto dead_surfel_id_stack_offset		 = offset_calculator + sizeof(uint32) * (1 + desc.max_surfel_count);
			c_auto alive_surfel_id_stack_prev_offset = offset_calculator + sizeof(uint32) * (1 + desc.max_surfel_count);
			c_auto alive_surfel_id_stack_curr_offset = offset_calculator + sizeof(uint32) * (1 + desc.max_surfel_count);

			c_auto cell_alloc_atomic_counter_offset	  = offset_calculator + sizeof(uint32);
			c_auto ray_count_ideal_sum_counter_offset = offset_calculator + sizeof(uint32);
			c_auto ray_alloc_ray_counter_offset		  = offset_calculator + sizeof(uint32);
			c_auto surfel_geometry_offset			  = offset_calculator + sizeof(shared_type::surfel_geometry) * desc.max_surfel_count;
			c_auto surfel_msme_offset				  = offset_calculator + sizeof(shared_type::surfel_msme) * desc.max_surfel_count;
			c_auto surfel_recycle_data_offset		  = offset_calculator + sizeof(shared_type::surfel_recycle_data) * desc.max_surfel_count;
			c_auto ray_count_ideal_offset			  = offset_calculator + sizeof(uint32) * desc.max_surfel_count;
			c_auto ray_prefix_offset				  = offset_calculator + sizeof(uint32) * desc.max_surfel_count;
			c_auto ray_group_sum_offset				  = offset_calculator + sizeof(uint32) * ray_count_reduce_group_count;
			c_auto ray_group_prefix_offset			  = offset_calculator + sizeof(uint32) * ray_count_reduce_group_count;
			c_auto ray_result_offset				  = offset_calculator + sizeof(shared_type::gibs_ray_result) * g::gibs_ray_budget;
			c_auto buffer_size						  = offset_calculator.size();

			AGE_ASSERT(buffer_size / 4 <= cast_to<uint64>(math::g::uint32_max));

			gibs_data_cpu.h_scratch_buffer = resource::create_committed_buf_uav(cast_to<uint32>(buffer_size));
			gibs_data_cpu.h_scratch_buffer->set_name(L"gibs_scratch_buffer");
			gibs_data_cpu.h_scratch_buffer_uav_desc = resource::create_view(gibs_data_cpu.h_scratch_buffer,
																			defaults::uav_view_desc::byte_address_buffer(cast_to<uint32>(buffer_size)));


			gibs_data_gpu.alive_surfel_id_stack_prev_offset = cast_to<uint32>(alive_surfel_id_stack_prev_offset);
			gibs_data_gpu.alive_surfel_id_stack_curr_offset = cast_to<uint32>(alive_surfel_id_stack_curr_offset);
			gibs_data_gpu.surfel_geometry_offset			= cast_to<uint32>(surfel_geometry_offset);
			gibs_data_gpu.surfel_msme_offset				= cast_to<uint32>(surfel_msme_offset);
			gibs_data_gpu.surfel_recycle_data_offset		= cast_to<uint32>(surfel_recycle_data_offset);
			gibs_data_gpu.ray_count_ideal_offset			= cast_to<uint32>(ray_count_ideal_offset);
			gibs_data_gpu.ray_prefix_offset					= cast_to<uint32>(ray_prefix_offset);
			gibs_data_gpu.ray_group_sum_offset				= cast_to<uint32>(ray_group_sum_offset);
			gibs_data_gpu.ray_group_prefix_offset			= cast_to<uint32>(ray_group_prefix_offset);
			gibs_data_gpu.ray_result_offset					= cast_to<uint32>(ray_result_offset);

			gibs_data_gpu.h_surfel_buffer_srv_id	= calc_desc_idx(gibs_data_cpu.h_surfel_buffer_srv_desc);
			gibs_data_gpu.h_cell_info_buffer_srv_id = calc_desc_idx(gibs_data_cpu.h_cell_info_srv_desc);
			gibs_data_gpu.h_irradiance_atlas_srv_id = calc_desc_idx(gibs_data_cpu.h_irradiance_atlas_srv_desc);
			gibs_data_gpu.h_visibility_atlas_srv_id = calc_desc_idx(gibs_data_cpu.h_visibility_atlas_srv_desc);
			gibs_data_gpu.h_gbuffer_srv_id			= calc_desc_idx(gibs_data_cpu.h_gbuffer_srv_desc);
			gibs_data_gpu.h_surfel_buffer_uav_id	= calc_desc_idx(gibs_data_cpu.h_surfel_buffer_uav_desc);
			gibs_data_gpu.h_cell_info_buffer_uav_id = calc_desc_idx(gibs_data_cpu.h_cell_info_uav_desc);
			gibs_data_gpu.h_scratch_buffer_uav_id	= calc_desc_idx(gibs_data_cpu.h_scratch_buffer_uav_desc);
			gibs_data_gpu.h_irradiance_atlas_uav_id = calc_desc_idx(gibs_data_cpu.h_irradiance_atlas_uav_desc);
			gibs_data_gpu.h_visibility_atlas_uav_id = calc_desc_idx(gibs_data_cpu.h_visibility_atlas_uav_desc);


			gibs_data_gpu.debug_flags				   = to_idx(desc.debug_flags);
			gibs_data_gpu.max_surfel_count			   = desc.max_surfel_count;
			gibs_data_gpu.ray_count_reduce_group_count = ray_count_reduce_group_count;
			gibs_data_gpu.cell_count				   = desc.cell_count;
			gibs_data_gpu.cell_count_total			   = cell_count_total;
			gibs_data_gpu.outer_layer_count			   = desc.outer_layer_count;

			for (auto& lut = gibs_data_cpu.gibs_lut_data_gpu;
				 auto  i : views::loop(desc.outer_layer_count + 1u))
			{
				if (i == 0)
				{
					lut.cell_size_arr[i]	  = desc.cell_size;
					lut.layer_boundary_arr[i] = desc.cell_size * desc.cell_count * 0.5f;
				}
				else
				{
					lut.cell_size_arr[i]	  = lut.cell_size_arr[i - 1] * desc.outer_cell_size_factor;
					lut.layer_boundary_arr[i] = lut.layer_boundary_arr[i - 1] + lut.cell_size_arr[i - 1];
				}
			}
		}

		if (desc.lock_origin and (gibs_data_cpu.lock_origin is_false))
		{
			gibs_data_cpu.origin = camera_data_vec[main_camera_id].pos;
		}

		gibs_data_cpu.render_surfels = has_all(desc.debug_flags, graphics::e::gibs_debug_flags::render_surfels);

		gibs_data_cpu.enabled = true;

		gibs_data_cpu.need_cleanup = true;
	}

	void
	pipeline::disable_gibs() noexcept
	{
		push_descriptor(gibs_data_cpu.h_surfel_buffer_srv_desc);
		push_descriptor(gibs_data_cpu.h_cell_info_srv_desc);
		push_descriptor(gibs_data_cpu.h_irradiance_atlas_srv_desc);
		push_descriptor(gibs_data_cpu.h_visibility_atlas_srv_desc);
		push_descriptor(gibs_data_cpu.h_gbuffer_srv_desc);
		push_descriptor(gibs_data_cpu.h_surfel_buffer_uav_desc);
		push_descriptor(gibs_data_cpu.h_cell_info_uav_desc);
		push_descriptor(gibs_data_cpu.h_scratch_buffer_uav_desc);
		push_descriptor(gibs_data_cpu.h_irradiance_atlas_uav_desc);
		push_descriptor(gibs_data_cpu.h_visibility_atlas_uav_desc);
		push_descriptor(gibs_data_cpu.h_gbuffer_rtv_desc);

		resource::release_deferred(gibs_data_cpu.h_surfel_buffer);
		resource::release_deferred(gibs_data_cpu.h_cell_info_buffer);
		resource::release_deferred(gibs_data_cpu.h_irradiance_atlas);
		resource::release_deferred(gibs_data_cpu.h_visibility_atlas);
		resource::release_deferred(gibs_data_cpu.h_gbuffer);
		resource::release_deferred(gibs_data_cpu.h_scratch_buffer);

		gibs_data_cpu.enabled = false;
	}

	void
	pipeline::update_gibs(const gibs_desc& desc) noexcept
	{
		// todo
		// not going to call this function often at runtime anyway
		AGE_ASSERT(gibs_data_cpu.enabled);
		disable_gibs();
		enable_gibs(desc);
	}

	bool
	pipeline::gibs_enabled() const noexcept
	{
		return gibs_data_cpu.enabled;
	}

	uint32
	pipeline::gibs_max_surfel_count() const noexcept
	{
		return g::gibs_max_surfel_count;
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus
{
	uint32
	pipeline::upload_data() noexcept
	{
		c_auto frame_idx = global::i_graphics.get_frame_buffer_idx();

		auto& h_mapping_static_buffer				   = h_mapping_static_ring_buffer_arr[frame_idx];
		auto& h_mapping_rt_instance_buffer			   = h_mapping_rt_instance_buffer_arr[frame_idx];
		auto& h_mapping_rt_instance_render_data_buffer = h_mapping_rt_instance_render_data_buffer_arr[frame_idx];
		auto& h_mapping_env_light_buffer			   = h_mapping_env_light_buffer_arr[frame_idx];

		h_mapping_static_buffer->upload(object_data_vec.data(), object_data_vec.byte_size<uint32>(), g::object_data_offset);
		h_mapping_static_buffer->upload(directional_light_vec.data(), directional_light_vec.byte_size<uint32>(), g::directional_light_offset);
		h_mapping_static_buffer->upload(unified_light_vec.data(), unified_light_vec.byte_size<uint32>(), g::unified_light_offset);

		auto opaque_mshlt_object_data_count = 0u;
		{
			auto opaque_offset = g::opaque_mshlt_object_data_offset;

			auto rt_instance_render_data_offset = 0ull;

			auto rt_instance_count	= 0ul;
			auto rt_instance_offset = 0ull;
			for (auto i : views::loop(global::thread_count))
			{
				auto& opaque_mshlt_render_data_vec = opaque_meshlet_render_data_vec[i];
				auto& rt_inst_render_data_vec	   = rt_instance_render_data_vec[i];
				auto& rt_instance_vec			   = rt_instance_data_vec[i];

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
				rt_instance_render_data_buffer_srv.bind(h_mapping_rt_instance_render_data_buffer, frame_idx);
			}

			rt_instance_offset			   = 0;
			rt_instance_render_data_offset = 0;
			for (auto i : views::loop(global::thread_count))
			{
				auto& rt_instance_vec		  = rt_instance_data_vec[i];
				auto& rt_inst_render_data_vec = rt_instance_render_data_vec[i];

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

			command::apply_barriers(barrier::rt_write_to_rt_read(h_rt_tlas_buffer, D3D12_BARRIER_SYNC_COMPUTE_SHADING));
		}

		// env_light
		{
			if (resource::resize_buffer(h_mapping_env_light_buffer, env_light_gpu_data_vec.byte_size()))
			{
				env_light_buffer.bind(h_mapping_env_light_buffer, frame_idx);
			}

			h_mapping_env_light_buffer->upload(env_light_gpu_data_vec.data(), env_light_gpu_data_vec.byte_size());
		}

		// raycast
		{
			auto& h_mapping_rt_raycast_request_buffer = h_mapping_rt_raycast_request_buffer_arr[frame_idx];
			auto& h_readback_rt_raycast_result_buffer = h_readback_rt_raycast_result_buffer_arr[frame_idx];

			auto& raycast_res_vec = raycast_result_vec[frame_idx];
			if (resource::resize_buffer(h_mapping_rt_raycast_request_buffer, raycast_request_vec.byte_size())) [[unlikely]]
			{
				rt_raycast_request_buffer_srv.bind(h_mapping_rt_raycast_request_buffer, frame_idx);
			}
			resource::resize_buffer(h_readback_rt_raycast_result_buffer, raycast_request_vec.size() * sizeof(shared_type::raycast_result));

			if (resource::resize_buffer(h_rt_raycast_result_buffer, raycast_request_vec.size() * sizeof(shared_type::raycast_result))) [[unlikely]]
			{
				rt_raycast_result_buffer_uav.bind(h_rt_raycast_result_buffer);
			}

			h_mapping_rt_raycast_request_buffer->upload(raycast_request_vec.data(), raycast_request_vec.byte_size());

			raycast_res_vec.resize(raycast_request_vec.size());
		}

		// selection outline
		{
			auto& h_mapping_render_data_buffer = h_mapping_selection_outline_meshlet_render_data_buffer_arr[frame_idx];
			auto& h_mapping_data_buffer		   = h_mapping_selection_outline_data_buffer_arr[frame_idx];

			if (resource::resize_buffer(h_mapping_render_data_buffer, selection_outline_meshlet_render_data_vec.byte_size())) [[unlikely]]
			{
				selection_outline_meshlet_render_data_buffer.bind(h_mapping_render_data_buffer, frame_idx);
			}
			if (resource::resize_buffer(h_mapping_data_buffer, selection_outline_data_vec.byte_size())) [[unlikely]]
			{
				selection_outline_data_buffer.bind(h_mapping_data_buffer, frame_idx);
			}

			h_mapping_render_data_buffer->upload(selection_outline_meshlet_render_data_vec.data(), selection_outline_meshlet_render_data_vec.byte_size());
			h_mapping_data_buffer->upload(selection_outline_data_vec.data(), selection_outline_data_vec.byte_size());
		}

		// ui
		{
			auto& h_mapping_ui_data_buffer = h_mapping_ui_data_buffer_arr[frame_idx];

			if (resource::resize_buffer(h_mapping_ui_data_buffer, ui_render_data_vec.byte_size<uint32>()))
			{
				ui_data_buffer.bind(h_mapping_ui_data_buffer, frame_idx);
			}
			h_mapping_ui_data_buffer->upload(ui_render_data_vec.data(), ui_render_data_vec.byte_size<uint32>());


			auto& h_mapping_ui_root_data_buffer = h_mapping_ui_root_data_buffer_arr[frame_idx];

			{
				auto root_offset = 0u;
				for (auto&& [space_mode, root_vec] : ui_root_data_vec_arr | std::views::enumerate)
				{
					ui_root_data_idx_arr[space_mode]  = root_offset;
					root_offset						 += root_vec.size<uint32>();
				}

				if (resource::resize_buffer(h_mapping_ui_root_data_buffer, root_offset * sizeof(shared_type::ui_root_data)))
				{
					ui_root_data_buffer.bind(h_mapping_ui_root_data_buffer, frame_idx);
				}
			}

			for (auto&& [space_mode, root_vec] : ui_root_data_vec_arr | std::views::enumerate)
			{
				h_mapping_ui_root_data_buffer->upload(root_vec.data(), root_vec.byte_size(), ui_root_data_idx_arr[space_mode] * sizeof(shared_type::ui_root_data));
			}
		}

		// debug
		{
			auto& h_mapping_debug = h_mapping_debug_meshlet_render_data_buffer_arr[frame_idx];
			auto& h_mapping_obj	  = h_mapping_debug_object_data_buffer_arr[frame_idx];

			if (resource::resize_buffer(h_mapping_debug, debug_aot_meshlet_render_data_vec.byte_size() + debug_meshlet_render_data_vec.byte_size())) [[unlikely]]
			{
				debug_meshlet_render_data_buffer.bind(h_mapping_debug, frame_idx);
			}

			if (resource::resize_buffer(h_mapping_obj, debug_object_data_vec.byte_size())) [[unlikely]]
			{
				debug_object_data_buffer.bind(h_mapping_obj, frame_idx);
			}

			h_mapping_debug->upload(debug_meshlet_render_data_vec.data(), debug_meshlet_render_data_vec.byte_size());
			h_mapping_debug->upload(debug_aot_meshlet_render_data_vec.data(), debug_aot_meshlet_render_data_vec.byte_size(), debug_meshlet_render_data_vec.byte_size());
			h_mapping_obj->upload(debug_object_data_vec.data(), debug_object_data_vec.byte_size());
		}

		// bloom
		{
			if (AGE_IS_INVALID_ID(active_bloom_id) is_false)
			{
				c_auto& desc = bloom_desc_vec[active_bloom_id];

				bloom_gpu.threshold = desc.threshold;
				bloom_gpu.knee		= desc.knee;
				bloom_gpu.intensity = desc.intensity;
				bloom_gpu.radius	= desc.radius;
				bloom_gpu.tint		= desc.tint;
			}

			h_mapping_static_buffer->upload(&bloom_gpu, sizeof(shared_type::bloom), g::bloom_offset);
		}

		// ddgi
		if (ddgi_data_cpu.enabled)
		{
			auto& ddgi_gpu = ddgi_data_cpu.ddgi_data_gpu;
			h_mapping_static_buffer->upload(&ddgi_gpu, sizeof(shared_type::ddgi_data), g::ddgi_data_offset);

			if (ddgi_data_cpu.need_cleanup)
			{
				command::clear_uav(ddgi_data_cpu.h_irradiance_atlas, ddgi_data_cpu.h_irradiance_clear_uav_desc, float4::zero());
				command::clear_uav(ddgi_data_cpu.h_visibility_atlas, ddgi_data_cpu.h_visibility_clear_uav_desc, float4::zero());
				command::clear_uav(ddgi_data_cpu.h_probe_buffer, ddgi_data_cpu.h_probe_buffer_clear_uav_desc, uint32_4::zero());

				command::apply_barriers(
					barrier::tex_uav_to_srv(ddgi_data_cpu.h_irradiance_atlas, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING | D3D12_BARRIER_SYNC_PIXEL_SHADING),
					barrier::tex_uav_to_srv(ddgi_data_cpu.h_visibility_atlas, D3D12_BARRIER_SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING | D3D12_BARRIER_SYNC_PIXEL_SHADING),
					barrier::buf_uav_to_uav(ddgi_data_cpu.h_probe_buffer));

				ddgi_data_cpu.need_cleanup = false;
			}
		}
		// gibs
		else if (gibs_data_cpu.enabled)
		{
			auto& gibs_gpu			= gibs_data_cpu.gibs_data_gpu;
			auto& gibs_lut_data_gpu = gibs_data_cpu.gibs_lut_data_gpu;

			c_auto& main_cam_desc = camera_desc_vec[main_camera_id];

			// todo, cache tan(fov_y/2)
			c_auto tan_half_fov = std::tan(main_cam_desc.perspective.fov_y * 0.5f);

			gibs_data_cpu.gibs_lut_data_gpu.surfel_distance_to_radius = 2.f * min(extent.width, extent.height) * g::gibs_surfel_screen_ratio / extent.height * tan_half_fov;

			h_mapping_static_buffer->upload(&gibs_gpu, sizeof(shared_type::gibs_data), g::gibs_data_offset);
			h_mapping_static_buffer->upload(&gibs_lut_data_gpu, sizeof(shared_type::gibs_lut_data), g::gibs_lut_data_offset);

			std::swap(gibs_gpu.alive_surfel_id_stack_prev_offset, gibs_gpu.alive_surfel_id_stack_curr_offset);
		}

		c_auto& main_cam_desc = camera_desc_vec[main_camera_id];
		root_constants.bind(shared_type::root_constants{
			.opaque_meshlet_render_data_count			  = static_cast<uint32>(opaque_mshlt_object_data_count),
			.directional_light_count_and_extra			  = static_cast<t_directional_light_id>(directional_light_vec.size()),
			.unified_light_count						  = unified_light_vec.size<t_unified_light_id>(),
			.env_light_brdf_lut_id						  = calc_desc_idx(h_env_light_brdf_lut),
			.env_light_count							  = env_light_gpu_data_vec.size<uint32>(),
			.ui_space_mode_and_extra					  = 0u,
			.selection_outline_meshlet_render_data_count  = selection_outline_meshlet_render_data_vec.size<uint32>(),
			.selection_outline_mask_buffer_srv_texture_id = calc_desc_idx(h_selection_outline_mask_buffer_srv_desc),
		});

		// todo multiple camera
		c_auto& main_cam_data = camera_data_vec[main_camera_id];
		c_auto	dt_ns		  = runtime::i_time.get_delta_time_ns();
		c_auto	dt_ms		  = std::chrono::duration<float, std::milli>(dt_ns).count();

		c_auto gi_origin = ddgi_enabled()
							 ? ddgi_data_cpu.lock_origin
								 ? ddgi_data_cpu.origin
								 : main_cam_data.pos
						 : gibs_enabled()
							 ? gibs_data_cpu.lock_origin
								 ? gibs_data_cpu.origin
								 : main_cam_data.pos
							 : float3::zero();

		auto frame_d = shared_type::frame_data{
			.view								  = main_cam_data.view,
			.view_proj							  = main_cam_data.view_proj,
			.view_proj_inv						  = main_cam_data.view_proj_inv,
			.camera_pos							  = main_cam_data.pos,
			.time								  = dt_ms,
			.inv_backbuffer_size				  = float2{ 1.f / extent.width, 1.f / extent.height },
			.backbuffer_size					  = float2{ static_cast<float>(extent.width), static_cast<float>(extent.height) },
			.camera_forward						  = main_cam_data.forward,
			.frame_index						  = runtime::i_time.get_frame_count(),
			.main_buffer_texture_id				  = graphics::calc_desc_idx(h_main_buffer_srv_desc),
			.post_buffer_texture_id				  = graphics::calc_desc_idx(h_post_buffer_srv_desc),
			.depth_buffer_texture_id			  = graphics::calc_desc_idx(h_depth_buffer_srv_desc),
			.rt_tlas_buffer_id					  = graphics::calc_desc_idx(h_rt_tlas_buffer_srv_desc),
			.rt_transparent_buffer_srv_texture_id = graphics::calc_desc_idx(h_rt_transparent_tex_buffer_srv_desc),
			.rt_transparent_buffer_uav_texture_id = graphics::calc_desc_idx(h_rt_transparent_tex_buffer_uav_desc),
			.rt_raycast_request_count			  = raycast_request_vec.size<uint32>(),
			.proj_00							  = main_cam_data.proj[0][0],
			.light_bin_origin					  = /*main_cam_data.pos*/ -float3{ 100.f },
			.proj_11							  = main_cam_data.proj[1][1],
			.light_bin_cell_size_inv			  = float3{ g::light_axis_slice_count_float } / float3{ 200.f },
			.cam_near_z							  = main_cam_desc.near_z,
			.cam_far_z							  = main_cam_desc.far_z,
			.ddgi_enabled_and_extra				  = ddgi_data_cpu.enabled ? 1u : gibs_data_cpu.enabled ? 2u
																									   : 0u,
			.ddgi_cranley_patterson_rotation	  = float2{ ddgi_dist(ddgi_rng), ddgi_dist(ddgi_rng) },
			.gi_origin							  = gi_origin,
			.object_count						  = object_data_vec.size<uint32>(),
			// todo, light bin config
		};

		std::ranges::copy(main_cam_data.frustum_plane_arr, frame_d.frustum_planes);

		h_mapping_frame_data->upload(&frame_d, sizeof(shared_type::frame_data), sizeof(shared_type::frame_data) * frame_idx);

		return opaque_mshlt_object_data_count;
	}
}	 // namespace age::graphics::render_pipeline::forward_plus
#endif