#pragma once

namespace age::platform
{
	enum class window_mode : uint8
	{
		windowed,
		boderless_windowed,
		fullscreen,
		count
	};
}

// interface
namespace age::platform
{
	template <typename t>
	struct interface
	{
	  private:
		no_unique_addr t data;

	  public:
		constexpr interface(auto&& arg) noexcept : data(FWD(arg)) { }

		AGE_PROP(name)

		AGE_PROP(running)
	};

	template <typename t>
	interface(t&&) -> interface<t>;
}	 // namespace age::platform

// handle
namespace age::platform
{
	using t_window_id = uint32;

	struct window_handle
	{
		t_window_id id;
	};
}	 // namespace age::platform

namespace age::platform
{
	struct window_desc
	{
		const uint32	  width;
		const uint32	  height;
		const std::string name;
	};

	void
	init() noexcept;

	void
	update() noexcept;

	void
	deinit() noexcept;

	window_handle
	create_window(window_desc&&) noexcept;

	void
		close_window(window_handle) noexcept;

	void
	move_window(window_handle, int32 x, int32 y) noexcept;
}	 // namespace age::platform

namespace age::platform
{
	FORCE_INLINE uint32
		get_client_width(window_handle) noexcept;

	FORCE_INLINE uint32
		get_client_height(window_handle) noexcept;
}	 // namespace age::platform

namespace age::platform
{
#if defined(AGE_PLATFORM_WINDOW)
	FORCE_INLINE HWND
		get_hwnd(window_handle) noexcept;

	FORCE_INLINE window_handle
		get_handle(HWND) noexcept;
#endif
}	 // namespace age::platform
