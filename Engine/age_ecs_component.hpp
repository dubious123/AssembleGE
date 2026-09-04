#pragma once
#include "age.hpp"

namespace age::ecs
{
	struct cmp_dispatch_key
	{
	  private:
		cmp_dispatch_key() = default;

		template <typename t_cmp>
		friend void
		on_create_component(t_cmp& cmp, auto&& ctx) noexcept;

		template <typename t_cmp>
		friend void
		on_destroy_component(t_cmp& cmp, auto&& ctx) noexcept;

		template <typename t_cmp>
		friend void
		serialize_component(const t_cmp& cmp, byte_buf& buf, auto&& ctx) noexcept;

		template <typename t_cmp>
		friend void
		deserialize_component(t_cmp& cmp, auto& buf, auto&& ctx) noexcept;
	};

	template <typename t_component>
	consteval uint32
	get_component_version()
	{
		return t_component::age_component_version();
	}

	template <typename t_component>
	consteval uint32
	get_byte_size()
	{
		if constexpr (requires { { t_component::byte_size() } -> std::unsigned_integral; })
		{
			return static_cast<uint32>(t_component::byte_size());
		}
		else
		{
			return static_cast<uint32>(sizeof(t_component));
		}
	}

	template <typename t_cmp>
	FORCE_INLINE void
	on_create_component(t_cmp& cmp, auto&& ctx) noexcept
	{
		if constexpr (requires { t_cmp::on_create(cmp_dispatch_key{}, cmp, ctx); })
		{
			t_cmp::on_create(cmp_dispatch_key{}, cmp, ctx);
		}
	}

	template <typename t_cmp>
	FORCE_INLINE void
	on_destroy_component(t_cmp& cmp, auto&& ctx) noexcept
	{
		if constexpr (requires { t_cmp::on_destroy(cmp_dispatch_key{}, cmp, ctx); })
		{
			t_cmp::on_destroy(cmp_dispatch_key{}, cmp, ctx);
		}
	}

	template <typename t_cmp>
	FORCE_INLINE void
	serialize_component(const t_cmp& cmp, byte_buf& buf, auto&& ctx) noexcept
	{
		if constexpr (requires { t_cmp::write_to(cmp_dispatch_key{}, cmp, buf, ctx); })
		{
			t_cmp::write_to(cmp_dispatch_key{}, cmp, buf, ctx);
		}
		else
		{
			buf.write(cmp);
		}
	}

	template <typename t_cmp>
	FORCE_INLINE void
	serialize_component_from_ptr(const void* p_cmp, byte_buf& buf, auto&& ctx) noexcept
	{
		serialize_component(*std::launder(static_cast<const t_cmp*>(p_cmp)), buf, ctx);
	}

	template <typename t_cmp>
	FORCE_INLINE void
	deserialize_component(t_cmp& cmp, auto& buf, auto&& ctx) noexcept
	{
		if constexpr (requires { t_cmp::read_from(cmp_dispatch_key{}, cmp, buf, ctx); })
		{
			t_cmp::read_from(cmp_dispatch_key{}, cmp, buf, ctx);
		}
		else
		{
			buf.read(cmp);
		}
	}

#define AGE_COMPONENT(name, ...)                                                                                   \
	struct name;                                                                                                   \
	template <>                                                                                                    \
	consteval bool is_ecs_component<name>()                                                                        \
	{ return true; }                                                                                               \
	template <>                                                                                                    \
	consteval auto get_component_name<name>()                                                                      \
	{ return age::util::to_fixed_str_arr<age::config::max_component_name_len>(#name __VA_OPT__(, ) __VA_ARGS__); } \
	template <typename t, std::size_t i>                                                                           \
	requires std::is_same_v<t, name>                                                                               \
	consteval auto get_component_name_at()                                                                         \
	{ return get_component_name<name>()[i]; }                                                                      \
	struct name

#define AGE_COMPONENT_VERSION(version) \
	static consteval auto age_component_version() { return version; }

#define AGE_CUSTOM_BYTE_SIZE(...) \
	static consteval uint32 byte_size() { return static_cast<uint32>(FOR_EACH_SEP(sizeof, AGE_PP_PLUS_I, __VA_ARGS__)); };

	AGE_COMPONENT(transform_3d, "transform")
	{
		AGE_COMPONENT_VERSION(1);
		float3 position;
		float4 quaternion;
		float3 scale;
	};

	AGE_COMPONENT(position, "pos", "position_3d") : public float3
	{
		AGE_COMPONENT_VERSION(1);

		using float3::float3;
		constexpr position() noexcept : float3(0.f, 0.f, 0.f) { }
		constexpr position(auto&&... arg) noexcept requires(sizeof...(arg) > 0)
			: float3{ FWD(arg)... }
		{
		}
	};

	AGE_COMPONENT(rotation, "rot", "quat", "quaternion") : public float4
	{
		AGE_COMPONENT_VERSION(1);

		using float4::float4;
		constexpr rotation() noexcept : float4{ 0, 0, 0, 1.f } { }
		constexpr rotation(auto&&... arg) noexcept requires(sizeof...(arg) > 0)
			: float4{ FWD(arg)... }
		{
		}
	};

	AGE_COMPONENT(scale, "sc", "scale_3d") : public float3
	{
		AGE_COMPONENT_VERSION(1);

		using float3::float3;
		constexpr scale() noexcept : float3{ 1.f, 1.f, 1.f } { }
		constexpr scale(auto&&... arg) noexcept requires(sizeof...(arg) > 0)
			: float3{ FWD(arg)... }
		{
		}
	};

	AGE_COMPONENT(render_object, "render_obj", "renderable_instance")
	{
		AGE_COMPONENT_VERSION(1);

		uint32 render_id = age::get_invalid_id<uint32>();

		static consteval uint32
		byte_size() noexcept
		{
			return 0ul;
		}

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, render_object & cmp, auto& ctx) noexcept
		{
			cmp.render_id = ctx.renderer.add_object(float3::zero(), age::math::g::quaternion_identity, float3::one());
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, render_object & cmp, auto& ctx) noexcept
		{
			ctx.renderer.remove_object(cmp.render_id);
		}

		static void
		write_to(cmp_dispatch_key, const render_object& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			return;
		}

		static void
		read_from(cmp_dispatch_key, render_object & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != render_object::age_component_version())
			{
				// handle migrate
				AGE_ASSERT(false);
			}
		}
	};

