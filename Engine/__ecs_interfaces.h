#pragma once

template <typename t_ref_game>
struct interface_game
{
	t_ref_game ref_game;

	interface_game(t_ref_game ref_game) : ref_game(ref_game) { }

	void inline init()
	{
		ref_game.init();
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