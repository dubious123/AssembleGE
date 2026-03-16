#include "age_demo_pch.hpp"
#include "age_demo.hpp"

namespace age_demo::game
{
	FORCE_INLINE void
	update_input() noexcept
	{
		using namespace age::input;

		auto	i_input		 = global::get<interface_input>();
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

	FORCE_INLINE void
	handle_scene_change() noexcept
	{
		auto i_scene = global::get<interface_scene>();

		switch (i_scene.get_scene_id())
		{
		case 0:
		{
			scene_0::deinit();
			break;
		}
		case 1:
		{
			scene_1::deinit();
			break;
		}
		case invalid_id_uint32:
		{
			break;
		}
		default:
		{
			AGE_UNREACHABLE();
		}
		}

		switch (i_scene.get_scene_id_next())
		{
		case 0:
		{
			scene_0::init();
			break;
		}
		case 1:
		{
			scene_1::init();
			break;
		}
		case invalid_id_uint32:
		{
			break;
		}
		default:
		{
			AGE_UNREACHABLE();
		}
		}

		i_scene.set_scene_id(i_scene.get_scene_id_next());
	}

	FORCE_INLINE bool
	scene_changed() noexcept
	{
		auto i_scene = global::get<interface_scene>();

		return i_scene.get_scene_id() != i_scene.get_scene_id_next();
	}

	constexpr FORCE_INLINE decltype(auto)
	get_sys_init() noexcept
	{
		// clang-format off
		using namespace age::ecs::system;
		return on_ctx{

			AGE_LAMBDA((), {
				auto i_init = global::get<interface_init>();
				i_init.set_scene_id(invalid_id_uint32);
				i_init.set_scene_id_next(1);
			}),

			AGE_FUNC(global::get<interface_init>().get_render_pipeline().init),
			
			identity{ age::platform::window_desc{ 1080 * 2, 920 * 2, "test_render_surface" } } 
				| age::platform::create_window 
				| AGE_FUNC(global::get<interface_init>().set_h_window)
				| age::graphics::create_render_surface 
				| AGE_FUNC(global::get<interface_init>().set_h_render_surface),

			AGE_FUNC(age::input::create_context) 
				| AGE_LAMBDA((auto h_ctx), { global::get<interface_init>().set_h_input_ctx(h_ctx); return h_ctx; })
				| AGE_LAMBDA((auto&& h_ctx), { age::platform::register_input_context(global::get<interface_init>().get_h_window(), h_ctx); }),
			
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

		c_auto dt = age::global::get<age::runtime::interface>().delta_time_s();

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

	constexpr FORCE_INLINE decltype(auto)
	get_sys_loop() noexcept
	{
		using namespace age::ecs::system;

		return loop{
			AGE_FUNC(age::global::get<age::runtime::interface>().get_running),

			break_if_unlikely{ AGE_LAMBDA((), { return age::platform::window_count() == 0; }) },

			AGE_FUNC(global::get<interface_loop>().get_h_input_ctx)
				| AGE_FUNC(age::input::begin_frame)
				| AGE_FUNC(age::platform::update),

			age::runtime::update,

			age::graphics::begin_frame,

			age::runtime::when_window_alive(global::get<interface_loop>().get_h_window())
				| AGE_FUNC(print_frame_rate)
				| AGE_FUNC(update_input)
				| AGE_LAMBDA(
					(),
					{
						auto i_ctx = global::get<interface_loop>();
						if (i_ctx.get_h_input_ctx()->is_pressed(age::input::e::key_kind::key_0))
						{
							i_ctx.set_scene_id_next(0);
						}
						else if (i_ctx.get_h_input_ctx()->is_pressed(age::input::e::key_kind::key_1))
						{
							i_ctx.set_scene_id_next(1);
						}
					})

				| cond_unlikely{ scene_changed, handle_scene_change }
				| match{ AGE_FUNC(global::get<interface_loop>().get_scene_id),
						 on<0>		= scene_0::update,
						 on<1>		= scene_1::update,
						 default_to = AGE_LAMBDA((), { AGE_UNREACHABLE(); }) },
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
			match{ AGE_FUNC(global::get<interface_deinit>().get_scene_id),
				   on<0> = AGE_FUNC(scene_0::deinit),
				   on<1> = AGE_FUNC(scene_1::deinit),
				   /* default_to = AGE_LAMBDA((), { AGE_UNREACHABLE(); }) */ },

			AGE_FUNC(global::get<interface_deinit>().get_render_pipeline().deinit),
			age::ecs::system::exec_inline{}
		};
	}
}	 // namespace age_demo::game
