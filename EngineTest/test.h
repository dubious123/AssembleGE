#pragma once
#include "../Engine/__reflection.h"
#include "../Engine/__engine.h"
#include "../Engine/__data_structures.h"
#include "../Engine/__meta.h"
#include "../Engine/__ecs_system.h"
COMPONENT_BEGIN(transform)
__SERIALIZE_FIELD(float3, position, 0, 0, 0)
__SERIALIZE_FIELD(float3, scale, 1, 1, 1)
__SERIALIZE_FIELD(float4, rotation, 0, 0, 0, 1)
COMPNENT_END()

COMPONENT_BEGIN(bullet)
__SERIALIZE_FIELD(float3, vel, 1, 0, 1)
__SERIALIZE_FIELD(float3, ac, 1, 1, 1)
COMPNENT_END()

COMPONENT_BEGIN(rigid_body)
__SERIALIZE_FIELD(float3, ac, 1, 1, 1)
COMPNENT_END()

SCENE_BEGIN(my_second_scene)
__WORLD_BEGIN(world_000, transform, bullet)
____ENTITY_BEGIN(entity_000, transform, bullet)
______SET_COMPONENT(transform, .position.x, 1)
____ENTITY_END()
__WORLD_END()

__WORLD_BEGIN(world_001, transform, rigid_body)
____ENTITY_BEGIN(entity_000, transform, rigid_body)
______SET_COMPONENT(rigid_body, , { { 1, 2, 3 } })
____ENTITY_END()
__WORLD_END()

__WORLD_BEGIN(world_002, transform, bullet, rigid_body)
____ENTITY_BEGIN(entity_000, transform, rigid_body)
______SET_COMPONENT(rigid_body, , { { 3, 1, 2 } })
____ENTITY_END()
__WORLD_END()

__WORLD_BEGIN(world_003, transform)
____ENTITY_BEGIN(entity_000, transform)
______SET_COMPONENT(transform, .position, { 0, 0, 1 })
____ENTITY_END()
__WORLD_END()
SCENE_END()
// GAME(my_game, ...) => struct my_game_t {...} , static inline auto my_game = my_game_t();

// member function pointer, function pointer, system struct, lambda
// each first arguement can be empty or igame or iscene or iworld or entity
// SYSTEM_GROUP_BEGIN(game_system, interface)
// SEQ(&game::init)
// VALUE(current_scene, &game::get_current_scene)
// LOOP_BEGIN(&game::running)
// SWITCH_BEGIN(current_scene)
// CASE(0, SEQ(sys_scene_0, &interface.get_scene<scene_0>))
// CASE(1, SEQ(sys_scene_1, scene_1))
// CASE(2, SEQ(sys_scene_2, scene_2))
// SWITCH_END()
// LOOP_END()
//
// SEQ(&game::deinit)
// SYSTEM_END()

// using game_system = system_group<game, seq<&game::init>, switch<...
//
// my_game_system.run(my_game);
//

// game, scene... => data only, no member functions
// all member functions => interfaces
// some interface : unique e.g.) interface_scene, interface_world
// some custom interfaces : interface_scene_0

using world_t1 = ecs::world<transform, rigid_body>;
using world_t2 = ecs::world<transform>;
using world_t3 = ecs::world<transform, rigid_body, bullet>;

// Game(my_game, scene_t1, scene_t2, scene_t3, scene_t4)

struct worlds
{
	world_t1 world_1;
	world_t2 world_2;
	world_t3 world_3;

	template <typename world_t>
	world_t& get_world()
	{
		if constexpr (std::is_same_v<world_t, world_t1>)
		{
			return world_1;
		}
		else if constexpr (std::is_same_v<world_t, world_t2>)
		{
			return world_2;
		}
		else if constexpr (std::is_same_v<world_t, world_t3>)
		{
			return world_3;
		}
		else
		{
			static_assert(false, "invalid world type");
		}
	}

	// worlds()						 = default;
	// worlds(const worlds&)			 = delete;
	// worlds& operator=(const worlds&) = delete;

	// worlds(worlds&&) noexcept			 = default;
	// worlds& operator=(worlds&&) noexcept = default;
};

struct my_scene_state
{
};

struct scene_t1 : worlds, my_scene_state
{
	void init()
	{
		DEBUG_LOG("_scene_t1 init");
	};
};

struct scene_t2 : worlds, my_scene_state
{
	void init()
	{
		DEBUG_LOG("_scene_t2 init");
	};
};

struct scene_t3 : worlds, my_scene_state
{
	void init()
	{
		DEBUG_LOG("_scene_t3 init");
	};
};

struct scene_t4 : worlds, my_scene_state
{
	void init()
	{
		DEBUG_LOG("_scene_t4 init");
	};
};

struct scenes
{
	scene_t1 scene_1;
	scene_t2 scene_2;
	scene_t3 scene_3;
	scene_t4 scene_4;

	template <typename scene_t>
	scene_t& get_scene()
	{
		if constexpr (std::is_same_v<scene_t, scene_t1>)
		{
			return scene_1;
		}
		else if constexpr (std::is_same_v<scene_t, scene_t2>)
		{
			return scene_2;
		}
		else if constexpr (std::is_same_v<scene_t, scene_t3>)
		{
			return scene_3;
		}
		else if constexpr (std::is_same_v<scene_t, scene_t4>)
		{
			return scene_4;
		}
		else
		{
			static_assert(false, "invalid scene type");
		}
	}

