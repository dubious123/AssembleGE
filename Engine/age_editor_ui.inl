#include "age.hpp"

namespace age::editor
{
	// ui
	void
	ui_camera(age::ecs::position& pos, age::ecs::camera& cam, auto& renderer) noexcept
	{
		if (auto h = ui::widget::vertical())
		{
			ui::widget::text_secondary(age::graphics::e::to_string(cam.kind).data());

			ui::widget::numeric_field(pos, "pos");

			ui::widget::numeric_field(cam.euler_deg, "rotation", float3{ -90.f, -180.f, -180.f }, float3{ 90.f, 180.f, 180.f });

			ui::widget::numeric_field(cam.near_z, "near_z", cam.far_z);
			ui::widget::numeric_field(cam.far_z, "far_z", 0.f);

			if (cam.kind == age::graphics::e::camera_kind::perspective)
			{
				ui::widget::numeric_field(cam.fov_y, "fov_y");
				ui::widget::numeric_field(cam.aspect_ratio, "aspect_ratio");
			}
			else
			{
				ui::widget::numeric_field(cam.view_width, "view_width");
				ui::widget::numeric_field(cam.view_height, "view_height");
			}
		}

		renderer.update_camera(
			cam.render_id,
			{
				.kind		 = cam.kind,
				.pos		 = pos,
				.quaternion	 = age::math::euler_deg_to_quat(cam.euler_deg),
				.near_z		 = cam.near_z,
				.far_z		 = cam.far_z,
				.perspective = {
					.fov_y		  = cam.fov_y,
					.aspect_ratio = cam.aspect_ratio,

				},
			});
	}

	void
	ui_directional_light(age::ecs::directional_light& light, auto& renderer) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_secondary(c_buf);

		if (auto h = ui::widget::checkbox("cast shadow", light.cast_shadow))
		{
			if (h.clicked())
			{
				renderer.remove_directional_light(light.render_id);

				light.render_id = renderer.add_directional_light({}, light.cast_shadow);
			}
		}

		ui::widget::numeric_field(light.direction, "direction");
		ui::widget::numeric_field(light.intensity, "intensity");
		ui::widget::numeric_field(light.color, "color");

