#pragma once
#include "age.hpp"

namespace age::editor
{
	namespace detail
	{
		void
		init_impl() noexcept;
	}

	void
	init(auto& ecs_game, auto& renderer) noexcept
	{
		g::host_ops.p_ecs_game = std::addressof(ecs_game);
		g::host_ops.p_renderer = std::addressof(renderer);

		using t_ecs_game = BARE_OF(ecs_game);
		using t_renderer = BARE_OF(renderer);

		g::host_ops.p_mesh_gpu_load = [](std::string_view mesh_name, const asset::primitive_desc& desc, asset::e::vertex_kind v_kind) noexcept -> asset::handle {
			return asset::mesh_baked::gpu_load(mesh_name, *static_cast<t_renderer*>(g::host_ops.p_renderer), desc, v_kind);
		};

		g::host_ops.p_mesh_full_unload = [](asset::handle h_mesh) noexcept {
			age::asset::mesh_baked::full_unload(h_mesh, *static_cast<t_renderer*>(g::host_ops.p_renderer));
		};

		g::host_ops.p_material_full_unload = [](asset::handle h_mesh) noexcept {
			age::asset::material::full_unload(h_mesh, *static_cast<t_renderer*>(g::host_ops.p_renderer));
		};

		g::host_ops.p_texture_full_unload = [](asset::handle h_mesh) noexcept {
			age::asset::texture::full_unload(h_mesh, *static_cast<t_renderer*>(g::host_ops.p_renderer));
		};

		g::host_ops.p_env_light_full_unload = [](asset::handle h_mesh) noexcept {
			age::asset::env_light::full_unload(h_mesh, *static_cast<t_renderer*>(g::host_ops.p_renderer));
		};

		g::host_ops.p_model_full_unload = [](asset::handle h_mesh) noexcept {
			age::asset::model::full_unload(h_mesh, *static_cast<t_renderer*>(g::host_ops.p_renderer));
		};

		g::host_ops.p_new_entity = [](uint32 ecs_scene_id, uint32 ecs_storage_id) noexcept -> uint64 {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);
			return ecs_game.visit_storage_at(ecs_scene_id, ecs_storage_id, [](auto& entities) noexcept -> uint64 {
				return entities.new_entity(get_ecs_context(*static_cast<t_renderer*>(g::host_ops.p_renderer)));
			});
		};

