#pragma once


//---[ configs, extra ]------------------------------------------------------------------------------------------------------
#undef reg
#undef cbuffer
#undef row_major
#undef semantics
#undef SHARED_TYPE

#undef MAX_SELECTION_OUTLINE_THICKNESS
#undef MAX_BLOOM_MIP_COUNT
#undef MIN_BLOOM_MIP_PIXEL
#undef MAX_UV_COUNT
#undef MAX_RAY_HIT

//---[ sort ]------------------------------------------------------------------------------------------------------
#undef SORT_THREAD_COUNT
#undef SORT_ELEMENT_COUNT_PER_THREAD
#undef MAX_SORT_COUNT
#undef SORT_BLOCK_SIZE
#undef SORT_BLOCK_COUNT
#undef SORT_GROUP_COUNT

#undef SORT_BLOCK_COUNT_PER_GROUP
#undef SORT_BIN_BIT_WIDTH
#undef SORT_BIN_COUNT
#undef SORT_HISTOGRAM_TABLE_SIZE

//---[ light ]------------------------------------------------------------------------------------------------------
#undef MAX_LIGHT_COUNT
#undef MAX_ENV_LIGHT
#undef MAX_DIRECTIONAL_LIGHT_COUNT
#undef MAX_SHADOW_LIGHT_COUNT

#undef LIGHT_BIN_INIT_THREAD_COUNT
#undef LIGHT_CULL_THREAD_COUNT
#undef LIGHT_ZBIN_THREAD_COUNT
#undef LIGHT_BITMASK_UINT32_COUNT
#undef X_SLICE_COUNT
#undef Y_SLICE_COUNT
#undef Z_SLICE_COUNT
#undef LIGHT_AXIS_SLICE_MAX
#undef LIGHT_BIN_ENTRY_X_OFFSET
#undef LIGHT_BIN_ENTRY_Y_OFFSET
#undef LIGHT_BIN_ENTRY_Z_OFFSET
#undef LIGHT_BIN_MASK_X_OFFSET
#undef LIGHT_BIN_MASK_Y_OFFSET
#undef LIGHT_BIN_MASK_Z_OFFSET
#undef LIGHT_BIN_BUFFER_SIZE
#undef LIGHT_KIND_DIRECTIONAL
#undef LIGHT_KIND_POINT
#undef LIGHT_KIND_SPOT
#undef LIGHT_KIND_AREA
#undef LIGHT_KIND_VOLUMN

//---[ object, meshlet ]------------------------------------------------------------------------------------------------------
#undef MAX_OPAQUE_MESHLET_RENDER_DATA_COUNT
#undef MAX_OBJECT_DATA_COUNT
#undef VERTEX_KIND_P_UV0
#undef VERTEX_KIND_PN_UV0
#undef VERTEX_KIND_PNT_UV0
#undef VERTEX_KIND_P_UV1
#undef VERTEX_KIND_PN_UV1
#undef VERTEX_KIND_PNT_UV1
#undef VERTEX_KIND_P_UV2
#undef VERTEX_KIND_PN_UV2
#undef VERTEX_KIND_PNT_UV2
#undef VERTEX_KIND_P_UV3
#undef VERTEX_KIND_PN_UV3
#undef VERTEX_KIND_PNT_UV3

#undef VERTEX_SIZE_P_UV0
#undef VERTEX_SIZE_PN_UV0
#undef VERTEX_SIZE_PNT_UV0
#undef VERTEX_SIZE_P_UV1
#undef VERTEX_SIZE_PN_UV1
#undef VERTEX_SIZE_PNT_UV1
#undef VERTEX_SIZE_P_UV2
#undef VERTEX_SIZE_PN_UV2
#undef VERTEX_SIZE_PNT_UV2
#undef VERTEX_SIZE_P_UV3
#undef VERTEX_SIZE_PN_UV3
#undef VERTEX_SIZE_PNT_UV3

//---[ static buffer offset ]------------------------------------------------------------------------------------------------------
#undef OPAQUE_MSHLT_OBJECT_DATA_OFFSET
#undef OBJECT_DATA_OFFSET
#undef DIRECTIONAL_LIGHT_OFFSET
#undef UNIFIED_LIGHT_OFFSET
#undef BLOOM_OFFSET
#undef DDGI_DATA_OFFSET
#undef STATIC_BUFFER_SIZE

