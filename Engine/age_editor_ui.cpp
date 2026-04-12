#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor
{
	void
	ui_select(uint64 ent_id) noexcept
	{
		char c_buf[21];
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

	void
	ui_component(ecs::position& pos) noexcept
	{
		ui::widget::numeric_field(pos, "possition");
	}

	void
	ui_component(ecs::render_object& obj) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, obj.render_id);
		ui::widget::text_secondary(c_buf);
	}

	void
	ui_component(ecs::rotation& rot) noexcept
	{
		ui::widget::rotation_field(rot, "rotation");
	}

	void
	ui_component(ecs::scale& scale) noexcept
	{
		ui::widget::numeric_field(scale, "scale");
	}

	void
	ui_component(ecs::mesh& mesh) noexcept
	{
		ui::widget::numeric_field(mesh.render_id, "mesh", 0u, 2u);
	}

	void
	ui_component(ecs::material& mat) noexcept
	{
		ui::widget::checkbox("is_opaque", mat.is_opaque);
	}

	void
	ui_component(ecs::directional_light& light) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_secondary(c_buf);

		ui::widget::checkbox("cast shadow", light.cast_shadow);
		ui::widget::numeric_field(light.direction, "direction");
		ui::widget::numeric_field(light.intensity, "intensity");
		ui::widget::numeric_field(light.color, "color");
	}

	void
	ui_component(ecs::point_light& light) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_secondary(c_buf);

		ui::widget::checkbox("cast shadow", light.cast_shadow);
		ui::widget::numeric_field(light.range, "range");
		ui::widget::numeric_field(light.color, "color");
		ui::widget::numeric_field(light.intensity, "intensity");
	}

	void
	ui_component(ecs::spot_light& light) noexcept
	{
		char c_buf[12];
		age::util::to_str(c_buf, light.render_id);
		ui::widget::text_secondary(c_buf);

		ui::widget::checkbox("cast shadow", light.cast_shadow);
		ui::widget::numeric_field(light.range, "range");
		ui::widget::numeric_field(light.direction, "direction");
		ui::widget::numeric_field(light.intensity, "intensity");
		ui::widget::numeric_field(light.color, "color");
		ui::widget::numeric_field(light.cos_inner, "cos_inner", 0.f, 1.f);
		ui::widget::numeric_field(light.cos_outer, "cos_outer", light.cos_inner, 1.f);
	}

	void
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
}	 // namespace age::editor