		g::host_ops.p_new_entity_with_archetype = [](uint32 ecs_scene_id, uint32 ecs_storage_id, uint64 archetype) noexcept -> uint64 {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);
			return ecs_game.visit_storage_at(ecs_scene_id, ecs_storage_id, AGE_LAMBDA((auto& entities, uint64 archetype), {
												 return entities.new_entity(static_cast<BARE_OF(entities)::t_archetype>(archetype), get_ecs_context(*static_cast<t_renderer*>(g::host_ops.p_renderer)));
											 }),
											 archetype);
		};

		g::host_ops.p_copy_entity = [](uint32 ecs_scene_id, uint32 ecs_storage_id, uint64 ecs_entity_id) noexcept -> uint64 {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);

			return ecs_game.visit_storage_at(
				ecs_scene_id,
				ecs_storage_id,
				[](auto& entities, uint64 ecs_entity_id) noexcept -> uint64 {
					return entities.copy_entity(static_cast<BARE_OF(entities)::t_ent_id>(ecs_entity_id), get_ecs_context(*static_cast<t_renderer*>(g::host_ops.p_renderer)));
				},
				ecs_entity_id);
		};

		g::host_ops.p_remove_entity = [](uint32 ecs_scene_id, uint32 ecs_storage_id, uint64 ecs_entity_id) noexcept {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);
			ecs_game.visit_storage_at(
				ecs_scene_id,
				ecs_storage_id,
				[](auto& entities, uint64 ecs_entity_id) noexcept {
					entities.remove_entity(static_cast<BARE_OF(entities)::t_ent_id>(ecs_entity_id), get_ecs_context(*static_cast<t_renderer*>(g::host_ops.p_renderer)));
				},
				ecs_entity_id);
		};

		g::host_ops.p_get_archetype = [](uint32 ecs_scene_id, uint32 ecs_storage_id, uint64 ecs_entity_id) noexcept -> uint64 {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);
			return ecs_game.visit_storage_at(
				ecs_scene_id,
				ecs_storage_id,
				[](c_auto& entities, uint64 ecs_entity_id) noexcept -> uint64 {
					return entities.get_archetype(static_cast<BARE_OF(entities)::t_ent_id>(ecs_entity_id));
				},
				ecs_entity_id);
		};

		g::host_ops.p_get_component_count = [](uint32 ecs_scene_id, uint32 ecs_storage_id) noexcept -> uint32 {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);
			return ecs_game.visit_storage_at(
				ecs_scene_id,
				ecs_storage_id,
				[](c_auto& entities) noexcept -> uint32 {
					return BARE_OF(entities)::component_count();
				});
		};

		g::host_ops.p_get_component_name = [](uint32 ecs_scene_id, uint32 ecs_storage_id, uint32 ecs_component_id) noexcept -> age::array<char, config::max_component_name_len> {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);
			return ecs_game.visit_storage_at(
				ecs_scene_id,
				ecs_storage_id,
				[](c_auto& entities, uint32 ecs_component_id) noexcept -> age::array<char, config::max_component_name_len> {
					return BARE_OF(entities)::t_archetype_traits::visit_component(
						ecs_component_id,
						AGE_LAMBDA(<typename t_cmp>(), {
							return ecs::get_component_name_at<t_cmp, 0>();
						}));
				},
				ecs_component_id);
		};

		g::host_ops.p_add_components = [](uint32 ecs_scene_id, uint32 ecs_storage_id, uint64 ecs_entity_id, uint64 ecs_archetype_to_add) noexcept {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);
			ecs_game.visit_storage_at(
				ecs_scene_id,
				ecs_storage_id,
				[](auto& entities, uint64 ecs_entity_id, uint64 ecs_archetype_to_add) noexcept {
					for (c_auto ecs_cmp_idx : age::views::each_set_bit_idx(ecs_archetype_to_add))
					{
						BARE_OF(entities)::t_archetype_traits::visit_component(
							ecs_cmp_idx,
							AGE_LAMBDA(<typename t_cmp>(auto& entities, auto ecs_entity_id, auto&& ctx), {
								entities.template add_component<t_cmp>(ecs_entity_id, FWD(ctx));
							}),
							entities, static_cast<BARE_OF(entities)::t_ent_id>(ecs_entity_id), get_ecs_context(*static_cast<t_renderer*>(g::host_ops.p_renderer)));
					}
				},
				ecs_entity_id, ecs_archetype_to_add);
		};

		g::host_ops.p_remove_components = [](uint32 ecs_scene_id, uint32 ecs_storage_id, uint64 ecs_entity_id, uint64 ecs_archetype_to_remove) noexcept {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);
			ecs_game.visit_storage_at(
				ecs_scene_id,
				ecs_storage_id,
				[](auto& entities, uint64 ecs_entity_id, uint64 ecs_archetype_to_remove) noexcept {
					for (c_auto ecs_cmp_idx : age::views::each_set_bit_idx(ecs_archetype_to_remove))
					{
						BARE_OF(entities)::t_archetype_traits::visit_component(
							ecs_cmp_idx,
							AGE_LAMBDA(<typename t_cmp>(auto& entities, auto ecs_entity_id, auto&& ctx), {
								entities.template remove_component<t_cmp>(ecs_entity_id, FWD(ctx));
							}),
							entities, static_cast<BARE_OF(entities)::t_ent_id>(ecs_entity_id), get_ecs_context(*static_cast<t_renderer*>(g::host_ops.p_renderer)));
					}
				},
				ecs_entity_id, ecs_archetype_to_remove);
		};

		g::host_ops.p_get_components = [](uint32 ecs_scene_id, uint32 ecs_storage_id, uint64 ecs_entity_id, uint32 ecs_component_id) noexcept -> void* {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);
			return ecs_game.visit_storage_at(
				ecs_scene_id,
				ecs_storage_id,
				[](auto& entities, uint64 ecs_entity_id, uint32 ecs_component_id) noexcept -> void* {
					return BARE_OF(entities)::t_archetype_traits::visit_component(
						ecs_component_id,
						AGE_LAMBDA(<typename t_cmp>(auto& entities, auto ecs_entity_id), {
							auto&& [cmp] = entities.template get_component<t_cmp>(ecs_entity_id);
							return static_cast<void*>(std::addressof(cmp));
						}),
						entities, static_cast<BARE_OF(entities)::t_ent_id>(ecs_entity_id));
				},
				ecs_entity_id, ecs_component_id);
		};

		g::host_ops.p_deserialize_component = [](uint32 ecs_scene_id, uint32 ecs_storage_id, uint64 ecs_entity_id, uint32 ecs_component_id, uint32 cmp_version, aligned_byte_buf& buf) noexcept -> void {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);
			auto& renderer = *static_cast<t_renderer*>(g::host_ops.p_renderer);

			ecs_game.visit_storage_at(
				ecs_scene_id,
				ecs_storage_id,
				[](auto& entities, uint64 ecs_entity_id, uint32 ecs_component_id, aligned_byte_buf& buf, auto&& rw_ctx) noexcept -> void {
					BARE_OF(entities)::t_archetype_traits::visit_component(
						ecs_component_id,
						AGE_LAMBDA(<typename t_cmp>(auto& entities, c_auto ecs_entity_id, aligned_byte_buf& buf, auto&& rw_ctx), {
							auto&& [cmp] = entities.template get_component<t_cmp>(ecs_entity_id);
							ecs::deserialize_component<t_cmp>(cmp, buf, FWD(rw_ctx));
						}),
						entities, static_cast<BARE_OF(entities)::t_ent_id>(ecs_entity_id), buf, FWD(rw_ctx));
				},
				ecs_entity_id, ecs_component_id, buf, get_rw_context(cmp_version, renderer));
		};

		g::host_ops.p_serialize_entity_storage = [](const storage_editor_data& editor_storage, uint32 ecs_scene_id, uint32 ecs_storage_id, uint64 archetype, std::size_t archetype_byte_size, AGE_INOUT byte_buf& buf) noexcept -> void {
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);

			ecs_game.visit_storage_at(ecs_scene_id,
									  ecs_storage_id,
									  AGE_LAMBDA((c_auto & entities, const storage_editor_data& editor_storage, uint64 archetype, std::size_t archetype_byte_size, AGE_INOUT byte_buf& buf), {
										  using t_archetype_traits = BARE_OF(entities)::t_archetype_traits;
										  using t_local_cmp_idx	   = BARE_OF(entities)::t_local_cmp_idx;
										  using t_archetype		   = BARE_OF(entities)::t_archetype;

										  c_auto buf_base_pos = buf.size();
										  for (c_auto& block : entities | ecs::each_block(archetype))
										  {
											  AGE_ASSERT(cast_to<t_archetype>(archetype) == block.local_archetype());

											  for (c_auto local_ent_id : views::loop(block.entity_count()))
											  {
												  c_auto ent_id = block.ent_id(local_ent_id);

												  c_auto it = editor_storage.ecs_ent_id_to_editor_location_map.find(ent_id);
												  AGE_ASSERT(it != editor_storage.ecs_ent_id_to_editor_location_map.end());
												  AGE_ASSERT(archetype == editor_storage.archetype_data_vec[it->second.first].archetype);
												  c_auto editor_entity_idx = it->second.second;

												  buf.move_write_pos(buf_base_pos + editor_entity_idx * archetype_byte_size);

												  for (const auto&& [local_cmp_idx, storage_cmp_idx] : views::each_set_bit_idx(cast_to<t_archetype>(archetype)) | views::enumerate<t_local_cmp_idx>)
												  {
													  c_auto* p_cmp = block.cmp_ptr(local_cmp_idx, local_ent_id);
													  t_archetype_traits::visit_component(
														  storage_cmp_idx,
														  []<typename t_cmp>(c_auto* p_cmp, byte_buf& buf) {
															  ecs::serialize_component_from_ptr<t_cmp>(p_cmp, buf, get_rw_context(ecs::get_component_version<t_cmp>(), *static_cast<t_renderer*>(g::host_ops.p_renderer)));
														  },
														  p_cmp, buf);
												  }
											  }
										  }
									  }),
									  editor_storage, archetype, archetype_byte_size, buf);
		};
		g::host_ops.p_renderer_init_main_cam = [](const editor::camera_data& cam) noexcept -> void {
			auto& renderer = *static_cast<t_renderer*>(g::host_ops.p_renderer);

			auto cam_desc					  = renderer.get_camera_desc(0);
			cam_desc.pos					  = cam.pos;
			cam_desc.quaternion				  = age::euler_deg_to_quat(cam.euler_deg);
			cam_desc.perspective.aspect_ratio = cam.aspect_ratio;
			renderer.update_camera(0, cam_desc);
			renderer.set_main_camera(0);
		};


		g::host_ops.p_renderer_update_material = [](asset::handle h_mat) noexcept -> void {
			static_cast<t_renderer*>(g::host_ops.p_renderer)->update_material(h_mat);
		};
		g::host_ops.p_renderer_update_env_light_runtime = [](asset::handle h_env_light) noexcept -> void {
			static_cast<t_renderer*>(g::host_ops.p_renderer)->update_env_light_runtime(h_env_light);
		};
		g::host_ops.p_renderer_update_gi = [](const ecs::gi_config& cmp, bool update_debug_flags) noexcept -> void {
			auto& renderer = *static_cast<t_renderer*>(g::host_ops.p_renderer);
			if (cmp.enable_ddgi)
			{
				renderer.update_ddgi({
					.probe_per_level_axis = cmp.ddgi_probe_per_level_axis,
					.base_probe_spacing	  = cmp.ddgi_base_probe_spacing,
					.level_count		  = cmp.ddgi_level_count,
					.debug_flags		  = cmp.ddgi_debug_flags,
					.lock_origin		  = cmp.ddgi_lock_origin,
				});
				if (update_debug_flags)
				{
					renderer.update_ddgi_debug_flags(cmp.ddgi_debug_flags);
				}
			}
			else if (cmp.enable_gibs)
			{
				renderer.update_gibs({
					.max_surfel_count		= cmp.max_surfel_count,
					.debug_flags			= cmp.gibs_debug_flags,
					.lock_origin			= cmp.gibs_lock_origin,
					.cell_count				= cmp.gibs_cell_count,
					.outer_layer_count		= cmp.gibs_outer_layer_count,
					.cell_size				= cmp.gibs_cell_size,
					.outer_cell_size_factor = cmp.outer_cell_size_factor,
				});
				if (update_debug_flags)
				{
					renderer.update_gibs_debug_flags(cmp.gibs_debug_flags);
				}
			}
			else if (cmp.enable_gist)
			{
				renderer.update_gist({
					.diffuse_ray_period			   = cmp.gist_diffuse_ray_period,
					.specular_ray_period		   = cmp.gist_specular_ray_period,
					.cell_surfel_ray_count_min	   = cmp.gist_cell_surfel_ray_count_min,
					.cell_surfel_ray_count_max	   = cmp.gist_cell_surfel_ray_count_max,
					.max_cell_surfel_count		   = cmp.gist_max_cell_surfel_count,
					.cell_surfel_ray_budget_factor = cmp.gist_cell_surfel_ray_budget_factor,
					.debug_flags				   = cmp.gist_debug_flags,
					.lock_origin				   = cmp.gist_lock_origin,
					.cell_count_per_axis		   = cmp.gist_cell_count_per_axis,
					.outer_layer_count			   = cmp.gist_outer_layer_count,
					.cell_size					   = cmp.gist_cell_size,
					.outer_cell_size_factor		   = cmp.gist_outer_cell_size_factor,
				});
				if (update_debug_flags)
				{
					renderer.update_gist_debug_flags(cmp.gist_debug_flags);
				}
			}
		};
		g::host_ops.p_renderer_update_ao = [](const age::ecs::ao_config& cmp) noexcept -> void {
			static_cast<t_renderer*>(g::host_ops.p_renderer)->update_ao({
				.slice_count   = cmp.slice_count,
				.offset_count  = cmp.offset_count,
				.radius		   = cmp.radius,
				.max_px_radius = cmp.max_px_radius,
				.intensity	   = cmp.intensity,
				.power		   = cmp.power,
				.thickness	   = cmp.thickness,
				.fade_distance = cmp.fade_distance,
				.fade_range	   = cmp.fade_range,
				.debug_flags   = cmp.debug_flags,
			});
		};
		g::host_ops.p_renderer_update_aa = [](const age::ecs::aa_config& cmp) noexcept -> void {
			static_cast<t_renderer*>(g::host_ops.p_renderer)->update_aa({
				.fxaa_on_offscreen			  = cmp.fxaa_on_offscreen,
				.opaque_aa_ray_per_px		  = cmp.opaque_aa_ray_per_px,
				.transparent_aa_ray_per_px	  = cmp.transparent_aa_ray_per_px,
				.aa_px_cap					  = cmp.aa_px_cap,
				.aa_px_headroom				  = cmp.aa_px_headroom,
				.edge_plane_dist_tolerance_px = cmp.edge_plane_dist_tolerance_px,
				.edge_normal_threshold		  = cmp.edge_normal_threshold,
			});
		};
		g::host_ops.p_renderer_update_debug_view = [](const age::ecs::debug_view_config& cmp) noexcept -> void {
			static_cast<t_renderer*>(g::host_ops.p_renderer)->update_debug_view(cmp_to_desc(cmp));
		};

		g::host_ops.p_renderer_gibs_max_surfel_count = []() noexcept -> uint32 {
			return static_cast<const t_renderer*>(g::host_ops.p_renderer)->gibs_max_surfel_count();
		};
		g::host_ops.p_renderer_gist_max_cell_surfel_count = []() noexcept -> uint32 {
			return static_cast<const t_renderer*>(g::host_ops.p_renderer)->gist_max_cell_surfel_count();
		};
		g::host_ops.p_renderer_aa_enabled = []() noexcept -> bool {
			return static_cast<const t_renderer*>(g::host_ops.p_renderer)->aa_enabled();
		};
		g::host_ops.p_renderer_ao_enabled = []() noexcept -> bool {
			return static_cast<const t_renderer*>(g::host_ops.p_renderer)->ao_enabled();
		};
		g::host_ops.p_renderer_ddgi_enabled = []() noexcept -> bool {
			return static_cast<const t_renderer*>(g::host_ops.p_renderer)->ddgi_enabled();
		};
		g::host_ops.p_renderer_gibs_enabled = []() noexcept -> bool {
			return static_cast<const t_renderer*>(g::host_ops.p_renderer)->gibs_enabled();
		};
		g::host_ops.p_renderer_gist_enabled = []() noexcept -> bool {
			return static_cast<const t_renderer*>(g::host_ops.p_renderer)->gist_enabled();
		};
		g::host_ops.p_renderer_debug_view_enabled = []() noexcept -> bool {
			return static_cast<const t_renderer*>(g::host_ops.p_renderer)->debug_view_enabled();
		};

		// editor ui
		g::host_ops.p_ui_component_section = [](uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept -> void {
			using namespace age::ui;
			auto& ecs_game = *static_cast<t_ecs_game*>(g::host_ops.p_ecs_game);

			c_auto& editor_scene   = g::current_game.scene_data_vec[editor_scene_idx];
			c_auto& editor_storage = editor_scene.storage_data_vec[editor_storage_idx];
			c_auto	ecs_scene_id   = editor_scene.code_idx;
			c_auto	ecs_storage_id = editor_storage.code_idx;

			ecs_game.visit_storage_at(
				ecs_scene_id,
				ecs_storage_id,
				[](auto& entities, uint64 ecs_entity_id) noexcept {
					using t_archetype_traits = BARE_OF(entities)::t_archetype_traits;
					using t_entity_id		 = BARE_OF(entities)::t_ent_id;

					for (c_auto archetype = entities.get_archetype(cast_to<t_entity_id>(ecs_entity_id));
						 c_auto ecs_component_id : age::views::each_set_bit_idx(archetype))
					{
						t_archetype_traits::visit_component(
							ecs_component_id,
							[]<typename t_cmp>(auto& entities, t_entity_id ecs_entity_id, uint32 ecs_component_id) noexcept {
								auto& renderer = *static_cast<t_renderer*>(g::host_ops.p_renderer);

								if (auto _ = widget::begin(style::section() | set_horizontal() | set_height_fit() | set_width_grow()))
								{
									widget::separator_h(set_body_brush_data(get_component_color(ecs_component_id), theme::opacity_medium()), set_width_fixed(theme::thickness_thick()));

									auto remove_cmp = false;
									if (auto _ = ui_component_header(ecs::get_component_name_at<t_cmp, 0>().data(), AGE_OUT remove_cmp))
									{
										c_auto disclosure_size = font::get_line_height(theme::text_heading_font_size());
										c_auto gap			   = theme::header_bar_child_gap();
										c_auto padding_l	   = theme::header_bar_padding().x;

										if (auto _ = widget::vertical(set_padding_left(disclosure_size + padding_l + gap)))
										{
											auto&& [cmp] = entities.template get_component<t_cmp>(ecs_entity_id);

											if constexpr (requires { ui_component(FWD(cmp), renderer); })
											{
												ui_component(FWD(cmp), renderer);
											}
											else
											{
												ui_component(FWD(cmp));
											}
										}
									}

									if (remove_cmp)
									{
										entities.template remove_component<t_cmp>(ecs_entity_id, get_ecs_context(renderer));
									}
								}
							},
							entities, cast_to<t_entity_id>(ecs_entity_id), ecs_component_id);
					}
				},
				ecs_entity_id);

			c_auto new_archetype = g::host_ops.p_get_archetype(ecs_scene_id, ecs_storage_id, ecs_entity_id);
			relocate_editor_entity(editor_scene_idx, editor_storage_idx, ecs_entity_id, new_archetype);
		};

		detail::init_impl();
	}

	template <typename... t_cmp>
	requires(sizeof...(t_cmp) >= 2)
	std::tuple<t_cmp&...>
	get_components(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept
	{
		static_assert((std::is_reference_v<t_cmp> or ...) is_false, "don't call get_component<position&>, call get_component<position> instead. gen_components always return tuple of references");
		constexpr auto hash_id_arr = array{ ecs::get_component_name_hash<std::remove_cvref_t<t_cmp>>()... };
		auto		   cmp_ptr_arr = age::array<void*, sizeof...(t_cmp)>{};

		get_component_ptrs(editor_scene_idx, editor_storage_idx, ecs_entity_id, hash_id_arr, AGE_OUT cmp_ptr_arr);

		return AGE_LAMBDA(
			<auto... i>(std::index_sequence<i...>, c_auto & cmp_ptr_arr), {
				return std::tuple<t_cmp&...>{ *static_cast<t_cmp*>(cmp_ptr_arr[i])... };
			})(std::index_sequence_for<t_cmp...>{}, cmp_ptr_arr);
	}

	template <typename t_cmp>
	t_cmp&
	get_components(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept
	{
		static_assert(std::is_reference_v<t_cmp> is_false, "don't call get_component<position&>, call get_component<position> instead. gen_components always return tuple of references");

		c_auto ecs_component_id = get_ecs_component_id(editor_scene_idx, editor_storage_idx, ecs::get_component_name_hash<std::remove_cvref_t<t_cmp>>());

		AGE_ASSERT(runtime::is_invalid_idx(ecs_component_id) is_false, "component not fount, cmp : {}, hash : {}", ecs::get_component_name<std::remove_cvref_t<t_cmp>>(), ecs::get_component_name_hash<std::remove_cvref_t<t_cmp>>());

		return *static_cast<t_cmp*>(get_component_ptr(editor_scene_idx, editor_storage_idx, ecs_entity_id, ecs_component_id));
	}

	template <typename... t_cmp>
	decltype(auto)
	add_components(uint32 editor_scene_idx, uint32 editor_storage_idx, uint64 ecs_entity_id) noexcept
	{
		c_auto ecs_archetype = ((1uz << get_ecs_component_id(editor_scene_idx, editor_storage_idx, ecs::get_component_name_hash<std::remove_cvref_t<t_cmp>>())) | ... | 0uz);

		add_components(editor_scene_idx, editor_storage_idx, ecs_entity_id, ecs_archetype);

		return get_components<t_cmp...>(editor_scene_idx, editor_storage_idx, ecs_entity_id);
	}
}	 // namespace age::editor

namespace age::editor
{
	void
	update_camera(auto& renderer, bool update, platform::window_handle h_window) noexcept
	{
		auto& current_scene = g::current_game.get_current_scene();
		auto& cam			= current_scene.cam;
		cam.aspect_ratio	= age::platform::get_client_width(h_window)
							/ static_cast<float>(age::platform::get_client_height(h_window));

		if (update is_false)
		{
			auto cam_desc					  = renderer.get_camera_desc(0);
			cam_desc.perspective.aspect_ratio = cam.aspect_ratio;
			renderer.update_camera(0, cam_desc);
			renderer.set_main_camera(0);
			return;
		}

		using enum age::input::e::key_kind;
		c_auto& editor_input_ctx = *ui::g::p_input_ctx;

		cam.move = editor_input_ctx.is_down(mouse_right)
					 ? float2{
						   editor_input_ctx.is_down(key_d) - editor_input_ctx.is_down(key_a),
						   editor_input_ctx.is_down(key_w) - editor_input_ctx.is_down(key_s),
					   }
					 : float2::zero();

		cam.look   = editor_input_ctx.mouse_delta;
		cam.zoom   = editor_input_ctx.wheel_delta;
		cam.sprint = editor_input_ctx.is_down(key_shift);

		c_auto dt_s = std::min(runtime::i_time.get_delta_time_s(), 1.f / 160);

		c_auto speed = cam.sprint ? cam.move_speed * cam.sprint_mult : cam.move_speed;

		c_auto move_smoothing_factor = 1.f - std::exp(-cam.move_smoothing * dt_s);
		c_auto look_smoothing_factor = 1.f - std::exp(-cam.look_smoothing * dt_s);
		c_auto zoom_smoothing_factor = 1.f - std::exp(-cam.zoom_smoothing * dt_s);

		cam.smoothed_move = math::lerp(cam.smoothed_move, cam.move, move_smoothing_factor);
		cam.smoothed_zoom = math::lerp(cam.smoothed_zoom, cam.zoom, zoom_smoothing_factor);

		auto look_target  = editor_input_ctx.is_down(mouse_right) ? cam.look : float2::zero();
		cam.smoothed_look = math::lerp(cam.smoothed_look, look_target, look_smoothing_factor);

		auto pan_target	 = editor_input_ctx.is_down(mouse_middle) ? cam.look : float2::zero();
		cam.smoothed_pan = math::lerp(cam.smoothed_pan, pan_target, look_smoothing_factor);

		cam.euler_deg.y += cam.smoothed_look.x * cam.sensitivity;
		cam.euler_deg.x += cam.smoothed_look.y * cam.sensitivity;
		cam.euler_deg.x	 = std::clamp(cam.euler_deg.x, -89.f, 89.f);

		c_auto xm_look_quat = cam.euler_deg * age::g::degree_to_radian
							| simd::load()
							| simd::euler_to_quat();

		c_auto forward = simd::rotate3(xm_look_quat, simd::g::xm_forward_f4) | simd::to<float3>();
		c_auto right   = simd::rotate3(xm_look_quat, simd::g::xm_right_f4) | simd::to<float3>();
		c_auto up	   = simd::rotate3(xm_look_quat, simd::g::xm_up_f4) | simd::to<float3>();

		cam.pos -= right * cam.smoothed_pan.x * cam.pan_speed * dt_s;
		cam.pos += up * cam.smoothed_pan.y * cam.pan_speed * dt_s;
		cam.pos += forward * cam.smoothed_zoom * cam.zoom_speed;
		cam.pos += (right * cam.smoothed_move.x + forward * cam.smoothed_move.y) * speed * dt_s;

		auto cam_desc					  = renderer.get_camera_desc(0);
		cam_desc.pos					  = cam.pos;
		cam_desc.quaternion				  = age::euler_deg_to_quat(cam.euler_deg);
		cam_desc.perspective.aspect_ratio = cam.aspect_ratio;
		cam_desc.perspective.fov_y		  = cam.fov_y;
		renderer.update_camera(0, cam_desc);
		renderer.set_main_camera(0);
	}

	void
	focus_camera(auto& renderer, const float3& aabb_min, const float3& aabb_max) noexcept
	{
		c_auto origin = (aabb_min + aabb_max) * 0.5f;

		c_auto radius = (aabb_max - origin) | simd::load() | simd::length_3() | simd::to<float>();

		auto& cam = g::current_game.get_current_scene().cam;

		c_auto xm_look_quat = cam.euler_deg * age::g::degree_to_radian
							| simd::load()
							| simd::euler_to_quat();

		c_auto forward = simd::g::xm_forward_f4 | simd::rotate3(xm_look_quat) | simd::to<float3>();

		auto distance = float{};

		if (radius < age::g::epsilon_1e6)
		{
			distance = 1.f / std::sin(cam.fov_y * 0.5f);
		}
		else
		{
			c_auto fov_x_half = std::atan(1.f * std::tan(cam.fov_y * 0.5f) * cam.aspect_ratio);

			distance = radius / std::sin(std::min(cam.fov_y * 0.5f, fov_x_half)) * 1.15f;
		}

		cam.pos = origin - forward * distance;

		auto cam_desc = renderer.get_camera_desc(0);
		cam_desc.pos  = cam.pos;
		renderer.update_camera(0, cam_desc);
	}
}	 // namespace age::editor

namespace age::editor
{
	// return pair { aabb_min, aabb_max }
	decltype(auto)
	calc_entity_aabb(auto& storage, auto& renderer, storage_editor_data& editor_storage, auto ecs_ent_id) noexcept
	{
		using t_storage = BARE_OF(storage);
		using t_ent_id	= typename t_storage::t_ent_id;

		auto&& [src_arch_idx, src_ent_idx] = editor_storage.ecs_ent_id_to_editor_location_map[ecs_ent_id];

		auto& arch_data = editor_storage.archetype_data_vec[src_arch_idx];

		do
		{
			if constexpr (storage.has_component<ecs::render_object, ecs::model>())
			{
				if (storage.has_component<ecs::render_object, ecs::model>(static_cast<t_ent_id>(ecs_ent_id)) is_false) { break; }

				auto&& [obj, model] = storage.get_component<const ecs::render_object, const ecs::model>(static_cast<t_ent_id>(ecs_ent_id));
				AGE_ASSERT(AGE_IS_INVALID_ID(obj.render_id) is_false);

				if (runtime::is_handle_invalid(model.h_model) is_false)
				{
					c_auto& entry = model.h_model.get_entry<asset::e::kind::model>();
					if (entry.is_loaded() is_false) { break; }

					if (runtime::is_handle_invalid(entry.h_mesh)) { break; }

					c_auto& mesh_entry = entry.h_mesh.get_entry<asset::e::kind::mesh_baked>();

					auto&& [xm_aabb_min, xm_aabb_max, xm_trans] = simd::load(mesh_entry.aabb_min, mesh_entry.aabb_max, renderer.get_object_transform_matrix(obj.render_id));

					c_auto aabb_min = simd::transform3(xm_trans, xm_aabb_min) | simd::to<float3>();
					c_auto aabb_max = simd::transform3(xm_trans, xm_aabb_max) | simd::to<float3>();
					return std::pair{ age::min(aabb_min, aabb_max), age::max(aabb_min, aabb_max) };
				}
				else
				{
					c_auto pos = simd::transform3(simd::load(renderer.get_object_transform_matrix(obj.render_id)), simd::load(float3::zero()))
							   | simd::to<float3>();
					return std::pair{ pos, pos };
				}
			}
		}
		while (false);

		if constexpr (storage.has_component<ecs::position>())
		{
			if (storage.has_component<ecs::position>(static_cast<t_ent_id>(ecs_ent_id)))
			{
				auto&& [pos] = storage.get_component<const ecs::position>(static_cast<t_ent_id>(ecs_ent_id));
				return std::pair{ static_cast<float3>(pos), static_cast<float3>(pos) };
			}
		}

		return std::pair{ float3{ std::numeric_limits<float>::max() }, float3{ std::numeric_limits<float>::lowest() } };
	}
}	 // namespace age::editor

namespace age::editor::detail
{
	void
	update_storage(auto& ecs_storage, auto& renderer, auto& update_storage_ctx) noexcept
	{
		using namespace ecs;
		for (auto&& [obj, model] : ecs_storage | each_entity_soft<render_object, model>())
		{
			if (AGE_IS_INVALID_ID(obj.render_id) or runtime::is_handle_invalid(model.h_model)) { continue; }

			asset::model::load(model.h_model, renderer);
		}

		for (auto&& [env_light] : ecs_storage | each_entity_soft<env_light>())
		{
			if (runtime::is_handle_invalid(env_light.h_env_light) is_false)
			{
				asset::env_light::gpu_load(env_light.h_env_light, renderer);
			}
		}

		for (auto&& [light] : ecs_storage | each_entity_soft<directional_light>())
		{
			renderer.update_directional_light(light.render_id,
											  { .direction	 = age::math::normalize(light.direction),
												.intensity	 = light.intensity,
												.color		 = light.color,
												.cast_shadow = light.cast_shadow });
		}


		for (auto&& [light, pos] : ecs_storage | each_entity_soft<point_light, position>())
		{
			renderer.update_point_light(
				light.render_id,
				{ .position	   = pos,
				  .range	   = light.range,
				  .color	   = light.color,
				  .intensity   = light.intensity,
				  .cast_shadow = light.cast_shadow });
		}

		for (auto&& [light, pos] : ecs_storage | each_entity_soft<spot_light, position>())
		{
			renderer.update_spot_light(
				light.render_id,
				{ .position	   = pos,
				  .range	   = light.range,
				  .direction   = age::math::normalize(light.direction),
				  .intensity   = light.intensity,
				  .color	   = light.color,
				  .cos_inner   = light.cos_inner,
				  .cos_outer   = light.cos_outer,
				  .cast_shadow = light.cast_shadow });
		}

		for (auto&& [cmp] : ecs_storage | each_entity_soft<bloom>())
		{
			if (AGE_IS_INVALID_ID(cmp.render_id)) { continue; }

			renderer.update_bloom(cmp.render_id, { .threshold = cmp.threshold,
												   .knee	  = cmp.knee,
												   .intensity = cmp.intensity,
												   .radius	  = cmp.radius,
												   .tint	  = cmp.tint });

			renderer.set_bloom_active(cmp.render_id, cmp.active);
		}


		if (update_storage_ctx.gi_active_found is_false)
		{
			for (auto&& [cmp] : ecs_storage | each_entity_soft<gi_config>())
			{
				if (cmp.enable_ddgi)
				{
					update_storage_ctx.gi_active_found = true;
					if (renderer.gibs_enabled())
					{
						renderer.disable_gibs();
					}
					if (renderer.gist_enabled())
					{
						renderer.disable_gist();
					}
					if (renderer.ddgi_enabled() is_false)
					{
						renderer.enable_ddgi({
							.probe_per_level_axis = cmp.ddgi_probe_per_level_axis,
							.base_probe_spacing	  = cmp.ddgi_base_probe_spacing,
							.level_count		  = cmp.ddgi_level_count,
						});
					}

					break;
				}
				else if (cmp.enable_gibs)
				{
					update_storage_ctx.gi_active_found = true;
					if (renderer.ddgi_enabled())
					{
						renderer.disable_ddgi();
					}
					if (renderer.gist_enabled())
					{
						renderer.disable_gist();
					}
					if (renderer.gibs_enabled() is_false)
					{
						renderer.enable_gibs({
							.max_surfel_count		= cmp.max_surfel_count,
							.debug_flags			= cmp.gibs_debug_flags,
							.lock_origin			= cmp.gibs_lock_origin,
							.cell_count				= cmp.gibs_cell_count,
							.outer_layer_count		= cmp.gibs_outer_layer_count,
							.cell_size				= cmp.gibs_cell_size,
							.outer_cell_size_factor = cmp.outer_cell_size_factor,
						});
					}

					break;
				}
				else if (cmp.enable_gist)
				{
					update_storage_ctx.gi_active_found = true;
					if (renderer.ddgi_enabled())
					{
						renderer.disable_ddgi();
					}
					if (renderer.gibs_enabled())
					{
						renderer.disable_gibs();
					}
					if (renderer.gist_enabled() is_false)
					{
						renderer.enable_gist({
							.diffuse_ray_period			   = cmp.gist_diffuse_ray_period,
							.specular_ray_period		   = cmp.gist_specular_ray_period,
							.cell_surfel_ray_count_min	   = cmp.gist_cell_surfel_ray_count_min,
							.cell_surfel_ray_count_max	   = cmp.gist_cell_surfel_ray_count_max,
							.max_cell_surfel_count		   = cmp.gist_max_cell_surfel_count,
							.cell_surfel_ray_budget_factor = cmp.gist_cell_surfel_ray_budget_factor,
							.debug_flags				   = cmp.gist_debug_flags,
							.lock_origin				   = cmp.gist_lock_origin,
							.cell_count_per_axis		   = cmp.gist_cell_count_per_axis,
							.outer_layer_count			   = cmp.gist_outer_layer_count,
							.cell_size					   = cmp.gist_cell_size,
							.outer_cell_size_factor		   = cmp.gist_outer_cell_size_factor,
						});
					}

					break;
				}
			}
		}


		for (auto&& [cmp] : ecs_storage | each_entity_soft<editor_cam_setting>())
		{
			auto& cam = g::current_game.get_current_scene().cam;

			cam.move_speed	   = cmp.move_speed;
			cam.sprint_mult	   = cmp.sprint_mult;
			cam.sensitivity	   = cmp.sensitivity;
			cam.zoom_speed	   = cmp.zoom_speed;
			cam.zoom_distance  = cmp.zoom_distance;
			cam.pan_speed	   = cmp.pan_speed;
			cam.move_smoothing = cmp.move_smoothing;
			cam.look_smoothing = cmp.look_smoothing;
			cam.zoom_smoothing = cmp.zoom_smoothing;
		}

		for (auto&& [cmp] : ecs_storage | each_entity_soft<ao_config>())
		{
			if (cmp.enabled and (renderer.ao_enabled() is_false))
			{
				renderer.enable_ao({
					.slice_count   = cmp.slice_count,
					.offset_count  = cmp.offset_count,
					.radius		   = cmp.radius,
					.max_px_radius = cmp.max_px_radius,
					.intensity	   = cmp.intensity,
					.power		   = cmp.power,
					.thickness	   = cmp.thickness,
					.fade_distance = cmp.fade_distance,
					.fade_range	   = cmp.fade_range,
					.debug_flags   = cmp.debug_flags,
				});
			}
			else if ((cmp.enabled is_false) and renderer.ao_enabled())
			{
				renderer.disable_ao();
			}
		}

		for (auto&& [cmp] : ecs_storage | each_entity_soft<aa_config>())
		{
			if (cmp.enabled and (renderer.aa_enabled() is_false))
			{
				renderer.enable_aa({
					.fxaa_on_offscreen			  = cmp.fxaa_on_offscreen,
					.opaque_aa_ray_per_px		  = cmp.opaque_aa_ray_per_px,
					.transparent_aa_ray_per_px	  = cmp.transparent_aa_ray_per_px,
					.aa_px_cap					  = cmp.aa_px_cap,
					.aa_px_headroom				  = cmp.aa_px_headroom,
					.edge_plane_dist_tolerance_px = cmp.edge_plane_dist_tolerance_px,
					.edge_normal_threshold		  = cmp.edge_normal_threshold,
				});
			}
			else if ((cmp.enabled is_false) and renderer.aa_enabled())
			{
				renderer.disable_aa();
			}
		}

		for (auto&& [cmp] : ecs_storage | each_entity_soft<debug_view_config>())
		{
			if (cmp.enabled and (renderer.debug_view_enabled() is_false))
			{
				renderer.enable_debug_view(cmp_to_desc(cmp));
			}
			else if (cmp.enabled is_false and renderer.debug_view_enabled())
			{
				renderer.disable_debug_view();
			}

			if (renderer.debug_view_enabled())
			{
				if (ui::g::p_input_ctx->is_released(input::e::key_kind::key_i)
					and ui::g::p_input_ctx->is_down(input::e::key_kind::key_ctrl)
					and ui::g::p_input_ctx->is_down(input::e::key_kind::key_shift))
				{
					cmp.pick_enabled = not cmp.pick_enabled;
				}

				c_auto is_pick = cmp.pick_enabled and ui::g::p_input_ctx->is_released(input::e::key_kind::mouse_left);
				renderer.update_debug_view(ui::g::p_input_ctx->mouse_pos, is_pick, ui::g::p_input_ctx->is_released(input::e::key_kind::key_escape));
			}
		}
	}
}	 // namespace age::editor::detail

// gizmo
namespace age::editor
{
	void
	widget_transform(auto& ecs_game, auto& renderer, const float3& world_pos, const float4& quat) noexcept
	{
		using namespace ui;
		using namespace ui::widget;
		using enum input::e::key_kind;

		constexpr c_auto world_size_base = 1.f;
		constexpr c_auto screen_size	 = 180.f;

		c_auto& active_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];
		c_auto& cam			 = active_scene.cam;

		c_auto xm_look_quat = cam.euler_deg * age::g::degree_to_radian
							| simd::load()
							| simd::euler_to_quat();

		c_auto cam_forward = simd::rotate3(xm_look_quat, simd::g::xm_forward_f4) | simd::to<float3>();


		if (ui::g::p_input_ctx->is_down(mouse_right) is_false)
		{
			if (ui::g::p_input_ctx->is_down(key_q))
			{
				g::gizmo_transform_mode = e::transform_mode_kind::select;
			}
			else if (ui::g::p_input_ctx->is_down(key_w))
			{
				g::gizmo_transform_mode = e::transform_mode_kind::translation;
			}
			else if (ui::g::p_input_ctx->is_down(key_e))
			{
				g::gizmo_transform_mode = e::transform_mode_kind::rotation;
			}
			else if (ui::g::p_input_ctx->is_down(key_r))
			{
				g::gizmo_transform_mode = e::transform_mode_kind::scale;
			}
		}

		c_auto mode		   = g::gizmo_transform_mode;
		c_auto translation = mode == e::transform_mode_kind::translation
							   ? gizmo::translation(cam.fov_y, cam.pos, cam_forward, world_pos, quat, screen_size)
							   : float3::zero();

		c_auto && [ rotation_res, pivot_pos, rotation_drag_start, rotation_dragging ] = mode == e::transform_mode_kind::rotation
																						  ? gizmo::rotation(cam.fov_y, cam.pos, cam_forward, world_pos, quat, screen_size)
																						  : std::tuple{ math::g::quaternion_identity, float3::zero(), false, false };

		c_auto && [ scale_res, scale_drag_start, scale_dragging ] = mode == e::transform_mode_kind::scale
																	  ? gizmo::scale(cam.fov_y, cam.pos, cam_forward, world_pos, quat, screen_size)
																	  : std::tuple{ float3::one(), false, false };

		if (rotation_dragging is_false)
		{
			for (auto& map : g::rotation_snapshot_vec)
			{
				map.clear();
			}
		}

		if (scale_dragging is_false)
		{
			for (auto& map : g::scale_snapshot_vec)
			{
				map.clear();
			}
		}

		g::scale_snapshot_vec.resize(g::select_vec.size());
		g::rotation_snapshot_vec.resize(g::select_vec.size());

		for (auto&& [storage_code_idx, vec] : g::select_vec | std::views::enumerate /*editor::all_selected()*/)
		{
			if (storage_code_idx >= ecs_game.scene_count()) { continue; }

			ecs_game.visit_storage_at(
				active_scene.code_idx, static_cast<uint32>(storage_code_idx),
				[&](auto& entities) {
					using t_storage = BARE_OF(entities);
					using t_ent_id	= typename t_storage::t_ent_id;
					for (auto ecs_ent_id : vec)
					{
						c_auto id = static_cast<t_ent_id>(ecs_ent_id);
						if (mode == e::transform_mode_kind::translation)
						{
							if (entities.has_component<ecs::position>(id))
							{
								auto&& [pos]  = entities.get_component<ecs::position>(id);
								pos			 += translation;
							}
						}
						else if (mode == e::transform_mode_kind::rotation)
						{
							if (entities.has_component<ecs::rotation, ecs::position>(id))
							{
								AGE_ASSERT(rotation_dragging is_false or g::rotation_snapshot_vec[active_scene.code_idx].contains(ecs_ent_id));

								auto& snap			   = g::rotation_snapshot_vec[active_scene.code_idx][ecs_ent_id];
								auto&& [pos, rotation] = entities.get_component<ecs::position, ecs::rotation>(id);
								if (rotation_drag_start)
								{
									snap.position = pos;
									snap.rotation = rotation;
								}
								else if (rotation_dragging)
								{
									pos		 = math::rotate_around(rotation_res, snap.position, pivot_pos);
									rotation = math::quat_mul(rotation_res, snap.rotation);
								}
							}
							else if (entities.has_component<ecs::rotation>(id))
							{
								auto& snap		  = g::rotation_snapshot_vec[active_scene.code_idx][ecs_ent_id];
								auto&& [rotation] = entities.get_component<ecs::rotation>(id);
								if (rotation_drag_start)
								{
									snap.rotation = rotation;
								}
								else if (rotation_dragging)
								{
									rotation = math::quat_mul(rotation_res, snap.rotation);
								}
							}
						}
						else if (mode == e::transform_mode_kind::scale)
						{
							if (entities.has_component<ecs::scale>(id))
							{
								auto&& [scale] = entities.get_component<ecs::scale>(id);
								if (scale_drag_start)
								{
									g::scale_snapshot_vec[active_scene.code_idx][ecs_ent_id] = scale;
								}
								else if (scale_dragging)
								{
									AGE_ASSERT(g::scale_snapshot_vec[active_scene.code_idx].contains(ecs_ent_id));
									scale = g::scale_snapshot_vec[active_scene.code_idx][ecs_ent_id] * scale_res;
								}
							}
						}
					}
				});


			// editor::command::copy(g::current_select_kind, ecs_game, renderer);
		}
	}
}	 // namespace age::editor

