
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
#include <random>
#include "test.h"
// #include <type_traits>
// #include <sstream>
// #include <format>
// #include <cstdlib>
// #include <ctime>

// import std;

using namespace data_structure;

int
main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	auto _game = my_game();

	// print_type<meta::tuple_cat_t<>>();
	using t_entity_id = uint32;

	run_benchmark(_game, 1'000);

	{
		using namespace ecs::system;
		using ecs::system::operator|;

		auto sys = seq{
			sys_game_init{},
			[]() { return "hi"; },
			pipe{ [](auto&& _) -> decltype(auto) { std::println("hello"); return 1; }, [](auto&& _) { std::println("{}", _ + 1); return 1 ; } },
			par{
				[](auto&& _) { std::println("hello"); },
				pipe{ [](auto&& _) { std::println("hello"); return 1; }, [](auto&& _) { std::println("{}", _ + 1); } },
			},
			[](auto&& _) { std::println("hello"); return 1; } | [](auto&& _) { std::println("{}", _ + 1); return _ + 1; } | [](auto&& _) { std::println("{}", _ + 1); return _ + 1; },
			cond{ [](auto&& _) { return true; },
				  [](auto&& _) { std::println("true"); return 1; },
				  [](auto&& _) { std::println("false"); return 2; } }
				| cond{ [](auto&& _) { return true; }, [](auto&& _) { std::println("true"); } }
				| cond{ []() { return false; }, []() { std::println("false"); } },
			[](auto) {},

			[] { return 10; }
				| loop{ [](auto&& i) { return i-- > 0; },
						[](auto&& i) { std::println("i : {}", i); },
						continue_if{ [](auto i) { return i % 2; } },
						par{
							[](auto&&) { Sleep(200); std::println("hi-1"); },
							[]() { Sleep(100); std::println("hi-2"); } },
						break_if{ [](auto i) { return i == 5; } } },
			match{
				[] { return 0; },
				on<3> = [](auto&& _) { return 2; } | [](auto&& _) { std::println("0"); return 1; },
				default_to = [] { std::println("default"); return 1; },
			},
			[]<typename g>(interface_game<g> igame)
				-> decltype(auto) { return (igame.get_scene<scene_t1>()); }
					   | []<typename s>(interface_scene<s> iscene)
					-> world_t3& { return iscene.get_world<world_t3>(); }
						   | seq{ each_entity{
									  query{ with<transform, bullet> },
									  [x = 0.f, y = 1.f, z = 2.f](transform& t) mutable {
										  t.position.x = ++x;
										  t.position.y = ++y;
										  t.position.z = ++z;
									  } },
								  each_group{
									  query{ with<transform, bullet>, without<rigid_body> },
									  seq{ [id = 0]<typename g>(i_entity_group<g> i_ent_group) mutable {
											  auto&& [t] = i_ent_group.get_component<transform&>(19);
											  std::println("group [{}], size : {}, last_entity_id : {}, first_entity_position = [x : {}, y : {}, z : {}]",
														   id++,
														   i_ent_group.entity_count(),
														   i_ent_group.ent_id(i_ent_group.entity_count() - 1),
														   t.position.x,
														   t.position.y,
														   t.position.z);
										  },
										   each_entity{ query{ without<transform> }, [](transform& t, const rigid_body& rb) {
														   std::println("done");
													   } } } },
								  sys_game_deinit{} }
		};
		// _game | get_world<>{} | query<>{} | each_seq{ sys... }
		// _game | get_world<>{} | each_group_seq{query{}, sys... }
		// _game | get_world<>{} | filter( query{} ) | each( as_par{},  sys )
		// _game | get_world<>{} | filter( {} ).cache().refresh_if( {} )
		//
		// _game | get_world<>{} | std::views::filter() | ...
		// _game | get_world<>{} | each( cond() | seq() )
		// _game | get_world<>{} | each() | cond{ group_query{} } | each() |
		//
		// _game | seq { ... , each_seq{} | filter{} | seq{ group_sys{}, each() | entity_sys{} } }
		// _game | seq { ... , each_par{} | filter{} | seq{ group_sys{}, each() | entity_sys{} } }
		// _game | seq { ... , filter{} | for_each_seq{ seq{ group_sys{}, each() | entity_sys{} } } }
		//
		// _game | seq { ... , each_par{} | filter{} | seq{ group_sys{}, each() | entity_sys{} } }
		//
		// for(auto& group : world){
		// }
		//
		// auto p = seq{ [](auto&& game) -> decltype(auto) { return game.get_scene<scene_t1>().get_world<world_t3>(); } } | [](auto&& tpl) -> decltype(auto) { return std::get<0>(std::forward<decltype(tpl)>(tpl)); } | each_group{ query{}, [](auto&& i_group) {} };
		// auto p = seq{ [](auto&& game) -> decltype(auto) { return game.get_scene<scene_t1>().get_world<world_t3>(); } } | each_group{ query{}, [](auto&& i_group) {} };
		// p(_game);

		[](int a, int&& b, int c) {
			std::println("e: {}, f: {}, g: {}", a, b, c);
		}(
			[] { return (std::println("2"), []() { return 4; }()); }(),
			[] { return (std::println("4"), []() { return 6.1f; }()); }(),
			[] {
				auto res = (std::println("6"), []() { return 5; }());
				std::println("8");
				return res;
			}());

		auto test_seq = seq{
			[]() { return 1; },
			[]() { return std::tuple(2); },	   // <-fixme : user tuple is also removed
			[]() {},
			[]() { return std::tuple<>(); },
			[]() { return 2; }
		};

		auto test_pipe = par{
			[]() { std::println("1"); },
			[]() { return 1; },
			[]() { std::println("3"); },
			[]() { return 2.1f; },
			[]() { std::println("5"); },
			[]() { return 3; },
			[]() { std::println("7"); },
		} | [](int a, int&& b, int c) {
			std::println("a: {}, b: {}, c: {}", a, b, c);
		};
		using t_res_seq = decltype(seq{
			[]() { return 1; },
			[]() { return 2.1f; },
			[]() { return 3; },
		}());

		auto s = seq{
			[]() { std::println("1"); },
			[]() { return 1; },
			[]() { std::println("3"); },
			[]() { return 2.1f; },
			[]() { std::println("5"); },
			[]() { return 3; },
			[]() { std::println("7"); },
		}();

		test_pipe();

		// tuple{ tuple(1), tuple(tuple()), tuple(), tuple(2) }

		auto test_res_2 = test_seq();

		// meta::print_type<decltype(test_res_2)>();

		auto test_tpl_cat  = std::tuple_cat(std::tuple(2), std::tuple(std::tuple(3, 3)), std::tuple(2));
		auto test_tpl_cat2 = std::tuple_cat(std::tuple(std::tuple(1), std::tuple(2)), std::tuple(4));
		auto test_tpl_cat3 = std::tuple_cat(std::tuple(1), std::tuple(std::tuple()), std::tuple(), std::tuple(2));


		auto test_seq_void = seq{
			[]() {},
			[]() {},
			[]() {},
			[]() {},

		};
		// meta::print_type<decltype(test_seq())>();
		// meta::print_type<decltype(test_seq_void())>();

		auto res = sys(_game);

		auto _ = ecs::system::detail::scope_guard{ []() noexcept { std::println("scope_guard"); } };

		// meta::print_type<decltype(res)>();
		{
			auto tpl_test = std::tuple{ [] { std::print("a"); return 1; }(), ([] { std::print("b"); }(), [] { std::print("c"); return 2; }()) };
			// meta::print_type<decltype(tpl_test)>();
			// meta::print_type<ecs::system::detail::index_range_t<0, 0>>();

			std::println("size : {}", sizeof(ecs::system::detail::index_ranges_seq_t<13, std::index_sequence<0, 3, 4, 7, 10>>));
			// meta::print_type<decltype(ecs::system::detail::make_index_range<4, 7>())>();
		}

		// meta::print_type<decltype(res)>();
	}
}