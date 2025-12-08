#pragma once

namespace age::literal
{
	consteval std::size_t
	operator"" _KiB(std::size_t k)
	{
		return k * 1024ull;
	}
}	 // namespace age::literal