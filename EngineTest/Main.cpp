
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
// #include <algorithm>	// for max
// #include <cstdio>		// for printf
// #include <utility>		// for exchange, move
// #include <vector>		// for vector
// #include <any>
// #include <variant>
// #include <iostream>
// #include <functional>


#include <libloaderapi.h>
#define Find(Type, Id) Model::Type::Find(Id)

typedef int (*import_func)();

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

// #include <type_traits>
// #include <sstream>
// #include <format>
// #include <cstdlib>
// #include <ctime>
// #include <chrono>
import std;

using namespace data_structure;
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

	void on_system_begin(auto& world)
	{
	}

	void on_thread_init(auto& world) { }

	void update(auto& world, ecs::entity_idx e_idx, transform& t, rigid_body& v) {};

	// void update(ecs::entity_idx e_idx, transform& t, rigid_body& v) {};

	void on_thread_dispose() { }

	void on_system_end(auto& world) { }

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
	// void update_w(auto& world, transform& t, bullet& v) {};
	void update(transform& t, bullet& v) {};

	static void test_fu(int a, int b) { }
};

void test_add3(int a, int b) { }

using namespace ecs;


// 1. reflection scene
// 2. reflection world
// 3. reflection entity
// 4. reflection components
// 5. return ecs scene

// scene_begin
// world_begin
// entity_begin
// set_component
// entity_end
// world_end
// scene_end

static inline auto& new_scene = []() -> auto& {
	return reflection::scene_wrapper<"new_scene">::init(
		0

		,
		[]() {
			using new_world__wrapper_type	   = reflection::world_wrapper<"new_world">;
			new_world__wrapper_type::init_func = [](ecs::world_base& world) -> void {
				using world_t = ecs::world<>;
				reflection::register_world("new_world", world);

				{
					// entity begin
					auto entity = ((world_t&)world).new_entity<>();
					reflection::register_entity("new_entity", entity, world);
					// entity end
				}
			};

			return new_world__wrapper_type();
		}()

	);
}();

SCENE_BEGIN(new_scene_3)
__WORLD_BEGIN(new_world_2, transform)
____ENTITY_BEGIN(new_entity)
____ENTITY_END()
____ENTITY_BEGIN(new_entity2, transform)
______SET_COMPONENT(transform, .position.x, 100.f)
____ENTITY_END()
__WORLD_END()
SCENE_END()

int main()
{
	static_assert(ecs::has_update<system_1> == false);
	// static_assert(ecs::has_update_w<system_1, decltype(world_2)> == true, "!!!");

	// auto empty_entity = world_2.new_entity<>();
	// world_2.add_component<transform>(empty_entity); // will crash

	// auto entity_1 = world_2.new_entity<transform>();
	// world_2.remove_component<transform>(entity_1);

	//  static_assert(false);
	// std::cout << std::format("{:04x} - {:016x}\n", 1, -1);
	// uint2 uint22 { (uint32)(0 - 1), 2 };
	// std::cout << std::format("{},{}\n", uint22.x, uint22.y);

	// float2 float22 { -1.f, 2.f };
	// std::cout << std::format("{:.3f},{:.4f}\n", float22.x, float22.y);

	// double64 ddddd = -10.252525;
	// std::cout << std::format("{}\n", ddddd);

	// uint64 iiii = (0ull - 1ull);
	// std::cout << std::format("{}\n", iiii);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	ecs::scene<>();
	ecs::world<>();

	static_assert(meta::param_constains_v<ecs::entity_idx, test_func2> == true);
	meta::param_at<1, test_func2> eeeee;

	using group_t = system_group<system_1, system_2>;
	auto group	  = system_group<system_1, par<system_1, seq<system_1, system_2>, cond<cond_func1, system_2, group_t>>, group_t>();
	// auto group = system_group<system_2>();

	auto tpl2 = meta::func_args<decltype(system_2::test_fu)>::type();
	auto tpl3 = meta::func_args_t<decltype(test_add3)>();

	using tpl		   = tuple_sort<component_comparator, transform, bullet, rigid_body>::type;
	constexpr auto idx = tuple_index<transform, tpl>::value;

	// static_assert(std::same_as<std::tuple<transform, rigid_body, bullet>, tuple_sort<component_comparator, transform, bullet, rigid_body>::type> == false);
	//  static_assert(std::same_as<tuple_sort<component_comparator, transform, bullet, rigid_body>::type, std::tuple<>>);
	//  using components = component_wrapper<transform, bullet, rigid_body>;
	//  components::calc_archetype<transform, bullet>();
	int world_idx = 0;

	auto e2 = world_2.new_entity<transform>();
	auto e3 = world_2.new_entity<rigid_body, transform>();
	auto e4 = world_2.new_entity<bullet>();

	{
		auto test_e1 = world_2.new_entity<>();

		auto test_e2 = world_2.new_entity<transform>();
		auto test_e3 = world_2.new_entity<bullet>();
		auto test_e4 = world_2.new_entity<rigid_body>();

		auto test_e5 = world_2.new_entity<bullet, rigid_body>();
		auto test_e6 = world_2.new_entity<transform, rigid_body>();
		auto test_e7 = world_2.new_entity<transform, bullet>();

		auto test_e8 = world_2.new_entity<transform, bullet, rigid_body>();

		auto& test_transform1 = world_2.get_component<transform>(test_e2);
		auto& test_transform2 = world_2.get_component<transform>(test_e6);
		auto& test_transform3 = world_2.get_component<transform>(test_e7);
		auto& test_transform4 = world_2.get_component<transform>(test_e8);

		auto& test_bullet1 = world_2.get_component<bullet>(test_e3);
		auto& test_bullet2 = world_2.get_component<bullet>(test_e5);
		auto& test_bullet3 = world_2.get_component<bullet>(test_e7);
		auto& test_bullet4 = world_2.get_component<bullet>(test_e8);

		auto& test_rigid_body1 = world_2.get_component<rigid_body>(test_e4);
		auto& test_rigid_body2 = world_2.get_component<rigid_body>(test_e5);
		auto& test_rigid_body3 = world_2.get_component<rigid_body>(test_e6);
		auto& test_rigid_body4 = world_2.get_component<rigid_body>(test_e8);
	}


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