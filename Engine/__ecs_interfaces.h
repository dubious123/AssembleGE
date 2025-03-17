#pragma once

template <typename t_game>
struct interface_game
{
	t_game* p_game;

	interface_game(t_game* p_game) : p_game(p_game) { }

	inline void init()
	{
		p_game->init();
	}

	template <typename scene_t>
	inline scene_t* get_scene()
	{
		return p_game->get_scene<scene_t>();
	}
};

template <typename t>
struct interface_init
{
	t* p;

	interface_init(t* p) : p(p) { }

	inline void init()
	{
		p->init();
	}
};

template <typename t_scene>
struct interface_scene
{
	t_scene* p_scene;

	interface_scene(t_scene* p_scene) : p_scene(p_scene) { }

	void inline init()
	{
	}
};

template <typename t_world>
struct interface_world
{
	t_world* p_world;

	interface_world(t_world* p_world) : p_world(p_world) { }

	void inline init()
	{
	}
};