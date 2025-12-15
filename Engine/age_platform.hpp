#pragma once

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
	struct window_handle
	{
	  private:
		std::uintptr_t data;

	  public:
#if defined(AGE_PLATFORM_WINDOW)
		FORCE_INLINE HWND
		hwnd() const noexcept;
#endif
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

	void
	creat_window(window_desc&&) noexcept;

	void close_window(window_handle);
}	 // namespace age::platform