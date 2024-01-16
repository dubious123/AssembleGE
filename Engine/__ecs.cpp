#include "__ecs.h"

namespace ecs
{
	namespace
	{
	}	 // namespace

	inline bool is_idx_valid(uint64 idx)
	{
		return idx != invalid_idx_uint64;
	}

	inline bool is_id_valid(uint64 id)
	{
		return id != invalid_id_uint64;
	}
}	 // namespace ecs