namespace age::editor
{
	void
	update_game(auto& ecs_game, auto& renderer) noexcept
	{
		using enum age::asset::e::kind;
		using enum age::input::e::key_kind;

		asset_mgr::update(ecs_game);

		auto& active_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];

		static auto raycast_req_vec = age::make_filled_array<uint32, global::frame_buffer_count>(get_invalid_idx<uint32>());

		c_auto raycast_res = renderer.get_raycast_result(raycast_req_vec[global::i_graphics.get_frame_buffer_idx]);

		auto need_object_click = AGE_IS_INVALID_IDX(raycast_res.object_id) is_false
							 and raycast_res.object_deleted is_false
							 and ui::g::p_input_ctx->is_released(mouse_left)
							 and (ui::is_any_focused() is_false);

		do
		{
			c_auto target_world = math::ndc_to_world(renderer.get_camera_data(0).view_proj_inv, float3{ math::screen_to_ndc(float2{ ui::g::window_width, ui::g::window_height }, ui::g::p_input_ctx->mouse_pos), 0.f });

			raycast_req_vec[global::i_graphics.get_frame_buffer_idx] = renderer.request_raycast(active_scene.cam.pos, math::normalize(target_world - active_scene.cam.pos), std::numeric_limits<float>::max());

			if (ui::is_any_hovered()) { break; }

			ecs_game.visit_all_storages_at(
				active_scene.code_idx,
				[&](c_auto storage_idx, auto& entities) noexcept {
					if (need_object_click is_false) { return; }

					for (auto&& [obj, ent_id] : entities | ecs::each_entity_soft<ecs::render_object, ecs::sv_entity_id>())
					{
						if (obj.render_id != raycast_res.object_id) { continue; }

						if (ui::g::p_input_ctx->is_shift_down())
						{
							add_select(e::select_kind::entity, storage_idx, ent_id);
						}
						else
						{
							clear_select();
							add_select(e::select_kind::entity, storage_idx, ent_id);
						}

						need_object_click = false;
						return;
					}
				});
		}
		while (false);

