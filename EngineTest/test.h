#pragma once
#include "../Engine/__reflection.h"
#include "../Engine/__engine.h"
#include "../Engine/__data_structures.h"
#include "../Engine/__meta.h"
#include "../Engine/__ecs_system.h"
#include "../Engine/__ecs_entity_storage_basic.h"
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

using world_t1 = ecs::entity_storage::basic<uint32, transform, rigid_body>;
using world_t2 = ecs::entity_storage::basic<uint32, transform>;
using world_t3 = ecs::entity_storage::basic<uint32, transform, rigid_body, bullet>;

// Game(my_game, scene_t1, scene_t2, scene_t3, scene_t4)

struct scene_t1
{
	world_t1 world_1;
	world_t2 world_2;
	world_t3 world_3;

	template <typename world_t>
	world_t&
	get_world()
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

	void
	init()
	{
		DEBUG_LOG("_scene_t1 init");
	};

	void
	deinit()
	{
		DEBUG_LOG("_scene_t1 deinit");
		world_1.deinit();
		world_2.deinit();
		world_3.deinit();
	}
};

struct scene_t2
{
	world_t1 world_1;
	world_t2 world_2;
	world_t3 world_3;

	template <typename world_t>
	world_t&
	get_world()
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

	void
	init()
	{
		DEBUG_LOG("_scene_t2 init");
	};

	void
	deinit()
	{
		DEBUG_LOG("_scene_t2 deinit");
		world_1.deinit();
		world_2.deinit();
		world_3.deinit();
	}
};

struct scene_t3
{
	world_t1 world_1;
	world_t2 world_2;
	world_t3 world_3;

	template <typename world_t>
	world_t&
	get_world()
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

	void
	init()
	{
		DEBUG_LOG("_scene_t3 init");
	};

	void
	deinit()
	{
		DEBUG_LOG("_scene_t3 deinit");
		world_1.deinit();
		world_2.deinit();
		world_3.deinit();
	}
};

struct scene_t4
{
	world_t1 world_1;
	world_t2 world_2;
	world_t3 world_3;

	template <typename world_t>
	world_t&
	get_world()
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

	void
	init()
	{
		DEBUG_LOG("_scene_t4 init");
		world_1.init();
		world_2.init();
		world_3.init();
	};

	void
	deinit()
	{
		DEBUG_LOG("_scene_t4 deinit");
		world_1.deinit();
		world_2.deinit();
		world_3.deinit();
	}
};

struct par_exec_test : ecs::system::detail::__parallel_executor_base
{
#define FWD(x) std::forward<decltype(x)>(x)

	template <typename... t>
	static decltype(auto)
	tuple_cat_all(std::tuple<t...>&& tpl)
	{
		return std::apply([](auto&&... arg) { return std::tuple_cat(FWD(arg)...); }, FWD(tpl));
	}

	template <typename... t_arg>
	decltype(auto)
	make_arg_tpl(t_arg&&... arg)
	{
		using t_arg_tpl = std::tuple<
			std::conditional_t<
				std::is_lvalue_reference_v<t_arg&&>,
				t_arg&&,
				std::remove_reference_t<t_arg>>...>;

		return t_arg_tpl{ std::forward<t_arg>(arg)... };
	}

	// template <typename... t_func, typename... t_arg>
	// decltype(auto)
	// run_par(t_func&&... func, t_arg&&... arg)
	//{
	//	return [&func..., &arg...]<auto... i>(std::index_sequence<i...>) {
	//		return [](auto&&... async_op) {
	//			return tuple_cat_all(std::tuple{ async_op.wait()... });
	//		}(std::async(std::launch::async, func, FWD(arg)...)...);
	//	}(std::index_sequence_for<t_func...>{});
	// }

	template <typename... t_func>
	decltype(auto)
	run_par(t_func&&... func)
	{
		return [](auto&&... async_op) {
			return tuple_cat_all(std::tuple{ async_op.get()... });
		}(std::async(std::launch::async, FWD(func))...);
	}

	void
	wait() const
	{
		// no-op
	}

#undef FWD
};

struct my_game
{
	par_exec_test __parallel_executor;