	// scenes()						 = default;
	// scenes(const scenes&)			 = delete;
	// scenes& operator=(const scenes&) = delete;

	// scenes(scenes&&) noexcept			 = default;
	// scenes& operator=(scenes&&) noexcept = default;
};

struct my_game_state
{
	uint16 current_scene_idx = 0;
	bool   running			 = true;
};

struct par_exec_test : ecs::system::detail::__parallel_executor_base
{
	// template <typename... t_sys, typename... t_data>
	// void run_par(std::tuple<t_sys...>& systems, t_data&&... data)
	//{
	//	return run_par_impl(systems, std::index_sequence_for<t_sys...> {}, std::forward<t_data>(data)...);
	// }

	template <typename... t_func>
	decltype(auto) run_par(t_func&&... func)
	{
		return run_par_impl(std::index_sequence_for<t_func...> {}, std::forward<t_func>(func)...);
	}

	void wait() const
	{
		// no-op
	}

  private:
	template <typename std::size_t... i, typename... t_func>
	decltype(auto) run_par_impl(std::index_sequence<i...>, t_func&&... func)
	{
		auto futures = std::make_tuple(std::async(std::launch::async, func)...);
		(..., (std::get<i>(futures).wait()));
	}

	// template <typename... t_sys, std::size_t... i, typename... t_data>
	// decltype(auto) run_par_impl(std::tuple<t_sys...>& systems, std::index_sequence<i...>, t_data&&... data)
	//{
	//	auto futures = std::make_tuple(
	//		std::async(std::launch::async, [&] {
	//			ecs::_run_sys(std::get<i>(systems), std::forward<t_data>(data)...);
	//		})...);
	//	(..., (std::get<i>(futures).wait()));
	// }
};

struct my_game : scenes, my_game_state
{
	par_exec_test __parallel_executor;

	void init()
	{
		DEBUG_LOG("my game init");
	};

	void deinit()
	{
		DEBUG_LOG("my game deinit");
	};

	my_game()						   = default;
	my_game(const my_game&)			   = delete;
	my_game& operator=(const my_game&) = delete;

	my_game(my_game&&) noexcept			   = default;
	my_game& operator=(my_game&&) noexcept = default;
};

struct my_game_system
{
	template <typename t>
	void run(interface_game<t> igame)
	{
	}
};

struct my_scene_system_0
{
	template <typename s>
	void run(interface_scene<s> iscene)
	{
		DEBUG_LOG("---my_scene_system_0 run---");
		int a;
	}
};

struct my_scene_system_1
{
	void run()
	{
		DEBUG_LOG("---my_scene_system_1 run---");
	}
};

struct my_cond_system_false
{
	bool run()
	{
		DEBUG_LOG("---my_cond_system_false run---");
		return false;
	}
};

struct my_cond_system_true
{
	template <typename s>
	bool run(interface_scene<s> iscene)
	{
		DEBUG_LOG("---my_cond_system_true run---");
		return true;
	}
};

struct my_world_system_0
{
	template <typename w>
	void run(interface_world<w> iworld)
	{
	}
};

struct my_entity_system_0
{
	template <typename w>
	void entity_block_begin(interface_world<w> iworld)
	{
	}

	template <typename w>
	void entity_update(interface_world<w> iworld, ecs::entity_idx idx)
	{
	}

	template <typename w>
	void entity_block_end(interface_world<w> iworld)
	{
	}
};

struct sys_scene_init
{
	template <typename s>
	void run(interface_scene<s> iscene)
	{
		iscene.init();
	}
};

struct sys_non_templated
{
	void run()
	{
		std::println("non_templated");
	}
};

struct sys_game_init
{
	constexpr sys_game_init() {};

	template <typename g>
	void run(interface_game<g> igame)
	{
		igame.init();
	}

	// void run(int _)
	//{
	// }
};

struct sys_game_running
{
	template <typename g>
	bool run(interface_game<g> igame)
	{
		return igame.get_running();
	}
};

struct sys_game_deinit
{
	constexpr sys_game_deinit() {};

	template <typename g>
	void run(interface_game<g> igame)
	{
		igame.deinit();
	}
};

struct sys_get_scene0
{
	template <typename g>
	decltype(auto) run(interface_game<g> igame)
	{
		return igame.get_scene<scene_t1>();
	}
};

std::string get_type(auto&& val)
{
	if constexpr (std::is_lvalue_reference_v(val))
	{
		return "lvalue_ref";
	}
	else if constexpr (std::is_rvalue_reference_v(val))
	{
		return "rvalue_ref";
	}
}

template <typename t>
struct sys_node
{
	t sys;

	constexpr sys_node(t&& sys) : sys(std::forward<t>(sys)) { }

	std::remove_reference_t<t>& operator()()
	{
		return sys;
	}
};

// {
// using detail::operator|;
// auto logic =
//	 my_game
//	 | sys_game_init()
//	 | loop([]<typename g>(interface_game<g> igame){ return igame.running(); },
//			switch([]<typename g>(interface_game<g> igame){ return igame.get_current_scene_idx(); },
//				case(0, []<typename g>(interface_game<g> igame){ return igame.get_scene<scene_t1>(); } | sys_group_scene_1() ),
//				case([]<typename g>(interface_game<g> igame){ return igame.get_current_scene_idx() == 2; },
//					[]<typename g>(interface_game<g> igame){ return igame.get_scene<scene_t1>(); } | sys_group_scene_1() )
//	 | sys_game_deinit();
// }

// seq(sys_game_init)
// | seq(sys_game_init())
//