		c_auto need_copy = ui::g::p_input_ctx->is_ctrl_down() and ui::g::p_input_ctx->is_pressed(key_d);

		do
		{
			if (g::current_select_kind != e::select_kind::entity) { break; }

			auto   aabb_min	  = float3::max();
			auto   aabb_max	  = float3::lowest();
			auto   quat_sum	  = float4::zero();
			c_auto need_focus = g::set_focus or ui::g::p_input_ctx->is_pressed(key_f);

			for (auto&& [storage_code_idx, vec] : g::select_vec | std::views::enumerate /*editor::all_selected()*/)
			{
				if (storage_code_idx >= ecs_game.scene_count()) { continue; }

				ecs_game.visit_storage_at(
					active_scene.code_idx, static_cast<uint32>(storage_code_idx),
					[&](auto& entities) {
						using t_storage = BARE_OF(entities);
						using t_ent_id	= typename t_storage::t_ent_id;
						for (auto ecs_ent_id : vec)
						{
							c_auto id = static_cast<t_ent_id>(ecs_ent_id);
							if (need_copy)
							{
								copy_entity(g::current_game.current_active_scene_idx, active_scene.find_storage_idx(static_cast<uint32>(storage_code_idx)), ecs_ent_id);
							}

							auto&& [min, max] = calc_entity_aabb(entities, renderer, active_scene.find_storage_data(static_cast<uint32>(storage_code_idx)), id);
							aabb_min		  = age::min(aabb_min, min);
							aabb_max		  = age::max(aabb_max, max);

							if constexpr (entities.has_component<ecs::rotation>())
							{
								if (g::gizmo_space == e::transform_space_kind::local and entities.has_component<ecs::rotation>(id))
								{
									auto&& [quat] = entities.get_component<const ecs::rotation>(id);

									quat_sum += quat * std::copysign(1.f, math::dot(quat_sum, quat));
								}
							}

							if constexpr (entities.has_component<ecs::render_object, ecs::model>())
							{
								if (entities.has_component<ecs::render_object, ecs::model>(id))
								{
									auto&& [obj, model] = entities.get_component<const ecs::render_object, const ecs::model>(id);

									if (AGE_IS_INVALID_ID(obj.render_id) or runtime::is_handle_invalid(model.h_model)) { continue; }

									if (c_auto& entry = model.h_model.get_entry<asset::e::kind::model>();
										entry.is_loaded())
									{
										renderer.render_selection_outline(obj.render_id, entry.h_mesh, math::srgb_to_linear(float4{ 1, 0, 0, 1 }), 2.f, 0.f);
									}
								}
							}
						}
					});


				// editor::command::copy(g::current_select_kind, ecs_game, renderer);
			}

			if (aabb_min > aabb_max) { break; }
			if (need_focus)
			{
				focus_camera(renderer, aabb_min, aabb_max);
			}

			auto orientation = math::g::quaternion_identity;
			if (g::gizmo_space == e::transform_space_kind::local)
			{
				AGE_ASSERT(quat_sum.x != 0.f or quat_sum.y != 0.f or quat_sum.z != 0.f or quat_sum.w != 0.f);

				orientation = math::normalize(quat_sum);
			}

			widget_transform(ecs_game, renderer, (aabb_min + aabb_max) * 0.5f, orientation);
		}
		while (false);

