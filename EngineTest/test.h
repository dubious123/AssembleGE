#pragma once
#include "../Engine/__reflection.h"
#include "../Engine/__engine.h"
#include "../Engine/__data_structures.h"
#include "../Engine/__meta.h"
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

using scene_t1 = ecs::scene<world_t1, world_t2>;
using scene_t2 = ecs::scene<world_t1, world_t2, world_t3>;
using scene_t3 = ecs::scene<world_t2, world_t3>;
using scene_t4 = ecs::scene<world_t1>;

// Game(my_game, scene_t1, scene_t2, scene_t3, scene_t4)

struct scenes
{
	scene_t1 scene_1;
	scene_t2 scene_2;
	scene_t3 scene_3;
	scene_t4 scene_4;

	template <typename scene_t>
	scene_t* get_scene()
	{
		if constexpr (std::is_same_v<scene_t, scene_t1>)
		{
			return &scene_1;
		}
		else if constexpr (std::is_same_v<scene_t, scene_t1>)
		{
			return &scene_2;
		}
		else if constexpr (std::is_same_v<scene_t, scene_t1>)
		{
			return &scene_3;
		}
		else if constexpr (std::is_same_v<scene_t, scene_t1>)
		{
			return &scene_4;
		}
	}
};

struct my_game_state
{
};

struct my_game : scenes, my_game_state
{
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

template <template <typename> typename _interface, typename t, typename = void>
struct interface_or_void
{
	using type = void;
};

template <template <typename> typename _interface, typename t>
struct interface_or_void<_interface, t, std::void_t<_interface<t>>>
{
	using type = _interface<t>;
};

template <template <typename> typename _interface, typename t>
using interface_or_void_t = interface_or_void<_interface, t>::type;

template <typename sys, template <typename> typename _interface, typename _data, typename = void>
struct sys_trait
{
	using func_trait = meta::function_traits<&sys::run>;
};

template <typename sys, template <typename> typename _interface, typename _data>
struct sys_trait<sys, _interface, _data, std::void_t<meta::function_traits<&sys::template run<_interface<_data>>>>>
{
	using func_trait = meta::function_traits<&sys::template run<_interface<_data>>>;
};

template <typename t_sys, typename t_data>
concept has_run_templated = requires(t_sys sys, t_data* p_data) {
	{
		sys.template run<t_data>(p_data)
	} -> std::same_as<void>;
};

template <typename t_sys>
concept has_run_non_templated = requires(t_sys sys) {
	{
		sys.run()
	} -> std::same_as<void>;
};

template <typename t_sys, typename t_data>
void run_system(t_sys& sys, t_data* p_data)
{
	if constexpr (has_run_templated<t_sys, t_data>)
	{
		// If the templated run exists, call it with Data.
		sys.template run<t_data>(p_data);
	}
	else if constexpr (has_run_non_templated<t_sys>)
	{
		// Otherwise, if a non-templated run exists, call that.
		sys.run();
	}
	else
	{
		static_assert(false and "System does not provide a run method that can be called.");
	}
}

template <typename... t_sys>
struct _seq
{
	std::tuple<t_sys...> systems;

	template <typename t_data>
	void run(t_data* p_data)
	{
		DEBUG_LOG("---new seq start (func)---");
		std::apply(
			[p_data](auto&... sys) {
				((run_system(sys, p_data)), ...);
			},
			systems);
		DEBUG_LOG("---new seq end (func)---");
	}
};

template <typename... t_sys>
struct _par
{
	std::tuple<t_sys...> systems;

	template <typename tpl_sys, typename t_data, std::size_t... i>
	void parallel_apply(tpl_sys& systems, t_data* p_data, std::index_sequence<i...>)
	{
		auto futures = std::array<std::future<void>, sizeof...(i)> {
			(std::async([p_data, &systems]() { run_system(std::get<i>(systems), p_data); }))...
		};

		std::apply([](auto&... fut) { ((void)fut.get(), ...); }, futures);
	}

	template <typename t_data>
	void run(t_data* p_data)
	{
		DEBUG_LOG("---new par start (func)---");
		parallel_apply(systems, p_data, std::make_index_sequence<sizeof...(t_sys)>());
		DEBUG_LOG("---new par end (func)---");
	}
};

template <template <typename> typename _interface, typename... nodes>
struct _system_group
{
	std::tuple<nodes...> nodes;

	template <typename t>
	void run(_interface<t> i_t)
	{
		// if constexpr ()
		//{
		// }
		// else if constexpr ()
		//{
		// }
	}
};
