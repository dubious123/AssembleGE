#pragma once

namespace age::ecs
{
	class renderable_tag { };

	template <typename t>
	concept cx_renderable = requires(t cmp) {
		typename std::remove_cvref_t<t>::renderable_tag;
		cmp.render_id;
	};

	template <typename t>
	class is_renderable : public std::bool_constant<cx_renderable<t>> { };

	// 1. transform_3d
	// 2. light: type, id, intensity, ...
	// 3. mesh: mesh_id
	// 4. material : 1.opaque 2.transform, render_object_id (draw is true)
	// 5. camera : 1. perspective, orthographics, camera_id

	template <typename t>
	consteval auto
	get_component_name()
	{
		return "unnamed component";
	}

	template <typename t, std::size_t i>
	consteval auto
	get_component_name_at()
	{
		return "unnamed component";
	}

#define AGE_COMPONENT(name, ...)                                                                                   \
	struct name;                                                                                                   \
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

#define AGE_COMPONENT_CUSTOM_RW(boolean) static constexpr bool age_custom_read_write = boolean;

#define AGE_COMPONENT_CUSTOM_RW_WITH_CTX(boolean) static constexpr bool age_custom_read_write_with_ctx = boolean;

#define AGE_CUSTOM_BYTE_SIZE(...) \
	static consteval uint32 byte_size() { return static_cast<uint32>(FOR_EACH_SEP(sizeof, AGE_PP_PLUS_I, __VA_ARGS__)); };

	AGE_COMPONENT(transform_3d, "transform")
	{
		AGE_COMPONENT_VERSION(1);
		AGE_COMPONENT_CUSTOM_RW(false);
		float3 position;
		float4 quaternion;
		float3 scale;
	};

	AGE_COMPONENT(position, "pos", "position_3d") : public float3
	{
		AGE_COMPONENT_VERSION(1);
		AGE_COMPONENT_CUSTOM_RW(false);

		using float3::float3;
		constexpr position() noexcept : float3(0.f, 0.f, 0.f) { }
	};

	AGE_COMPONENT(rotation, "rot", "quat", "quaternion") : public float4
	{
		AGE_COMPONENT_VERSION(1);
		AGE_COMPONENT_CUSTOM_RW(false);

		using float4::float4;
		constexpr rotation() noexcept : float4{ 0, 0, 0, 1.f } { }
	};

	AGE_COMPONENT(scale, "sc", "scale_3d") : public float3
	{
		AGE_COMPONENT_VERSION(1);
		AGE_COMPONENT_CUSTOM_RW(false);

		using float3::float3;
		constexpr scale() noexcept : float3{ 1.f, 1.f, 1.f } { }
	};

