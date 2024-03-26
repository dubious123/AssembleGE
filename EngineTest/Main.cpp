
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

WORLD_BEGIN(world_004)
WORLD_END()


SERIALIZE_SCENE(my_second_scene, world_001, world_002)
//
// SERIALIZE_SCENE(my_third_scene, world_002, world_003)
//  1. scene_wrapper() => scene reflection
//  2. world_lambda()
//		a. connect init lambda
//		b. call world_wrapper constructor => world reflection
//  3. scene_wrapper::value()
//		a. call scene constructor => create scene and world
//		b. call world_wrapper::init(w) => create entity and reflection for entity

// SERIALIZE_SCENE(my_first_scene, world_003)

static inline auto& v = []() -> auto& {
	static data_structure::vector<uint32> s;
	s.emplace_back(2u);
	return s;
}();


auto  s		  = ecs::scene<ecs::world<transform, bullet, rigid_body>, ecs::world<transform, rigid_body>>();
auto  s_2	  = ecs::scene<ecs::world<transform, rigid_body>, ecs::world<transform, rigid_body>>();
auto& world_2 = s.get_world<0>();
auto& world_3 = s_2.get_world<0>();

void test_func(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	DEBUG_LOG("test_func1 : " << a++);
	t.position.x += 1;
}

void test_func2(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	DEBUG_LOG("test_func2 : " << a++);
	t.position.y += 1;
}

void test_func3(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	DEBUG_LOG("test_func3 : " << a++);
	t.position.z += 1;
}

void test_func4(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	DEBUG_LOG("test_func4 : " << a++);
	t.position.x += 1;
}

void test_func5(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	DEBUG_LOG("test_func5 : " << a++);
	t.position.y += 1;
}

void test_func6(ecs::entity_idx idx, transform& t, bullet& b)
{
	static int a = 0;
	DEBUG_LOG("test_func6 : " << a++);
	t.position.z += 1;
}

void test_func7(ecs::entity_idx idx, transform& t, rigid_body& rb)
{
	static int a = 0;
	DEBUG_LOG("test_func7 : " << a++);
	t.position.z += 1;
}

// world a b c d
// archetype a d
// c_idx from world => 0, 3
// c_idx from archetype => 0, 1
//  with archetype 1001 => c_idx for d is 1 ( __popcnz(((1 << (index_of(d) + 1)) - 1) & 1001 )
//  decode 0 => 0, 3 => 1
// from c_idx => component_size, component_offset (4byte)

uint8 get_c_idx(ecs::archetype_t a, uint8 idx)
{
	return __popcnt(((1 << idx) - 1) & a);
}

