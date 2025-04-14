
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
#include <source_location>
#include <print>
#include <string>
#include <variant>

#include <future>

#include "test.h"

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

void update(auto& world, ecs::entity_idx e_idx, transform& t, rigid_body& v) {};

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

	void update(auto& world, ecs::entity_idx e_idx, transform& t, rigid_body& v) {};
	void update2(ecs::entity_idx e_idx, transform& t, rigid_body& v) {};

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
	void update(transform& t, bullet& v) {};

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
	std::array<std::variant<scene_ts...>, sizeof...(scene_ts)> arr { v_t { scene_ts() }... };
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
	decltype(auto) run(t_data&&... data)
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

int main()
{
	// loop<&system_1::f>();
	auto _scene0 = scene_t1();
	// auto ss =
	//	_seq<
	//		my_scene_system_0,
	//		my_scene_system_1,
	//		_par<my_scene_system_0,
	//			 _seq<
	//				 my_scene_system_1,
	//				 my_scene_system_0,
	//				 _cond<my_cond_system_true, my_scene_system_0, my_scene_system_1>,
	//				 _cond<my_cond_system_false, my_scene_system_0, my_scene_system_1>>>>();
	// ss.run(&_scene0);

	auto _game = my_game();
	static_assert(std::is_integral_v<decltype([]() { return true; })> is_false);
	// SWITCH([]() { return 0; }, (ecs::_case<0, ecs::bind<sys_game_init {}, []<typename g>(interface_game<g> igame) { return igame.get_scene<scene_t1>(); }> {}> {}));
	auto _sys_group_game = seq<
		sys_game_init {},
		loop<[]<typename g>(interface_game<g> igame) { return igame.get_running(); },
			 ecs::_switch<[]<typename g>(interface_game<g> igame) { return igame.get_current_scene_idx(); },
						  ecs::bind<sys_scene_init {}, []<typename g>(interface_game<g> igame) -> auto& { return igame.get_scene<scene_t1>(); }> {},
						  ecs::bind<sys_scene_init {}, []<typename g>(interface_game<g> igame) -> auto& { return igame.get_scene<scene_t2>(); }> {}> {}> {},
		sys_game_deinit {}>();
	{
		sys_node<my_game&> node(_game);
		static_assert(std::is_lvalue_reference_v<decltype(node())>);
	}
	{
		sys_node<my_game> node(my_game {});
		static_assert(std::is_lvalue_reference_v<decltype(node())>);
	}

	{

		using namespace _temp;
		// clang-format off
	{
		auto sys_group = 
			sys_game_init{}
			+ ([]<typename g>(interface_game<g> igame ) -> decltype(auto){return igame.get_scene<scene_t1>(); }
				| sys_scene_init{}
				+ sys_scene_init{})
			+ sys_game_init{} ;

			

		sys_group.run(_game);
		std::println("====================================");
	}
	{
		auto sys_group = 
			sys_game_init{} 
			+ sys_game_init{} 
			+ sys_non_templated{}
			+ ([]<typename g>(interface_game<g> igame ) -> decltype(auto){return igame.get_scene<scene_t1>(); } | sys_scene_init{}) 
			+ ([]<typename g>(interface_game<g> igame ) -> decltype(auto){return igame.get_scene<scene_t2>(); } 
				| (sys_scene_init{} 
				+ sys_scene_init{}))
			+ [](){std::println("empty");}
			+ sys_game_deinit{};

		sys_group.run(_game);
		std::println("====================================");
	}

	{
		auto sys_group = 
			_game 
			+ sys_game_init{} 
			+ [](){std::println("empty");}
			+ ([](){return my_game{};} | []<typename g>(interface_game<g> igame ) -> decltype(auto){return igame.get_scene<scene_t1>(); } | sys_scene_init{})
			+ ([]<typename g>(interface_game<g> igame ) -> decltype(auto){return igame.get_scene<scene_t2>(); } 
				| (sys_scene_init{} 
				+ sys_scene_init{}))
			+ sys_non_templated{}
			+ sys_game_deinit{};
		 sys_group.run();
		 std::println("====================================");
	}

	{
		auto l_1 = [](auto&& _ ){std::println("empty1");};
		auto l_2 = [](auto&& _ ){std::println("empty2");};
		auto sys_group = 
			my_game{}
			+ l_1
			+ l_2
			+ [](auto&& _ ){std::println("empty1");}
			+ [](auto&& _ ){std::println("empty2");}
			+ ([](){return my_game{};} | []<typename g>(interface_game<g> igame ) -> decltype(auto){return igame.get_scene<scene_t1>(); } | sys_scene_init{})
			+ ([]<typename g>(interface_game<g> igame ) -> decltype(auto){return igame.get_scene<scene_t2>(); } 
				| (sys_scene_init{} 
				+ sys_scene_init{}))
			//+ ([]<typename g>(interface_game<g> igame ) -> decltype(auto){return igame.get_scene<scene_t1>(); }
			//    				+ sys_scene_init{}
			//    			    + sys_scene_init{})
			    
			/*+= []<typename g>(interface_invalid<g> should_not_build){ should_not_build.invalid(); }*/
			+ sys_non_templated{}
			+ sys_game_init{}
				;

				//| [](){std::println("empty");}
				//| sys_game_deinit{};
		sys_group.run();
		std::println("====================================");
	}
	{
		auto sys_group = 
			/*[](){std::println("first"); }*/
			my_game{}
			+ [](auto&& _ ){std::println("empty1");}
			+ [](){return 5; }
			+ [](auto&& _ ){std::println("empty2");}
			+ []<typename g>(interface_game<g> igame ) -> decltype(auto){return igame.get_scene<scene_t1>(); }
			+ sys_non_templated{}
			/*+= []<typename g>(interface_invalid<g> should_not_build){ should_not_build.invalid(); }*/
			/*+= sys_game_init{}*/;
				//| sys_game_init{} 
				//| [](){std::println("empty");}
				//| sys_game_deinit{};
		sys_group.run();
		std::println("====================================");
		//sys_group.run(my_game{});
	}


	////// clang-format on
	}


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