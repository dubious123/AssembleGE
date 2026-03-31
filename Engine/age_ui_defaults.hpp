#pragma once
#include "age.hpp"

namespace age::ui::size_mode
{
	FORCE_INLINE constexpr widget_size_mode
	fixed(auto value) noexcept
	{
		return widget_size_mode{
			.min	   = static_cast<float>(value),
			.max	   = static_cast<float>(value),
			.size_mode = e::size_mode_kind::fixed,
		};
	}

	FORCE_INLINE constexpr widget_size_mode
	grow(auto min, auto max) noexcept
	{
		return widget_size_mode{
			.min	   = static_cast<float>(min),
			.max	   = static_cast<float>(max),
			.size_mode = e::size_mode_kind::grow,
		};
	}

	FORCE_INLINE constexpr widget_size_mode
	grow() noexcept
	{
		return widget_size_mode{
			.min	   = 0.f,
			.max	   = std::numeric_limits<float>::max(),
			.size_mode = e::size_mode_kind::grow,
		};
	}

	FORCE_INLINE constexpr widget_size_mode
	fit(auto min, auto max) noexcept
	{
		return widget_size_mode{
			.min	   = static_cast<float>(min),
			.max	   = static_cast<float>(max),
			.size_mode = e::size_mode_kind::fit,
		};
	}

	FORCE_INLINE constexpr widget_size_mode
	fit() noexcept
	{
		return widget_size_mode{
			.min	   = 0.f,
			.max	   = std::numeric_limits<float>::max(),
			.size_mode = e::size_mode_kind::fit,
		};
	}

	FORCE_INLINE constexpr widget_size_mode
	text(auto min, auto max) noexcept
	{
		return widget_size_mode{
			.min	   = static_cast<float>(min),
			.max	   = static_cast<float>(max),
			.size_mode = e::size_mode_kind::text,
		};
	}
}	 // namespace age::ui::size_mode

namespace age::ui::brush_data
{
	FORCE_INLINE constexpr ui_brush_data
	color(float r, float g, float b, float a) noexcept
	{
		return ui_brush_data{
			.data = uint32_4{
				std::bit_cast<uint32>(r),
				std::bit_cast<uint32>(g),
				std::bit_cast<uint32>(b),
				std::bit_cast<uint32>(a) }
		};
	}
}	 // namespace age::ui::brush_data
