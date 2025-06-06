
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

#define NOMINMAX
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

#pragma comment(lib, "engine.lib")


#include <cstdlib>
#include <crtdbg.h>
#include <Sysinfoapi.h>
#include <source_location>
#include <print>
#include <string>
#include <variant>

#include <future>

#include "test.h"
#include "system_test.h"
// #include <type_traits>
// #include <sstream>
// #include <format>
// #include <cstdlib>
// #include <ctime>
// #include <chrono>
import std;

using namespace data_structure;

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


SYS_BEGIN(sys_2)

void on_system_begin(auto& world)
{
}

void on_thread_begin(auto& world) { }

void update(auto& world, ecs::entity_idx e_idx, transform& t, rigid_body& v) { };

// void update(ecs::entity_idx e_idx, transform& t, rigid_body& v) {};

void on_thread_end() { }

void on_system_end(auto& world) { }

SYS_END()

// struct sys_2
//{
//   private:
//	typedef sys_2 __detail_self;
//
//   public:
//	sys_2(const sys_2& other)			 = delete;
//	sys_2& operator=(const sys_2& other) = delete;
//	sys_2(sys_2&& other)				 = delete;
//	sys_2& operator=(sys_2&& other)		 = delete;
//
//   private:
//	static inline const char* __detail__struct_name = []() {
//		reflection::register_struct("transform2", 2, reflection::malloc_struct<transform>());
//		return "transform2";
//	}();
// };

struct system_1

{
	int* p_int;

	void on_system_begin(auto& world)
	{
	}

	void on_thread_begin(auto& world) { }

	void update(auto& world, ecs::entity_idx e_idx, transform& t, rigid_body& v) { };
	void update2(ecs::entity_idx e_idx, transform& t, rigid_body& v) { };

	// void update(ecs::entity_idx e_idx, transform& t, rigid_body& v) {};

	void on_thread_end() { }

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

	system_1(const system_1& other)			   = delete;
	system_1& operator=(const system_1& other) = delete;
	system_1(system_1&& other)				   = delete;
	system_1& operator=(system_1&& other)	   = delete;

	void f(int, int) { }
};

struct system_2
{
	// void update_w(auto& world, transform& t, bullet& v) {};
	void update(transform& t, bullet& v) { };

	static void test_fu(int a, int b) { }
};

void test_add3(int a, int b) { }

using namespace ecs;

// SCENE_BEGIN(new_scene_3)
static inline auto& new_scene_3 =
	[]() -> auto& { return reflection::scene_wrapper<"new_scene_3">::init(
						0
						//__WORLD_BEGIN(new_world_2, transform)
						,
						[]() { using w_wrapper = reflection::world_wrapper<"new_world_2", transform>; w_wrapper::init_func = [](ecs::world_base& world) -> void { using world_t = ecs::world<transform>; reflection::register_world("new_world_2", world);  reflection::register_component_to_world(transform::id);;
//____ENTITY_BEGIN(new_entity)
			{
				auto entity = ((world_t&)world).new_entity<>();
				reflection::register_entity("new_entity", entity, world);
//____ENTITY_END()
			}
//____ENTITY_BEGIN(new_entity2, transform)
			{
				auto entity = ((world_t&)world).new_entity<transform>();
				reflection::register_entity("new_entity2", entity, world);
//______SET_COMPONENT(transform, .position.x, 100.f)
				((world_t&)world).get_component<transform>(entity).position.x = 100.f;
//____ENTITY_END()
			}
//__WORLD_END()
}; return w_wrapper(); }()
						// SCENE_END()
					); }();

// SYS_GROUP_BEGIN(sys_group_name)
// __SEQ(system_1)
// __PAR(system_1, other_sys_group)
// __COND(cond_system_1, system_1, system_2)
// SYS_GROUP_END()

// option 1 : system in world
//__WORLD_BEGIN(world_0)
//____SYSTEM_BEGIN()
//______SEQ(system_1)
//______PAR(system_group1, system_2)
//____SYSTEM_END()
//__WORLD_END()

// => scene.perform()

// option 2 : system out of world
// SYS_GROUP_BEGIN(sys_group_name)
//  => lambda returning an instance of sys_group
//  => templated functions, cannot be called.
//  => all interfaces are called inside system_group::perform(auto&& world) functions
//  => editor_functions are called inside system_group::perform functions
//  => editor_functions are injected. how?
//  => expose function pointer and pass a function pointer after.
// sys_scene_perform_begin(scene_name)
// world_perform(world_name, sys_group)
//

// => function_enter, function exist : can be injected
// => function name??
// => std::source_location::current()
// => function_enter only count enter tick
// => perform => returns function name and lambda (or function pointer)
// => function_exist count exist tict and print

// => or when editor, each system has a name
// => each function call, we know the function name
// => hard-code

// => tick count should be done on each system instance
// => each system has an id (0 ~ instances)
// => static initialization is single threaded so each id is unique
// => use array

