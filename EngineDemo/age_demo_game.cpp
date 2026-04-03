#include "age_demo_pch.hpp"
#include "age_demo.hpp"

namespace age_demo::game
{
	FORCE_INLINE void
	update_input() noexcept
	{
		using namespace age::input;

		c_auto& engine_input = *i_input.get_h_input_ctx();

		i_input.set_move(float2{
			engine_input.is_down(e::key_kind::key_d) - engine_input.is_down(e::key_kind::key_a),
			engine_input.is_down(e::key_kind::key_w) - engine_input.is_down(e::key_kind::key_s),
		});

		i_input.set_look(engine_input.mouse_delta);
		i_input.set_zoom(engine_input.wheel_delta);
		i_input.set_sprint(engine_input.is_down(e::key_kind::key_shift));
		i_input.set_right_mouse_down(engine_input.is_down(e::key_kind::mouse_right));
		i_input.set_middle_mouse_down(engine_input.is_down(e::key_kind::mouse_middle));
	}

	constexpr FORCE_INLINE decltype(auto)
	get_sys_init() noexcept
	{
		// clang-format off
		using namespace age::ecs::system;
		return on_ctx{

			AGE_LAMBDA((), {
				i_init.set_scene_id = invalid_id_uint32;
				i_init.set_scene_id_next = g::first_scene_idx;
			}),

			AGE_FUNC(i_init.get_render_pipeline->init),
			
			identity{ age::platform::window_desc{ 1080 * 2, 920 * 2, "test_render_surface" } } 
				| age::platform::create_window 
				| AGE_FUNC(i_init.set_h_window)
				| age::graphics::create_render_surface 
				| AGE_FUNC(i_init.set_h_render_surface),

			AGE_FUNC(age::input::create_context) 
				| AGE_LAMBDA((auto h_ctx), { i_init.set_h_input_ctx(h_ctx); return h_ctx; })
				| AGE_LAMBDA((auto&& h_ctx), { age::platform::register_input_context(i_init.get_h_window(), h_ctx); }),
			
			age::ui::init,

			AGE_LAMBDA((), { age::ui::font::load("resources\\font\\NotoSansKR-Regular", i_init.get_render_pipeline()); }),
			AGE_LAMBDA((), { age::ui::font::set_default("resources\\font\\NotoSansKR-Regular"); }),
			
			loop{ [warm_up_frame = 10] mutable { return --warm_up_frame > 0; },
				  age::platform::update,	// pump platform msg
				  age::runtime::update, 
				  age::graphics::begin_frame, 
				  age::graphics::end_frame },
			exec_inline{}
		};
		// clang-format on
	}

	constexpr FORCE_INLINE decltype(auto)
	print_frame_rate() noexcept
	{
		static uint32 frame_count		 = 0;
		static double accumulated_time_s = 0.0;

		c_auto dt = age::runtime::i_time.get_delta_time_s();

		++frame_count;
		accumulated_time_s += dt;

		if (accumulated_time_s >= 1.0)
		{
			c_auto average_fps = static_cast<double>(frame_count) / accumulated_time_s;

			std::println("[Profiler] Average FPS : {:.1f} ({} frames in {:.4f}s)",
						 average_fps, frame_count, accumulated_time_s);

			frame_count		   = 0;
			accumulated_time_s = 0.0;
		}
		// std::println("now : {:%F %T}", std::chrono::system_clock::now());
		// std::println("now : {}ns, {}s",
		//	   age::global::get<age::runtime::interface>().delta_time_ns().count(),
		//	   age::global::get<age::runtime::interface>().delta_time_s());
		// std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	FORCE_INLINE bool
	scene_changed() noexcept
	{
		return i_scene.get_scene_id() != i_scene.get_scene_id_next();
	}

	constexpr FORCE_INLINE decltype(auto)
	get_sys_loop() noexcept
	{
		using namespace age::ecs::system;

		return loop{
			AGE_FUNC(age::runtime::i.get_running),

			break_if_unlikely{ AGE_LAMBDA((), { return age::platform::window_count() == 0; }) },

			AGE_FUNC(i_loop.get_h_input_ctx)
				| AGE_FUNC(age::input::begin_frame)
				| AGE_FUNC(age::platform::update),

			age::runtime::update,

			age::graphics::begin_frame,

			age::runtime::when_window_alive(i_loop.get_h_window())
				| AGE_FUNC(print_frame_rate)
				| AGE_FUNC(update_input)
				| AGE_LAMBDA(
					(),
					{
						if (i_loop.get_h_input_ctx->is_pressed(age::input::e::key_kind::key_0))
						{
							i_loop.set_scene_id_next = 0;
						}
						else if (i_loop.get_h_input_ctx->is_pressed(age::input::e::key_kind::key_1))
						{
							i_loop.set_scene_id_next = 1;
						}
						else if (i_loop.get_h_input_ctx->is_pressed(age::input::e::key_kind::key_2))
						{
							i_loop.set_scene_id_next = 2;
						}
					})

				| cond_unlikely{
					scene_changed,
					on_ctx{
						match{
							AGE_FUNC(i_loop.get_scene_id),
							on<0>				  = scene_0::deinit,
							on<1>				  = scene_1::deinit,
							on<2>				  = scene_2::deinit,
							on<invalid_id_uint32> = AGE_LAMBDA((), {}),
							default_to			  = AGE_LAMBDA((), { AGE_UNREACHABLE(); }) },
						match{
							AGE_FUNC(i_loop.get_scene_id_next),
							on<0>	   = scene_0::init,
							on<1>	   = scene_1::init,
							on<2>	   = scene_2::init,
							default_to = AGE_LAMBDA((), { AGE_UNREACHABLE(); }) },

						AGE_LAMBDA((), { i_loop.set_scene_id = i_loop.get_scene_id_next(); }),
						exec_inline{} } }

				| match{
					AGE_FUNC(i_loop.get_scene_id),
					on<0>	   = scene_0::update,
					on<1>	   = scene_1::update,
					on<2>	   = scene_2::update,
					default_to = AGE_LAMBDA((), { AGE_UNREACHABLE(); }),
				},
			// age::runtime::when_window_alive(h_window_test_app_1),
			// age::runtime::when_window_alive(h_window_test_app_2),
			// age::runtime::when_window_alive(h_window_test_app_3),
			age::graphics::end_frame
		};
	}

	constexpr FORCE_INLINE decltype(auto)
	get_sys_deinit() noexcept
	{
		using namespace age::ecs::system;

		return on_ctx{
			match{ AGE_FUNC(i_deinit.get_scene_id),
				   on<0> = AGE_FUNC(scene_0::deinit),
				   on<1> = AGE_FUNC(scene_1::deinit),
				   on<2> = AGE_FUNC(scene_2::deinit),
				   /* default_to = AGE_LAMBDA((), { AGE_UNREACHABLE(); }) */ },

			AGE_LAMBDA((), { age::ui::deinit(i_deinit.get_render_pipeline()); }),

			AGE_FUNC(i_deinit.get_render_pipeline().deinit),
			age::ecs::system::exec_inline{}
		};
	}
}	 // namespace age_demo::game
