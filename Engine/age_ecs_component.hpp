#pragma once

namespace age::ecs
{
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

	AGE_COMPONENT(position) : public float3{};

	AGE_COMPONENT(rotation) : public float4{};

	AGE_COMPONENT(scale) : public float3{};

	AGE_COMPONENT(render_object)
	{
		uint32 render_id;
	};

	AGE_COMPONENT(camera)
	{
		uint32 render_id;

		graphics::e::camera_kind kind;
		uint8_3					 _;

		float3 euler_deg;

		float near_z;
		float far_z;

		float fov_y;
		float aspect_ratio;

		float view_width;
		float view_height;
	};

	AGE_COMPONENT(directional_light)
	{
		uint16 render_id;
		bool   cast_shadow = false;

		float3 direction;	 // 12
		float  intensity;	 // 4
		float3 color;		 // 12

		uint8 _;
	};

	AGE_COMPONENT(point_light)
	{
		uint32 render_id;

		float  range	 = 1.f;					 // 4
		float3 color	 = float3{ 0, 1, 0 };	 // 12
		float  intensity = 1.f;					 // 4

		bool	cast_shadow = false;
		uint8_3 _;
	};

	AGE_COMPONENT(spot_light)
	{
		uint32 render_id;

		float  range	 = 1.f;					 // 4
		float3 direction = float3::one();		 // 12
		float  intensity = 1.f;					 // 4
		float3 color	 = float3{ 1, 0, 0 };	 // 12
		float  cos_inner = 0.1f;				 // 4  (falloff begin, cosine)
		float  cos_outer = 0.5f;				 // 4  (cosine)

		bool	cast_shadow = false;
		uint8_3 _;
	};

	AGE_COMPONENT(mesh)
	{
		uint32 render_id;
	};

	AGE_COMPONENT(material)
	{
		uint32 render_id;	 // object_id
		bool   is_opaque;
	};

#undef AGE_COMPONENT
}	 // namespace age::ecs