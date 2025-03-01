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