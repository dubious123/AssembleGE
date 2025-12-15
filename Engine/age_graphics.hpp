#pragma once

namespace age::graphics
{
	enum class color_space : uint8
	{
		srgb,
		hdr,
		count
	};
}

namespace age::graphics
{
	template <typename t>
	struct interface
	{
	  private:
		no_unique_addr t data;

	  public:
		constexpr interface(auto&& arg) noexcept : data(FWD(arg)) { }

		AGE_PROP(display_color_space)
	};

	template <typename t>
	interface(t&&) -> interface<t>;
}	 // namespace age::graphics

namespace age::graphics
{
	void
	init() noexcept;

	void
	deinit() noexcept;

	void
	begin_frame() noexcept;

	void
	end_frame() noexcept;

	void
	render() noexcept;
}	 // namespace age::graphics