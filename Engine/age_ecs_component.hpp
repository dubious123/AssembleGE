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

	AGE_COMPONENT(ddgi_config, "ddgi")
	{
		AGE_COMPONENT_VERSION(1);

		bool	 enabled	  = false;
		bool	 render_probe = false;
		uint8_2	 _;
		uint32_3 probe_per_level_axis = uint32_3{ 32, 16, 32 };
		float3	 base_probe_spacing	  = float3{ 1.f, 2.f, 1.f };
		uint32	 level_count		  = 6;

		AGE_CUSTOM_BYTE_SIZE(enabled, render_probe, probe_per_level_axis, base_probe_spacing, level_count);

		FORCE_INLINE static void
		on_create(cmp_dispatch_key, ddgi_config & cmp, auto& ctx) noexcept
		{
			if (cmp.enabled)
			{
				ctx.renderer.enable_ddgi({
					.probe_per_level_axis = cmp.probe_per_level_axis,
					.base_probe_spacing	  = cmp.base_probe_spacing,
					.level_count		  = cmp.level_count,
				});
			}
		}


		FORCE_INLINE static void
		on_destroy(cmp_dispatch_key, ddgi_config & cmp, auto& ctx) noexcept
		{
			if (cmp.enabled)
			{
				ctx.renderer.disable_ddgi();
			}
		}

		static void
		write_to(cmp_dispatch_key, const ddgi_config& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			buf.write(cmp.enabled, cmp.render_probe, cmp.probe_per_level_axis, cmp.base_probe_spacing, cmp.level_count);
			return;
		}

		static void
		read_from(cmp_dispatch_key, ddgi_config & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != ddgi_config::age_component_version())
			{
				AGE_ASSERT(false);
				return;
			}

			buf.read(cmp.enabled, cmp.render_probe, cmp.probe_per_level_axis, cmp.base_probe_spacing, cmp.level_count);
		}
	};

#undef AGE_COMPONENT
#undef AGE_CUSTOM_BYTE_SIZE
}	 // namespace age::ecs
