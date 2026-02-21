#pragma once

namespace age::util
{
	template <auto base, std::size_t size>
	constexpr auto
	iota_array() noexcept
	{
		auto arr = std::array<BARE_OF(base), size>{};

		auto it = std::ranges::begin(std::views::iota(base) | std::views::take(size));
		for (auto& elem : arr)
		{
			elem = *it++;
		}
		return arr;
	}
}	 // namespace age::util