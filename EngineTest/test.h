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

struct my_game : scenes, my_game_state
{
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
		int a;
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

template <template <typename> typename _interface, typename... nodes>
struct _system_group
{
	std::tuple<nodes...> nodes;

	template <typename t>
	void run(_interface<t>)
	{
		// if constexpr ()
		//{
		// }
		// else if constexpr ()
		//{
		// }
	}
};

inline static auto _sys_group_game = ecs::seq<
	sys_game_init {},
	ecs::loop<[]<typename g>(interface_game<g> igame) { return igame.get_running(); },
			  ecs::_switch<[]<typename g>(interface_game<g> igame) { return igame.get_current_scene_idx(); },
						   ecs::bind<sys_scene_init {}, []<typename g>(interface_game<g> igame) -> auto& { return igame.get_scene<scene_t1>(); }> {},
						   ecs::bind<sys_scene_init {}, []<typename g>(interface_game<g> igame) -> auto& { return igame.get_scene<scene_t2>(); }> {}> {}> {},
	sys_game_deinit {}>();

// std::ranges::for_each(vec, [](auto& e){})
// vec
//	| std::views::filter()
//	| std::views::transform()
//  | std::ranges::to<std::list>();

struct sys_group_base
{
};

template <typename t>
concept is_system_group = std::derived_from<std::decay_t<t>, sys_group_base>;

template <typename t>
concept is_raw_system = not is_system_group<t>;

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

template <typename t_sys, typename... t_data>
decltype(auto) _run_sys(t_sys&& sys, t_data&&... args)
{
	if constexpr (ecs::detail::has_run<t_sys, decltype(args)...>)
	{
		if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::run, decltype(args)...>)
		{
			return sys.run(std::forward<decltype(args)>(args)...);
		}
		else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::run>)
		{
			return sys.run();
		}
		else
		{
			static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
		}
	}
	else if constexpr (ecs::detail::has_run_templated<t_sys, decltype(args)...>)
	{
		// return sys.run<t_data...>(std::forward<decltype(args)>(args)...);
		if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template run<t_data...>, decltype(args)...>)
		{
			return sys.run<t_data...>(std::forward<decltype(args)>(args)...);
		}
		else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template run<decltype(args)...>>)
		{
			return sys.run<>();
		}
		else
		{
			static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
		}
	}
	else if constexpr (ecs::detail::has_operator<t_sys>)
	{
		if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::operator(), decltype(args)...>)
		{
			return sys(std::forward<decltype(args)>(args)...);
		}
		else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::operator()>)
		{
			return sys();
		}
		else
		{
			static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
		}
	}
	else if constexpr (ecs::detail::has_operator_templated<t_sys, decltype(args)...>)
	{
		if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template operator()<t_data...>, decltype(args)...>)
		{
			return sys.template operator()<t_data...>(std::forward<decltype(args)>(args)...);
		}
		else if constexpr (ecs::detail::invocable<&std::decay_t<t_sys>::template operator()<t_data...>>)
		{
			return sys.template operator()<t_data...>();
		}
		else
		{
			static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
		}
	}
	else if constexpr (sizeof...(args) == 0)
	{
		return std::forward<t_sys>(sys);
	}
	else
	{
		static_assert(false, "sys is data type but tries to run with arguments or sys cannot be invoked with given arguments");
		// return;
	}
}

template <typename... t_sys>
struct system_seq2 : sys_group_base
{
	std::tuple<t_sys...> systems;

	constexpr system_seq2(t_sys&&... sys)
		: systems(std::forward<t_sys>(sys)...)
	{
	}

	constexpr explicit system_seq2(std::tuple<t_sys...>&& tpl)
		: systems(std::forward<std::tuple<t_sys...>>(tpl)) { }

	template <typename... t_data>
	decltype(auto) run(t_data&&... data)
	{
		if constexpr (sizeof...(t_data) == 0)
		{
			return run_impl(meta::offset_sequence<1, std::tuple_size_v<decltype(systems)> - 1> {}, _run_sys(std::get<0>(systems)));
		}
		else
		{
			return run_impl(std::index_sequence_for<t_sys...> {}, std::forward<t_data>(data)...);
		}
	}