int main()
{
	std::cout << std::format("{:04x}-{:016x}", 1, -1);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	// reflection::register_entity("entity", world_3.new_entity<transform>(), world_2);
	return 0;
	// a b c d , a => 0
	assert(get_c_idx(0b1111, 0) == 0);
	assert(get_c_idx(0b1111, 1) == 1);
	assert(get_c_idx(0b1111, 2) == 2);
	assert(get_c_idx(0b1111, 3) == 3);


	assert(get_c_idx(0b1110, 1) == 0);
	assert(get_c_idx(0b1110, 2) == 1);
	assert(get_c_idx(0b1110, 3) == 2);

	assert(get_c_idx(0b1101, 0) == 0);
	assert(get_c_idx(0b1101, 2) == 1);
	assert(get_c_idx(0b1101, 3) == 2);

	assert(get_c_idx(0b1011, 0) == 0);
	assert(get_c_idx(0b1011, 1) == 1);
	assert(get_c_idx(0b1011, 3) == 2);

	assert(get_c_idx(0b0111, 0) == 0);
	assert(get_c_idx(0b0111, 1) == 1);
	assert(get_c_idx(0b0111, 2) == 2);


	assert(get_c_idx(0b0011, 0) == 0);
	assert(get_c_idx(0b0011, 1) == 1);

	assert(get_c_idx(0b0101, 0) == 0);
	assert(get_c_idx(0b0101, 2) == 1);

	assert(get_c_idx(0b1001, 0) == 0);
	assert(get_c_idx(0b1001, 3) == 1);

	assert(get_c_idx(0b0110, 1) == 0);
	assert(get_c_idx(0b0110, 2) == 1);

	assert(get_c_idx(0b1010, 1) == 0);
	assert(get_c_idx(0b1010, 3) == 1);

	assert(get_c_idx(0b1100, 2) == 0);
	assert(get_c_idx(0b1100, 3) == 1);

	assert(get_c_idx(0b0001, 0) == 0);
	assert(get_c_idx(0b0010, 1) == 0);
	assert(get_c_idx(0b0100, 2) == 0);
	assert(get_c_idx(0b1000, 3) == 0);


	using namespace ecs;
	using tpl		   = tuple_sort<component_comparator, transform, bullet, rigid_body>::type;
	constexpr auto idx = tuple_index<transform, tpl>::value;
	using components   = component_wrapper<transform, bullet, rigid_body>;
	components::calc_archetype<transform, bullet>();

	auto e2 = world_2.new_entity<transform>();
	auto e3 = world_2.new_entity<rigid_body, transform>();
	auto e4 = world_2.new_entity<bullet>();

	// auto e5 = world_2.new_entity<bullet, transform, rigid_body>();

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

	for (auto i = 0; i < 10000; ++i)
	{

		auto eeeee = world_2.new_entity<transform>();
		world_2.add_component<rigid_body, bullet>(eeeee);
		world_2.add_component<bullet, rigid_body>(world_2.new_entity<transform>());
		world_2.add_component<transform>(world_2.new_entity<bullet, rigid_body>());
		world_2.remove_component<rigid_body>(world_2.new_entity<transform, rigid_body>());
		world_2.remove_component<transform, rigid_body>(world_2.new_entity<bullet, transform, rigid_body>());

		if (i % 7 == 0)
		{
			world_2.delete_entity(eeeee);
		}
	}

	world_2.new_entity<bullet, transform, rigid_body>();
	world_2.new_entity<bullet, transform, rigid_body>();

	world_2.add_component<rigid_body>(e2);

	auto& rb = world_2.get_component<rigid_body>(e2);
	world_2.add_component<bullet>(e2);

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

	// static_assert(variadic_auto_unique<test_func, test_func2, test_func3> == true);
	// static_assert(variadic_unique<int, double, float> == true);


	using pipe1_2 = pipeline<
		par<test_func4, test_func5>,
		seq<test_func2>,
		wt<test_func4, test_func5>>;
	using pipe2_2 = pipeline<
		seq<test_func3>,
		par<test_func6, test_func7>,
		wt<test_func6, test_func7>>;
	using pipe3_2 = pipeline<
		seq<test_func>,
		par<pipe1_2, pipe2_2>,
		seq<test_func>,
		wt<pipe1_2, pipe2_2>>;
	pipe3_2().update(world_2);

	// auto& t = world_2.get_component<transform>(e5);

	auto  list_list = data_structure::list<data_structure::list<int>>();
	auto& list		= list_list.emplace_front();
	auto& list2		= list_list.emplace_back();
	list.emplace_front(4);
	list.emplace_front(3);
	list.emplace_front(2);
	list.emplace_front(1);
	list.emplace_front(0);

	list.emplace_back(5);
	list.emplace_back(6);
	list.emplace_back(7);
	list.emplace_back(8);
	list.emplace_back(9);
	list.emplace_back(10);

	auto* node = list.back();
	node	   = node->prev->prev;
	list.insert(node, 100);
	list.insert(node, 99);
	list.insert(node, 98);
	list.insert(node, 97);

	list.pop_back();
	list.pop_back();

	list.pop_front();
	list.pop_front();

	list.erase(node);

	list2.emplace_front(4);
	list2.emplace_front(3);
	list2.emplace_front(2);
	list2.emplace_front(1);
	list2.emplace_front(0);

	list2.emplace_back(5);
	list2.emplace_back(6);
	list2.emplace_back(7);
	list2.emplace_back(8);
	list2.emplace_back(9);
	list2.emplace_back(10);

	auto* node2 = list2.back();
	node2		= node2->prev->prev;
	list2.insert(node2, 100);
	list2.insert(node2, 99);
	list2.insert(node2, 98);
	list2.insert(node2, 97);

	list2.pop_back();
	list2.pop_back();

	list2.pop_front();
	list2.pop_front();

	list2.erase(node2);


	// vector<chunk_entry*> __v;
	//__v.emplace_back();
}