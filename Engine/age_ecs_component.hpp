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

#define AGE_COMPONENT(name)                   \
	struct name;                              \
	template <>                               \
	consteval auto get_component_name<name>() \
	{ return #name; }                         \
	struct name

	AGE_COMPONENT(transform_3d)
	{
		float3 position;
		float4 quaternion;
		float3 scale;
	};

	AGE_COMPONENT(position) : public float3
	{
		using float3::float3;
		constexpr position() noexcept : float3(0.f, 0.f, 0.f) { }
	};

	AGE_COMPONENT(rotation) : public float4
	{
		using float4::float4;
		constexpr rotation() noexcept : float4{ 0, 0, 0, 1.f } { }
	};

	AGE_COMPONENT(scale) : public float3
	{
		using float3::float3;
		constexpr scale() noexcept : float3{ 1.f, 1.f, 1.f } { }
	};

	AGE_COMPONENT(render_object)
	{
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
	};

	AGE_COMPONENT(camera)
	{
		using renderable_tag = age::ecs::renderable_tag;

		uint32 render_id = age::get_invalid_id<uint32>();

		graphics::e::camera_kind kind;
		uint8_3					 _;

		float3 euler_deg;

		float near_z;
		float far_z;

		float fov_y;
		float aspect_ratio;

		float view_width;
		float view_height;

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
	};

	AGE_COMPONENT(directional_light)
	{
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
	};

	AGE_COMPONENT(point_light)
	{
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
	};

	AGE_COMPONENT(spot_light)
	{
		using renderable_tag = age::ecs::renderable_tag;

		uint32 render_id = age::get_invalid_id<uint32>();

		float  range	 = 1.f;					 // 4
		float3 direction = float3::one();		 // 12
		float  intensity = 1.f;					 // 4
		float3 color	 = float3{ 1, 0, 0 };	 // 12
		float  cos_inner = 0.1f;				 // 4  (falloff begin, cosine)
		float  cos_outer = 0.5f;				 // 4  (cosine)

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
	};

	AGE_COMPONENT(mesh)
	{
		uint32 render_id = 0;
	};

	AGE_COMPONENT(material)
	{
		uint32 render_id = 0;	 // object_id
		bool   is_opaque = true;
	};

#undef AGE_COMPONENT
}	 // namespace age::ecs