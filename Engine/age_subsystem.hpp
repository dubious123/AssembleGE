#pragma once

namespace age::subsystem
{
	enum class type : uint8
	{
		platform = 0,
		graphics,

		count,
	};

	enum class flags : uint8
	{
		platform = 1ul << (uint8)type::platform,
		graphics = 1ul << (uint8)type::graphics,
	};

	AGE_ENUM_FLAG_OPERATORS(flags)
}	 // namespace age::subsystem