#include "age_demo.hpp"

namespace age_demo::global
{
	struct ctx
	{
		age::platform::window_handle		 h_window;
		age::graphics::render_surface_handle h_render_surface;
		age::input::context_handle			 h_input_ctx;

		age::graphics::render_pipeline::forward_plus::pipeline render_pipeline;

		uint32 scene_id;
		uint32 scene_id_next;

		// input
		float2 move;
		float2 look;
		float  zoom;
		bool   sprint;
		bool   right_mouse_down;
		bool   middle_mouse_down;

		age_demo::scene_0::ctx scene_0_ctx;

		age_demo::scene_1::ctx scene_1_ctx;
	};

	namespace detail
	{
		inline ctx context;
	}

	template <template <typename> typename t_interface>
	FORCE_INLINE decltype(auto)
	get() noexcept
	{
		return t_interface{ detail::context };
	}
};	  // namespace age_demo::global