// => on_thread_begin/end, _update_entity : par thread
// => how?
// => lock + counter?

// => how to run code from editor?
// => get current scene
// => all scenes should be in one place (in a array)
// => all pass the function pointer to editor; (better)
// => editor::run(current_scene_ecs_idx)

// => how to move from one scene to another scene?
// => need upper layer?
// => call move_scene(scene_idx) : impossible
// => call game::move_scene<some_scene_type>() : how?
//
// => vector<variant<all scene types...>> + concept
//
//
// => each world has a pointer to scene_base

// Game Loop
// game_init
// while(game_running)
//	scene_init
//	while(scene_running)
//		scene_perform
//	scene_deinit
// game_deinit

// game::set_current_scene
// game::move_scene

// 1. system : func/lambda vs struct vs both
// if func/lambda (easier to use) how expose them to editor?
// using macro
// FUNC(name_to_editor, function pointer, argumentss...)
// FUNC(function pointer, arguements...) => name == function pointer
// if function or lambda... is allowed, any system call inside function or lambda is lost.

// 2. game in scene as a member variable?
// O : no more templates functions, no more scene_base
// X : ?

// 3. scene macro vs template
// macro : more flexable, same world different type
// template : same world same type ... nah...

// 4. system in the scene, out of scene
// in :
//	different scene size
//	no more node
//	X
// out :
//	O
//
//
//


//

// expose to editor
// ???

template <template <typename> typename t>
struct test_temp
{
	template <typename k>
	using real_type = t<k>;
};

struct system4
{
	void update(auto&& world)
	{
		std::println("func name : {}", std::source_location::current().function_name());
		std::println("line : {}", std::source_location::current().line());
		std::println("column : {}", std::source_location::current().column());
	}
};

template <typename... scene_ts>
struct scene_types_container
{
	using tpl_t = std::tuple<scene_ts...>;
	using v_t	= std::variant<scene_ts...>;
	std::array<std::variant<scene_ts...>, sizeof...(scene_ts)> arr{ v_t{ scene_ts() }... };
};

//
// game(scene_t1, scene_t2, scene_t3)

#define SCENE_TYPE_ENUM(scene_type) e_##scene_type

#define SCENE_TYPE_ENUMS(...)                      \
	enum e_scene_type_                             \
	{                                              \
		FOR_EACH_ARG(SCENE_TYPE_ENUM, __VA_ARGS__) \
	};

#define SCENE_INSTANCE(scene_type) \
	scene_type _##scene_type;


#define SCENE_INSTANCES(...) \
	FOR_EACH(SCENE_INSTANCE, __VA_ARGS__)

#define SCENE_CASE(scene_type) \
	case e_##scene_type:       \
	{                          \
		_##scene_type.foo();   \
		break;                 \
	}

#define SCENE_CASES(...) \
	FOR_EACH(SCENE_CASE, __VA_ARGS__)


#define GAME(...)                            \
	struct game                              \
	{                                        \
		SCENE_TYPE_ENUMS(__VA_ARGS__)        \
		SCENE_INSTANCES(__VA_ARGS__)         \
		int	 _current_scene_idx;             \
		bool _game_running;                  \
                                             \
		void run()                           \
		{                                    \
			while (_game_running)            \
			{                                \
				switch (_current_scene_idx)  \
				{                            \
					SCENE_CASES(__VA_ARGS__) \
				}                            \
			}                                \
		}                                    \
	};

// GAME(scene_t1, scene_t2, scene_t3)
//
// struct a
//{
//   private:
//	static const a detail_a;
//
//   public:
//	float3 ab;
// };
//
// struct game2
//{
//	enum e_scene_type
//	{
//		e_scene_t1,
//		e_scene_t2,
//		e_scene_t3,
//	};
//
//	// too large space
//	scene_t1 scene_scene_t1;
//	scene_t2 scene_scene_t2;
//	scene_t3 scene_scene_t3;
//
//	scene_base* _p_scenes[3] { nullptr };
//	bool		_scene_loaded[3] { false };
//	uint32		_current_scene_idx = -1;
//	uint32		_next_scene_idx;
//	bool		_game_running;
//
//	game2(uint32 start_scene_idx) : _next_scene_idx(start_scene_idx), _game_running(true)
//	{
//		// new (_scenes + e_scene_t1) scene_t1();
//		// new (_scenes + e_scene_t1) scene_t1();
//		// new (_scenes + e_scene_t1) scene_t1();
//	}
//
//	game2(const game2& other)			 = delete;
//	game2(game2&& other)				 = delete;
//	game2& operator=(const game2& other) = delete;
//	game2& operator=(game2&& other)		 = delete;
//
//   public:
//	void end()
//	{
//		_game_running = false;
//	}
//
//	void load_scene(uint32 load_scene_idx)
//	{
//	}
//
//	bool scene_loaded(uint32 scene_idx)
//	{
//	}
//
//	void init();
//
//	void run()
//	{
//		while (_game_running)
//		{
//			if (_current_scene_idx != _next_scene_idx)
//			{
//			}
//
//			switch (_current_scene_idx)
//			{
//			case e_scene_t1:
//			{
//				scene_scene_t1.foo();
//				break;
//			}
//			case e_scene_t2:
//			{
//				static_assert(std::is_same_v<game2&, decltype(*this)>);
//				scene_scene_t2.run<game2>(*this);
//				break;
//			}
//			case e_scene_t3:
//			{
//				scene_scene_t3.foo();
//				break;
//			}
//			}
//		}
//	}
//
//	void deinit();
// };

