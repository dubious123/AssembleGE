#pragma once

namespace age
{
	namespace config
	{
	}

	inline namespace math
	{
	}

	namespace literals
	{
	}

	namespace meta
	{
	}

	namespace util
	{
	}

	namespace data_structure
	{
	}

	namespace ecs
	{
	}

	namespace ecs::component
	{
	}

	namespace ecs::system
	{
	}

	namespace ecs::entity_block
	{
	}

	namespace ecs::entity_storage
	{
	}

	namespace platform
	{
	}

	namespace graphics
	{
	}

	namespace global
	{
	}

	namespace runtime
	{
	}

	namespace request
	{
	}
}	 // namespace age

//---[ age_pch.hpp ]------------------------------------------------------------
// Precompiled Header (build-only)
#include "age_config.hpp"
#include "age_macro.h"
#include "age_macro_foreach.h"

#include "age_external.h"
//------------------------------------------------------------------------------

#include "age_math.hpp"

#include "age_meta.hpp"
#include "age_meta_compressed_pack.hpp"
#include "age_meta_hash.hpp"

#include "age_literal.hpp"

#include "age_util.hpp"
#include "age_util_layout.hpp"

#include "age_data_structure.hpp"

#include "age_ecs.hpp"

#include "age_ecs_interface.hpp"

#include "age_ecs_component.hpp"

#include "age_ecs_system.hpp"
#include "age_ecs_system_exec_inline.hpp"
#include "age_ecs_system_ctx.hpp"

#include "age_ecs_system_pipe.hpp"
#include "age_ecs_system_cond.hpp"
#include "age_ecs_system_loop.hpp"
#include "age_ecs_system_match.hpp"

#include "age_ecs_system_adaptor.hpp"

#include "age_ecs_entity_block_basic.hpp"
#include "age_ecs_entity_storage_basic.hpp"

#include "age_subsystem.hpp"

#include "age_platform.hpp"

#include "age_graphics.hpp"

#include "age_global.hpp"

#include "age_runtime.hpp"

#include "age_request.hpp"