	scene_t1 scene_1;
	scene_t2 scene_2;
	scene_t3 scene_3;
	scene_t4 scene_4;

	uint16 current_scene_idx = 0;
	bool   running			 = true;

	void
	init()
	{
		DEBUG_LOG("my_game init");
		scene_1.init();
		scene_2.init();
		scene_3.init();
		scene_4.init();
	};

	void
	deinit()
	{
		DEBUG_LOG("my_game deinit");
		scene_1.deinit();
		scene_2.deinit();
		scene_3.deinit();
		scene_4.deinit();
	};

	template <typename scene_t>
	scene_t&
	get_scene()
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

	my_game()				= default;
	my_game(const my_game&) = delete;
	my_game&
	operator=(const my_game&) = delete;

	my_game(my_game&&) noexcept = default;
	my_game&
	operator=(my_game&&) noexcept = default;
};

struct my_game_system
{
	template <typename t>
	void
	run(interface_game<t> igame)
	{
	}
};

struct my_scene_system_0
{
	template <typename s>
	void
	run(interface_scene<s> iscene)
	{
		DEBUG_LOG("---my_scene_system_0 run---");
		int a;
	}
};

struct my_scene_system_1
{
	void
	run()
	{
		DEBUG_LOG("---my_scene_system_1 run---");
	}
};

struct my_cond_system_false
{
	bool
	run()
	{
		DEBUG_LOG("---my_cond_system_false run---");
		return false;
	}
};

struct my_cond_system_true
{
	template <typename s>
	bool
	run(interface_scene<s> iscene)
	{
		DEBUG_LOG("---my_cond_system_true run---");
		return true;
	}
};

struct my_world_system_0
{
	template <typename w>
	void
	run(i_world<w> iworld)
	{
	}
};

struct my_entity_system_0
{
	template <typename w>
	void
	entity_block_begin(i_world<w> iworld)
	{
	}

	template <typename w>
	void
	entity_update(i_world<w> iworld, ecs::entity_idx idx)
	{
	}

	template <typename w>
	void
	entity_block_end(i_world<w> iworld)
	{
	}
};

struct sys_scene_init
{
	template <typename s>
	void
	operator()(interface_scene<s> iscene)
	{
		iscene.init();
	}
};

struct sys_non_templated
{
	void
	operator()()
	{
		std::println("non_templated");
	}
};

struct sys_game_init
{
	constexpr sys_game_init() { };

	template <typename g>
	decltype(auto)
	operator()(interface_game<g> igame)
	{
		igame.init();
		return 1.f;
	}

	// void run(int _)
	//{
	// }
};

struct sys_game_running
{
	template <typename g>
	bool
	run(interface_game<g> igame)
	{
		return igame.get_running();
	}
};

struct sys_game_deinit
{
	constexpr sys_game_deinit() { };

	template <typename g>
	decltype(auto)
	operator()(interface_game<g> igame)
	{
		igame.deinit();
		return 1;
	}
};

struct sys_get_scene0
{
	template <typename g>
	decltype(auto)
	run(interface_game<g> igame)
	{
		return igame.get_scene<scene_t1>();
	}
};

struct sys_init
{
	decltype(auto)
	run(auto&& obj)
	{
		return obj.init();
	}
};

struct sys_deinit
{
	decltype(auto)
	run(auto&& obj)
	{
		return obj.deinit();
	}
};

template <typename t_scene>
struct sys_get_scene
{
	template <typename g>
	decltype(auto)
	run(interface_game<g> igame)
	{
		return igame.get_scene<t_scene>();
	}
};

template <typename t_world>
struct sys_get_world
{
	template <typename s>
	decltype(auto)
	run(interface_scene<s> iscene)
	{
		return iscene.get_world<t_world>();
	}
};

// using namespace ecs::system::op;
// inline constexpr auto sys_scene_0_init_builder = [] { return sys_init{}
//														   + ((sys_get_world<world_t1>{} | [](auto&& _) { std::println("world_t1 init"); })
//															  ^ (sys_get_world<world_t2>() | [](auto&& _) { std::println("world_t2 init"); })
//															  ^ (sys_get_world<world_t3>() | [](auto&& _) { std::println("world_t3 init"); })); };

std::string
get_type(auto&& val)
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