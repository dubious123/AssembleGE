
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
							[](auto&&) { Sleep(100); std::println("hi-1"); },
							[]() { Sleep(200); std::println("hi-2"); } },
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
											  auto&& [t] = i_ent_group.get_component<transform>(50);
											  std::println("group [{}], size : {}, last_entity_id : {}, first_entity_position = [x : {}, y : {}, z : {}]",
														   id++,
														   i_ent_group.entity_count(),
														   i_ent_group.ent_id(i_ent_group.entity_count() - 1),
														   t.position.x,
														   t.position.y,
														   t.position.z);
										  },
										   each_entity{ query{}, [](transform& t, const rigid_body& rb) {} } } },
								  sys_game_deinit{} }
		};


		// auto p = seq{ [](auto&& game) -> decltype(auto) { return game.get_scene<scene_t1>().get_world<world_t3>(); } } | [](auto&& tpl) -> decltype(auto) { return std::get<0>(std::forward<decltype(tpl)>(tpl)); } | each_group{ query{}, [](auto&& i_group) {} };
		// auto p = seq{ [](auto&& game) -> decltype(auto) { return game.get_scene<scene_t1>().get_world<world_t3>(); } } | each_group{ query{}, [](auto&& i_group) {} };
		// p(_game);

		auto test_pipe = seq{
			[]() { return 1; },
			[]() { return 2.1f; },
			[]() { return 3; },
		} | [](int a, int&& b, int c) {
			std::println("a: {}, b: {}, c: {}", a, b, c);
		};


		auto res2 = seq{
			[]() { return 1; },
			[]() { return 2.1f; },
			[]() { return 3; },

		}();
		// meta::print_type<decltype(seq{ detail::run_sys([]() { return 1; }) })>();
		// meta::print_type<decltype([]() -> decltype(auto) { return 1; }())>();
		// meta::print_type<decltype(res2)>();


		test_pipe();

		// auto test_par = par{
		//	[](auto&& _) { std::println("hello"); },
		//	pipe{ [](auto&& _) { std::println("hello"); return 1; }, [](auto&& _) { std::println("{}", _ + 1); } },
		// };

		// meta::print_type<decltype(test_par(_game))>();

		// seq return type :
		//  1. result_tpl { ... }
		//  2. result_tpl { }

		// 1.

		// 1. 모든 lambda return -> std::tuple로 감싸기
		//	1.1 만약 void return (leaf system만 해당? seq나 par는 result_tpl이니깐?) -> tuple<> 로 감싸기?
		//	1.2 만약 tuple<> 을 반환하면 (유저가) -> 이를 유지해야 하는데 어떻게 구분?
		//	=> 모든 sys를 result_wrapper로 묶는다. 유저는 사용금지
		//	=> return void : result_wrapper<void> , return tuple<> : result<std::tuple<>>
		//  => 그리고 tuple_cat_all (tie_results : 시스템 실행 순서 보장)

		// 1. 함수 사용 불가 ( 실행순서 반대)
		// 2. { ... } 필수
		// 3. void 가 문제
		// 4. wrapper로 감싸기 (하나씩)
		// 5. tuple_cat_all로 void 지우기
		// 6. std::tuple<std::tuple<>> 이 지워짐
		// 7. 유저에게는 result_tpl 대신 std::tuple만 주고 싶음
		// 8. void -> result_wrapper<void>, tuple -> result_wrapper<tuple<>>,
		// 9.

		// seq { seq { seq { seq { [](){return 1; } } } } } -> tuple{tuple{tuple{tuple{tuple{1}}}}} ??

		// debug를 위한 sys_result 가 있긴 해야할듯.

		// ex. depth, size, print 등등...

		// 1. std::namespace에 sys_result 전용 tuple_element, tuple_size 특수화를 넣고 include 순서 신경쓰기
		// 2. 모든 시스템 result를 std::tuple로 반환,

		// 1. 그냥 모든 시스템은 sys_result을 반환
		//	1.1 tuple(1, 2, 3), tuple(), tuple( tuple() )
		// 2. pipeline은 1. binding 가능시 언제나 bind 2. sys_result는 언제나 unwrap 3. 유저가 직접 sys_result 생성은 금지 (how?)
		// 3. sys_result<sys_result<>> 는 금지 ( 알아서 죽인다 )


		auto test_seq = seq{
			[]() { return 1; },
			[]() { return std::tuple(2); },	   // <-fixme : user tuple is also removed
			[]() {},
			[]() { return ecs::system::detail::sys_result<>{}; },
			[]() { return 2; }
		};

		// tuple{ tuple(1), tuple(tuple()), tuple(), tuple(2) }

		static_assert(std::tuple_size_v<std::tuple<std::tuple<>>> == 1);
		static_assert(std::tuple_size_v<std::tuple<>> == 0);

		auto test_res_2 = test_seq();

		// meta::print_type<decltype(test_res_2)>();

		auto test_tpl_cat  = std::tuple_cat(std::tuple(2), std::tuple(std::tuple(3, 3)), std::tuple(2));
		auto test_tpl_cat2 = std::tuple_cat(std::tuple(std::tuple(1), std::tuple(2)), std::tuple(4));
		auto test_tpl_cat3 = std::tuple_cat(std::tuple(1), std::tuple(std::tuple()), std::tuple(), std::tuple(2));


		auto test_seq_void = seq{
			[]() { },
			[]() { },
			[]() { },
			[]() { },

		};
		// meta::print_type<decltype(test_seq())>();
		// meta::print_type<decltype(test_seq_void())>();

		// std::tuple<ecs::system::detail::result_tpl<>>{
		//	ecs::system::detail::result_tpl{ std::tuple<>{} }
		// };

		// todo dangling 문제 해결, return type이 모두 ref임
		auto res = sys(_game);


		meta::print_type<decltype(res)>();
	}
}