		g::set_focus = false;

		{
			struct
			{
				bool gi_active_found;
			} update_storage_ctx{ false };

			for (c_auto& editor_storage : active_scene.storage_data_vec)
			{
				ecs_game.visit_storage_at(active_scene.code_idx, editor_storage.code_idx, AGE_FUNC(detail::update_storage), renderer, update_storage_ctx);
			}

			if (renderer.ddgi_enabled() is_true and update_storage_ctx.gi_active_found is_false)
			{
				renderer.disable_ddgi();
			}

			if (renderer.gibs_enabled() is_true and update_storage_ctx.gi_active_found is_false)
			{
				renderer.disable_gibs();
			}

			if (renderer.gist_enabled() is_true and update_storage_ctx.gi_active_found is_false)
			{
				renderer.disable_gist();
			}
		}

		if (ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_ctrl) and ui::g::p_input_ctx->is_pressed(input::e::key_kind::key_s))
		{
			editor::save_game();
			std::println("game saved");
		}
	}
}	 // namespace age::editor

void
age::editor::render_current_scene(auto& ecs_game, auto& renderer, age::platform::window_handle h_window) noexcept
{
	using namespace age::ecs;
	auto& active_scene = g::current_game.scene_data_vec[g::current_game.current_active_scene_idx];

	editor::update_camera(renderer, ui::g::p_input_ctx->is_down(input::e::key_kind::mouse_right), h_window);

	ecs_game.visit_all_storages_at(
		active_scene.code_idx,
		[&](auto& entities) {
			if constexpr (entities.has_component<position, rotation, scale, render_object, model, model_render_option>())
			{
				for (auto&& [ent_id, pos, rot, scale, obj, model] :
					 entities | each_entity<sv_entity_id, const position, const rotation, const scale, const render_object, const model>())
				{
					renderer.update_object(obj.render_id, pos, rot, scale);

					if (age::runtime::is_handle_invalid(model.h_model)) { continue; }

					if (c_auto& entry = model.h_model.get_entry<age::asset::e::kind::model>();
						entry.is_loaded() is_false)
					{
						continue;
					}

					if (asset::model::is_renderable(model.h_model) is_false) { continue; }

					if (entities.has_component<model_render_option>(ent_id))
					{
						auto&& [option] = entities.get_component<const model_render_option>(ent_id);
						renderer.render_model(0, obj.render_id, model.h_model, cmp_to_desc(option));
					}
					else
					{
						renderer.render_model(0, obj.render_id, model.h_model);
					}
				}
			}
		});
}