	AGE_COMPONENT(camera, "cam", "camera_3d")
	{
		AGE_COMPONENT_VERSION(1);

		uint32 render_id = age::get_invalid_id<uint32>();

		graphics::e::camera_kind kind;
		uint8_3					 _;

		float3 euler_deg = float3::zero();

		float near_z = 0.1f;
		float far_z	 = 1000.f;

		float fov_y		   = age::cvt_to_radian(75.f);
		float aspect_ratio = (16.f / 9.f);

		float view_width  = 1.f;
		float view_height = 1.f;

		AGE_CUSTOM_BYTE_SIZE(kind, euler_deg, near_z, far_z, fov_y, aspect_ratio, view_width, view_height)

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, camera & cmp, auto& ctx) noexcept
		{
			if (cmp.kind == graphics::e::camera_kind::perspective)
			{
				cmp.render_id = ctx.renderer.add_camera({ .kind		  = cmp.kind,
														  .quaternion = age::math::euler_deg_to_quat(cmp.euler_deg),
														  .near_z	  = cmp.near_z,
														  .far_z	  = cmp.far_z,
														  .perspective{
															  .fov_y		= cmp.fov_y,
															  .aspect_ratio = cmp.aspect_ratio } });
			}
			else
			{
				cmp.render_id = ctx.renderer.add_camera({ .kind			= cmp.kind,
														  .quaternion	= age::math::euler_deg_to_quat(cmp.euler_deg),
														  .near_z		= cmp.near_z,
														  .far_z		= cmp.far_z,
														  .orthographic = {
															  .view_width  = cmp.view_width,
															  .view_height = cmp.view_height } });
			}
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, camera & cmp, auto& ctx) noexcept
		{
			ctx.renderer.remove_camera(cmp.render_id);
		}

		static void
		write_to(cmp_dispatch_key, const camera& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			buf.write(cmp.kind, cmp.euler_deg, cmp.near_z, cmp.far_z, cmp.fov_y, cmp.aspect_ratio, cmp.view_width, cmp.view_height);
			return;
		}

		static void
		read_from(cmp_dispatch_key, camera & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != camera::age_component_version())
			{
				// handle migrate
				AGE_ASSERT(false);
			}

