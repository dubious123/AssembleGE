#pragma once

template <typename t_game>
struct interface_game
{
	t_game& game;

	interface_game(t_game& game) : game(game) { }

	inline void init()
	{
		game.init();
	}

	template <typename scene_t>
	inline scene_t& get_scene()
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

	interface_scene(t_scene& scene) : scene(scene) { }

	void inline init()
	{
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