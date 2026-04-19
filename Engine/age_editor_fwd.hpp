#pragma once
#include "age.hpp"

namespace age::editor::e
{
	AGE_DEFINE_ENUM(mode_kind, uint8, edit, play);
}

namespace age::editor
{
	struct camera_data
	{
		float move_speed	 = 2.f;
		float sprint_mult	 = 4.f;
		float sensitivity	 = 0.17f;
		float zoom_speed	 = 2.f;
		float zoom_distance	 = 4.f;
		float pan_speed		 = 0.6f;
		float move_smoothing = 15.f / 2.f;
		float look_smoothing = 25.f / 2.f;
		float zoom_smoothing = 12.f / 2.f;

		float2 move;
		float2 look;
		float  zoom;
		bool   sprint;

		float euler_x = 0.f;
		float euler_y = 0.f;

		float2 smoothed_move = float2::zero();
		float2 smoothed_look = float2::zero();
		float  smoothed_zoom = 0.f;
		float2 smoothed_pan	 = float2::zero();

		float3 pos			= float3::zero();
		float3 euler_deg	= float3::zero();
		float  aspect_ratio = 16.f / 9.f;
	};

	struct entity_editor_data
	{
		uint64										  id;
		std::array<char, config::max_entity_name_len> name;
	};

	struct archetype_editor_data
	{
		std::array<char, config::max_archetype_name_len> name;	  // editor_only name
		uint64											 archetype;
		age::vector<entity_editor_data>					 entity_data_vec;
	};

	struct component_editor_data
	{
		age::vector<std::array<char, config::max_component_name_len>> names;
		uint32														  version;
		uint32														  byte_size;
	};

	struct storage_editor_data
	{
		age::vector<std::array<char, config::max_entity_storage_name_len>> names;
		uint32															   code_idx;
		uint64															   entity_count;
		age::vector<component_editor_data>								   component_data_vec;
		age::vector<archetype_editor_data>								   archetype_data_vec;

		age::unordered_map<uint64, std::pair<uint32, uint64>> id_to_editor_location_map;	// editor_only map
	};

	struct scene_editor_data
	{
		age::vector<std::array<char, config::max_scene_name_len>> names;
		uint32													  code_idx;
		bool													  loaded = false;
		uint8_3													  _;
		age::vector<storage_editor_data>						  storage_data_vec;

		std::filesystem::path dir_path;
	};

	struct game_editor_data
	{
		age::vector<std::array<char, config::max_game_name_len>> names;
		uint32													 default_active_scene_idx;
		uint32													 current_active_scene_idx;
		age::vector<scene_editor_data>							 scene_data_vec;

		std::filesystem::path dir_path;
	};

	template <typename t_renderer>
	struct rw_context
	{
		uint32		version;
		t_renderer& renderer;
	};

	FORCE_INLINE constexpr auto
	get_rw_context(uint32 version, auto&& renderer) noexcept
	{
		return rw_context<BARE_OF(renderer)>{ .version = version, .renderer = FWD(renderer) };
	}
}	 // namespace age::editor

namespace age::editor::g
{
	inline auto select_vec			 = age::vector<age::vector<uint64>>{};
	inline auto ui_new_entity_buffer = age::vector<uint64>{};
	inline auto command_buf			 = ecs::command_buffer{};

	inline auto current_game = game_editor_data{};

	inline auto cam = camera_data{};

	inline auto current_mode = e::mode_kind::edit;
}	 // namespace age::editor::g