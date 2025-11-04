
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
		// seq =>
		// sys<exec_seq>{
		//	exec_seq executer;
		//	systems
		//
		//	decltype run(auto&&... arg)
		//		return executer.run( sys...,  FWD(arg)...)
		// }
		//
		// get_exec{}
		// | just{ _game }
		// |
		//
		//
		// identity{ _game, ...,  }
		// | sys<seq>( sys, then( ... ), sys... )
		// | sys<par>()
		// | sys<custom_par>()
		// | with_custom_exec{} | then() | then() |
		// |
		// ...
		//
		//
		//
		// _game
		// | on_parallel_ctx{}
		// | do { systems.... }
		// | on_seq_ctx{}
		// | do { [](auto& g){ return exec_ctx{g}; } }
		//
		//
		// _game
		// | seq{
		// par { sys... },
		// par { some_custom_executor, sys... }
		// par<some_custom_executor> { sys... }
		// par { get_some_parallel_executer , sys... }
		// }
		//
		// _game
		// | on_ctx{ ctx::cpu::seq{} } <- returns what? ( == [](auto&& g){ return ecs::system::ctx::cpu::seq{ FWD(_game) }; }
		// | run(..., on_ctx{ ctx::cpu::par{} } | sys | ...,   )
		// | run( [](auto&&... res){ ... } )
		//
		// new_ctx { ctx::cpu::seq{}, _game, other_data, 10, [](){ return some_strange_data{}; } }
		// | run( ..., new_ctx{ ctx::cpu::par{} } | [](){}... )
		//
		//
		// new_ctx { ctx::cpu::seq{}, ctx::no_execption{}, set_args{ _game, 10, some_data, [](){ return 10; }, new_ctx { ... } } }
		// | run( sys1, sys2, ...,
		//		new_ctx{ ctx::cpu::par{}, ctx::with_execption{}, input::identity{} <- projection }
		//			| run( [](auto& game, int ten, auto& some_data, auto& some_lambda, auto& some_ctx){}, ... ),
		//		sys_3, sys_4, ...)
		// | run( [](auto&& some_tpl){}, [](auto&&... res){}, ... )
		// | [](auto& game, int ten, auto& some_data, auto& some_lambda, auto& some_ctx){ return game; } <- run can be omitted when single sys
		// | sys_get_world<>{}
		// | run( for_each{ ... } )
		// | run( new_ctx{ some_custom_ctx{},  | rv::order_by{} } | for_each{ run{ [](auto& igroup){ ... }, some_sys, ...,  } } )
		// | cond{ [](){return true; }, sys1, sys2 }
		//
		// ->
		// new_ctx{}() -> a -> ctx.run(sys1), ctx.run(sys2), ..., ctx.run( new_ctx | sys3 ) ...
		//
		//
		//
		// new_ctx{ ctx::cpu::seq{}, ctx::no_exception{}, input{ _game } }
		// | test::loop{ [](auto& game){return game.running; },
		//	get_world<>{} | query{} | new_ctx{ ctx::cpu::par{}, identity{} } | for_each{ },
		//	get_world<>{} | for_each{ entity_group_query{} | new_ctx{ par{} } | for_each{ entity_query<>{} | [](transform, ...){} }  },
		//	get_ctx{} | ...
		// }
		//
		// 0. ctx -> run, set_args()
		// 1. system::test::ctx::seq{} -> run(...)
		// 2. system::test::operator | ( l, r) -> system::test::detail::sys{ ctx, sys }
		// 3. system::test::operator | ( l, r) -> system::test::detail::sys{ ctx, sys | sys }
		// 4. sys | sys
		// 5. system::test::detail::sys_run_impl -> run(ctx) { return ctx.run( ... ); }\
		// 6. system::test::detail::sys_loop_impl -> run(ctx) { return while( ctx.run( sys_cond ) ) {
		//


		static_assert(std::is_empty_v<decltype(filter{ [](int i) { return i % 2 == 0; } })>);

		for (const auto& x : (identity{ std::views::iota(0) } | filter{ [](int i) { return i % 2 == 0; } } | take(4))())
		{
			std::println("x : {}", x);
		}

		constexpr auto r2es = std::ranges::fold_left((identity{ std::views::iota(0) } | filter{ [](int i) { return i % 2 == 0; } } | take(4))(), 0, std::plus{});
		constexpr auto r3es = (identity{ std::views::iota(0) } | filter{ [](int i) { return i % 2 == 0; } } | take(4) | sum())();

		static_assert(r2es == 12);
		static_assert(r3es == 12);


		{
			auto test_sys =
				identity{ std::views::iota(0) }
				| filter{ [](int i) { return i % 2 == 0; } }
				| ecs::system::map([](auto arg) { return arg + 1; })
				| take(4)
				| for_each([](auto i) { std::println("x : {}", i); });

			test_sys();
		}
		// make( expr ) -> [](){ return expr; }
		//
		// _game | get_world<>{} | make( rv::filter | rv::take | ...  ) |
		//
		// identity( rv::filter() | () | ... |   )


		// if(run_sys_left) return run_sys_right


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

		static_assert(([]() { return 1; } | [](auto i) { return i == 1; })());

		test_pipe();

		// tuple{ tuple(1), tuple(tuple()), tuple(), tuple(2) }

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


		auto res = (identity{ _game } | FWD(sys))();

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