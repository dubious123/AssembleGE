#pragma once

namespace age::platform
{
	template <typename t>
	struct interface
	{
	  private:
		no_unique_addr t data;

	  public:
		constexpr interface(auto&& arg) noexcept : data(FWD(arg)) { }

		FORCE_INLINE constexpr std::string&
		name()
		{
			return data.name;
		}

		FORCE_INLINE constexpr bool&
		running()
		{
			return data.running;
		}
	};

	template <typename t>
	interface(t&&) -> interface<t>;
}	 // namespace age::platform

namespace age::platform
{
	struct window_handle;

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