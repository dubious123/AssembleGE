#pragma once

namespace std
{
	template <std::size_t n_size>
	struct formatter<std::array<char, n_size>> : formatter<std::string_view>
	{
		auto
		format(const std::array<char, n_size>& arr, auto& ctx) const
		{
			return formatter<std::string_view>::format(
				std::string_view{ arr.data() }, ctx);
		}
	};
}	 // namespace std

namespace age::data_structure
{

}

namespace age::ecs
{
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

	template <typename t>
	consteval bool
	is_ecs_component()
	{
		return false;
	}

	class entity_storage_tag { };

	class entity_block_tag { };

	template <typename t_cmp>
	concept cx_component = requires {
		requires std::is_trivially_copyable_v<std::decay_t<t_cmp>>;
		requires std::is_trivially_destructible_v<std::decay_t<t_cmp>>;
		requires std::is_standard_layout_v<std::decay_t<t_cmp>>;
	} and ecs::is_ecs_component<std::decay_t<t_cmp>>();

	template <typename t>
	concept cx_entity_storage = requires { typename std::remove_cvref_t<t>::ecs_tag; }
							and std::is_same_v<typename std::remove_cvref_t<t>::ecs_tag, entity_storage_tag>;

	template <typename t>
	concept cx_entity_block = requires { typename std::remove_cvref_t<t>::ecs_tag; }
						  and std::is_same_v<typename std::remove_cvref_t<t>::ecs_tag, entity_block_tag>;
}	 // namespace age::ecs

namespace age::graphics
{
	AGE_DEFINE_ENUM(color_space, uint8, srgb, hdr);
}	 // namespace age::graphics

namespace age::graphics::e
{
	AGE_DEFINE_ENUM(camera_kind, uint8, perspective, orthographic);

	AGE_DEFINE_ENUM_WITH_VALUE(light_kind, uint16,
							   (directional, 0),
							   (point, 1),
							   (spot, 2),
							   (area, 3),
							   (volumn, 4));
}	 // namespace age::graphics::e

namespace age::graphics
{
	using t_resource_id = uint32;

	struct resource_handle
	{
		t_resource_id id = age::get_invalid_id<t_resource_id>();

		FORCE_INLINE auto*
		operator->() noexcept;

		FORCE_INLINE c_auto*
		operator->() const noexcept;
	};
}	 // namespace age::graphics

namespace age::asset
{
	struct mesh_editable;


}	 // namespace age::asset