//---[ scratch buffer offset ]------------------------------------------------------------------------------------------------------
#undef SCRATCH_SORT_BUFFER_OFFSET
#undef SORT_KEYS_OFFSET
#undef SORT_KEYS_ALT_OFFSET
#undef SORT_VALUES_OFFSET
#undef SORT_VALUES_ALT_OFFSET
#undef SORT_HISTOGRAM_OFFSET
#undef SORT_BIN_COUNT_OFFSET
#undef SCRATCH_BUFFER_TOTAL_SIZE


//---[ rt ]------------------------------------------------------------------------------------------------------
#undef RT_MASK_OPAQUE
#undef RT_MASK_TRANSPARENT
#undef RT_MASK_MASK
#undef RT_MASK_DEBUG
#undef RT_MASK_AOT
#undef RT_MASK_ALL


//---[ ui ]------------------------------------------------------------------------------------------------------
#undef UI_SPACE_MODE_SCREEN
#undef UI_SPACE_MODE_WORLD
#undef UI_SPACE_MODE_WORLD_ALWAYS_ON_TOP
#undef UI_SPACE_MODE_WORLD_BILLBOARD

#undef UI_BRUSH_KIND_COLOR


//---[ material ]------------------------------------------------------------------------------------------------------
#undef MATERIAL_ALPHA_BLEND_OPAQUE
#undef MATERIAL_ALPHA_BLEND_MASK
#undef MATERIAL_ALPHA_BLEND_BLEND


//---[ ddgi ]------------------------------------------------------------------------------------------------------

#undef DDGI_BORDER

// do not change
#undef DDGI_IRRADIANCE_RESOLUTION
#undef DDGI_VISIBILITY_RESOLUTION
#undef DDGI_IRRADIANCE_TILE_SIZE
#undef DDGI_VISIBILITY_TILE_SIZE

#undef DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD
#undef DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP	   // wave_count

#undef DDGI_PROBE_STATE_OFF
#undef DDGI_PROBE_STATE_ACTIVE
#undef DDGI_PROBE_STATE_SLEEP
#undef DDGI_PROBE_STATE_NEW_BORN
#undef DDGI_PROBE_STATE_INSIDE_WALL

#undef DDGI_PROBE_RAY_COUNT_NEW_BORN

#undef DDGI_RAY_BUDGET
#undef DDGI_MSME_SHORT_WINDOW_BLEND

#undef DDGI_IRRADIANCE_ENERGY_CONSERVATION

#undef DDGI_VISIBILITY_SHARPNESS
#undef DDGI_VISIBILITY_BLEND_FACTOR

#undef DDGI_PROBE_WEIGHT_SCALE

#undef DDGI_DEBUG_FLAGS_RENDER_PROBE_IN_HOLE
#undef DDGI_DEBUG_FLAGS_RENDER_IRRADIANCE
#undef DDGI_DEBUG_FLAGS_RENDER_VISIBILITY
#undef DDGI_DEBUG_FLAGS_RENDER_FRONT_BACK
#undef DDGI_DEBUG_FLAGS_RENDER_LEVEL
#undef DDGI_DEBUG_FLAGS_RENDER_WEIGHT_SUM
#undef DDGI_DEBUG_FLAGS_RENDER_RAY_COUNT
#undef DDGI_DEBUG_FLAGS_RENDER_STATE
#undef DDGI_DEBUG_FLAGS_RENDER_MSME
#undef DDGI_DEBUG_FLAGS_RENDER_RAY_FACTOR
#undef DDGI_DEBUG_FLAGS_RENDER_PROBE

#undef DDGI_PREFIX_THREAD_COUNT
#undef DDGI_PREFIX_ELEMENT_PER_THREAD
#undef DDGI_PREFIX_ELEMENT_PER_GROUP

#undef DDGI_TRACE_THREAD_PER_GROUP

#undef DDGI_PROBE_MAX

#undef DDGI_NORMAL_BIAS
#undef DDGI_VIEW_BIAS

#undef DDGI_MEAN_SQ_THRESHOLD
#undef DDGI_NORMAL_BLEND