		renderer.update_directional_light(
			light.render_id,
			{
				.direction = age::math::normalize(light.direction),
				.intensity = light.intensity,
				.color	   = light.color,
			});
	}

	void
	ui_point_light(age::ecs::position& pos, age::ecs::point_light& light, auto& renderer) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_secondary(c_buf);

		if (auto h = ui::widget::checkbox("cast shadow", light.cast_shadow))
		{
			if (h.clicked())
			{
				renderer.remove_point_light(light.render_id);

				light.render_id = renderer.add_point_light({}, light.cast_shadow);
			}
		}

		ui::widget::numeric_field(pos, "pos");
		ui::widget::numeric_field(light.range, "range");
		ui::widget::numeric_field(light.color, "color");
		ui::widget::numeric_field(light.intensity, "intensity");

		renderer.update_point_light(
			light.render_id,
			{
				.position  = pos,
				.range	   = light.range,
				.color	   = light.color,
				.intensity = light.intensity,
			});
	}

	void
	ui_spot_light(age::ecs::position& pos, age::ecs::spot_light& light, auto& renderer) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_secondary(c_buf);

		if (auto h = ui::widget::checkbox("cast shadow", light.cast_shadow))
		{
			if (h.clicked())
			{
				renderer.remove_spot_light(light.render_id);

				light.render_id = renderer.add_spot_light({}, light.cast_shadow);
			}
		}

		ui::widget::numeric_field(pos, "pos");
		ui::widget::numeric_field(light.range, "range");
		ui::widget::numeric_field(light.direction, "direction");
		ui::widget::numeric_field(light.intensity, "intensity");
		ui::widget::numeric_field(light.color, "color");
		ui::widget::numeric_field(light.cos_inner, "cos_inner", 0.f, 1.f);
		ui::widget::numeric_field(light.cos_outer, "cos_outer", light.cos_inner, 1.f);

		renderer.update_spot_light(
			light.render_id,
			{
				.position  = pos,
				.range	   = light.range,
				.direction = age::math::normalize(light.direction),
				.intensity = light.intensity,
				.color	   = light.color,
				.cos_inner = light.cos_inner,
				.cos_outer = light.cos_outer,
			});
	}

	void
	ui_render_object(age::ecs::render_object render_obj,
					 age::ecs::position&	 pos,
					 age::ecs::rotation&	 rot,
					 age::ecs::scale&		 scale,
					 age::ecs::mesh&		 mesh,
					 age::ecs::material&	 mat,
					 auto&					 renderer) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, render_obj.render_id);
		ui::widget::text_secondary(c_buf);

		ui::widget::numeric_field(pos, "pos");
		ui::widget::rotation_field(rot, "rotation");
		ui::widget::numeric_field(scale, "scale");
		ui::widget::numeric_field(mesh.render_id, "mesh", 0u, 2u);

		ui::widget::checkbox("is_opaque", mat.is_opaque);

		renderer.update_object(render_obj.render_id, pos, rot, scale);
	}

	void
	ui_select(auto ent_id) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, ent_id);

		auto selected = age::editor::is_selected(ent_id);

		if (auto sel = ui::widget::selectable(c_buf, selected))
		{
			if (ui::g::p_input_ctx->is_shift_down())
			{
				if (sel.clicked())
				{
					if (selected)
					{
						age::editor::remove_select(ent_id);
					}
					else
					{
						age::editor::add_select(ent_id);
					}
				}
			}
			else
			{
				if (sel.clicked())
				{
					age::editor::clear_select();
					age::editor::add_select(ent_id);
				}
			}
		}
	}

	inline void
	ui_component(ecs::position& pos) noexcept
	{
		ui::widget::numeric_field(pos, "possition");
	}

	inline void
	ui_component(ecs::render_object& obj) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, obj.render_id);
		ui::widget::text_secondary(c_buf);
	}

	inline void
	ui_component(ecs::rotation& rot) noexcept
	{
		ui::widget::rotation_field(rot, "rotation");
	}

	inline void
	ui_component(ecs::scale& scale) noexcept
	{
		ui::widget::numeric_field(scale, "scale");
	}

	inline void
	ui_component(ecs::mesh& mesh) noexcept
	{
		ui::widget::numeric_field(mesh.render_id, "mesh", 0u, 2u);
	}

	inline void
	ui_component(ecs::material& mat) noexcept
	{
		ui::widget::checkbox("is_opaque", mat.is_opaque);
	}

	inline void
	ui_component(ecs::directional_light& light) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_secondary(c_buf);

		if (auto h = ui::widget::checkbox("cast shadow", light.cast_shadow))
		{
			// if (h.clicked())
			//{
			//	renderer.remove_directional_light(light.render_id);

			//	light.render_id = renderer.add_directional_light({}, light.cast_shadow);
			//}
		}

		ui::widget::numeric_field(light.direction, "direction");
		ui::widget::numeric_field(light.intensity, "intensity");
		ui::widget::numeric_field(light.color, "color");
	}

	inline void
	ui_component(ecs::point_light& light) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_secondary(c_buf);

		if (auto h = ui::widget::checkbox("cast shadow", light.cast_shadow))
		{
			// if (h.clicked())
			//{
			//	renderer.remove_point_light(light.render_id);

			//	light.render_id = renderer.add_point_light({}, light.cast_shadow);
			//}
		}

		ui::widget::numeric_field(light.range, "range");
		ui::widget::numeric_field(light.color, "color");
		ui::widget::numeric_field(light.intensity, "intensity");
	}

	inline void
	ui_component(ecs::spot_light& light) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_secondary(c_buf);

		if (auto h = ui::widget::checkbox("cast shadow", light.cast_shadow))
		{
			// if (h.clicked())
			//{
			//	renderer.remove_spot_light(light.render_id);

			//	light.render_id = renderer.add_spot_light({}, light.cast_shadow);
			//}
		}

		ui::widget::numeric_field(light.range, "range");
		ui::widget::numeric_field(light.direction, "direction");
		ui::widget::numeric_field(light.intensity, "intensity");
		ui::widget::numeric_field(light.color, "color");
		ui::widget::numeric_field(light.cos_inner, "cos_inner", 0.f, 1.f);
		ui::widget::numeric_field(light.cos_outer, "cos_outer", light.cos_inner, 1.f);
	}

	inline void
	ui_component(age::ecs::camera& cam) noexcept
	{
		ui::widget::text_secondary(age::graphics::e::to_string(cam.kind).data());

		ui::widget::numeric_field(cam.euler_deg, "rotation", float3{ -90.f, -180.f, -180.f }, float3{ 90.f, 180.f, 180.f });

		ui::widget::numeric_field(cam.near_z, "near_z", cam.far_z);
		ui::widget::numeric_field(cam.far_z, "far_z", 0.f);

		if (cam.kind == age::graphics::e::camera_kind::perspective)
		{
			ui::widget::numeric_field(cam.fov_y, "fov_y");
			ui::widget::numeric_field(cam.aspect_ratio, "aspect_ratio");
		}
		else
		{
			ui::widget::numeric_field(cam.view_width, "view_width");
			ui::widget::numeric_field(cam.view_height, "view_height");
		}
	}

	inline void
	ui_component(auto&& cmp) noexcept
	{
		ui::widget::text_primary("ui for component ?? not implemented yet");
	}

	void
	ui_inspector(auto& entities) noexcept
	{
		if (g::select_vec.is_empty()) { return; }

		using t_storage			 = BARE_OF(entities);
		using t_ent_id			 = typename t_storage::t_ent_id;
		using t_archetype_traits = typename t_storage::t_archetype_traits;

		// todo : implement multiselection
		if (g::select_vec.size() > 1) { return; }

		c_auto ent_id = static_cast<t_ent_id>(g::select_vec[0]);

		c_auto archetype = entities.get_archetype(ent_id);

		for (auto storage_cmp_idx : age::views::each_set_bit_idx(archetype))
		{
			t_archetype_traits::visit_component(entities, ent_id, storage_cmp_idx, AGE_FUNC(ui_component));
		}
	}
}	 // namespace age::editor