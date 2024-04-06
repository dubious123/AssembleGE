
// #include <iostream>
// #include <string>
// #include <vector>
//
//// #include "../Engine/Entity.cpp"
//
// int main()
//{
//	std::string				 s = "hi";
//	std::vector<std::string> _vec;
//
//	_vec.push_back(s);
//	s.clear();
//	int a = 1;
//	// std::cout << "Hello World!" << test();
//}

#define _CRTDBG_MAP_ALLOC

#ifdef _DEBUG
	#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
	#define DBG_NEW new
#endif

#include <windows.h>
#include <algorithm>	// for max
#include <cstdio>		// for printf
#include <utility>		// for exchange, move
#include <vector>		// for vector
#include <any>
#include <variant>
#include <iostream>
#include <functional>

#include <libloaderapi.h>
#define Find(Type, Id) Model::Type::Find(Id)

typedef int (*import_func)();
#include <map>

#define LOAD_FUN(func_type, func_name, library)                        \
	[library]() {                                                      \
		using lib_func = func_type;                                    \
		auto func	   = (lib_func)GetProcAddress(library, func_name); \
		return func;                                                   \
	}()

#define LOAD_RUN_FUNC(func_type, func_name, library)                   \
	[library]() {                                                      \
		using lib_func = func_type;                                    \
		auto func	   = (lib_func)GetProcAddress(library, func_name); \
		return func();                                                 \
	}()
#include <array>
constexpr auto arr = []() {
	std::array<int, 100> result {};
	for (size_t i = 0; i < 100; ++i)
	{
		result[i] = i * i;
	}
	return result;
}();

#pragma comment(lib, "engine.lib")


#include <cstdlib>
#include <crtdbg.h>
#include <Sysinfoapi.h>
#include "../Engine/__reflection.h"
#include "../Engine/__engine.h"
#include "../Engine/__data_structures.h"
#include "../Engine/__meta.h"

#include <type_traits>
#include <sstream>
#include <format>
#include <cstdlib>
#include <ctime>
#include <chrono>
using namespace data_structure;

COMPONENT_BEGIN(transform)
SERIALIZE_FIELD(float3, position, 0, 0, 0)
SERIALIZE_FIELD(float3, scale, 1, 1, 1)
SERIALIZE_FIELD(float4, rotation, 0, 0, 0, 1)
COMPNENT_END()

COMPONENT_BEGIN(bullet)
SERIALIZE_FIELD(float3, vel, 1, 0, 1)
SERIALIZE_FIELD(float3, ac, 1, 1, 1)
COMPNENT_END()

COMPONENT_BEGIN(rigid_body)
SERIALIZE_FIELD(float3, ac, 1, 1, 1)
COMPNENT_END()

WORLD_BEGIN(world_000, transform, bullet)
ENTITY_BEGIN(entity_000, transform, bullet)
SET_COMPONENT(transform, .position.x, 1)
ENTITY_END()
WORLD_END()

WORLD_BEGIN(world_001, transform, rigid_body)
ENTITY_BEGIN(entity_000, transform, rigid_body)
SET_COMPONENT(rigid_body, , { { 1, 2, 3 } })
ENTITY_END()
WORLD_END()

WORLD_BEGIN(world_002, transform, bullet, rigid_body)
ENTITY_BEGIN(entity_000, transform, rigid_body)
SET_COMPONENT(rigid_body, , { { 3, 1, 2 } })
ENTITY_END()
WORLD_END()

WORLD_BEGIN(world_003, transform)
ENTITY_BEGIN(entity_000, transform)
SET_COMPONENT(transform, .position, { 0, 0, 1 })
ENTITY_END()
WORLD_END()

// static inline auto world_003 = []() {
//	using world_wrapper_t = reflection::world_wrapper<"world_003", transform>;
//	world_wrapper_t::init_func = [](world_wrapper_t::world_type& w) {
//		world_wrapper_t::serialize();
//		{
//			auto e = w.new_entity<transform>(); reflection::register_entity("entity_000", e, (const ecs::world_base&)w);
//			w.get_component<transform>(e).position = { 0, 0, 1 };
//		}
//		{
//
//		}
//	}; return world_wrapper_t(); };


// static inline auto& my_second_scene = []() -> auto& {
//	using t_scene_wrapper = reflection::scene_wrapper<"my_second_scene",
//		decltype(world_001()),
//		decltype(world_002())>;
//	t_scene_wrapper();  // scene reflection
//	world_001();;       // connect init, world reflection, entity reflection
//	world_002();;
//	return t_scene_wrapper::value(); }();

SERIALIZE_SCENE(my_second_scene, world_001, world_002)

//
// SERIALIZE_SCENE(my_third_scene, world_002, world_003)
//  1. scene_wrapper() => scene reflection
//  2. world_lambda() => connect init lambda
//  3. scene_wrapper::value()
//		a. call scene constructor => create scene and world
//		b. call world_wrapper::init(w) => create entity and reflection for entity
// 			1. world::serialize() => register world and structs
//			2. create entity
// 			3. register_entity

// SERIALIZE_SCENE(my_first_scene, world_003)

void test_func(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	// DEBUG_LOG("test_func1 : " << a++);
	t.position.x += 1;
}

void test_func2(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	// DEBUG_LOG("test_func2 : " << a++);
	t.position.y += 1;
}

void test_func3(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	// DEBUG_LOG("test_func3 : " << a++);
	t.position.z += 1;
}

void test_func4(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	// DEBUG_LOG("test_func4 : " << a++);
	t.position.x += 1;
}

void test_func5(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	// DEBUG_LOG("test_func5 : " << a++);
	t.position.y += 1;
}

