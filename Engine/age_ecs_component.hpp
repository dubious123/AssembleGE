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
	};

	AGE_COMPONENT(rotation, "rot", "quat", "quaternion") : public float4
	{
		AGE_COMPONENT_VERSION(1);

		using float4::float4;
		constexpr rotation() noexcept : float4{ 0, 0, 0, 1.f } { }
	};

	AGE_COMPONENT(scale, "sc", "scale_3d") : public float3
	{
		AGE_COMPONENT_VERSION(1);

		using float3::float3;
		constexpr scale() noexcept : float3{ 1.f, 1.f, 1.f } { }
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

	AGE_COMPONENT(directional_light, "dir_light")
	{
		AGE_COMPONENT_VERSION(1);

		uint16 render_id   = age::get_invalid_id<uint16>();
		bool   cast_shadow = true;

		float3 direction = age::normalize(float3{ -0.3f, -1.0f, 0.5f });
		float  intensity = 0.80f;
		float3 color	 = float3{ 1.0f, 0.9f, 0.9f };

		uint8 _;

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

		uint32 render_id = 0;

		static consteval uint32
		byte_size() noexcept
		{
			return config::max_asset_path_len;
		}

		static void
		write_to(cmp_dispatch_key, const mesh& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			char mesh_name[config::max_asset_path_len] = { "mesh_cube" };
			buf.write(mesh_name);

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

			char mesh_name[config::max_asset_path_len];
			buf.read(mesh_name);
		}
	};

	AGE_COMPONENT(material, "mat", "pbr_mat", "material_3d")
	{
		AGE_COMPONENT_VERSION(1);

		uint32 render_id = 0;
		bool   is_opaque = true;

		AGE_CUSTOM_BYTE_SIZE(is_opaque)

		static void
		write_to(cmp_dispatch_key, const material& cmp, byte_buf& buf, auto&& rw_ctx) noexcept
		{
			buf.write(cmp.is_opaque);
			return;
		}

		static void
		read_from(cmp_dispatch_key, material & cmp, auto& buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != material::age_component_version())
			{
				// handle migrate
				AGE_ASSERT(false);
			}

			buf.read(cmp.is_opaque);
		}
	};

#undef AGE_COMPONENT
#undef AGE_CUSTOM_BYTE_SIZE
}	 // namespace age::ecs
