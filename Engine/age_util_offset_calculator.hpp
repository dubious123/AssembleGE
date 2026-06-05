#pragma once

namespace age::util
{
	struct offset_calculator
	{
		uint64 offset;

		uint64
		operator+(uint64 byte_size) noexcept
		{
			c_auto temp	 = offset;
			offset		+= byte_size;
			return temp;
		}

		uint64
		size() const noexcept
		{
			return offset;
		}
	};
}	 // namespace age::util