	AGE_COMPONENT(render_object, "render_obj", "renderable_instance")
	{
		AGE_COMPONENT_VERSION(1);
		AGE_COMPONENT_CUSTOM_RW_WITH_CTX(true);

		using renderable_tag = age::ecs::renderable_tag;

		uint32 render_id = age::get_invalid_id<uint32>();

		FORCE_INLINE static void
		add_renderable(auto& renderer, auto&& self) noexcept
			requires(std::is_same_v<render_object, BARE_OF(self)>)
		{
			self.render_id = renderer.add_object(float3::zero(), age::math::g::quaternion_identity, float3::one());
		}

		FORCE_INLINE static void
		remove_renderable(auto& renderer, auto&& self) noexcept
			requires(std::is_same_v<render_object, BARE_OF(self)>)
		{
			renderer.remove_object(self.render_id);
		}

		static consteval uint32
		byte_size() noexcept
		{
			return 0ul;
		}

		void
		write_to(byte_buf & buf, auto&& rw_ctx) const noexcept
		{
			return;
		}

		static render_object
		read_from(byte_buf & buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != render_object::age_component_version)
			{
				// handle migrate
				AGE_ASSERT(false);
			}

			return render_object{
				.render_id = rw_ctx.renderer.add_object(float3::zero(), age::math::g::quaternion_identity, float3::one())
			};
		}
	};

	AGE_COMPONENT(camera, "cam", "camera_3d")
	{
		AGE_COMPONENT_VERSION(1);
		AGE_COMPONENT_CUSTOM_RW_WITH_CTX(true);

		using renderable_tag = age::ecs::renderable_tag;

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

		FORCE_INLINE static void
		add_renderable(auto& renderer, auto&& self) noexcept
			requires(std::is_same_v<camera, BARE_OF(self)>)
		{
			if (self.kind == graphics::e::camera_kind::perspective)
			{
				self.render_id = renderer.add_camera({ .kind	   = self.kind,
													   .quaternion = age::math::euler_deg_to_quat(self.euler_deg),
													   .near_z	   = self.near_z,
													   .far_z	   = self.far_z,
													   .perspective{
														   .fov_y		 = self.fov_y,
														   .aspect_ratio = self.aspect_ratio } });
			}
			else
			{
				self.render_id = renderer.add_camera({ .kind	   = self.kind,
													   .quaternion = age::math::euler_deg_to_quat(self.euler_deg),
													   .near_z	   = self.near_z,
													   .far_z	   = self.far_z,
													   .perspective{
														   .fov_y		 = self.view_width,
														   .aspect_ratio = self.view_height } });
			}
		}

		FORCE_INLINE static void
		remove_renderable(auto& renderer, auto&& self) noexcept
			requires(std::is_same_v<camera, BARE_OF(self)>)
		{
			renderer.remove_camera(self.render_id);
		}

		AGE_CUSTOM_BYTE_SIZE(kind, euler_deg, near_z, far_z, fov_y, aspect_ratio, view_width, view_height)

		void
		write_to(byte_buf & buf, auto&& rw_ctx) const noexcept
		{
			buf.write(kind, euler_deg, near_z, far_z, fov_y, aspect_ratio, view_width, view_height);
			return;
		}

		static camera
		read_from(byte_buf & buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != camera::age_component_version)
			{
				// handle migrate
				AGE_ASSERT(false);
			}

			auto res = camera{};
			buf.read(res.kind, res.euler_deg, res.near_z, res.far_z, res.fov_y, res.aspect_ratio, res.view_width, res.view_height);

			add_renderable(rw_ctx.renderer, res);

			return res;
		}
	};

	AGE_COMPONENT(directional_light, "dir_light")
	{
		AGE_COMPONENT_VERSION(1);
		AGE_COMPONENT_CUSTOM_RW_WITH_CTX(true);

		using renderable_tag = age::ecs::renderable_tag;

		uint16 render_id   = age::get_invalid_id<uint16>();
		bool   cast_shadow = true;

		float3 direction = age::normalize(float3{ -0.3f, -1.0f, 0.5f });
		float  intensity = 0.80f;
		float3 color	 = float3{ 1.0f, 0.9f, 0.9f };

		uint8 _;

		FORCE_INLINE static void
		add_renderable(auto& renderer, auto&& self) noexcept
			requires(std::is_same_v<directional_light, BARE_OF(self)>)
		{
			self.render_id = renderer.add_directional_light({
																.direction = self.direction,
																.intensity = self.intensity,
																.color	   = self.color,
															},
															self.cast_shadow);
		}

		FORCE_INLINE static void
		remove_renderable(auto& renderer, auto&& self) noexcept
			requires(std::is_same_v<directional_light, BARE_OF(self)>)
		{
			renderer.remove_directional_light(self.render_id);
		}

		AGE_CUSTOM_BYTE_SIZE(cast_shadow, direction, intensity, color)

		void
		write_to(byte_buf & buf, auto&& rw_ctx) const noexcept
		{
			buf.write(cast_shadow, direction, intensity, color);
		}

		static directional_light
		read_from(byte_buf & buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != directional_light::age_component_version)
			{
				AGE_ASSERT(false);
			}

			auto res = directional_light{};
			buf.read(res.cast_shadow, res.direction, res.intensity, res.color);

			add_renderable(rw_ctx.renderer, res);

			return res;
		}
	};

	AGE_COMPONENT(point_light, "pt_light")
	{
		AGE_COMPONENT_VERSION(1);
		AGE_COMPONENT_CUSTOM_RW_WITH_CTX(true);

		using renderable_tag = age::ecs::renderable_tag;

		uint32 render_id = age::get_invalid_id<uint32>();

		float  range	 = 1.f;
		float3 color	 = float3{ 0, 1, 0 };
		float  intensity = 1.f;

		bool	cast_shadow = false;
		uint8_3 _;

		FORCE_INLINE static void
		add_renderable(auto& renderer, auto&& self) noexcept
			requires(std::is_same_v<point_light, BARE_OF(self)>)
		{
			self.render_id = renderer.add_point_light({
														  .range	 = self.range,
														  .color	 = self.color,
														  .intensity = self.intensity,
													  },
													  self.cast_shadow);
		}

		FORCE_INLINE static void
		remove_renderable(auto& renderer, auto&& self) noexcept
			requires(std::is_same_v<point_light, BARE_OF(self)>)
		{
			renderer.remove_point_light(self.render_id);
		}

		AGE_CUSTOM_BYTE_SIZE(range, color, intensity, cast_shadow)

		void
		write_to(byte_buf & buf, auto&& rw_ctx) const noexcept
		{
			buf.write(range, color, intensity, cast_shadow);
		}

		static point_light
		read_from(byte_buf & buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != point_light::age_component_version)
			{
				AGE_ASSERT(false);
			}

			auto res = point_light{};
			buf.read(res.range, res.color, res.intensity, res.cast_shadow);

			add_renderable(rw_ctx.renderer, res);

			return res;
		}
	};

	AGE_COMPONENT(spot_light, "sp_light")
	{
		AGE_COMPONENT_VERSION(1);
		AGE_COMPONENT_CUSTOM_RW_WITH_CTX(true);

		using renderable_tag = age::ecs::renderable_tag;

		uint32 render_id = age::get_invalid_id<uint32>();

		float  range	 = 1.f;
		float3 direction = float3::one();
		float  intensity = 1.f;
		float3 color	 = float3{ 1, 0, 0 };
		float  cos_inner = 0.1f;
		float  cos_outer = 0.5f;

		bool	cast_shadow = false;
		uint8_3 _;

		FORCE_INLINE static void
		add_renderable(auto& renderer, auto&& self) noexcept
			requires(std::is_same_v<spot_light, BARE_OF(self)>)
		{
			self.render_id = renderer.add_spot_light({
														 .range		= self.range,
														 .direction = self.direction,
														 .intensity = self.intensity,
														 .color		= self.color,
														 .cos_inner = self.cos_inner,
														 .cos_outer = self.cos_outer,
													 },
													 self.cast_shadow);
		}

		FORCE_INLINE static void
		remove_renderable(auto& renderer, auto&& self) noexcept
			requires(std::is_same_v<spot_light, BARE_OF(self)>)
		{
			renderer.remove_spot_light(self.render_id);
		}

		AGE_CUSTOM_BYTE_SIZE(range, direction, intensity, color, cos_inner, cos_outer, cast_shadow)

		void
		write_to(byte_buf & buf, auto&& rw_ctx) const noexcept
		{
			buf.write(range, direction, intensity, color, cos_inner, cos_outer, cast_shadow);
		}

		static spot_light
		read_from(byte_buf & buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != spot_light::age_component_version)
			{
				AGE_ASSERT(false);
			}

			auto res = spot_light{};
			buf.read(res.range, res.direction, res.intensity, res.color, res.cos_inner, res.cos_outer, res.cast_shadow);

			add_renderable(rw_ctx.renderer, res);

			return res;
		}
	};

	AGE_COMPONENT(mesh, "msh", "meshlet mesh")
	{
		AGE_COMPONENT_VERSION(1);
		AGE_COMPONENT_CUSTOM_RW_WITH_CTX(true);

		uint32 render_id = 0;

		static consteval uint32
		byte_size() noexcept
		{
			return 0ul;
		}

		void
		write_to(byte_buf & buf, auto&& rw_ctx) const noexcept
		{
			// todo, asset binding

			// auto info = rw_ctx.renderer.get_mesh_asset_info(render_id);
			// auto mesh_name = asset::get_header(info.h_asset).name;

			char mesh_name[config::max_asset_path_len] = { "mesh_cube" };
			buf.write(mesh_name);
		}

		static mesh
		read_from(byte_buf & buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != mesh::age_component_version)
			{
				AGE_ASSERT(false);
			}

			// todo, asset binding
			char mesh_name[config::max_asset_path_len];
			buf.read(mesh_name);

			// auto h_mesh	   = asset::mesh::load(mesh_name);
			// auto render_id = rw_ctx.renderer.upload_mesh(h_mesh);

			return mesh{ .render_id = 0 };
		}
	};

	AGE_COMPONENT(material, "mat", "pbr_mat", "material_3d")
	{
		AGE_COMPONENT_VERSION(1);
		AGE_COMPONENT_CUSTOM_RW_WITH_CTX(true);

		uint32 render_id = 0;
		bool   is_opaque = true;

		AGE_CUSTOM_BYTE_SIZE(is_opaque)

		void
		write_to(byte_buf & buf, auto&& rw_ctx) const noexcept
		{
			// todo
			buf.write(is_opaque);
		}

		static material
		read_from(byte_buf & buf, auto&& rw_ctx) noexcept
		{
			if (rw_ctx.version != material::age_component_version)
			{
				AGE_ASSERT(false);
			}

			auto res = material{};
			buf.read(res.is_opaque);
			return res;
		}
	};