struct some_big_struct
{
	uint64 a;
	uint64 b;
	uint64 c;
	uint64 d;

	template <typename... t_data>
	decltype(auto) run(t_data&&... arg)
	{
		return false;
	}
};

template <typename t_game>
struct interface_game2
{
	t_game game;

	interface_game2(t_game&& game) : game(std::forward<t_game>(game)) { }
};

struct foo
{
	int a;
};

struct boo : foo
{
	int b;
};

template <typename t1>
struct match_type
{
	template <typename t_elem>
	struct pred
	{
		static constexpr bool value = std::is_same_v<t1, t_elem>;
	};
};

template <typename t1, typename t2>
struct comparator : std::integral_constant<bool, (alignof(t1) > alignof(t2))>
{
};

template <typename t>
struct tag
{
	using type = t;
};

template <typename t1, typename t2>
struct align_comparator : std::integral_constant<bool, (t1::alignment < t2::alignment)>
{
};

template <std::size_t i1, std::size_t i2>
struct item
{
	static constexpr std::size_t l = i1;
	static constexpr std::size_t r = i2;
};

using unsorted = std::tuple<item<3, 1>, item<3, 0>, item<1, 2>, item<2, 3>, item<1, 4>>;
// item<3, 1> , item<1, 2>, item<2, 3>, item<1, 4>
// item<3, 1> , item<2, 3>, item<1, 4>  item<1, 2>, item<1, 4>
using sorted_stable	  = std::tuple<item<3, 1>, item<3, 0>, item<2, 3>, item<1, 2>, item<1, 4>>;
using sorted_unstable = std::tuple<item<3, 0>, item<3, 1>, item<2, 3>, item<1, 4>, item<1, 2>>;

// cond<a,b>

template <typename A, typename B>
struct item_comparator_descend_unstable
{
	static constexpr bool value = (A::l <= B::l);
};

template <typename A, typename B>
struct item_comparator_descend
{
	static constexpr int value = []() {
		if constexpr (A::l < B::l)
		{
			return -1;
		}
		else if constexpr (A::l == B::l)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}();
};

