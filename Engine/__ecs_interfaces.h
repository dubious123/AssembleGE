#pragma once

template <typename t_game>
struct interface_game
{
	t_game game;

	interface_game(t_game&& game) : game(game) { }

	// t_game game;

	// interface_game(t_game& game) : game(game) { }

	// interface_game(t_game&& game) : game(std::move(game)) { }

	inline void init()
	{
		game.init();
	}

	inline auto get_current_scene_idx()
	{
		return game.current_scene_idx;
	}

	inline auto get_running()
	{
		return game.running;
	}

	inline void deinit()
	{
		game.deinit();
	}

	template <typename scene_t>
	inline decltype(auto) get_scene()
	{
		return game.get_scene<scene_t>();
	}
};

template <typename t>
struct interface_init
{
	t& obj;

	interface_init(t& obj) : obj(obj) { }

	inline void init()
	{
		obj.init();
	}
};

template <typename t_scene>
struct interface_scene
{
	t_scene& scene;

	interface_scene(t_scene& scene) : scene(scene) {};

	void inline init()
	{
		scene.init();
	}
};

template <typename t_world>
struct interface_world
{
	t_world& world;

	interface_world(t_world& world) : world(world) { }

	void inline init()
	{
	}
};

template <typename t>
struct interface_invalid
{
	t& val;

	interface_invalid(t& val) : val(val) { }

	decltype(auto) inline invalid()
	{
		return val.invalid();
	}
};

template <typename t_entity_group>
struct interface_entity_group
{
	t_entity_group& entity_group;

	interface_entity_group(t_entity_group& entity_group) : entity_group(entity_group) { }
};