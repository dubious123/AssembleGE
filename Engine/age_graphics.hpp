#pragma once

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