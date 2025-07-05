#pragma once

template <typename t_game>
struct interface_game
{
	t_game& game;

	// t_game game;

	// interface_game(t_game& game) : game(game) { }

	// interface_game(t_game&& game) : game(std::move(game)) { }

	inline void
	init()
	{
		game.init();
	}

	inline auto
	get_current_scene_idx()
	{
		return game.current_scene_idx;
	}

	inline auto
	get_running()
	{
		return game.running;
	}

	inline void
	deinit()
	{
		game.deinit();
	}

	template <typename scene_t>
	inline decltype(auto)
	get_scene()
	{
		return game.get_scene<scene_t>();
	}
};

template <typename t>
struct interface_init
{
	t& obj;

	inline void
	init()
	{
		obj.init();
	}
};

template <typename t_scene>
struct interface_scene
{
	t_scene& scene;

	inline void
	init()
	{
		scene.init();
	}

	template <typename t_world>
	decltype(auto)
	get_world()
	{
		return scene.get_world<t_world>();
	}
};

template <typename t_world>
struct i_world
{
	t_world& world;

	void inline init()
	{
	}
};

template <typename t_entity_group>
struct i_entity_group
{
	// static constexpr bool b = [] {
	//	meta::print_type<t_entity_group>();
	//	return true;
	// }();
	using t_local_entity_idx = std::remove_reference_t<t_entity_group>::t_local_entity_idx;
	// using t_local_entity_idx = t_entity_group::t_local_entity_idx;

	t_entity_group& entity_group;

	FORCE_INLINE decltype(auto)
	entity_group_idx()
	{
		return entity_group.entity_group_idx();
	}

	FORCE_INLINE decltype(auto)
	entity_count()
	{
		return entity_group.entity_count();
	}

	FORCE_INLINE decltype(auto)
	capacity()
	{
		return entity_group.capacity();
	}

	FORCE_INLINE decltype(auto)
	component_count()
	{
		return entity_group.component_count();
	}

	FORCE_INLINE decltype(auto)
	local_archetype()
	{
		return entity_group.local_archetype();
	}

	FORCE_INLINE decltype(auto)
	component_size_arr_base()
	{
		return entity_group.component_size_arr_base();
	}

	FORCE_INLINE decltype(auto)
	component_offset_arr_base()
	{
		return entity_group.component_offset_arr_base();
	}

	FORCE_INLINE decltype(auto)
	entity_id_arr_base()
	{
		return entity_group.entity_id_arr_base();
	}

	FORCE_INLINE decltype(auto)
	ent_id(auto ent_idx)
	{
		return entity_group.ent_id(ent_idx);
	}
};