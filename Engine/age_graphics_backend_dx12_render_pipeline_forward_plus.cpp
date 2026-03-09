#include "age_pch.hpp"
#include "age.hpp"
#if defined(AGE_GRAPHICS_BACKEND_DX12)

namespace age::graphics::render_pipeline::forward_plus
{
	void
	calc_directional_shadow(const camera_data&												  cam,
							const std::array<float, g::directional_shadow_cascade_count + 1>& cascade_splits,
							const float3&													  light_dir,
							std::span<shared_type::shadow_light>							  res)
	{
		c_auto xm_view_proj_inv = simd::load(cam.view_proj_inv);

		constexpr float4 ndc_corners[8] = {
			{ -1, -1, 1, 1 },	 // near bottom-left   (reverse-z: near = z=1)
			{ +1, -1, 1, 1 },	 // near bottom-right
			{ -1, +1, 1, 1 },	 // near top-left
			{ +1, +1, 1, 1 },	 // near top-right
			{ -1, -1, 0, 1 },	 // far bottom-left    (reverse-z: far = z=0)
			{ +1, -1, 0, 1 },	 // far bottom-right
			{ -1, +1, 0, 1 },	 // far top-left
			{ +1, +1, 0, 1 },	 // far top-right
		};

		auto world_corners = std::array<float3, 8>{};
		for (auto&& [i, world_corner] : world_corners | std::views::enumerate)
		{
			auto xm_corner = ndc_corners[i] | simd::load();
			xm_corner	   = xm_view_proj_inv | simd::transform4(xm_corner);

			c_auto xm_w = xm_corner | simd::get_w() | simd::replicate();
			xm_corner	= xm_corner | simd::div(xm_w);

			world_corner = xm_corner | simd::to<float3>();
		}

		for (auto i : std::views::iota(0u) | std::views::take(g::directional_shadow_cascade_count))
		{
			auto cascade_corners = std::array<float3, 8>{};

			for (auto j : std::views::iota(0u) | std::views::take(4))
			{
				c_auto near_z = cascade_splits[0];
				c_auto far_z  = cascade_splits[g::directional_shadow_cascade_count];

				c_auto near_corner = world_corners[j];		  // near plane corners
				c_auto far_corner  = world_corners[j + 4];	  // far plane corners
				c_auto dir		   = far_corner - near_corner;

				c_auto range  = far_z - near_z;
				c_auto t_near = (cascade_splits[i] - near_z) / range;
				c_auto t_far  = (cascade_splits[i + 1] - near_z) / range;

				// cascade near
				cascade_corners[j] = near_corner + dir * t_near;

				// cascade far
				cascade_corners[j + 4] = near_corner + dir * t_far;
			}

			auto frustum_center = float3{ 0, 0, 0 };
			for (auto& corner : cascade_corners)
			{
				frustum_center = frustum_center + corner;
			}
			frustum_center = frustum_center / static_cast<float>(cascade_corners.size());

			auto radius = 0.f;
			for (auto	xm_center = frustum_center | simd::load();
				 uint32 j : std::views::iota(0u) | std::views::take(8))
			{
				c_auto xm_corner = cascade_corners[j] | simd::load();

				c_auto dist = xm_corner
							| simd::sub(xm_center)
							| simd::length_3()
							| simd::get_x();

				radius = std::max(radius, dist);
			}

			c_auto texels_per_unit = (float)g::shadow_map_width / (radius * 2.f);

			c_auto light_pos = frustum_center - light_dir * radius;

			c_auto xm_light_pos = light_pos | simd::load();
			c_auto xm_light_dir = light_dir | simd::load();

			c_auto xm_light_view = simd::view_look_to(xm_light_pos, xm_light_dir, simd::g::xm_up_f4);

			c_auto center = xm_light_view | simd::transform3(simd::load(frustum_center)) | simd::to<float3>();

			c_auto snap_x	= std::floor(center.x * texels_per_unit) / texels_per_unit;
			c_auto snap_y	= std::floor(center.y * texels_per_unit) / texels_per_unit;
			c_auto offset_x = snap_x - center.x;
			c_auto offset_y = snap_y - center.y;

			c_auto xm_light_proj = simd::proj_orthographic_reversed(
				radius * 2.f,	 // width
				radius * 2.f,	 // height
				0.01f,			 // near
				radius * 2.f	 // far
			);

			c_auto xm_light_view_proj = xm_light_proj * simd::translation(offset_x, offset_y, 0.f) * xm_light_view;

			c_auto frustum_plane_arr = std::array{
				xm_light_view_proj.r[3] | simd::add(xm_light_view_proj.r[0]) | simd::plane_normalize() | simd::to<float4>(),	// left
				xm_light_view_proj.r[3] | simd::sub(xm_light_view_proj.r[0]) | simd::plane_normalize() | simd::to<float4>(),	// right
				xm_light_view_proj.r[3] | simd::sub(xm_light_view_proj.r[1]) | simd::plane_normalize() | simd::to<float4>(),	// top
				xm_light_view_proj.r[3] | simd::add(xm_light_view_proj.r[1]) | simd::plane_normalize() | simd::to<float4>(),	// bottom
				xm_light_view_proj.r[2] | simd::plane_normalize() | simd::to<float4>(),											// near
				xm_light_view_proj.r[3] | simd::sub(xm_light_view_proj.r[2]) | simd::plane_normalize() | simd::to<float4>(),	// far
			};

			res[i].view_proj = xm_light_view_proj | simd::to<float4x4>();

			std::memcpy(res[i].frustum_planes, frustum_plane_arr.data(), sizeof(frustum_plane_arr));
		}
	}

}	 // namespace age::graphics::render_pipeline::forward_plus

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

			h_mapping_unified_light_buffer = resource::create_buffer_committed(sizeof(shared_type::unified_light) * g::max_light_count);

			h_mapping_shadow_light_buffer = resource::create_buffer_committed(sizeof(shared_type::shadow_light) * g::max_shadow_light_count * graphics::g::frame_buffer_count);

			h_mapping_frame_data_rw_buffer = resource::create_buffer_committed(
				sizeof(shared_type::frame_data_rw) * graphics::g::frame_buffer_count,
				nullptr,
				resource::e::memory_kind::cpu_to_gpu_direct,
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			h_mapping_debug_buffer_uav = resource::create_buffer_committed(
				sizeof(shared_type::debug_77) * graphics::g::frame_buffer_count,
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

			unified_light_buffer.bind(h_mapping_unified_light_buffer->h_resource->p_resource->GetGPUVirtualAddress());

			shadow_light_buffer.bind(h_mapping_shadow_light_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::shadow_light) * g::max_shadow_light_count * 0, 0);
			shadow_light_buffer.bind(h_mapping_shadow_light_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::shadow_light) * g::max_shadow_light_count * 1, 1);
			shadow_light_buffer.bind(h_mapping_shadow_light_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::shadow_light) * g::max_shadow_light_count * 2, 2);

			frame_data_rw_buffer.bind(h_mapping_frame_data_rw_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data_rw) * 0, 0);
			frame_data_rw_buffer.bind(h_mapping_frame_data_rw_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data_rw) * 1, 1);
			frame_data_rw_buffer.bind(h_mapping_frame_data_rw_buffer->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::frame_data_rw) * 2, 2);

			debug_buffer_uav.bind(h_mapping_debug_buffer_uav->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::debug_77) * 0, 0);
			debug_buffer_uav.bind(h_mapping_debug_buffer_uav->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::debug_77) * 1, 1);
			debug_buffer_uav.bind(h_mapping_debug_buffer_uav->h_resource->p_resource->GetGPUVirtualAddress() + sizeof(shared_type::debug_77) * 2, 2);
		}


		h_main_buffer_srv_desc	= graphics::g::cbv_srv_uav_desc_pool.pop();
		h_depth_buffer_srv_desc = graphics::g::cbv_srv_uav_desc_pool.pop();
		h_shadow_atlas_srv_desc = graphics::g::cbv_srv_uav_desc_pool.pop();


		{
			h_culled_light_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(t_global_light_index) * g::max_visible_light_count),
				  .initial_state	   = D3D12_RESOURCE_STATE_COMMON,
				  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
				  .has_clear_value	   = false });


			p_culled_light_buffer = h_culled_light_buffer->p_resource;

			h_light_sort_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(g::sort_buffer_total_byte_size),
				  .initial_state	   = D3D12_RESOURCE_STATE_COMMON,
				  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
				  .has_clear_value	   = false });
			p_light_sort_buffer = h_light_sort_buffer->p_resource;

			h_zbin_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(g::z_slice_count * sizeof(shared_type::zbin_entry)),
				  .initial_state	   = D3D12_RESOURCE_STATE_COMMON,
				  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
				  .has_clear_value	   = false });
			p_zbin_buffer = h_zbin_buffer->p_resource;

			h_unified_sorted_light_buffer = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(g::max_visible_light_count * sizeof(shared_type::unified_light)),
				  .initial_state	   = D3D12_RESOURCE_STATE_COMMON,
				  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
				  .has_clear_value	   = false });
			p_unified_sorted_light_buffer = h_unified_sorted_light_buffer->p_resource;

			h_shadow_atlas = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::texture_ds_2d(g::shadow_atlas_width, g::shadow_atlas_height),
				  .clear_value{
					  .Format		= DXGI_FORMAT_D32_FLOAT,
					  .DepthStencil = { .Depth = 0.f, .Stencil = 0 } },
				  .initial_state	= D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				  .heap_memory_kind = resource::e::memory_kind::gpu_only,
				  .has_clear_value	= true });

			p_shadow_atlas = h_shadow_atlas->p_resource;

			resource::create_view(h_shadow_atlas,
								  h_shadow_atlas_srv_desc,
								  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R32_FLOAT));

			culled_light_buffer.bind(p_culled_light_buffer->GetGPUVirtualAddress());

			light_sort_buffer_uav.bind(p_light_sort_buffer->GetGPUVirtualAddress());
			light_sort_buffer_srv.bind(p_light_sort_buffer->GetGPUVirtualAddress());

			zbin_buffer_uav.bind(p_zbin_buffer->GetGPUVirtualAddress());
			zbin_buffer_srv.bind(p_zbin_buffer->GetGPUVirtualAddress());

			unified_sorted_light_buffer_uav.bind(p_unified_sorted_light_buffer->GetGPUVirtualAddress());
			unified_sorted_light_buffer_srv.bind(p_unified_sorted_light_buffer->GetGPUVirtualAddress());
		}

		create_resolution_dependent_buffers();

		stage_init.init(h_root_sig);
		stage_depth.init(h_root_sig);
		stage_shadow.init(h_root_sig);
		stage_light_culling.init(h_root_sig);
		stage_opaque.init(h_root_sig);
		stage_presentation.init(h_root_sig);

		stage_opaque.bind_rtv_dsv(h_main_buffer, h_depth_buffer);
		stage_depth.bind_dsv(h_depth_buffer);

		stage_shadow.bind_dsv(h_shadow_atlas);
	}

	void
	pipeline::deinit() noexcept
	{
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

		resource::unmap_and_release(h_mapping_shadow_light_buffer);

		resource::unmap_and_release(h_mapping_debug_buffer_uav);


		resource::release_resource(h_main_buffer);
		resource::release_resource(h_depth_buffer);

		resource::release_resource(h_culled_light_buffer);
		resource::release_resource(h_light_sort_buffer);

		resource::release_resource(h_zbin_buffer);
		resource::release_resource(h_tile_mask_buffer);

		resource::release_resource(h_unified_sorted_light_buffer);

		resource::release_resource(h_shadow_atlas);


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

			stage_opaque.bind_rtv_dsv(h_main_buffer, h_depth_buffer);
			stage_depth.bind_dsv(h_depth_buffer);
		}


		cmd_list.SetDescriptorHeaps(1, &graphics::g::cbv_srv_uav_desc_pool.p_descriptor_heap);
		cmd_list.SetGraphicsRootSignature(p_root_sig);
		cmd_list.SetComputeRootSignature(p_root_sig);


		{
			frame_data_buffer.apply(cmd_list);
			job_data_buffer.apply(cmd_list);
			object_data_buffer.apply(cmd_list);
			mesh_data_buffer.apply(cmd_list);

			directional_light_buffer.apply(cmd_list);

			frame_data_buffer.apply_compute(cmd_list);
			job_data_buffer.apply_compute(cmd_list);
			object_data_buffer.apply_compute(cmd_list);
			mesh_data_buffer.apply_compute(cmd_list);

			directional_light_buffer.apply_compute(cmd_list);

			frame_data_rw_buffer.apply(cmd_list);
			frame_data_rw_buffer.apply_compute(cmd_list);

			culled_light_buffer.apply_compute(cmd_list);

			light_sort_buffer_srv.apply(cmd_list);
			light_sort_buffer_uav.apply_compute(cmd_list);

			zbin_buffer_srv.apply(cmd_list);
			zbin_buffer_uav.apply_compute(cmd_list);

			shadow_light_buffer.apply(cmd_list);

			// unified_light_buffer.apply(cmd_list);
			unified_light_buffer.apply_compute(cmd_list);

			unified_sorted_light_buffer_srv.apply(cmd_list);
			unified_sorted_light_buffer_uav.apply_compute(cmd_list);

			tile_mask_buffer_srv.apply(cmd_list);

			tile_mask_buffer_uav.apply(cmd_list);
			tile_mask_buffer_uav.apply_compute(cmd_list);

			debug_buffer_uav.apply(cmd_list);
			debug_buffer_uav.apply_compute(cmd_list);
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
		barrier.add_transition(*p_shadow_atlas,
							   D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
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
					.inv_backbuffer_size	= float2{ 1.f / extent.width, 1.f / extent.height },
					.backbuffer_size		= float2{ static_cast<float>(extent.width), static_cast<float>(extent.height) },
					.camera_forward			= cam_data.forward,
					.frame_index			= age::global::get<runtime::interface>().frame_count(),
					.camera_right			= cam_data.right,
					.main_buffer_texture_id = graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_main_buffer_srv_desc),
				};

				std::ranges::copy(cam_data.frustum_plane_arr, frame_d.frustum_planes);

				std::memcpy(frame_d.cascade_splits[0].data(), cascade_splits.data() + 1, sizeof(cascade_splits[0]) * g::directional_shadow_cascade_count);

				std::memcpy(h_mapping_frame_data->ptr + sizeof(shared_type::frame_data) * graphics::g::frame_buffer_idx, &frame_d, sizeof(shared_type::frame_data));
			}

			{
				c_auto& cam_desc = camera_desc_vec[0];
				root_constants.bind(shared_type::root_constants{
					.job_count						   = total_job_count,
					.directional_light_count_and_extra = static_cast<t_directional_light_id>(directional_light_data_vec.count()),
					.unified_light_count			   = static_cast<t_unified_light_id>(unified_light_data_vec.size()),
					.cluster_tile_count_x			   = light_tile_count_x,
					.cluster_tile_count_y			   = light_tile_count_y,
					//.cluster_near_z					   = cam_desc.near_z,
					//.cluster_far_z					   = cam_desc.far_z,
					.cluster_near_z				= 0.5,
					.cluster_far_z				= cam_desc.far_z,
					.cluster_log_far_near_ratio = std::log2(cam_desc.far_z / cam_desc.near_z),
					.shadow_atlas_id			= graphics::g::cbv_srv_uav_desc_pool.calc_idx(h_shadow_atlas_srv_desc),
				});

				root_constants.apply(cmd_list);
				root_constants.apply_compute(cmd_list);
			}
		}

		{
			c_auto& cam_data = camera_data_vec[0];
			c_auto& cam_desc = camera_desc_vec[0];

			for (auto		 offset = 0u;
				 const auto& data : directional_light_data_vec)
			{
				calc_directional_shadow(
					cam_data,
					cascade_splits,
					data.desc.direction,
					std::span{ shadow_light_arr.data() + offset, g::directional_shadow_cascade_count });

				offset += g::directional_shadow_cascade_count;
			}

			std::memcpy(
				h_mapping_shadow_light_buffer->ptr + sizeof(shadow_light_arr) * graphics::g::frame_buffer_idx,
				shadow_light_arr.data(),
				sizeof(shadow_light_arr));
		}

		stage_init.execute(cmd_list, light_tile_count_x * light_tile_count_y * g::light_bitmask_uint32_count);
		barrier.add_uav(*p_zbin_buffer);
		barrier.add_uav(*p_tile_mask_buffer);
		barrier.add_uav(*h_mapping_frame_data_rw_buffer->h_resource->p_resource);
		barrier.apply_and_reset(cmd_list);


		stage_shadow.execute(cmd_list, shadow_light_header_vec, total_job_count);

		cmd_list.RSSetViewports(1, &rs.default_viewport);
		cmd_list.RSSetScissorRects(1, &rs.default_scissor_rect);

		stage_depth.execute(cmd_list, total_job_count);

		barrier.add_transition(*p_depth_buffer,
							   D3D12_RESOURCE_STATE_DEPTH_WRITE,
							   D3D12_RESOURCE_STATE_DEPTH_READ);

		barrier.add_transition(*p_shadow_atlas,
							   D3D12_RESOURCE_STATE_DEPTH_WRITE,
							   D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		{
			stage_light_culling.execute(cmd_list,
										barrier,
										light_tile_count_x,
										light_tile_count_y,
										*p_culled_light_buffer,
										*h_mapping_frame_data_rw_buffer->h_resource->p_resource,
										*p_light_sort_buffer,
										*p_zbin_buffer,
										*p_tile_mask_buffer,
										*p_unified_sorted_light_buffer,
										static_cast<t_unified_light_id>(unified_light_data_vec.size()));

			barrier.add_uav(*h_mapping_debug_buffer_uav->h_resource->p_resource);
			barrier.apply_and_reset(cmd_list);

			// #if 1
			//		auto* ptr = (shared_type::debug_77*)h_mapping_debug_buffer_uav->ptr + graphics::g::frame_buffer_idx;


			//		std::println("invalid_count : {}, visible_count : {}", ptr->invalid_count, std::min(g::max_visible_light_count, ptr->visible_count));
			// #endif
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
				  .DepthStencil = { .Depth = 0.f, .Stencil = 0 } },
			  .initial_state	= D3D12_RESOURCE_STATE_DEPTH_READ,
			  .heap_memory_kind = resource::e::memory_kind::gpu_only,
			  .has_clear_value	= true });

		p_main_buffer  = h_main_buffer->p_resource;
		p_depth_buffer = h_depth_buffer->p_resource;

		resource::create_view(h_main_buffer,
							  h_main_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R16G16B16A16_FLOAT));

		resource::create_view(h_depth_buffer,
							  h_depth_buffer_srv_desc,
							  defaults::srv_view_desc::tex2d(DXGI_FORMAT_R32_FLOAT));

		h_tile_mask_buffer = resource::create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(sizeof(uint32) * light_tile_count_x * light_tile_count_y * g::light_bitmask_uint32_count),
			  .initial_state	   = D3D12_RESOURCE_STATE_COMMON,
			  .heap_memory_kind	   = resource::e::memory_kind::gpu_only,
			  .has_clear_value	   = false });

		p_tile_mask_buffer = h_tile_mask_buffer->p_resource;

		tile_mask_buffer_uav.bind(p_tile_mask_buffer->GetGPUVirtualAddress());
		tile_mask_buffer_srv.bind(p_tile_mask_buffer->GetGPUVirtualAddress());
	}

	void
	pipeline::resize_resolution_dependent_buffers(const age::extent_2d<uint16>& new_extent) noexcept
	{
		extent = new_extent;

		light_tile_count_x = (extent.width + g::light_tile_size - 1) / g::light_tile_size;
		light_tile_count_y = (extent.height + g::light_tile_size - 1) / g::light_tile_size;

		resource::release_resource(h_main_buffer);
		resource::release_resource(h_depth_buffer);

		resource::release_resource(h_tile_mask_buffer);

		create_resolution_dependent_buffers();
	}
}	 // namespace age::graphics::render_pipeline::forward_plus

#endif