void test_func6(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	// DEBUG_LOG("test_func6 : " << a++);
	t.position.z += 1;
}

void test_func7(ecs::entity_idx idx, transform& t, rigid_body& rb)
{
	static int a = 0;
	// DEBUG_LOG("test_func7 : " << a++);
	t.position.z += 1;
}

bool cond_func1()
{
	return true;
}

bool cond_func2()
{
	return false;
}

void test_add(int a, int b)
{
	a + b;
}

void test_add2(uint64 a, uint64 b, uint64 c, uint64 d, uint64 e, uint64 f)
{
	a + b;
}

void test_update(auto& w, auto p)
{
	p().update(w);
}

static inline auto& v = []() -> auto& {
	static data_structure::vector<uint32> s;
	s.emplace_back(2u);
	return s;
}();


auto  s		  = ecs::scene<ecs::world<transform, bullet, rigid_body>, ecs::world<transform, rigid_body>>();
auto  s_2	  = ecs::scene<ecs::world<transform, rigid_body>, ecs::world<transform, rigid_body>>();
auto& world_2 = s.get_world<0>();
auto& world_3 = s_2.get_world<0>();

template <typename world>
concept ecs_world = std::derived_from<world, ecs::world_base>;

struct system_1
{
	int* p_int;

	void on_system_begin_w(auto& world)
	{
	}

	void on_thread_init_w(auto& world) { }

	void update_w(auto& world, ecs::entity_idx e_idx, transform& t, rigid_body& v) {};

	// void update(ecs::entity_idx e_idx, transform& t, rigid_body& v) {};

	void on_thread_dispose() { }

	void on_system_end() { }

	void test_mem_func(int a, int b) { }

	system_1()
	{
		int a = 1;
		p_int = new (int);
	};

	~system_1()
	{
		delete p_int;
	}
};

struct system_2
{
	void update_w(auto& world, transform& t, bullet& v) {};
};

using namespace ecs;

int main()
{
	std::cout << std::format("{:04x}-{:016x}", 1, -1);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	static_assert(meta::param_constains_v<ecs::entity_idx, test_func2> == true);
	meta::param_at<1, test_func2> eeeee;

	using group_t = system_group<system_1, system_2>;
	auto group	  = system_group<system_1, par<system_1, seq<system_1, system_2>, cond<cond_func1, system_2, group_t>>, group_t>();

	using tpl		   = tuple_sort<component_comparator, transform, bullet, rigid_body>::type;
	constexpr auto idx = tuple_index<transform, tpl>::value;
	using components   = component_wrapper<transform, bullet, rigid_body>;
	components::calc_archetype<transform, bullet>();
	int world_idx = 0;

	auto e2 = world_2.new_entity<transform>();
	auto e3 = world_2.new_entity<rigid_body, transform>();
	auto e4 = world_2.new_entity<bullet>();

	auto ee = world_2.new_entity<transform>();
	world_2.add_component<rigid_body>(ee);
	assert(world_2.has_component<rigid_body>(ee));
	assert(world_2.has_component<transform>(ee));

	ee = world_2.new_entity<transform, rigid_body, bullet>();
	world_2.remove_component<rigid_body>(ee);
	assert(world_2.has_component<rigid_body>(ee) == false);
	assert((world_2.has_component<transform, bullet>(ee) == true));

	ee = world_2.new_entity<transform, rigid_body, bullet>();
	world_2.remove_component<transform, rigid_body>(ee);
	assert(world_2.has_component<bullet>(ee));
	assert((world_2.has_component<rigid_body, transform>(ee) == false));

	std::srand(std::time(nullptr));
	for (auto i = 0; i < 10000; ++i)
	{
		auto eeeee = world_2.new_entity<transform>();
		world_2.add_component<rigid_body, bullet>(eeeee);
		world_2.add_component<bullet, rigid_body>(world_2.new_entity<transform>());
		world_2.add_component<transform>(world_2.new_entity<bullet, rigid_body>());
		world_2.remove_component<rigid_body>(world_2.new_entity<transform, rigid_body>());
		world_2.remove_component<transform, rigid_body>(world_2.new_entity<bullet, transform, rigid_body>());
	}

	world_2.new_entity<bullet, transform, rigid_body>();
	world_2.new_entity<bullet, transform, rigid_body>();

	world_2.add_component<rigid_body>(e2);

	auto& rb = world_2.get_component<rigid_body>(e2);
	world_2.add_component<bullet>(e2);
	group.update(world_2);

	static_assert(std::is_same_v<typename tuple_sort<component_comparator, transform, bullet, rigid_body>::type, typename tuple_sort<component_comparator, transform, bullet, rigid_body>::type>);

	static_assert(std::is_same_v<auto_wrapper<test_func>, auto_wrapper<test_func>>);
	static_assert(std::is_same_v<auto_wrapper<test_func>, auto_wrapper<test_func2>> == false);
	static_assert(variadic_contains<int, double>() == false);
	static_assert(variadic_contains<int, double, float>() == false);
	static_assert(variadic_contains<int, double, float, int>() == true);
	static_assert(variadic_contains<test_func, test_func, test_func2, test_func3>() == true);
	static_assert(variadic_contains<test_func, test_func2>() == false);
	static_assert(variadic_constains_v<int, double> == false);
	static_assert(variadic_constains_v<int, double, float> == false);
	static_assert(variadic_constains_v<int, double, float, int> == true);

	static_assert(variadic_auto_unique<test_func, test_func, test_func, test_func3> == false);
	static_assert(variadic_unique<int, double, float, int> == false);
}