int main()
{
	{
		// meta::print_type<tuple_sort_stable_t<item_comparator_descend, unsorted>>();
		// meta::print_type<tuple_sort_t<item_comparator_descend_unstable, unsorted>>();
		static_assert(meta::index_sequence_front_v<std::index_sequence<4, 2, 3>> == 4);
		static_assert(std::tuple_size<std::array<int, 10>>::value == 10);

		static_assert(std::is_same_v<tuple_sort_stable_t<item_comparator_descend, unsorted>, sorted_stable>);
		// static_assert(std::is_same_v<tuple_sort_t<item_comparator_descend, unsorted>, sorted_unstable>);
		//   meta::tuple_sort_t<comparator, uint8, uint16, uint32, uint64, int8, int16, int32, int64>;


		// print_type<meta::tuple_sort_t<comparator, uint8>>();

		using t_random = uint8;
		static_assert(meta::find_index_impl<match_type<uint8>::pred, uint8, uint16, uint32>::value == 0);
		static_assert(match_type<uint8>::pred<uint8>::value);
		static_assert(meta::find_index_tuple_v<match_type<uint8>::template pred, std::tuple<uint8, uint16, uint32>> == 0);
		// using t_temp = ecs::utility::aligned_layout_info<7, tag<uint8>, tag<uint16>, tag<uint32>>;
		// auto i		 = t_temp::total_size();
		// i			 = 2;
		//  print_type<std::tuple_element_t<0, t_temp::__detail::tpl_sorted>>();
		//      t_temp::offset_of<uint8>();

		// using t_tmp2 = t_temp::with<uint32, 10>;

		// t_tmp2::offset_of<uint8>();

		static_assert(std::is_trivial_v<foo>);
		static_assert(std::is_trivial_v<boo>);

		using temp = ecs::utility::layout_builder<tag<int>, tag<float>>::template with_n<tag<double>, 3> /*::build<0, 4096>*/;

		// print_type<ecs::utility::aligned_layout_info_builder<tag<int>, tag<float>, tag<double>>::build<4096>>();

		// constexpr auto off = temp::offset_of<tag<int>>();
		//   static_assert(std::is_same_v<typename meta::pop_back<std::tuple<int, float, double>>::type, std::tuple<int, float>>);
		//     static_assert(std::is_same_v<meta::pop_back_t<int, float, double>, std::tuple<int, float>>);
		//      using layout_info = ecs::utility::aligned_layout_info<uint8, uint16, t_random, uint64, uint32>;

		// auto s = std::tuple_size_v<layout_info::tpl_sorted>;
		// static_assert(std::tuple_size_v<layout_info::tpl_sorted> == 5);

		// static_assert(layout_info::template offset_of<uint64>() == 0);
		// static_assert(layout_info::template offset_of<uint32>() == 8);
		// static_assert(layout_info::template offset_of<uint16>() == 12);
		// static_assert(layout_info::template offset_of<uint8>() == 14);
		// static_assert(layout_info::template offset_of<t_random>() == 14);

		// layout_info::offset_of<uint32>();
		//  static_assert(std::is_same_v<decltype(tpl), std::tuple<uint8, uint16, uint32, uint64>>);
	}

	auto _game = my_game();

	// print_type<meta::tuple_cat_t<>>();
	using t_entity_id = uint32;
	// auto it_num			  = 1000'0000;
	auto it_num			  = 0;
	auto benchmakr_offset = 0;
	{
		std::mt19937					gen(19990827);
		std::uniform_int_distribution<> dist(0, 7);

		auto ent_count = 1;
		auto time_now  = std::chrono::high_resolution_clock::now();
		for (auto rand_prev = dist(gen); auto i : std::views::iota(0, it_num))
		{
			auto rand_curr = dist(gen);
			switch (rand_curr / 3)
			{
			case 0:
			{
				--ent_count;
				[[fallthrough]];
			}
			case 1:
			{
				// new
				switch (rand_prev)
				{
				case 0:
				{
					ent_count += 1;
					break;
				}
				case 1:
				{
					ent_count += 3;
					break;
				}
				case 2:
				{
					ent_count += 109;
					break;
				}
				case 3:
				{
					ent_count -= 200;
					break;
				}
				case 4:
				{
					ent_count += 20;
					break;
				}
				case 5:
				{
					ent_count += 80;
					break;
				}
				case 6:
				{
					ent_count -= 1002;
					break;
				}
				case 7:
				{
					ent_count += 302;
					break;
				}
				default:
					break;
				}
				++ent_count;
			}
			case 2:
			{
				if (ent_count >> 2 & 1)
				{
					ent_count ^= ent_count << 13;
				}
				else
				{
					ent_count ^= ent_count >> 13;
				}
				if (ent_count & 0x1000)
				{
					ent_count ^= ent_count >> 17;
				}
				else
				{
					ent_count ^= ent_count << 17;
				}
				if (ent_count & 0x1000'0000)
				{
					ent_count ^= ent_count >> 19;
				}
				else
				{
					ent_count ^= ent_count << 19;
				}
			}
			default:
				break;
			}

			rand_prev = rand_curr;
		}

		auto du = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - time_now);
		std::println("duration : {}", du);
		std::println("{}", ent_count);
	}

	//
	auto ecs_benchmark = 1;
	{
		std::mt19937					gen(19990827);
		std::uniform_int_distribution<> dist(0, 7);

		auto   b		 = ecs::entity_storage::basic<uint32, transform, bullet, rigid_body>();
		uint32 ent		 = b.new_entity<>();
		auto   ent_count = 1;
		auto   time_now	 = std::chrono::high_resolution_clock::now();
		for (auto rand_prev = dist(gen); auto i : std::views::iota(0, it_num))
		{
			auto rand_curr = dist(gen);
			switch (rand_curr / 3)
			{
			case 0:
			{
				// remove
				b.remove_entity(ent);
				--ent_count;
				[[fallthrough]];
			}
			case 1:
			{
				// new
				switch (rand_prev)
				{
				case 0:
				{
					ent = b.new_entity<>();
					break;
				}
				case 1:
				{
					ent = b.new_entity<transform>();
					break;
				}
				case 2:
				{
					ent = b.new_entity<bullet>();
					break;
				}
				case 3:
				{
					ent = b.new_entity<rigid_body>();
					break;
				}
				case 4:
				{
					ent = b.new_entity<transform, bullet>();
					break;
				}
				case 5:
				{
					ent = b.new_entity<bullet, rigid_body>();
					break;
				}
				case 6:
				{
					ent = b.new_entity<rigid_body, transform>();
					break;
				}
				case 7:
				{
					ent = b.new_entity<rigid_body, bullet, transform>();
					break;
				}
				default:
					break;
				}
				++ent_count;
			}
			case 2:
			{
				// revert
				if (b.has_component<transform>(ent))
				{
					b.remove_component<transform>(ent);
				}
				else
				{
					b.add_component<transform>(ent);
				}
				if (b.has_component<rigid_body>(ent))
				{
					b.remove_component<rigid_body>(ent);
				}
				else
				{
					b.add_component<rigid_body>(ent);
				}
				if (b.has_component<bullet>(ent))
				{
					b.remove_component<bullet>(ent);
				}
				else
				{
					b.add_component<bullet>(ent);
				}
			}
			default:
				break;
			}

			rand_prev = rand_curr;
		}

		auto du = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - time_now);
		std::println("duration : {}", du);
		if (ent_count != b.entity_count())
		{
			throw std::exception();
		}
		assert(ent_count == b.entity_count());
		b.remove_entity(b.new_entity<transform, bullet, rigid_body>());
		b.add_component<transform, bullet>(b.new_entity<>());
		b.remove_component<transform>(b.new_entity<transform, bullet, rigid_body>());
		b.has_component<rigid_body>(b.new_entity<transform, bullet>());
		b.get_component<rigid_body>(b.new_entity<rigid_body>());
	}

	auto ecs_temp = 2;
	{
		auto b = ecs::entity_storage::basic<uint32, transform, bullet, rigid_body>();
		b.new_entity<transform, bullet>();

		// some_group_system = [](ecs::ent_group_view<transform, bullet> group_view) { ..., return group_view; };
		// some_each_system  = [](ecs::ent_view<transform, bullet> ent_view) { };
		//
		////ecs::each_group : from storage -> get ent_group_view, and iterate them
		////ecs::each_entity : from ent_group_view -> get ent_view, and iterate them
		// b | ecs::each_group<transform, bullet>(some_system) | ecs::each_entity<>();

		using namespace ecs::system::test;

		using ecs::system::test::operator|;

		auto sys = seq{
			sys_game_init{},
			par{
				// sys_game_deinit{},
				sys_game_init{},
				[](auto&& _) { std::println("hello"); },
				pipe{ [](auto&& _) { std::println("hello"); return 1; }, [](auto&& _) { std::println("{}", _ + 1); } },
			},
			[](auto&& _) { std::println("hello"); return 1; } | [](auto&& _) { std::println("{}", _ + 1); return _ + 1; } | [](auto&& _) { std::println("{}", _ + 1); return _ + 1; },
			cond{ [](auto&& _) { return true; }, [](auto&& _) { std::println("true"); return 1; }, [](auto&& _) { std::println("false"); return 2; } }
				| cond{ [](auto&& _) { return true; }, [](auto&& _) { std::println("true"); } }
				| cond{ []() { return false; }, []() { std::println("false"); } },
			[](auto) {},

			[] { return 10; }
				| loop{ [](auto&& i) { return i-- > 0; },
						[](auto&& i) { std::println("i : {}", i); },
						continue_if{ [](auto i) { return i % 2; } },
						par{
							[](auto&&) { Sleep(100); std::println("hi-1"); },
							[]() { Sleep(200); std::println("hi-2"); } },
						break_if{ [](auto i) { return i == 5; } } },

			sys_game_deinit{},
			sys_game_init{},
			sys_game_deinit{},

			match{
				[] { return 0; },
				on<0> = sys_game_deinit{} | sys_game_init{},
				on<1> = sys_game_deinit{},
				on<2> = sys_game_deinit{},
				on<3> = sys_game_deinit{},
				default_to = [] { std::println("default"); } },

		};
		auto tpl1 = sys(_game);
		int	 a	  = 1;
		// print_type<decltype(tpl1)>();

		// print_type<decltype(t_test{}.systems)>();
		//     auto sys_game =
		//	my_game
		//	| seq{
		//		  system_1{},
		//		  system_2{},
		//		  system_2{},
		//		  par{
		//			  system_1{},
		//			  system_2{},
		//			  system_1{} }
		//			  .sync{},

		//		  par{ system_1{} },
		//		  par{ system_2{} },
		//		  par{ system_1{} },
		//		  system_1{} | system_2{} | system_1{},
		//		  sync{},
		//		  match{ sys_game_running{},
		//				 on<1>{},
		//				 default_to{} },
		//		  sys_get_world_1{} | for_each_entity{ some_entity_sys{} },
		//		  sys_get_world_1{} | for_each_group{ some_group_sys_begin{}, for_each_entity{ some_entity_sys{} }, some_group_sys_end{} },
		//		  sys_get_world_1{}
		//			  | for_each_group{
		//				  concat_data{ some_group_sys{} }
		//					  .seq{
		//						  get_data<1>{} | mem_func(&some_group_sys::_begin),
		//						  get_data<0>{} | for_each_entity(some_entity_sys{}),
		//						  get_data<some_group_sys>{} | mem_func{ &some_group_sys::_end } } },
		//		  sys_deinit{}
		//	  };
		// sys_game();
	}

	using namespace ecs::system;
	{
		using namespace ecs::system::op;

		auto sys_world_0_builder =
			[]() {


			};


		auto sys_scene_0_builder =
			[] {
				return ([](auto&& scene) -> decltype(auto) { return scene.get_world<world_t1>(); });
			};

		auto sys_game = my_game{}
					  + sys_game_init{}
					  + (sys_get_scene<scene_t1>{} | sys_scene_0_init_builder())
					  + loop(sys_game_running{},
							 match([](auto&& game) { return game.current_scene_idx; },
								   on<0>(sys_get_scene<scene_t1>{} | sys_scene_0_builder()),
								   default_to([]() { std::println("not valid scene_idx"); })))
					  + sys_game_deinit{};

		// sys_game.run();
	}

	//{
	//	auto m = match(
	//		[]() { return 1; },
	//		on<0>([]() { std::println("0"); }),
	//		on<1>([]() { std::println("1"); }),
	//		on<2>([]() { std::println("2"); }),
	//		default_to([]() { std::println("default"); }));
	//	m.run();

	//	std::println("on_size : {}", sizeof(on([](auto i) { return 4 == i; }, []() { std::println("0"); })));
	//	std::println("default_size : {}", sizeof(default_to([]() { std::println("default"); })));
	//	std::println("match_size : {}", sizeof(m));
	//}

	//{
	//	auto m = match(
	//		[](auto&& _) { return 1; },
	//		on<0>([](auto&& _) { std::println("0"); }),
	//		on<1>([](auto&& _) { std::println("1"); }),
	//		on<2>([](auto&& _) { std::println("2"); }),
	//		default_to([](auto&& _) { std::println("default"); }));
	//	_run_sys(m, _game);

	//	int	 idx = 0;
	//	auto m2	 = match(
	//		 [](auto&& _) { return 5; },
	//		 on<0>([](auto&& _) { std::println("0"); }),
	//		 on<1>([](auto&& _) { std::println("1"); }),
	//		 on<2>([](auto&& _) { std::println("2"); }),
	//		 on<3>([&idx](auto&& _) { ++idx; }),
	//		 default_to([](auto&& _) { std::println("default"); }));
	//	_run_sys(m2, _game);
	//	std::println("match_size : {}", sizeof(m2));
	//}

	//{
	//	using namespace ecs::system::op;
	//	auto idx   = 15;
	//	auto ll	   = []() { std::println("2"); } | [&]() { idx--; };
	//	auto tpl_0 = meta::make_filtered_tuple<meta::is_not_empty>(on<0>([](auto&& _) { std::println("0"); }),
	//															   on<1>([](auto&& _) { std::println("1"); }),
	//															   on<2>([](auto&& _) { std::println("2"); }));
	//	// print_type<decltype(tpl_0)>();
	//	auto tpl  = ecs::make_filtered_tuple<meta::is_not_empty>([]() { std::println("2"); } | [] {} | [&]() { idx--; });
	//	auto tpl2 = ecs::make_filtered_tuple<std::is_empty>([]() { });
	//	// print_type<decltype(tpl2)>();

	//	auto l = loop([]() { return true; },
	//				  []() { std::println("0"); }
	//					  + []() { std::println("1"); },
	//				  ([]() { std::println("2"); } | [&]() { idx--; }),
	//				  continue_if([&]() { std::println("idx == {}", idx); } | [&]() { return idx % 2 == 0; }),
	//				  break_if([&idx]() { return idx == 1; }));
	//}

	//{

	//	using namespace ecs::system::op;

	//	// static_assert(par_exec_found_v<my_game> == false);
	//	// clang-format off2
	//	{
	//		auto idx = 15;
	//		auto sys_group =
	//			sys_game_init{}
	//			+ ([]<typename g>(interface_game<g> igame) -> decltype(auto) { return igame.get_scene<scene_t1>(); }
	//															  | sys_scene_init{}
	//																	+ sys_scene_init{})
	//			+ cond([](auto&& game) { return game.running; }, []() { std::println("running"); },
	//				   (sys_game_init{}
	//					^ sys_game_init{}))
	//			+ ([]() { std::println("0 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("1 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("2 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("3 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("4 running on {}", std::this_thread::get_id()); }
	//			   //^ecs::loop([]() { return true; },
	//			   //			 []() { std::println("0"); }
	//			   //				+ []() { std::println("1"); }
	//			   //				+ ([]() { std::println("2"); } | [&](){idx--;}),
	//			   //			 ecs::continue_if([&](){std::println("idx == {}", idx);} | [&](){return idx % 2 == 0; }),
	//			   //			 ecs::break_if([&idx]() { return idx == 1; }))
	//			   )
	//			+ sys_game_init{};


	//		sys_group.run(_game);
	//		std::println("size : {}", sizeof(sys_group));
	//		std::println("====================================");
	//	}
	//	{
	//		auto sys_group =
	//			sys_game_init{}
	//			+ sys_game_init{}
	//			+ sys_non_templated{}
	//			+ cond([](auto&& game) { return game.running; }, []() { std::println("running"); })
	//			+ ([]<typename g>(interface_game<g> igame) -> decltype(auto) { return igame.get_scene<scene_t1>(); } | sys_scene_init{})
	//			+ ([]<typename g>(interface_game<g> igame) -> decltype(auto) { return igame.get_scene<scene_t2>(); }
	//															  | (sys_scene_init{}
	//																 + sys_scene_init{}))
	//			+ ([]() { std::println("0 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("1 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("2 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("3 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("4 running on {}", std::this_thread::get_id()); })
	//			+ []() { std::println("empty"); }
	//			+ sys_game_deinit{};

	//		sys_group.run(_game);
	//		std::println("size : {}", sizeof(sys_group));
	//		std::println("====================================");
	//	}

	//	{
	//		auto sys_group =
	//			_game
	//			+ sys_game_init{}
	//			+ cond([](auto&& game) { return !game.running; }, []() {},
	//				   []() { std::println("empty"); }
	//					   + ([]() { return my_game{}; } | []<typename g>(interface_game<g> igame) -> decltype(auto) { return igame.get_scene<scene_t1>(); } | sys_scene_init{})
	//					   + ([]<typename g>(interface_game<g> igame) -> decltype(auto) { return igame.get_scene<scene_t2>(); }
	//																		 | (sys_scene_init{}
	//																			+ sys_scene_init{}))
	//					   + ([]() { std::println("0 running on {}", std::this_thread::get_id()); }
	//						  ^ []() { std::println("1 running on {}", std::this_thread::get_id()); }
	//						  ^ []() { std::println("2 running on {}", std::this_thread::get_id()); }
	//						  ^ []() { std::println("3 running on {}", std::this_thread::get_id()); }
	//						  ^ []() { std::println("4 running on {}", std::this_thread::get_id()); })
	//					   + sys_non_templated{}
	//					   + sys_game_deinit{})

	//			;
	//		sys_group.run();
	//		std::println("size : {}", sizeof(sys_group));
	//		std::println("====================================");
	//	}

	//	{
	//		auto l_1 = [](auto&& _) { std::println("empty1"); };
	//		auto l_2 = [](auto&& _) { std::println("empty2"); };
	//		static_assert(std::is_same_v<meta::filtered_tuple_t<meta::is_not_empty, my_game, decltype(l_1)&, decltype(l_2)&>,
	//									 std::tuple<my_game, decltype(l_1)&, decltype(l_2)&>>);

	//		// print_type<decltype(make_non_empty_sys_tpl_from_tpl(std::tuple<my_game, decltype(l_1)&, decltype(l_2)&>(my_game{}, l_1, l_2)))>();
	//		// static_assert(std::is_same_v<decltype(make_non_empty_sys_tpl_from_tpl(std::tuple<my_game, decltype(l_1)&, decltype(l_2)&>(my_game{}, l_1, l_2))),
	//		//	std::tuple<my_game, decltype(l_1)&, decltype(l_2)&>>);

	//		auto sys_group =
	//			my_game{}
	//			+ l_1
	//			+ l_2
	//			+ [](auto&& _) { std::println("empty1"); }
	//			+ [](auto&& _) { std::println("empty2"); }
	//			+ ([]() { return my_game{}; } | []<typename g>(interface_game<g> igame) -> decltype(auto) { return igame.get_scene<scene_t1>(); } | sys_scene_init{})
	//			+ ([]() { std::println("0 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("1 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("2 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("3 running on {}", std::this_thread::get_id()); }
	//			   ^ []() { std::println("4 running on {}", std::this_thread::get_id()); })
	//			+ ([]<typename g>(interface_game<g> igame) -> decltype(auto) { return igame.get_scene<scene_t2>(); }
	//															  | (sys_scene_init{}
	//																 + sys_scene_init{}))
	//			//+ ([]<typename g>(interface_game<g> igame ) -> decltype(auto){return igame.get_scene<scene_t1>(); }
	//			//    				+ sys_scene_init{}
	//			//    			    + sys_scene_init{})

	//			/*+= []<typename g>(interface_invalid<g> should_not_build){ should_not_build.invalid(); }*/
	//			+ sys_non_templated{}
	//			+ sys_game_init{};

	//		//| [](){std::println("empty");}
	//		//| sys_game_deinit{};
	//		sys_group.run();
	//		std::println("size : {}", sizeof(sys_group));
	//		std::println("real size : {}", sizeof(sys_group) - sizeof(my_game));
	//		std::println("====================================");
	//	}
	//	{
	//		auto sys_group =
	//			/*[](){std::println("first"); }*/
	//			my_game{}
	//			+ [](auto&& _) { std::println("empty1"); }
	//			+ []() { return 5; }
	//			+ ([]() { return my_game{}; } | []<typename g>(interface_game<g> igame) -> decltype(auto) { return igame.get_scene<scene_t1>(); } | sys_scene_init{})
	//			+ ([]<typename g>(interface_game<g> igame) -> decltype(auto) { return igame.get_scene<scene_t2>(); }
	//															  | (sys_scene_init{}
	//																 + sys_scene_init{}))
	//			+ [](auto&& _) { std::println("empty2"); }
	//			+ []<typename g>(interface_game<g> igame) -> decltype(auto) { return igame.get_scene<scene_t1>(); }
	//															 + ([]() { std::println("0 running on {}", std::this_thread::get_id()); }
	//																^ []() { std::println("1 running on {}", std::this_thread::get_id()); }
	//																^ []() { std::println("2 running on {}", std::this_thread::get_id()); }
	//																^ []() { std::println("3 running on {}", std::this_thread::get_id()); }
	//																^ []() { std::println("4 running on {}", std::this_thread::get_id()); })
	//															 + sys_non_templated{}
	//		//+= []<typename g>(interface_invalid<g> should_not_build){ should_not_build.invalid(); }
	//		/*+= sys_game_init{}*/;
	//		//| sys_game_init{}
	//		//| [](){std::println("empty");}
	//		//| sys_game_deinit{};
	//		sys_group.run();
	//		std::println("size : {}", sizeof(sys_group));
	//		std::println("real size : {}", sizeof(sys_group) - sizeof(my_game));
	//		std::println("====================================");
	//		// sys_group.run(my_game{});
	//	}


	//	////// clang-format on
	//}


	//       auto _sys_group_game2 =
	//	_game
	//	+= sys_game_init {} // seq ([](){return _game});
	//		| sys_1			// pipe ( sys_game_init, sys_1) : 1
	//		| sys_2			// pipe ( sys_game_init, sys_1, sys_2) : 2
	//		| [](){}		// pipe ( sys_game_init, sys_1, sys_2, [](){} ) : 3
	//	+= sys_1
	//		| sys_2
	//		| sys_3
	//	+= loop(sys_game_running {},
	//		switch(sys_current_scene {},
	//			case(0, sys_get_scene<scene_t1> {}
	//						+= sys_scene_init{}
	//						^= sys_get_world<world_t1> {}
	//							| sys_group_world_t1 {},
	//						^= sys_get_world<world_t2> {}
	//							| sys_group_world_t1 {}
	//						+= sys_scene_deinit{} )
	//			case(1, sys_get_scene<scene_t2> {}
	//						^= sys_get_world<world_t1> {}
	//							| sys_group_world_t1 {},
	//						^= sys_get_world<world_t2> {}
	//							| sys_group_world_t1 {})
	//	+= sys_game_deinit {}
	//		| sys_2
	//		| sys_3;


	// static_assert(ecs::detail::is_system<decltype(_sys_group_game2)> == true, "");
	// ecs::detail::run_system(_sys_group_game2);
	//_sys_group_game2.run();
	// static_assert(ecs::detail::is_system_templated<sys_game_init, my_game>, "");
	//_sys_group_game.run(_game);
	////_bind<my_scene_system_0, []<typename g>(interface_game<g> igame, interface_init<g> i_init) { i_init.init(); return igame.get_scene<scene_t1>(); }>().run(&_game);

	//_sys_group_game.run(&_game);
	//  decltype([]<typename g>(interface_game<g> igame) { return igame.get_scene<scene_t1>(); }) lambda;
	//_bind<my_scene_system_1, decltype([]<typename g>(interface_game<g> igame) { return igame.get_scene<scene_t1>(); })>();
	//   loop<&game2::run>();
	// using traits  = function_traits<decltype(&system_1::update2)>;
	// using traits2 = function_traits<decltype(&system_1::update<int>)>;

	// meta::param_at<1, decltype(test_func2)> eeeee;
	using trats = meta::function_traits<&system_1::f>;
	// meta::function_traits<decltype(system_1::update2)> eee;


	// param_at<1, decltype(&system_1::update2)>::type;
	int a;
	system4().update(a);


	static_assert(std::is_same_v<ecs::entity_idx, ecs::entity_idx>);
	// meta::param_at<1, system_1::template update<int>>::type;
	// meta::param_at<0, &system_1::update2>::type;

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	auto d = rigid_body::id;


	auto s1 = ecs::scene<world_t1>();

	auto& w1 = s1.get_world<world_t1>();
	ecs::world<>();

	static_assert(meta::param_constains_v<ecs::entity_idx, test_func2> == true);

	// using group_t = system_group<system_1, system_2>;
	// auto group	  = system_group<system_1, par<system_1, system_2, seq<system_1, system_2>, cond<cond_func1, system_2, group_t>>, group_t>();
	//  auto group = system_group<system_2>();

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
	// group.update(world_2);

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