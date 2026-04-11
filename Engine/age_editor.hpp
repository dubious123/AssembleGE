#pragma once
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
		ui::widget::numeric_field(mesh.render_id, "mesh", 0u, 3u);

		ui::widget::checkbox("is_opaque", mat.is_opaque);

		renderer.update_object(render_obj.render_id, pos, rot, scale);
	}
}	 // namespace age::editor