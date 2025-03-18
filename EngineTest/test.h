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
	void init()
	{
		DEBUG_LOG("my game init");
	};
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

struct sys_game_init
{
	template <typename g>
	void run(interface_game<g> igame)
	{
		igame.init();
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
	};
};

template <typename t_sys>
concept has_run_non_templated = requires(t_sys sys) {
	{
		sys.run()
	};
};

// template <typename t>
// concept _is_node = is_specialization_of_v<t, _seq> || is_specialization_of_v<t, _par>;

template <typename t_sys, typename t_data>
auto run_system(t_sys& sys, t_data* p_data)
{
	if constexpr (has_run_templated<std::remove_const_t<t_sys>, t_data>)
	{
		// If the templated run exists, call it with Data.
		return sys.template run<t_data>(p_data);
	}
	else if constexpr (has_run_non_templated<std::remove_const_t<t_sys>>)
	{
		// Otherwise, if a non-templated run exists, call that.
		return sys.run();
	}
	else
	{
		static_assert(false and "System does not provide a run method that can be called.");
	}
}

template <auto... sys>
struct _seq
{
	std::tuple<decltype(sys)...> systems;

	constexpr _seq() : systems(sys...) { }

	template <typename t_data>
	void run(t_data* p_data)
	{
		DEBUG_LOG("---new seq start (func)---");

		//([p_data]() {
		//	run_system(sys, p_data);
		//}(),
		// ...);
		std::apply(
			[p_data](auto&... _sys) {
				((run_system(_sys, p_data)), ...);
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
			(std::async(std::launch::async, [p_data, &systems]() { run_system(std::get<i>(systems), p_data); }))...
		};

		std::ranges::for_each(futures, [](auto&& fut) { fut.get(); });
	}

	template <typename t_data>
	void run(t_data* p_data)
	{
		DEBUG_LOG("---new par start (func)---");
		parallel_apply(systems, p_data, std::make_index_sequence<sizeof...(t_sys)>());
		DEBUG_LOG("---new par end (func)---");
	}
};

template <typename t_sys_cond, typename t_sys_true, typename t_sys_false>
struct _cond
{
	t_sys_cond	sys_cond;
	t_sys_true	sys_true;
	t_sys_false sys_false;

	template <typename t_data>
	void run(t_data* p_data)
	{
		DEBUG_LOG("---new cond start (func)---");
		if (run_system(sys_cond, p_data))
		{
			run_system(sys_true, p_data);
		}
		else
		{
			run_system(sys_false, p_data);
		}
		DEBUG_LOG("---new cond end (func)---");
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

// GAME_BEGIN(my_game, ...)
//__STATE(my_game_state, game_state)
//...
// GAME_END()

// SCENE_BEGIN(my_scene_0, ...)
//__STATES(scene_state, direct12_state, ...)
// SCENE_END()

// ENTITY_COLLECTION(my_collection)

// WORLD_BEGIN(my_world_0, ...)
//__STATES(world_state, physics_state, ...)
//__ENTITY_COLLECTION(my_collection)
//__ENTITY_BEGIN(entity_name, ...)
//____SET_COMPONENT(transform, position, {1,1,1})
//__ENTITY_END()
// WORLD_END()

// interface_begin(my_interface)
//__METHOD(init)
//__PROPERTY(current_scene)
// interface_end()

// system_begin(my_game_system, interface_game)
//__expose_to_editor(some_data)
// system_end()

// system_group_begin(game_sys_group, interface_game)
// seq(sys_game_init)
// switch_begin(sys_game_current_scene)
// case(0, bind(sys_scene_0, []<typename g>(interface_game<g> igame){return igame.scene_0; })

// auto sys_group_game = []<typename g>(interface_game<g> igame) {
//	my_scene_system_0().run(igame.get_scene<???>());
// };

template <typename t>
struct extract_interface_template;

template <template <typename> typename... t_interface, typename t>
struct extract_interface_template<std::tuple<t_interface<t>...>>
{
	template <typename t_data>
	using tpl_interfaces = std::tuple<t_interface<t_data>...>;

	template <typename t_data, std::size_t... i>
	constexpr static tpl_interfaces<t_data> get_interfaces_imp(t_data* p_data, std::index_sequence<i...>)
	{
		return std::make_tuple((std::tuple_element_t<i, tpl_interfaces<t_data>>(p_data))...);
	}

	template <typename t_data>
	constexpr static tpl_interfaces<t_data> get_interfaces(t_data* p_data)
	{
		constexpr auto size = std::tuple_size_v<tpl_interfaces<t_data>>;
		return get_interfaces_imp(p_data, std::make_index_sequence<size> {});
	}
};

template <typename t_callable, typename t_data>
using lambda_interface_templates = extract_interface_template<typename meta::function_traits<&t_callable::template operator()<t_data>>::argument_types>;

template <auto sys, auto callable>
struct _bind
{
	decltype(sys) _sys;

	constexpr _bind() : _sys(sys) { }

	template <typename t_data>
	void run(t_data* p_data)
	{
		auto tpl = lambda_interface_templates<decltype(callable), t_data>::template get_interfaces(p_data);
		// auto* res = func(std::get<0>(tpl));
		auto* res = std::apply(callable, tpl);
		run_system(_sys, res);
	}
};

// todo generic lambda + system