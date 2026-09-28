#pragma once

namespace age::subsystem
{
	AGE_DEFINE_ENUM(type, uint8, platform, graphics);

	AGE_DEFINE_ENUM_FLAGS(flags, uint8,
						  (platform, (1ul << to_idx(type::platform))),
						  (graphics, (1ul << to_idx(type::graphics))));
}	 // namespace age::subsystem