#undef AGE_COMPONENT
#undef AGE_CUSTOM_BYTE_SIZE
}	 // namespace age::ecs

namespace age::ecs
{
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
		if constexpr (byte_buffer_cx::custom<t_component> or byte_buffer_cx::custom_with_ctx<t_component>)
		{
			return t_component::byte_size();
		}
		else
		{
			return static_cast<uint32>(sizeof(t_component));
		}
	}

	template <typename t_component>
	void
	serialize_component(age::byte_buf& buf, const void* p_cmp, auto&& rw_ctx) noexcept
	{
		c_auto& cmp = *std::launder(static_cast<std::add_const_t<t_component>*>(p_cmp));

		if constexpr (byte_buffer_cx::custom_with_ctx<t_component>)
		{
			buf.write_with_ctx(FWD(rw_ctx), cmp);
		}
		else
		{
			buf.write(cmp);
		}
	}

	template <typename t_component>
	t_component
	deserialize_component(age::byte_buf& buf, auto&& rw_ctx) noexcept
	{
		if constexpr (byte_buffer_cx::custom_with_ctx<t_component>)
		{
			return buf.read_with_ctx<t_component>(FWD(rw_ctx));
		}
		else
		{
			return buf.read<t_component>();
		}
	}
}	 // namespace age::ecs