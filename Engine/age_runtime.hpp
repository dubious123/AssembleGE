#pragma once

namespace age::runtime
{
	template <typename t>
	struct interface
	{
	  private:
		no_unique_addr t data;

	  public:
		constexpr interface(auto&& arg) noexcept : data(FWD(arg)) { }

		AGE_PROP(now)

		AGE_PROP(delta_time_ns)

		AGE_PROP(running)

		// FORCE_INLINE constexpr decltype((data.now))
		// now()
		//{
		//	return data.now;
		// }

		// FORCE_INLINE constexpr decltype((data.delta_time_ns))
		// delta_time_ns()
		//{
		//	return data.delta_time_ns;
		// }

		// FORCE_INLINE constexpr bool&
		// running()
		//{
		//	return data.running;
		// }
	};

	template <typename t>
	interface(t&&) -> interface<t>;
}	 // namespace age::runtime

namespace age::runtime
{
	void
	init() noexcept;

	void
	update() noexcept;
}	 // namespace age::runtime