  private:
	template <std::size_t... i, typename... t_data>
	decltype(auto) run_impl(std::index_sequence<i...>, t_data&&... data)
	{
		(_run_sys(std::get<i>(systems), std::forward<t_data>(data)...), ...);
		// return std::make_tuple(std::get<i>(systems).run(std::forward<t_data>(data)...), ...);
		//  return std::tuple<std::decay_t<Args>&...>(args...);	   // return refs to original data
	}
};

template <typename... t_sys>
struct system_pipeline
{
	std::tuple<t_sys...> systems;

	constexpr system_pipeline(t_sys&&... sys)
		: systems(std::forward<t_sys>(sys)...)
	{
	}

	constexpr explicit system_pipeline(std::tuple<t_sys...>&& tpl)
		: systems(std::forward<std::tuple<t_sys...>>(tpl)) { }

	template <typename... t_data>
	decltype(auto) run(t_data&&... data)
	{
		return run_impl<0>(std::forward<t_data>(data)...);
		//_run_sys(sys_right, _run_sys(sys_left, std::forward<decltype(data)>(data)...));
	}

  private:
	template <std::size_t i, typename... t_data>
	decltype(auto) run_impl(t_data&&... data)
	{
		if constexpr (i == std::tuple_size_v<decltype(systems)>)
		{
			if constexpr (sizeof...(data) == 1)
			{
				return _run_sys(std::forward<t_data>(data)...);
			}
			else
			{
				// todo
			}
		}
		else
		{
			return run_impl<i + 1>(_run_sys(std::get<i>(systems), std::forward<t_data>(data)...));
		}
	}
};

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

namespace _temp
{
	template <typename t_left, typename t_right>
	decltype(auto) operator+(t_left&& left, t_right&& right)
	{
		return system_seq2<t_left, t_right>(std::forward<t_left>(left), std::forward<t_right>(right));
	}

	template <typename... t_sys, template <typename...> typename t_sys_group, is_raw_system t_right>
	decltype(auto) operator+(t_sys_group<t_sys...>&& left, t_right&& right)
	{
		return system_seq2<t_sys..., t_right>(std::tuple_cat(std::forward<decltype(left.systems)>(left.systems), std::forward_as_tuple(std::forward<t_right>(right))));
	}

	template <typename t_left, typename t_right>
	decltype(auto) operator|(t_left&& left, t_right&& sys)
	{
		return system_pipeline<t_left, t_right>(std::forward<t_left>(left), std::forward<t_right>(sys));
	}

	template <typename... t_sys, template <typename...> typename t_sys_group, is_raw_system t_right>
	requires std::same_as<system_pipeline<t_sys...>, t_sys_group<t_sys...>>
	decltype(auto) operator|(t_sys_group<t_sys...>&& left, t_right&& right)
	{
		return system_pipeline<t_sys..., t_right>(std::tuple_cat(std::forward<decltype(left.systems)>(left.systems), std::forward_as_tuple(std::forward<t_right>(right))));
	}

	template <typename... t_sys, template <typename...> typename t_sys_group, is_raw_system t_left>
	requires std::same_as<system_pipeline<t_sys...>, t_sys_group<t_sys...>>
	decltype(auto) operator|(t_left&& left, t_sys_group<t_sys...>&& right)
	{
		return system_pipeline<t_left, t_sys...>(std::tuple_cat(std::forward_as_tuple(std::forward<t_left>(left)), std::forward<decltype(right.systems)>(right.systems)));
	}

	template <typename... t_sys_l, template <typename...> typename t_sys_group_l, typename... t_sys_r, template <typename...> typename t_sys_group_r>
	requires std::same_as<system_pipeline<t_sys_l...>, t_sys_group_l<t_sys_l...>> && std::same_as<system_pipeline<t_sys_r...>, t_sys_group_r<t_sys_r...>>
	decltype(auto) operator|(t_sys_group_l<t_sys_l...>&& left, t_sys_group_r<t_sys_r...>&& right)
	{
		return system_pipeline<t_sys_l..., t_sys_r...>(std::tuple_cat(std::forward<decltype(left.systems)>(left.systems), std::forward<decltype(right.systems)>(right.systems)));
	}
}	 // namespace _temp

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