			buf.read(cmp.kind, cmp.euler_deg, cmp.near_z, cmp.far_z, cmp.fov_y, cmp.aspect_ratio, cmp.view_width, cmp.view_height);
		}
	};

	AGE_COMPONENT(editor_cam_setting, "editor camera")
	{
		AGE_COMPONENT_VERSION(1);

		float move_speed	 = 2.f;
		float sprint_mult	 = 4.f;
		float sensitivity	 = 0.17f;
		float zoom_speed	 = 2.f;
		float zoom_distance	 = 4.f;
		float pan_speed		 = 0.6f;
		float move_smoothing = 15.f / 2.f;
		float look_smoothing = 25.f / 2.f;
		float zoom_smoothing = 12.f / 2.f;
	};

	AGE_COMPONENT(bloom, "post_process_bloom")
	{
		AGE_COMPONENT_VERSION(1);

		uint16 render_id = age::get_invalid_id<uint16>();

		bool  active;
		uint8 _;

		float  threshold = 0.04f;
		float  knee		 = 0.5f;
		float  intensity = 0.05f;
		float  radius	 = 1.0f;
		float3 tint		 = float3::one();

		AGE_CUSTOM_BYTE_SIZE(active, threshold, knee, intensity, radius, tint)

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, bloom & cmp, auto& ctx) noexcept
		{
			cmp.render_id = ctx.renderer.add_bloom({ .threshold = cmp.threshold,
													 .knee		= cmp.knee,
													 .intensity = cmp.intensity,
													 .radius	= cmp.radius,
													 .tint		= cmp.tint });

			ctx.renderer.set_bloom_active(cmp.render_id, cmp.active);
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, bloom & cmp, auto& ctx) noexcept
		{
			ctx.renderer.remove_bloom(cmp.render_id);
		}

		static void
		write_to(cmp_dispatch_key, const bloom& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			buf.write(cmp.active, cmp.threshold, cmp.knee, cmp.intensity, cmp.radius, cmp.tint);
		}

		static void
		read_from(cmp_dispatch_key, bloom & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != bloom::age_component_version())
			{
				// handle migrate
				AGE_ASSERT(false);
			}

			buf.read(cmp.active, cmp.threshold, cmp.knee, cmp.intensity, cmp.radius, cmp.tint);
		}
	};

	AGE_COMPONENT(directional_light, "dir_light")
	{
		AGE_COMPONENT_VERSION(1);

		uint16 render_id   = age::get_invalid_id<uint16>();
		bool   cast_shadow = true;
		uint8  _;

		float3 direction = age::normalize(float3{ -0.3f, -1.0f, 0.5f });
		float  intensity = 0.80f;
		float3 color	 = float3{ 1.0f, 0.9f, 0.9f };

		AGE_CUSTOM_BYTE_SIZE(cast_shadow, direction, intensity, color)

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, directional_light & cmp, auto& ctx) noexcept
		{
			cmp.render_id = ctx.renderer.add_directional_light({ .direction	  = cmp.direction,
																 .intensity	  = cmp.intensity,
																 .color		  = cmp.color,
																 .cast_shadow = cmp.cast_shadow });
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, directional_light & cmp, auto& ctx) noexcept
		{
			ctx.renderer.remove_directional_light(cmp.render_id);
		}

		static void
		write_to(cmp_dispatch_key, const directional_light& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			buf.write(cmp.cast_shadow, cmp.direction, cmp.intensity, cmp.color);
		}

		static void
		read_from(cmp_dispatch_key, directional_light & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != directional_light::age_component_version())
			{
				// handle migrate
				AGE_ASSERT(false);
			}

			buf.read(cmp.cast_shadow, cmp.direction, cmp.intensity, cmp.color);
		}
	};

	AGE_COMPONENT(point_light, "pt_light")
	{
		AGE_COMPONENT_VERSION(1);

		uint32 render_id = age::get_invalid_id<uint32>();

		float  range	 = 1.f;
		float3 color	 = float3{ 0, 1, 0 };
		float  intensity = 1.f;

		bool	cast_shadow = false;
		uint8_3 _;

		AGE_CUSTOM_BYTE_SIZE(range, color, intensity, cast_shadow)

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, point_light & cmp, auto& ctx) noexcept
		{
			cmp.render_id = ctx.renderer.add_point_light({ .range		= cmp.range,
														   .color		= cmp.color,
														   .intensity	= cmp.intensity,
														   .cast_shadow = cmp.cast_shadow });
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, point_light & cmp, auto& ctx) noexcept
		{
			ctx.renderer.remove_point_light(cmp.render_id);
		}

		static void
		write_to(cmp_dispatch_key, const point_light& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			buf.write(cmp.range, cmp.color, cmp.intensity, cmp.cast_shadow);
		}

		static void
		read_from(cmp_dispatch_key, point_light & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != point_light::age_component_version())
			{
				AGE_ASSERT(false);
			}

			buf.read(cmp.range, cmp.color, cmp.intensity, cmp.cast_shadow);
		}
	};

	AGE_COMPONENT(spot_light, "sp_light")
	{
		AGE_COMPONENT_VERSION(1);

		uint32 render_id = age::get_invalid_id<uint32>();

		float  range	 = 1.f;
		float3 direction = float3::one();
		float  intensity = 1.f;
		float3 color	 = float3{ 1, 0, 0 };
		float  cos_inner = 0.1f;
		float  cos_outer = 0.5f;

		bool	cast_shadow = false;
		uint8_3 _;

		AGE_CUSTOM_BYTE_SIZE(range, direction, intensity, color, cos_inner, cos_outer, cast_shadow)

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, spot_light & cmp, auto& ctx) noexcept
		{
			cmp.render_id = ctx.renderer.add_spot_light({ .range	   = cmp.range,
														  .direction   = cmp.direction,
														  .intensity   = cmp.intensity,
														  .color	   = cmp.color,
														  .cos_inner   = cmp.cos_inner,
														  .cos_outer   = cmp.cos_outer,
														  .cast_shadow = cmp.cast_shadow });
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, spot_light & cmp, auto& ctx) noexcept
		{
			ctx.renderer.remove_spot_light(cmp.render_id);
		}

		static void
		write_to(cmp_dispatch_key, const spot_light& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			buf.write(cmp.range, cmp.direction, cmp.intensity, cmp.color, cmp.cos_inner, cmp.cos_outer, cmp.cast_shadow);
		}

		static void
		read_from(cmp_dispatch_key, spot_light & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != spot_light::age_component_version())
			{
				AGE_ASSERT(false);
			}

			buf.read(cmp.range, cmp.direction, cmp.intensity, cmp.color, cmp.cos_inner, cmp.cos_outer, cmp.cast_shadow);
		}
	};

	AGE_COMPONENT(mesh, "msh", "meshlet mesh")
	{
		AGE_COMPONENT_VERSION(1);

		asset::handle h_mesh = {};

		FORCE_INLINE void
		update_h_mesh(asset::handle h_mesh_new) noexcept
		{
			if (runtime::is_handle_invalid(h_mesh) is_false)
			{
				asset::mesh_baked::remove_ref(h_mesh);
			}
			if (runtime::is_handle_invalid(h_mesh_new) is_false)
			{
				asset::mesh_baked::add_ref(h_mesh_new);
			}

			h_mesh = h_mesh_new;
		}

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, mesh & cmp, auto& ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_mesh) is_false)
			{
				asset::mesh_baked::add_ref(cmp.h_mesh);
			}
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, mesh & cmp, auto& ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_mesh) is_false)
			{
				asset::mesh_baked::remove_ref(cmp.h_mesh);
			}
		}

		static consteval uint32
		byte_size() noexcept
		{
			return config::max_asset_path_len;
		}

		static void
		write_to(cmp_dispatch_key, const mesh& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_mesh))
			{
				char mesh_path[config::max_asset_path_len] = { "invalid_mesh" };
				buf.write(mesh_path);
			}
			else
			{
				buf.write(cmp.h_mesh.get_path());
			}

			return;
		}

		static void
		read_from(cmp_dispatch_key, mesh & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != mesh::age_component_version())
			{
				// handle migrate
				AGE_ASSERT(false);
			}

			char mesh_path[config::max_asset_path_len] = {};
			buf.read(mesh_path);

			cmp.update_h_mesh(asset::find(age::asset::e::kind::mesh_baked, mesh_path));
		}
	};

	AGE_COMPONENT(material, "mat", "pbr_mat", "material_3d")
	{
		AGE_COMPONENT_VERSION(2);

		uint32 render_id = 0;

		asset::handle h_mat = {};

		FORCE_INLINE void
		update_h_mat(asset::handle h_mat_new) noexcept
		{
			if (runtime::is_handle_invalid(h_mat) is_false)
			{
				asset::material::remove_ref(h_mat);
			}
			if (runtime::is_handle_invalid(h_mat_new) is_false)
			{
				asset::material::add_ref(h_mat_new);
			}

			h_mat = h_mat_new;
		}

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, material & cmp, auto& ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_mat) is_false)
			{
				asset::material::add_ref(cmp.h_mat);
			}
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, material & cmp, auto& ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_mat) is_false)
			{
				asset::material::remove_ref(cmp.h_mat);
			}
		}

		static consteval uint32
		byte_size() noexcept
		{
			return config::max_asset_path_len;
		}

		static void
		write_to(cmp_dispatch_key, const material& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_mat))
			{
				char mat_path[config::max_asset_path_len] = { "invalid mat" };
				buf.write(mat_path);
			}
			else
			{
				buf.write(cmp.h_mat.get_path());
			}

			return;
		}

		static void
		read_from(cmp_dispatch_key, material & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != material::age_component_version())
			{
				if (rw_ctx.version == 1)
				{
					buf.read<bool>();
				}
				else
				{
					AGE_ASSERT(false);
				}

				return;
			}

			char mat_path[config::max_asset_path_len] = {};
			buf.read(mat_path);

			cmp.update_h_mat(asset::find(age::asset::e::kind::material, mat_path));
		}
	};

	AGE_COMPONENT(model_render_option, "render_option")
	{
		AGE_COMPONENT_VERSION(1);

		age::graphics::e::mesh_raster_override_kind		   raster_override_kind		   = age::graphics::e::mesh_raster_override_kind::none;
		age::graphics::e::mesh_rt_alpha_test_override_kind rt_alpha_test_override_kind = age::graphics::e::mesh_rt_alpha_test_override_kind::none;
		age::graphics::e::model_render_option_flags		   option_flags				   = age::graphics::e::model_render_option_flags::none;
		uint8											   fade_unorm8				   = 255u;
	};

	AGE_COMPONENT(model, "model_renderer")
	{
		AGE_COMPONENT_VERSION(1);

		asset::handle h_model = {};

		FORCE_INLINE void
		update_h_model(asset::handle h_model_new) noexcept
		{
			if (runtime::is_handle_invalid(h_model) is_false)
			{
				asset::model::remove_ref(h_model);
			}
			if (runtime::is_handle_invalid(h_model_new) is_false)
			{
				asset::model::add_ref(h_model_new);
			}

			h_model = h_model_new;
		}

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, model & cmp, auto& ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_model) is_false)
			{
				asset::model::add_ref(cmp.h_model);
			}
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, model & cmp, auto& ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_model) is_false)
			{
				asset::model::remove_ref(cmp.h_model);
			}
		}

		static consteval uint32
		byte_size() noexcept
		{
			return config::max_asset_path_len;
		}

		static void
		write_to(cmp_dispatch_key, const model& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_model))
			{
				char model_path[config::max_asset_path_len] = { "invalid model" };
				buf.write(model_path);
			}
			else
			{
				buf.write(cmp.h_model.get_path());
			}

			return;
		}

		static void
		read_from(cmp_dispatch_key, model & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != model::age_component_version())
			{
				AGE_ASSERT(false);
				return;
			}

			char mat_path[config::max_asset_path_len] = {};
			buf.read(mat_path);

			cmp.update_h_model(asset::find(age::asset::e::kind::model, mat_path));
		}
	};

	AGE_COMPONENT(env_light, "ibl")
	{
		AGE_COMPONENT_VERSION(1);

		uint32 render_id = 0;

		asset::handle h_env_light = {};

		FORCE_INLINE void
		update_h_env_light(asset::handle h_env_light_new) noexcept
		{
			if (runtime::is_handle_invalid(h_env_light) is_false)
			{
				asset::env_light::remove_ref(h_env_light);
			}
			if (runtime::is_handle_invalid(h_env_light_new) is_false)
			{
				asset::env_light::add_ref(h_env_light_new);
			}

			h_env_light = h_env_light_new;
		}

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, env_light & cmp, auto& ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_env_light) is_false)
			{
				asset::env_light::add_ref(cmp.h_env_light);
			}
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, env_light & cmp, auto& ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_env_light) is_false)
			{
				asset::env_light::remove_ref(cmp.h_env_light);
			}
		}

		static consteval uint32
		byte_size() noexcept
		{
			return config::max_asset_path_len;
		}

		static void
		write_to(cmp_dispatch_key, const env_light& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			if (runtime::is_handle_invalid(cmp.h_env_light))
			{
				char env_light_path[config::max_asset_path_len] = { "invalid env_light" };
				buf.write(env_light_path);
			}
			else
			{
				buf.write(cmp.h_env_light.get_path());
			}

			return;
		}

		static void
		read_from(cmp_dispatch_key, env_light & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != env_light::age_component_version())
			{
				AGE_ASSERT(false);
				return;
			}

			char env_light_path[config::max_asset_path_len] = {};
			buf.read(env_light_path);

			cmp.update_h_env_light(asset::find(age::asset::e::kind::env_light, env_light_path));
		}
	};

	AGE_COMPONENT(gi_config, "ddgi_config", "gibs_config", "gist_config")
	{
		AGE_COMPONENT_VERSION(6);

		// ddgi
		bool	 enable_ddgi			   = false;
		bool	 ddgi_lock_origin		   = false;
		uint32_3 ddgi_probe_per_level_axis = uint32_3{ 32, 16, 32 };
		float3	 ddgi_base_probe_spacing   = float3{ 1.f, 2.f, 1.f };
		uint32	 ddgi_level_count		   = 6;

		age::graphics::e::ddgi_debug_flags ddgi_debug_flags;

		// gibs
		bool enable_gibs	  = false;
		bool gibs_lock_origin = false;

		// pow of 2
		uint8 gibs_cell_count = 64u;
		// less than 16
		uint8 gibs_outer_layer_count = 16u;

		float gibs_cell_size = 1.f;

		// greater than 1.f
		float outer_cell_size_factor = 1.5f;

		// less than gibs_ray_budget
		uint32 max_surfel_count = age::g::uint32_max;

		age::graphics::e::gibs_debug_flags gibs_debug_flags;

		// gist
		bool  enable_gist			   = false;
		bool  gist_lock_origin		   = false;
		uint8 gist_diffuse_ray_period  = 4u;
		uint8 gist_specular_ray_period = 1u;

		uint8 gist_cell_surfel_ray_count_min = 1u;	   // pow_of_2
		uint8 gist_cell_surfel_ray_count_max = 64u;	   // pow_of_2
		uint8 gist_cell_count_per_axis		 = 32u;	   // pow_of_2
		uint8 gist_outer_layer_count		 = 8u;

		uint32 gist_max_cell_surfel_count		  = static_cast<uint32>((32 * 32 * 32 + 32 * 32 * 6 * 16) * 0.25f);
		float  gist_cell_surfel_ray_budget_factor = 1.5f;

		float							   gist_cell_size			   = 4.f;
		float							   gist_outer_cell_size_factor = 1.5f;
		age::graphics::e::gist_debug_flags gist_debug_flags			   = age::graphics::e::gist_debug_flags{ 0u };

		AGE_CUSTOM_BYTE_SIZE(
			enable_ddgi,
			ddgi_lock_origin,
			ddgi_probe_per_level_axis,
			ddgi_base_probe_spacing,
			ddgi_level_count,
			ddgi_debug_flags,

			enable_gibs,
			gibs_lock_origin,
			gibs_cell_count,
			gibs_outer_layer_count,
			gibs_cell_size,
			outer_cell_size_factor,
			max_surfel_count,
			gibs_debug_flags,

			enable_gist,
			gist_lock_origin,
			gist_diffuse_ray_period,
			gist_specular_ray_period,
			gist_cell_surfel_ray_count_min,
			gist_cell_surfel_ray_count_max,
			gist_cell_count_per_axis,
			gist_outer_layer_count,

			gist_max_cell_surfel_count,
			gist_cell_surfel_ray_budget_factor,

			gist_cell_size,
			gist_outer_cell_size_factor,
			gist_debug_flags

		);

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, gi_config & cmp, auto& ctx) noexcept
		{
			cmp.max_surfel_count = min(ctx.renderer.gibs_max_surfel_count(), cmp.max_surfel_count);

			if (cmp.enable_ddgi)
			{
				cmp.enable_gibs = false;
				cmp.enable_gist = false;
				ctx.renderer.enable_or_update_ddgi({
					.probe_per_level_axis = cmp.ddgi_probe_per_level_axis,
					.base_probe_spacing	  = cmp.ddgi_base_probe_spacing,
					.level_count		  = cmp.ddgi_level_count,
					.debug_flags		  = cmp.ddgi_debug_flags,
					.lock_origin		  = cmp.ddgi_lock_origin,
				});
			}
			else if (cmp.enable_gibs)
			{
				cmp.enable_ddgi = false;
				cmp.enable_gist = false;
				ctx.renderer.enable_or_update_gibs({
					.max_surfel_count		= cmp.max_surfel_count,
					.debug_flags			= cmp.gibs_debug_flags,
					.lock_origin			= cmp.gibs_lock_origin,
					.cell_count				= cmp.gibs_cell_count,
					.outer_layer_count		= cmp.gibs_outer_layer_count,
					.cell_size				= cmp.gibs_cell_size,
					.outer_cell_size_factor = cmp.outer_cell_size_factor,
				});
			}
			else if (cmp.enable_gist)
			{
				cmp.enable_ddgi = false;
				cmp.enable_gibs = false;
				ctx.renderer.enable_or_update_gist({
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
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, gi_config & cmp, auto& ctx) noexcept
		{
			AGE_ASSERT((cmp.enable_ddgi and cmp.enable_gibs) is_false);
			if (cmp.enable_ddgi)
			{
				ctx.renderer.disable_ddgi();
			}

			if (cmp.enable_gibs)
			{
				ctx.renderer.disable_gibs();
			}

			if (cmp.enable_gist)
			{
				ctx.renderer.disable_gist();
			}
		}

		static void
		write_to(cmp_dispatch_key, const gi_config& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			bool gibs_lock_origin = false;

			// pow of 2
			uint8 gibs_cell_count = 64u;
			// less than 16, pow of 2
			uint8 gibs_outer_layer_count = 16u;

			float gibs_cell_size = 1.f;

			// greater than 1.f
			float outer_cell_size_factor = 1.5f;

			// less than gibs_ray_budget
			uint32 max_surfel_count = age::g::uint32_max;

			age::graphics::e::gibs_debug_flags gibs_debug_flags = age::graphics::e::gibs_debug_flags::none;

			buf.write(cmp.enable_ddgi,
					  cmp.ddgi_lock_origin,
					  cmp.ddgi_probe_per_level_axis,
					  cmp.ddgi_base_probe_spacing,
					  cmp.ddgi_level_count,
					  to_idx(cmp.ddgi_debug_flags),

					  cmp.enable_gibs,
					  cmp.gibs_lock_origin,
					  cmp.gibs_cell_count,
					  cmp.gibs_outer_layer_count,
					  cmp.gibs_cell_size,
					  cmp.outer_cell_size_factor,
					  cmp.max_surfel_count,
					  to_idx(cmp.gibs_debug_flags),

					  cmp.enable_gist,
					  cmp.gist_lock_origin,
					  cmp.gist_diffuse_ray_period,
					  cmp.gist_specular_ray_period,
					  cmp.gist_cell_surfel_ray_count_min,
					  cmp.gist_cell_surfel_ray_count_max,
					  cmp.gist_cell_count_per_axis,
					  cmp.gist_outer_layer_count,
					  cmp.gist_max_cell_surfel_count,
					  cmp.gist_cell_surfel_ray_budget_factor,
					  cmp.gist_cell_size,
					  cmp.gist_outer_cell_size_factor,
					  to_idx(cmp.gist_debug_flags));
			return;
		}

		static void
		read_from(cmp_dispatch_key, gi_config & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != gi_config::age_component_version())
			{
				if (rw_ctx.version <= 2)
				{
					bool render_probe;
					buf.read(cmp.enable_ddgi, render_probe, cmp.ddgi_probe_per_level_axis, cmp.ddgi_base_probe_spacing, cmp.ddgi_level_count);
					cmp.ddgi_debug_flags = age::graphics::e::ddgi_debug_flags::none;
					if (render_probe)
					{
						cmp.ddgi_debug_flags |= age::graphics::e::ddgi_debug_flags::render_probe;
					}
					return;
				}
				else if (rw_ctx.version == 3)
				{
					buf.read(cmp.enable_ddgi, cmp.ddgi_probe_per_level_axis, cmp.ddgi_base_probe_spacing, cmp.ddgi_level_count, cmp.ddgi_debug_flags);
					cmp.ddgi_debug_flags = age::graphics::e::ddgi_debug_flags::none;
					return;
				}
				else if (rw_ctx.version == 4)
				{
					buf.read(cmp.enable_ddgi,
							 cmp.ddgi_lock_origin,
							 cmp.ddgi_probe_per_level_axis,
							 cmp.ddgi_base_probe_spacing,
							 cmp.ddgi_level_count,
							 cmp.ddgi_debug_flags);


					return;
				}
				else if (rw_ctx.version == 5)
				{
					buf.read(cmp.enable_ddgi,
							 cmp.ddgi_lock_origin,
							 cmp.ddgi_probe_per_level_axis,
							 cmp.ddgi_base_probe_spacing,
							 cmp.ddgi_level_count,
							 cmp.ddgi_debug_flags,
							 cmp.enable_gibs,
							 cmp.gibs_lock_origin,
							 cmp.gibs_cell_count,
							 cmp.gibs_outer_layer_count,
							 cmp.gibs_cell_size,
							 cmp.outer_cell_size_factor,
							 cmp.max_surfel_count,
							 cmp.gibs_debug_flags);
					return;
				}
				AGE_ASSERT(false);
				return;
			}

			buf.read(cmp.enable_ddgi,
					 cmp.ddgi_lock_origin,
					 cmp.ddgi_probe_per_level_axis,
					 cmp.ddgi_base_probe_spacing,
					 cmp.ddgi_level_count,
					 cmp.ddgi_debug_flags,
					 cmp.enable_gibs,
					 cmp.gibs_lock_origin,
					 cmp.gibs_cell_count,
					 cmp.gibs_outer_layer_count,
					 cmp.gibs_cell_size,
					 cmp.outer_cell_size_factor,
					 cmp.max_surfel_count,
					 cmp.gibs_debug_flags,

					 cmp.enable_gist,
					 cmp.gist_lock_origin,
					 cmp.gist_diffuse_ray_period,
					 cmp.gist_specular_ray_period,
					 cmp.gist_cell_surfel_ray_count_min,
					 cmp.gist_cell_surfel_ray_count_max,
					 cmp.gist_cell_count_per_axis,
					 cmp.gist_outer_layer_count,
					 cmp.gist_max_cell_surfel_count,
					 cmp.gist_cell_surfel_ray_budget_factor,
					 cmp.gist_cell_size,
					 cmp.gist_outer_cell_size_factor,
					 cmp.gist_debug_flags);
		}
	};

	AGE_COMPONENT(ao_config, "ao", "ambient_occlusion", "vbao")
	{
		AGE_COMPONENT_VERSION(1);

		bool  enabled	   = false;
		uint8 slice_count  = 4;
		uint8 offset_count = 6;
		uint8 _;
		float radius		= 0.5f;
		float max_px_radius = 128.f;
		float intensity		= 1.f;
		float power			= 1.f;
		float thickness		= 0.25f;
		float fade_distance = 50.f;	   // fade_distance > fade_range
		float fade_range	= 60.f;

		age::graphics::e::ao_debug_flags debug_flags;

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, ao_config & cmp, auto& ctx) noexcept
		{
			if (cmp.enabled)
			{
				ctx.renderer.enable_ao({
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
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, ao_config & cmp, auto& ctx) noexcept
		{
			if (cmp.enabled)
			{
				ctx.renderer.disable_ao();
			}
		}
	};

	AGE_COMPONENT(aa_config, "aa")
	{
		AGE_COMPONENT_VERSION(1);

		bool enabled		   = true;
		bool fxaa_on_offscreen = true;

		// 0 (disabled), 2, 4, 8, 16
		uint8 opaque_aa_ray_per_px = 8;

		// 0 (disabled), 2, 4, 8, 16
		uint8 transparent_aa_ray_per_px = 8;

		// max_aa_ray_budget = screen_px_count * aa_px_cap * (opaque_aa_rpp + transparent_aa_rpp)
		// (0,1]
		float aa_px_cap = 0.05f;

		// max_aa_px_count = screen_px_count * aa_px_cap * aa_px_headroom;
		// (1, 1/aa_px_cap]
		float aa_px_headroom = 4.f;

		// px
		float edge_plane_dist_tolerance_px = 3.f;

		// abs(cos), (0,1]
		float edge_normal_threshold = 0.9f;

		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, aa_config & cmp, auto& ctx) noexcept
		{
			if (cmp.enabled)
			{
				ctx.renderer.disable_aa();
			}
		}
	};

	AGE_COMPONENT(debug_view_config, "debug_view")
	{
		struct debug_view_slot_config
		{
			age::graphics::e::hrp_debug_view_system_kind	   system_kind							  = age::graphics::e::hrp_debug_view_system_kind::none;
			uint32											   system_debug_view_kind				  = 0u;
			uint32											   system_debug_view_overlay_flags		  = 0u;
			uint32											   system_debug_view_cursor_overlay_flags = 0u;
			uint32											   system_popup_view_kind				  = 0u;
			age::graphics::e::hrp_debug_view_slot_option_flags option_flags							  = age::graphics::e::hrp_debug_view_slot_option_flags::none;
			age::graphics::e::hrp_debug_view_color_map_kind	   color_map_kind						  = age::graphics::e::hrp_debug_view_color_map_kind::none;
			float2											   size_uv								  = float2{ 0.125f };
			float2											   offset_uv							  = float2{ 0.f };	   // default : 0, 0
			float2											   pos_uv								  = float2{ -1.f };	   // default : -1, -1, disabled
			float3											   scalar_range_min						  = float3::zero();
			float3											   scalar_range_max						  = float3::one();
			float											   alpha								  = 1.f;
			float											   popup_zoom							  = 4.f;
			float3											   background_color						  = float3::zero();
			uint32											   border_thickness						  = 1u;

			uint32_4 payload[4] = { uint32_4::zero() };
		};

		AGE_COMPONENT_VERSION(1);

		bool	enabled		 = false;
		bool	pick_enabled = false;
		uint8_2 _;
		uint32	slot_count = 1u;										   // min : 1, max : 16

		debug_view_slot_config					fullscreen_slot_config;	   // ignores size_uv
		age::array<debug_view_slot_config, 15u> slot_config_arr;

		float2 popup_view_size_uv	  = float2{ 0.125f };
		uint32 popup_border_thickness = 1u;

		int32_2 cursor_px = int32_2::zero();

		float3 nan_color	   = { 4.f, 0.f, 4.f };				// magenta
		float3 pos_inf_color   = { 4.f, 4.f, 4.f };				// white
		float3 neg_inf_color   = { 0.f, 4.f, 4.f };				// cyan
		float3 zero_color	   = { 0.015f, 0.015f, 0.015f };	// dark gray
		float3 below_min_color = { 0.f, 0.f, 4.f };				// blue
		float3 above_max_color = { 4.f, 0.f, 0.f };				// red

		uint32_4 payload[4];

		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, debug_view_config & cmp, auto& ctx) noexcept
		{
			if (cmp.enabled and ctx.renderer.debug_view_enabled())
			{
				ctx.renderer.disable_debug_view();
			}
		}
	};

#undef AGE_COMPONENT
#undef AGE_CUSTOM_BYTE_SIZE
}	 // namespace age::ecs
