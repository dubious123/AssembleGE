
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

struct A : decltype([]() {}), decltype([]() { std::println(""); })
{
};

template <typename... t>
void
func(t... f, auto&&... arg)
{
}

void
ctx_test(auto& _game)
{
	using namespace ecs::system::ctx;
	auto l = [](/*this auto&& ctx,*/ int) { std::println("moved"); return 3; };
	// on_ctx{ sys1, sys2, sys3... }
	// | on_ctx{ loop{ sys_cond, sys3, set_error_if{}, break_if{} }, on_error = {}, on_stopped = {} }

	// ctx{ ... }
	// | sys{}
	// | sys{}...
	// | sys{}

	func<int, float>(1, 2, 3);

	// sys -> noexception, always return

	// 문제 : sys1 | sys2 | sys3 -> exec pipe, exec pipe sys1, sys2 sys3 ? 가 된다.
	// 문제 : on_ctx | on_ctx 가 안된다.

	// 어떤 adpator는 ctx를 받아서 무엇을 한다. ( ex. with_ctx, for_each_par ... )
	// 어떤것은 아무것도 안한다.

	// executor는 여러개의 dag (pipe)를 소비한다.
	// executor는 여러개의 dag를 어떻게 실행할 것인지 정해야함 ( dag의 순서, 실제 return type 의 배치, error, cancel 등등 )
	// 각 노드들은 실제 코드와 executor를 가지고 어떻게 실행할 것인지를 정한다.
	// 각 노드들은 executor의 입장에서 "실제로" 실행되는 코드는 아니다.
	// 실제로 실행되는 코드는 (user의 코드) execute(sys, ctx, arg...)를 통해 실행됨
	//
	// dag는 dag.run(ctx, arg...) => 여러개의 dag run... 을 어떻게 배치할 것인지는 executor의 역할
	// dag의 각 노드 안에서 execute(sys, ctx, arg...) 를 어떻게 배치할 것인지는 각 node의 역할
	// dag( pipe )는 가장 간단한 노드?
	//
	// executor :
	//
	// run_pipes( ctx, pipes..., arg... ) <- on_ctx에서 한번 실행됨
	//
	// run_pipe( ctx, pipe..., arg... ) <- executor에서 각각의 pipe를 실행하기 위해 call됨
	//
	// execute( ctx, sys, arg... ) <- 각 노드에서 call됨.
	//
	//
	auto test_ctx = on_ctx{
		exec_inline{},
		[x = 1](auto) {
			std::println("not_moved, {}", 1);
			return 1;
		}
			| [](auto i) { std::println("{}", ++i); return i; }
			| with_ctx{ [](auto&& ctx, auto i) { std::println("inside_with_ctx"); std::println("{}", i); } },
		[](auto&& ctx, auto i) { std::println("inside_with_ctx"); std::println("{}", i); },
		std::move(l),
		[x = 1](auto) {
			std::println("not_moved, {}", 1);
		},
	} /*| on_ctx{ exec_inline{}, [](auto&&... arg) { ((std::cout << arg), ...); } }*/;

	static_assert(sizeof(A) == 1);
	static_assert(std::is_empty_v<A>);
	static_assert(std::is_trivial_v<A>);

	// is_constexpr_default_constructible<decltype([x = 1]() { })>();
	[](this auto&& _) {
		std::println("test");
	}();

	test_ctx(1);

	/*static constinit const */ auto c_i = on_ctx{
		exec_inline{},

		[]() { return 1; }
			| [](auto i) { return ++i; }
			| [](auto i) { return i * 2; },

		with_ctx{ [](auto ctx) { return 1.3; } },
	}();

	// auto res_tpl = test_ctx(1);

	std::println("asdfe");
	std::println("aft_moded");
}

int
main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	auto _game = my_game();

	using t_entity_id = uint32;

	run_benchmark(_game, 1'000);

	{
		using namespace ecs::system;
		using ecs::system::operator|;
		auto _l	 = [] {};
		auto sys = seq{
			sys_game_init{},
			std::move(_l),
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

		std::invoke([](auto e) { std::println("{}", e); }, std::tuple{ 1 });

		static_assert(([]() { return 1; } | [](auto i) { return i == 1; })());

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


		// auto res = (identity{ _game } | FWD(sys))();
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
		// sys(_game);

		// meta::print_type<decltype(res)>();
	}

	{
		ctx_test(_game);
	}
}