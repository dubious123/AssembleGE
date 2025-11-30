
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

struct A2 : decltype([]() {}), decltype([]() { std::println(""); })
{
};

template <typename... t_sys>
struct B : ecs::system::ctx::make_unique_bases<t_sys...>
{
	using t_unique_bases = ecs::system::ctx::make_unique_bases<t_sys...>;
	using t_unique_bases::t_unique_bases;
};

template <typename... t_arg>
B(t_arg&&...)
	-> B<meta::value_or_ref_t<t_arg&&>...>;

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
	// 1. run_sys => invoke의 확장판, ctx_bound 처리, user_code라면 ctx.get_exec() 한후 exec.execute(sys, ctx, arg...)
	// adaptor라면 그냥 invoke
	//
	// 2. exec.execute(sys, arg...) => invoke(sys, arg...)를 어떠한 방식으로든 실행하고(보장 X) return한다.
	// sys의 실행값을, thread를, async를, handle을 무엇을 반환할지는 사용자 맘
	// ctx가 arg에 포함되어 있는데 이는 run_sys에서 알아서 처리 되었기 때문?
	//
	// 같은 pipe라도 어떤 exec인지에 따라 실행이 될수도, 안될수도 (compile 실패) 있다.
	//
	// 3. exec.run_all(sys..., ctx, arg...) => 각 sys들을 run_sys 해서 적절하게 return한다.
	// execute가 아니라 run_sys인 이유는, 각 sys는 결국에는 user_code가 있을 것이고, 적어도 1번은 알아서 execute될 것이기 때문에
	// 중복 execute를 피하기 위해서.
	//
	// 결국 run_sys가 execute의 상위개념
	//
	// ctx(this auto&& self, arg...)
	// => self.execute<t_sys...>(arg...)
	//
	//
	// adaptor => run_sys만 call한다.
	//
	// ex. pipe => run_sys(sys_r, ctx, run_sys(sys_l, ctx, arg...) )
	//
	//
	//
	//
	// A : a1, a2, a3
	//
	// a1 : a11, a12, a13
	//
	// A : base_count = 9
	//
	//
	//
	//
	//
	//
	//
	//
	// auto test_ctx = on_ctx{
	//	exec_inline{},
	//	[x = 1](auto) {
	//		std::println("not_moved, {}", 1);
	//		return 1;
	//	}
	//		| [](auto i) { std::println("{}", ++i); return i; }
	//		| with_ctx{ [](auto&& ctx, auto i) { std::println("inside_with_ctx"); std::println("{}", i); } },
	//	[](auto&& ctx, auto i) { std::println("inside_with_ctx"); std::println("{}", i); },
	//	std::move(l),
	//	[x = 1](auto) {
	//		std::println("not_moved, {}", 1);
	//	},
	//} /*| on_ctx{ exec_inline{}, [](auto&&... arg) { ((std::cout << arg), ...); } }*/;

	// static_assert(sizeof(A) == 1);
	// static_assert(std::is_empty_v<A>);
	// static_assert(std::is_trivial_v<A>);

	//// is_constexpr_default_constructible<decltype([x = 1]() { })>();
	//[](this auto&& _) {
	//	std::println("test");
	//}();

	// test_ctx(1);

	///*static constinit const */ auto c_i = on_ctx{
	//	exec_inline{},

	//	[]() { return 1; }
	//		| [](auto i) { return ++i; }
	//		| [](auto i) { return i * 2; },

	//	with_ctx{ [](auto ctx) { return 1.3; } },
	//}();

	// auto res_tpl = test_ctx(1);

	// A, A, B{ A, A, C{ B{ A, A } }, A } , A , A
	// using t_rebound = rebind_unique_base<0, t_tree>::type;

	// meta::print_type<rebind_unique_base<0, t_tree>::type>();


	// ex. cond{ sys1, sys2, sys3 } => cond : tree<cond<sys1,sys2, sys3>,  sys1, sys2, sys3>

	// auto _ = make_unique_base_tree{ A{}, A{}, /*B{ A{}, A{}, A{} },*/ A{} };

	// leaf<A>, tree<B, A, A>


	// meta::print_type<decltype(_)>();
	// meta::print_type<decltype(_.get<0>())>();

	// meta::print_type<decltype(ecs::system::ctx::some_type{ 1, 1.f, 0 })>();

	// unique_bases { A{}, B{}, C{} ... } => unique_bases : unique_base<0, A>, unique_base<1, B> ,...
	// unique_bases { A{}, unique_bases{ B{}, C{} }, D{} ... } => unique_bases : unique_base<0, A>, unique_base<1, ??? >, unique_base<4, D>, ...

	// template<t...>
	// some_adaptor : make_unique_bases<t...>
	// ...
	// CTAD :
	// some_adaptor(t_sys&&... ) -> some_adaptor< ??? >

	// unique_bases<std::index_sequence<2, 3, 4>, A, A, A> _ = unique_bases /*<std::index_sequence<0, 1, 2>, A, A, A>*/ { A{}, A{}, A{} };
	{
	}
	{
		auto _ = unique_bases{ B{ A2{}, A{} }, unique_bases{ A2{}, A2{}, A{} }, A{}, A2{}, unique_bases{ A{}, A2{}, A{}, unique_bases{ A{}, A2{}, A{} } } };

		// meta::print_type<make_unique_base_t<3, decltype(unique_bases{ A{}, A2{}, A{} /*, unique_bases{ A{}, A2{}, A{} } */ })>>();

		// unique_base<0, A>{
		//	A{}
		// }.get<4>();

		// meta::print_type<decltype(_)>();
		//   meta::print_type<_.unique_base_count()>();

		// meta::print_type<make_unique_base_t<0, decltype(B{ A2{}, A{} })>>();
		// meta::print_type<std::remove_cvref_t<decltype(_.get<0>())>::unique_base_count()>();
		// meta::print_type<decltype(_.get<2>())>();
	}


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
		auto _l	 = [] { };
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
			[]() { },
			[]() { },
			[]() { },
			[]() { },

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