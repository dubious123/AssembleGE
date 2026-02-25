#pragma once
#include "age.hpp"

namespace age::util
{
	struct offset_allocator
	{
		uint32 total_size;
		uint32 free_size;
	};
}	 // namespace age::util