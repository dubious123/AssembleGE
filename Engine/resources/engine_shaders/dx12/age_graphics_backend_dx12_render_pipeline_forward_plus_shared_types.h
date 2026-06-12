#pragma once

// bloom
#define MAX_BLOOM_MIP_COUNT 7
#define MIN_BLOOM_MIP_PIXEL 8

#if !defined(AGE_SHADER)
	#include "age.hpp"

	#define reg(...)
	#define cbuffer struct
	#define row_major
	#define semantics(...)
	#define SHARED_TYPE shared_type::
#else
	#define SHARED_TYPE
#endif

#if !defined(AGE_SHADER)
namespace age::graphics::render_pipeline::forward_plus
{
#endif
	// shared type
	using t_object_id			 = uint32;
	using t_mesh_id				 = uint32;
	using t_camera_id			 = uint32;
	using t_directional_light_id = uint16;
	using t_unified_light_id	 = uint32;
	using t_texture_id			 = uint32;
	using t_material_id			 = uint32;
	using t_env_light_id		 = uint32;
	using t_env_light_id		 = uint32;
	using t_raycast_id			 = uint32;
	using t_bloom_id			 = uint16;
#if !defined(AGE_SHADER)

}	 // namespace age::graphics::render_pipeline::forward_plus

namespace age::graphics::render_pipeline::forward_plus::shared_type
{
#endif
	struct indirect_arg
	{
		uint32_3 dispatch_xyz;
	};

	//---[ gibs, surfel ]------------------------------------------------------------
	struct gibs_data
	{
		uint32 debug_flags;
		uint32 max_surfel_count;
		uint32 ray_count_reduce_group_count;
		uint32 cell_count;	  // per axis, level 0, power of 2
		uint32 cell_count_total;
		uint32 outer_layer_count;

		uint32 h_surfel_buffer_srv_id;
		uint32 h_cell_info_buffer_srv_id;
		uint32 h_irradiance_atlas_srv_id;
		uint32 h_visibility_atlas_srv_id;
		uint32 h_gbuffer_srv_id;

		uint32 h_surfel_buffer_uav_id;
		uint32 h_cell_info_buffer_uav_id;
		uint32 h_scratch_buffer_uav_id;
		uint32 h_irradiance_atlas_uav_id;
		uint32 h_visibility_atlas_uav_id;

		uint32 alive_surfel_id_stack_prev_offset;
		uint32 alive_surfel_id_stack_curr_offset;
		// todo, optimize
		// uint32 alive_dead_counter_offset = 0;
		// uint32 cell_atomic_counter_offset = sizeof(uint32) * 2;
		// uint32 alive_dead_list_offset = sizeof(uint32) * 3;
		uint32 surfel_geometry_offset;
		uint32 surfel_msme_offset;
		uint32 surfel_recycle_data_offset;
		uint32 ray_count_ideal_offset;
		uint32 ray_prefix_offset;
		uint32 ray_group_sum_offset;
		uint32 ray_group_prefix_offset;
		uint32 ray_result_offset;
	};

	struct gibs_lut_data
	{
		// cell_size_arr[0] : xyz size
		// cell_size_arr[i > 0] : xy size
		// z size = boundary[i] - boundary[i-1]
		float cell_size_arr[16 + 1];
		float layer_boundary_arr[16 + 1];
		float surfel_distance_to_radius;
	};

	struct gibs_counter
	{
		uint32 cell_to_surfel_id;
		uint32 ray_count_ideal;
	};

	struct surfel
	{
		float3 position;
		float  radius;
		float3 radiance;
		uint32 normal_oct_snorm16;

		// todo remove
		uint32 alive_idx;

		void
		kill()
		{
			radius = 0.f;
		}
	};

	struct gibs_cell_entry
	{
		// cell_entry_buffer[xyz] = cell_entry
		// cell_to_surfel_buffer[cell_enttry.offset + i] = surfel_id

		uint32 offset;
		uint32 count;

		static gibs_cell_entry
		init(uint32 offset, uint32 count)
		{
			gibs_cell_entry res;
			res.offset = offset;
			res.count  = count;
			return res;
		}
	};

	struct surfel_geometry
	{
		uint32 object_id;
		uint32 local_normal_oct_snorm16;
		float3 local_pos;
	};

	struct surfel_msme
	{
		float3 mean_long;
		float  inconsistency;
		float3 mean_short;
		float  vbbr;
		float3 variance;
	};

	struct surfel_recycle_data
	{
		uint16 frame_since_seen_and_extra;	  // [frame_since_seen(8)][frame_since_ref(8)]
		uint16 frame_since_born;

		uint16
		frame_since_ref()
		{
			return uint16((frame_since_seen_and_extra >> 8u) & 0xff);
		}

		void
		add_ref()
		{
			frame_since_seen_and_extra &= 0x00ff;	 // frame_since_ref = 0
		}

		uint16
		frame_since_seen()
		{
			return uint16(frame_since_seen_and_extra & 0xff);
		}

		bool
		is_new_born()
		{
			return frame_since_born == 0;
		}

		void
		next_frame(bool seen)
		{
			frame_since_born = uint16(min(frame_since_born, uint16(0xfffe)) + 1u);

			uint16 since_ref  = frame_since_ref();
			uint16 since_seen = seen ? uint16(0) : frame_since_seen();

			since_ref  = uint16(min(since_ref, uint16(0x00fe)) + 1u);
			since_seen = uint16(min(since_seen, uint16(0x00fe)) + 1u);

			frame_since_seen_and_extra = uint16(since_seen | (since_ref << 8u));
		}
	};

	struct gibs_ray_result
	{
		uint32 radiance_r11g11b10;
		float  distance;
		uint32 dir_oct_snorm8;	  // [local_dir_world_hemi_oct_snomr8(16)] [world_dir_oct_snorm8(16)]
		float  pdf;
	};

	struct gibs_ray_entry
	{
		uint32 surfel_id;
		uint32 local_ray_id;
	};

	//---[ ddgi ]------------------------------------------------------------
	struct ddgi_data
	{
		uint32 ppl_log_2_and_ppl_bitwidth;		  // [x_log2(8)][y_log2(8)][z_log2(8)][ bitwidth(x + y + z)(8) ]
		float3 base_probe_spacing;
		uint32 level_count__tile_count_w_log2;	  //[level_count(8)][tile_count_w_log2(8)][][]
		uint32 irradiance_atlas_srv_id;
		uint32 irradiance_atlas_uav_id;

		uint32 visibility_atlas_srv_id;
		uint32 visibility_atlas_uav_id;

		float tile_count_h_float;

		uint32 debug_flags;
		// [render probe in hole]
		// [render irradiance]
		// [render visibility]
		// [render front_back]
		// [render level]
		// [render weight_sum]
		// [render ray_count]
		// [render state]
		// [render probe]

		// uint32 weight_sum_offset = 0u;
		uint32 ray_count_offset;		   // ideal (probe_state_cs) -> real
		uint32 ray_prefix_offset;		   // ideal group sum (probe_state_cs) -> real prefix (block-local -> global offset)
		uint32 ray_group_count_offset;	   // real block_sum (prefix p1 -> p2)
		uint32 ray_group_prefix_offset;	   // ideal_sum (slot 0, probe_state_cs/reduction) -> real ray_count_total (idx 0) + group_prefix (idx 1..)
		uint32 ray_result_offset;		   // trace result
	};

	struct ddgi_msme
	{
		float mean_long;
		float mean_short;
		float variance;	   // relative variance = variance / mean_sq
		float inconsistency;
		float vbbr;
	};

	struct ddgi_probe
	{
		// uint16 normal_oct_snorm8;
		uint16 frame_since_seen;
		half3  offset;
		uint16 state;				  // [state(8)][reallocated(1) ]
		uint32 world_coord_packed;	  // 10 10 10 2

		ddgi_msme msme;
	};

	struct ddgi_ray_result
	{
		uint32 radiance_r11g11b10;		   // 4
		float  distance;				   // 4
		uint32 dir_oct_snorm_and_extra;	   // 4
	};

	//---[ bloom ]------------------------------------------------------------
	struct bloom
	{
		float  threshold;
		float  knee;
		float  intensity;
		float  radius;
		float3 tint;
		uint16 width;			  // mip 0
		uint16 height;			  // mip 0
		uint32 uav_texture_id_arr[MAX_BLOOM_MIP_COUNT];
		uint32 srv_texture_id;	  // invalid_id : no bloom
	};

	//---[ ui ]------------------------------------------------------------
	struct ui_root_data
	{
		float3 world_pos;
		float4 quaternion;

		float width;
		float height;

		float world_width;
		float world_height;
	};

	struct ui_shape_data
	{
		uint32 data[5];			// TBD, corner radius, circle radius, ...
	};

	struct ui_brush_data
	{
		uint32_4 data;			// TBD, texture_id, static_color, calculate from shadow with delta time...
	};

	struct ui_data
	{
		float2 pivot_pos;		// screen pos of pivot
		float2 pivot_uv;
		float2 size;			// pixel size
		float  rotation;		// z rotation, radian
		float  border_thickness;

		float4 clip_rect;

		uint32 packed_enums;	// [shape_kind(8bit)][body_brush_kind(8bit)][border_brush_kind(8bit)][fit_mode_kind(8bit)]

		ui_shape_data shape_data;
		ui_brush_data body_brush_data;
		ui_brush_data border_brush_data;
	};

	//---[ lights ]------------------------------------------------------------
	// struct light_cull_data
	//{
	//	uint32 not_culled_light_count;
	//};

	struct zbin_entry
	{
		uint32 min_idx;
		uint32 max_idx;
	};

	struct directional_light
	{
		float3 direction;				 // 12
		float  intensity;				 // 4
		float3 color;					 // 12
		uint32 cast_shadow_and_extra;	 // 4
	};	  // 32 bytes

	struct unified_light
	{
		float3 position;				 // 12
		float  range;					 // 4
		half3  color;					 // 6
		half   intensity;				 // 2
		half3  direction;				 // 6
		half   cos_inner;				 // 2
		half   cos_outer;				 // 2
		uint16 cast_shadow_and_extra;	 // 2
#if !defined(AGE_SHADER)

		float
		min_z() const
		{
			return position.z - range;
		}
#endif


	};	  // total: 36 bytes

	//---[ ibl ]------------------------------------------------------------
	struct env_light
	{
		uint32	 radiance_tex_id;		 // 4
		uint32	 prefilter_tex_id;		 // 4
		uint32	 irradiance_tex_id;		 // 4
		uint16	 prefilter_mip_count;	 // 2
		half	 intensity;				 // 2
		float3	 tint;					 // 12
		float3x3 rotation_mat;			 // 36
	};	  // total: 64 bytes

	//---[ mesh ]------------------------------------------------------------
	// data only used by amplification shader
	struct meshlet_header
	{
		uint16 cone_axis_oct;

		uint16 cone_cull_cutoff_and_extra;

		int16_3	 aabb_min;						 // 6byte
		uint16_3 aabb_size;						 // 6byte
	};

	struct meshlet
	{
		uint32 global_index_offset;				 // 4 bytes
		uint32 primitive_offset;				 // 4 bytes

		uint32 vertex_count_prim_count_extra;	 // [vertex_count(8bit)][primitive_count(8bit)][extra(16bit)]
	};

	struct object_data
	{
		float3 pos;								 // 12
												 // uint32 quaternion;						 // 4 | 10 10 10 2
												 // uint32_2 quaternion;					 // 8 | 16 16 16 2
		float4 quaternion;
		//  half3  scale;							 // 6
		//  uint16 extra;
		float3 scale;
		// float4 quaternion;
		//  uint16_t extra;	   // 2

		uint32 gen_and_extra;	 // [gen(4)][extra(28)]

		uint16
		gen()
		{
			return uint16(gen_and_extra & 0xf);
		}
	};	  // total: 24 bytes

	struct debug_object_data
	{
		uint32 object_id;
		float3 color;
	};

	struct opaque_meshlet_render_data
	{
		uint32 object_id;
		uint32 mesh_byte_offset;
		uint32 mesh_chunk_srv_id;
		uint32 meshlet_id;
		uint32 material_id;
	};

	struct rt_instance_render_data
	{
		uint32 object_id;
		uint32 mesh_byte_offset;
		uint32 mesh_chunk_srv_id;
		uint32 rt_index_buffer_offset;
		uint32 material_id;
		uint32 rt_mask_and_extra;
		// todo mat_id
	};

	struct debug_meshlet_render_data
	{
		uint32 debug_object_id;
		uint32 mesh_byte_offset;
		uint32 mesh_chunk_srv_id;
		uint32 meshlet_id;
	};

	struct mesh_header
	{
		// uint32 vertex_offset = sizeof(mesh_baked_header), sizeof(mesh_baked_header) == 20
#if defined(AGE_SHADER)
		uint32				vertex_buffer_offset;	 // not from gpu, calculated from read_mesh_header function
		byte_address_buffer mesh_chunk_srv;
#endif

		uint32 vertex_kind_and_extra;
		uint32 global_vertex_index_buffer_offset;
		uint32 local_vertex_index_buffer_offset;
		uint32 meshlet_header_buffer_offset;
		uint32 meshlet_buffer_offset;
		uint32 meshlet_count;
		float3 aabb_min;
		float3 aabb_size;
	};

	struct material
	{
		float4	 base_color_factor;
		float	 metallic_factor;
		float	 roughness_factor;
		float3	 emissive_factor;
		float	 normal_scale;
		float	 occlusion_strength;
		float	 alpha_cutoff;
		uint32	 alpha_mode_and_extra;
		uint32_2 _;

		uint32 base_color_texture_id;
		uint32 metallic_roughness_texture_id;
		uint32 normal_texture_id;
		uint32 occlusion_texture_id;
		uint32 emissive_texture_id;
	};

	//---[ raycast ]------------------------------------------------------------
	struct raycast_request
	{
		float3 origin;
		float3 dir;
		float  t_max;
		uint32 mask_and_extra;
	};

	struct raycast_result
	{
		uint32 object_id;	 // invalid_id == no hit
		float  t_hit;
		float3 world_pos;
	};

	//---[ selection outline ]------------------------------------------------------------
	struct selection_outline_meshlet_render_data
	{
		uint32 object_id;
		uint32 mesh_byte_offset;
		uint32 mesh_chunk_srv_id;
		uint32 meshlet_id;
		uint32 selection_outline_id_and_extra;	  // [id(8)][extra(24)]
	};

	struct selection_outline_data
	{
		float4 rgba;
		uint16 thickness_and_softness;	  // [thickness(2.6 fixed_point, 8bits)][softness(unorm8, 8bits)]
		uint16 extra;
	};

	//----------------------------------------------------------------------------

	cbuffer frame_data reg(b0)
	{
		row_major float4x4 view;						// 64
		row_major float4x4 view_proj;					// 64
		row_major float4x4 view_proj_inv;				// 64
		float3			   camera_pos;					// 12
		float			   time;						// 4

		float4 frustum_planes[6];						// 96

		float2 inv_backbuffer_size;						// 8
		float2 backbuffer_size;							// 8

		float3 camera_forward;							// 12
		uint32 frame_index;								// 4

		uint32 main_buffer_texture_id;					// 4
		uint32 post_buffer_texture_id;					// 4
		uint32 depth_buffer_texture_id;					// 4
		uint32 rt_tlas_buffer_id;						// 4

		uint32 rt_transparent_buffer_srv_texture_id;	// 4
		uint32 rt_transparent_buffer_uav_texture_id;	// 4
		uint32 rt_raycast_request_count;				// 4
		float  proj_00;

		float3 light_bin_origin;
		float  proj_11;
		float3 light_bin_cell_size_inv;
		float  cam_near_z;
		float  cam_far_z;
		uint32 ddgi_enabled_and_extra;	  // [ddgi_enabled(1)][gibs_enabled(1)]
		float2 ddgi_cranley_patterson_rotation;
		float3 gi_origin;
		uint32 object_count;

		uint32 selection_outline_meshlet_render_data_count;
		uint32 selection_outline_mask_buffer_srv_texture_id;
		uint32 env_light_brdf_lut_id;
		uint32 env_light_count;

		float	 tan_fov_y_half;	// tan(fov_y * 0.5f)
		uint32_3 _;

		uint32_4 extra[3];
		// total: 256 * 2 bytes
	};

	cbuffer root_constants reg(b1)
	{
		uint32			   opaque_meshlet_render_data_count;	 // 4 bytes
		uint32			   directional_light_count_and_extra;	 // 4 bytes
		t_unified_light_id unified_light_count;					 // 4 btyes


		uint32 radix_sort_pass;
		uint32 ui_space_mode_and_extra;							 // [ui_space_mode(8)][extra]
		uint32 ui_root_data_idx;
		uint32 ui_data_id_offset;
		uint32 ui_data_count;


		uint32 debug_meshlet_render_data_offset;
		uint32 debug_meshlet_render_data_count;

		uint32 bloom_mip_level_and_extra;	 // src_mip, target_mip == src_mip + 1 or src_mip - 1
	};
#if !defined(AGE_SHADER)
}	 // namespace age::graphics::render_pipeline::forward_plus::shared_type
namespace age::graphics::render_pipeline::forward_plus::g
{
#endif

	//---[ configs, extra ]------------------------------------------------------------------------------------------------------
#define MAX_SELECTION_OUTLINE_THICKNESS 2
#define MAX_UV_COUNT					2
#define MAX_RAY_HIT						8
	// bloom
#if !defined(AGE_SHADER)
	inline constexpr auto max_bloom_mip_count = uint16{ MAX_BLOOM_MIP_COUNT };
	inline constexpr auto min_bloom_mip_pixel = uint16{ MIN_BLOOM_MIP_PIXEL };
#endif

//---[ sort ]------------------------------------------------------------------------------------------------------
#define SORT_THREAD_COUNT			  128
#define SORT_ELEMENT_COUNT_PER_THREAD 4
#define MAX_SORT_COUNT				  (512 * 512)
#define SORT_BLOCK_SIZE				  (SORT_THREAD_COUNT * SORT_ELEMENT_COUNT_PER_THREAD)
#define SORT_BLOCK_COUNT			  ((MAX_SORT_COUNT + SORT_BLOCK_SIZE - 1) / SORT_BLOCK_SIZE)
#define SORT_GROUP_COUNT			  (min(SORT_BLOCK_SIZE, SORT_BLOCK_COUNT))

#define SORT_BLOCK_COUNT_PER_GROUP ((SORT_BLOCK_COUNT + SORT_GROUP_COUNT - 1) / SORT_GROUP_COUNT)
#define SORT_BIN_BIT_WIDTH		   4
#define SORT_BIN_COUNT			   (1 << SORT_BIN_BIT_WIDTH)
#define SORT_HISTOGRAM_TABLE_SIZE  (SORT_BIN_COUNT * SORT_GROUP_COUNT)

#if !defined(AGE_SHADER)
	static_assert(MAX_SORT_COUNT % SORT_THREAD_COUNT == 0);
	static_assert(MAX_SORT_COUNT <= SORT_GROUP_COUNT * SORT_BLOCK_COUNT_PER_GROUP * SORT_BLOCK_SIZE);
	static_assert(SORT_GROUP_COUNT <= SORT_BLOCK_SIZE);
	static_assert(SORT_BIN_BIT_WIDTH == 4);
	static_assert(SORT_THREAD_COUNT <= 0xff);
	static_assert(SORT_THREAD_COUNT > 0);
	static_assert(std::popcount<uint32>(SORT_THREAD_COUNT) == 1);

	inline constexpr uint32 max_sort_count		 = MAX_SORT_COUNT;
	inline constexpr uint32 sort_thread_count	 = SORT_THREAD_COUNT;
	inline constexpr uint32 sort_bin_count		 = SORT_BIN_COUNT;
	inline constexpr uint32 sort_iteration_count = sizeof(float) * 8 / SORT_BIN_BIT_WIDTH;
	inline constexpr uint32 sort_group_count	 = SORT_GROUP_COUNT;
#endif


	//---[ light ]------------------------------------------------------------------------------------------------------
#define MAX_LIGHT_COUNT				(512 * 512)
#define MAX_ENV_LIGHT				8
#define MAX_DIRECTIONAL_LIGHT_COUNT 2
#define MAX_SHADOW_LIGHT_COUNT		100

#define LIGHT_BIN_INIT_THREAD_COUNT 256
#define LIGHT_CULL_THREAD_COUNT		256
#define LIGHT_ZBIN_THREAD_COUNT		256
#define LIGHT_BITMASK_UINT32_COUNT	(MAX_LIGHT_COUNT / 32)
#define X_SLICE_COUNT				(512)
#define Y_SLICE_COUNT				(512)
#define Z_SLICE_COUNT				(512)
#define LIGHT_AXIS_SLICE_MAX		(512)
#define LIGHT_BIN_ENTRY_X_OFFSET	(0)
#define LIGHT_BIN_ENTRY_Y_OFFSET	(LIGHT_BIN_ENTRY_X_OFFSET + X_SLICE_COUNT * sizeof(SHARED_TYPE zbin_entry))
#define LIGHT_BIN_ENTRY_Z_OFFSET	(LIGHT_BIN_ENTRY_Y_OFFSET + Y_SLICE_COUNT * sizeof(SHARED_TYPE zbin_entry))
#define LIGHT_BIN_MASK_X_OFFSET		(LIGHT_BIN_ENTRY_Z_OFFSET + Z_SLICE_COUNT * sizeof(SHARED_TYPE zbin_entry))
#define LIGHT_BIN_MASK_Y_OFFSET		(LIGHT_BIN_MASK_X_OFFSET + LIGHT_BITMASK_UINT32_COUNT * sizeof(uint32) * X_SLICE_COUNT)
#define LIGHT_BIN_MASK_Z_OFFSET		(LIGHT_BIN_MASK_Y_OFFSET + LIGHT_BITMASK_UINT32_COUNT * sizeof(uint32) * Y_SLICE_COUNT)
#define LIGHT_BIN_BUFFER_SIZE		(LIGHT_BIN_MASK_Z_OFFSET + LIGHT_BITMASK_UINT32_COUNT * sizeof(uint32) * Z_SLICE_COUNT)
#define LIGHT_KIND_DIRECTIONAL		0
#define LIGHT_KIND_POINT			1
#define LIGHT_KIND_SPOT				2
#define LIGHT_KIND_AREA				3
#define LIGHT_KIND_VOLUMN			4
#if !defined(AGE_SHADER)
	static_assert(MAX_LIGHT_COUNT <= MAX_SORT_COUNT);
	static_assert(LIGHT_AXIS_SLICE_MAX == max(X_SLICE_COUNT, Y_SLICE_COUNT, Z_SLICE_COUNT));

	inline constexpr uint32 max_light_count				= MAX_LIGHT_COUNT;
	inline constexpr auto	max_env_light				= MAX_ENV_LIGHT;
	inline constexpr auto	max_directional_light_count = MAX_DIRECTIONAL_LIGHT_COUNT;
	inline constexpr auto	max_shadow_light_count		= MAX_SHADOW_LIGHT_COUNT;

	inline constexpr uint32 light_bin_stage_buffer_size	 = LIGHT_BIN_BUFFER_SIZE;
	inline constexpr float3 light_axis_slice_count_float = float3{ X_SLICE_COUNT, Y_SLICE_COUNT, Z_SLICE_COUNT };
	inline constexpr uint32 light_axis_slice_sum		 = X_SLICE_COUNT + Y_SLICE_COUNT + Z_SLICE_COUNT;
	inline constexpr uint32 light_bin_init_thread_count	 = LIGHT_BIN_INIT_THREAD_COUNT;
	inline constexpr uint32 light_cull_thread_count		 = LIGHT_CULL_THREAD_COUNT;
	inline constexpr uint32 light_bitmask_uint32_count	 = LIGHT_BITMASK_UINT32_COUNT;
	inline constexpr uint32 zbin_thread_count			 = LIGHT_ZBIN_THREAD_COUNT;
#endif

	//---[ object, meshlet ]------------------------------------------------------------------------------------------------------
#define MAX_OPAQUE_MESHLET_RENDER_DATA_COUNT (1u << 24)
#define MAX_OBJECT_DATA_COUNT				 (1u << 20)

#define VERTEX_KIND_P_UV0	0
#define VERTEX_KIND_PN_UV0	1
#define VERTEX_KIND_PNT_UV0 2
#define VERTEX_KIND_P_UV1	3
#define VERTEX_KIND_PN_UV1	4
#define VERTEX_KIND_PNT_UV1 5
#define VERTEX_KIND_P_UV2	6
#define VERTEX_KIND_PN_UV2	7
#define VERTEX_KIND_PNT_UV2 8
#define VERTEX_KIND_P_UV3	9
#define VERTEX_KIND_PN_UV3	10
#define VERTEX_KIND_PNT_UV3 11

#define VERTEX_SIZE_P_UV0	12
#define VERTEX_SIZE_PN_UV0	12
#define VERTEX_SIZE_PNT_UV0 16
#define VERTEX_SIZE_P_UV1	16
#define VERTEX_SIZE_PN_UV1	16
#define VERTEX_SIZE_PNT_UV1 20
#define VERTEX_SIZE_P_UV2	20
#define VERTEX_SIZE_PN_UV2	20
#define VERTEX_SIZE_PNT_UV2 24
#define VERTEX_SIZE_P_UV3	24
#define VERTEX_SIZE_PN_UV3	24
#define VERTEX_SIZE_PNT_UV3 28
#if !defined(AGE_SHADER)
	inline constexpr auto max_object_count = MAX_OBJECT_DATA_COUNT;

	static_assert(MAX_OBJECT_DATA_COUNT < (1u << 28u));

	static_assert(VERTEX_KIND_P_UV0 == to_idx(age::asset::e::vertex_kind::p_uv0));
	static_assert(VERTEX_KIND_PN_UV0 == to_idx(age::asset::e::vertex_kind::pn_uv0));
	static_assert(VERTEX_KIND_PNT_UV0 == to_idx(age::asset::e::vertex_kind::pnt_uv0));
	static_assert(VERTEX_KIND_P_UV1 == to_idx(age::asset::e::vertex_kind::p_uv1));
	static_assert(VERTEX_KIND_PN_UV1 == to_idx(age::asset::e::vertex_kind::pn_uv1));
	static_assert(VERTEX_KIND_PNT_UV1 == to_idx(age::asset::e::vertex_kind::pnt_uv1));
	static_assert(VERTEX_KIND_P_UV2 == to_idx(age::asset::e::vertex_kind::p_uv2));
	static_assert(VERTEX_KIND_PN_UV2 == to_idx(age::asset::e::vertex_kind::pn_uv2));
	static_assert(VERTEX_KIND_PNT_UV2 == to_idx(age::asset::e::vertex_kind::pnt_uv2));
	static_assert(VERTEX_KIND_P_UV3 == to_idx(age::asset::e::vertex_kind::p_uv3));
	static_assert(VERTEX_KIND_PN_UV3 == to_idx(age::asset::e::vertex_kind::pn_uv3));
	static_assert(VERTEX_KIND_PNT_UV3 == to_idx(age::asset::e::vertex_kind::pnt_uv3));

	static_assert(VERTEX_SIZE_P_UV0 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::p_uv0>));
	static_assert(VERTEX_SIZE_PN_UV0 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pn_uv0>));
	static_assert(VERTEX_SIZE_PNT_UV0 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pnt_uv0>));
	static_assert(VERTEX_SIZE_P_UV1 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::p_uv1>));
	static_assert(VERTEX_SIZE_PN_UV1 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pn_uv1>));
	static_assert(VERTEX_SIZE_PNT_UV1 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pnt_uv1>));
	static_assert(VERTEX_SIZE_P_UV2 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::p_uv2>));
	static_assert(VERTEX_SIZE_PN_UV2 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pn_uv2>));
	static_assert(VERTEX_SIZE_PNT_UV2 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pnt_uv2>));
	static_assert(VERTEX_SIZE_P_UV3 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::p_uv3>));
	static_assert(VERTEX_SIZE_PN_UV3 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pn_uv3>));
	static_assert(VERTEX_SIZE_PNT_UV3 == sizeof(age::asset::t_vertex_kind<age::asset::e::vertex_kind::pnt_uv3>));
#endif

	//---[ scratch buffer offset ]------------------------------------------------------------------------------------------------------
#define SCRATCH_SORT_BUFFER_OFFSET (0)
#define SORT_KEYS_OFFSET		   (SCRATCH_SORT_BUFFER_OFFSET)
#define SORT_KEYS_ALT_OFFSET	   (SORT_KEYS_OFFSET + MAX_SORT_COUNT * sizeof(uint32))
#define SORT_VALUES_OFFSET		   (SORT_KEYS_ALT_OFFSET + MAX_SORT_COUNT * sizeof(uint32))
#define SORT_VALUES_ALT_OFFSET	   (SORT_VALUES_OFFSET + MAX_SORT_COUNT * sizeof(uint32))
#define SORT_HISTOGRAM_OFFSET	   (SORT_VALUES_ALT_OFFSET + MAX_SORT_COUNT * sizeof(uint32))
#define SORT_BIN_COUNT_OFFSET	   (SORT_HISTOGRAM_OFFSET + SORT_HISTOGRAM_TABLE_SIZE * sizeof(uint32))
#define SCRATCH_BUFFER_TOTAL_SIZE  (SORT_BIN_COUNT_OFFSET + SORT_BIN_COUNT * sizeof(uint32))
#if !defined(AGE_SHADER)
	inline constexpr uint32 scratch_buffer_total_size = SCRATCH_BUFFER_TOTAL_SIZE;
#endif
	//---[ rt ]------------------------------------------------------------------------------------------------------
#define RT_MASK_OPAQUE		0x01
#define RT_MASK_TRANSPARENT 0x02
#define RT_MASK_MASK		0x04
#define RT_MASK_DEBUG		0x08
#define RT_MASK_AOT			0x80
#define RT_MASK_ALL			0xff
#if !defined(AGE_SHADER)
	static_assert(RT_MASK_OPAQUE == to_idx(age::graphics::e::rt_mask_kind::opaque));
	static_assert(RT_MASK_TRANSPARENT == to_idx(age::graphics::e::rt_mask_kind::transparent));
	static_assert(RT_MASK_MASK == to_idx(age::graphics::e::rt_mask_kind::mask));
	static_assert(RT_MASK_DEBUG == to_idx(age::graphics::e::rt_mask_kind::debug));
	static_assert(RT_MASK_AOT == to_idx(age::graphics::e::rt_mask_kind::always_on_top));
	static_assert(RT_MASK_ALL == to_idx(age::graphics::e::rt_mask_kind::all));
#endif
	//---[ ui ]------------------------------------------------------------------------------------------------------
#define UI_SPACE_MODE_SCREEN			  0
#define UI_SPACE_MODE_WORLD				  1
#define UI_SPACE_MODE_WORLD_ALWAYS_ON_TOP 2
#define UI_SPACE_MODE_WORLD_BILLBOARD	  3

#define UI_BRUSH_KIND_COLOR 0
#if !defined(AGE_SHADER)
	static_assert(sizeof(age::ui::render_data) == sizeof(shared_type::ui_data));

	static_assert(UI_SPACE_MODE_SCREEN == to_idx(age::ui::e::space_mode_kind::screen));
	static_assert(UI_SPACE_MODE_WORLD == to_idx(age::ui::e::space_mode_kind::world));
	static_assert(UI_SPACE_MODE_WORLD_ALWAYS_ON_TOP == to_idx(age::ui::e::space_mode_kind::world_always_on_top));
	static_assert(UI_SPACE_MODE_WORLD_BILLBOARD == to_idx(age::ui::e::space_mode_kind::world_billboard));

	static_assert(UI_BRUSH_KIND_COLOR == to_idx(age::ui::e::brush_kind::color));

	inline constexpr auto max_ui_z_count = 128;
#endif

	//---[ material ]------------------------------------------------------------------------------------------------------
#define MATERIAL_ALPHA_BLEND_OPAQUE 0
#define MATERIAL_ALPHA_BLEND_MASK	1
#define MATERIAL_ALPHA_BLEND_BLEND	2
#if !defined(AGE_SHADER)
	static_assert(MATERIAL_ALPHA_BLEND_OPAQUE == to_idx(age::asset::e::alpha_mode_kind::opaque));
	static_assert(MATERIAL_ALPHA_BLEND_MASK == to_idx(age::asset::e::alpha_mode_kind::mask));
	static_assert(MATERIAL_ALPHA_BLEND_BLEND == to_idx(age::asset::e::alpha_mode_kind::blend));
#endif


	//---[ ddgi ]------------------------------------------------------------------------------------------------------
#define DDGI_BORDER 1

// do not change
#define DDGI_IRRADIANCE_RESOLUTION 8
#define DDGI_VISIBILITY_RESOLUTION 16
#define DDGI_IRRADIANCE_TILE_SIZE  (DDGI_IRRADIANCE_RESOLUTION + DDGI_BORDER * 2)
#define DDGI_VISIBILITY_TILE_SIZE  (DDGI_VISIBILITY_RESOLUTION + DDGI_BORDER * 2)

#define DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD 32u
#define DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP 32u	// wave_count

#define DDGI_PROBE_STATE_OFF		 3u
#define DDGI_PROBE_STATE_ACTIVE		 1u
#define DDGI_PROBE_STATE_SLEEP		 2u
#define DDGI_PROBE_STATE_NEW_BORN	 0u
#define DDGI_PROBE_STATE_INSIDE_WALL 4u

#define DDGI_PROBE_RAY_COUNT_NEW_BORN (0xffu)

#define DDGI_RAY_BUDGET				 (1u << 20u)
#define DDGI_MSME_SHORT_WINDOW_BLEND 0.08f

#define DDGI_IRRADIANCE_ENERGY_CONSERVATION 0.95f

#define DDGI_VISIBILITY_SHARPNESS	 50
#define DDGI_VISIBILITY_BLEND_FACTOR 0.05f

#define DDGI_PROBE_WEIGHT_SCALE 100000u

#define DDGI_DEBUG_FLAGS_RENDER_PROBE_IN_HOLE (1u << 0u)
#define DDGI_DEBUG_FLAGS_RENDER_IRRADIANCE	  (1u << 1u)
#define DDGI_DEBUG_FLAGS_RENDER_VISIBILITY	  (1u << 2u)
#define DDGI_DEBUG_FLAGS_RENDER_FRONT_BACK	  (1u << 3u)
#define DDGI_DEBUG_FLAGS_RENDER_LEVEL		  (1u << 4u)
#define DDGI_DEBUG_FLAGS_RENDER_WEIGHT_SUM	  (1u << 5u)
#define DDGI_DEBUG_FLAGS_RENDER_RAY_COUNT	  (1u << 6u)
#define DDGI_DEBUG_FLAGS_RENDER_STATE		  (1u << 7u)
#define DDGI_DEBUG_FLAGS_RENDER_MSME		  (1u << 8u)
#define DDGI_DEBUG_FLAGS_RENDER_RAY_FACTOR	  (1u << 9u)
#define DDGI_DEBUG_FLAGS_RENDER_PROBE		  (1u << 31u)

#define DDGI_PREFIX_THREAD_COUNT	   1024u
#define DDGI_PREFIX_ELEMENT_PER_THREAD 16u
#define DDGI_PREFIX_ELEMENT_PER_GROUP  (DDGI_PREFIX_THREAD_COUNT * DDGI_PREFIX_ELEMENT_PER_THREAD)

#define DDGI_TRACE_THREAD_PER_GROUP 32u

#define DDGI_PROBE_MAX (DDGI_PREFIX_ELEMENT_PER_GROUP * DDGI_PREFIX_ELEMENT_PER_GROUP)

#define DDGI_NORMAL_BIAS 0.02f
#define DDGI_VIEW_BIAS	 0.00001f

#define DDGI_MEAN_SQ_THRESHOLD 0.01f
#define DDGI_NORMAL_BLEND	   0.05f

#if !defined(AGE_SHADER)
	inline constexpr auto ddgi_irradiance_tile_size = DDGI_IRRADIANCE_TILE_SIZE;
	inline constexpr auto ddgi_visibility_tile_size = DDGI_VISIBILITY_TILE_SIZE;

	inline constexpr auto ddgi_irradiance_resolution = DDGI_IRRADIANCE_RESOLUTION;
	inline constexpr auto ddgi_visibility_resolution = DDGI_VISIBILITY_RESOLUTION;

	inline constexpr auto ddgi_update_probe_state_probe_per_group = DDGI_UPDATE_PROBE_STATE_THREAD_PER_GROUP * DDGI_UPDATE_PROBE_STATE_PROBE_PER_THREAD;

	inline constexpr auto ddgi_prefix_element_per_group = DDGI_PREFIX_ELEMENT_PER_GROUP;
	inline constexpr auto ddgi_ray_budget				= DDGI_RAY_BUDGET;

	static_assert(DDGI_VISIBILITY_RESOLUTION >= DDGI_IRRADIANCE_RESOLUTION);
	static_assert(DDGI_PROBE_RAY_COUNT_NEW_BORN <= 0xff);
	static_assert(DDGI_VISIBILITY_RESOLUTION * DDGI_VISIBILITY_RESOLUTION >= DDGI_PROBE_RAY_COUNT_NEW_BORN);


	static_assert(DDGI_DEBUG_FLAGS_RENDER_PROBE_IN_HOLE == to_idx(age::graphics::e::ddgi_debug_flags::render_probe_in_hole));
	static_assert(DDGI_DEBUG_FLAGS_RENDER_IRRADIANCE == to_idx(age::graphics::e::ddgi_debug_flags::render_irradiance));
	static_assert(DDGI_DEBUG_FLAGS_RENDER_VISIBILITY == to_idx(age::graphics::e::ddgi_debug_flags::render_visibility));
	static_assert(DDGI_DEBUG_FLAGS_RENDER_FRONT_BACK == to_idx(age::graphics::e::ddgi_debug_flags::render_front_back));
	static_assert(DDGI_DEBUG_FLAGS_RENDER_LEVEL == to_idx(age::graphics::e::ddgi_debug_flags::render_level));
	static_assert(DDGI_DEBUG_FLAGS_RENDER_WEIGHT_SUM == to_idx(age::graphics::e::ddgi_debug_flags::render_weight_sum));
	static_assert(DDGI_DEBUG_FLAGS_RENDER_RAY_COUNT == to_idx(age::graphics::e::ddgi_debug_flags::render_ray_count));
	static_assert(DDGI_DEBUG_FLAGS_RENDER_STATE == to_idx(age::graphics::e::ddgi_debug_flags::render_state));
	static_assert(DDGI_DEBUG_FLAGS_RENDER_MSME == to_idx(age::graphics::e::ddgi_debug_flags::render_msme));
	static_assert(DDGI_DEBUG_FLAGS_RENDER_RAY_FACTOR == to_idx(age::graphics::e::ddgi_debug_flags::render_ray_factor));
	static_assert(DDGI_DEBUG_FLAGS_RENDER_PROBE == to_idx(age::graphics::e::ddgi_debug_flags::render_probe));
#endif

	//---[ gibs ]------------------------------------------------------------------------------------------------------
#define GIBS_ATLAS_WIDTH		3840u
#define GIBS_ATLAS_HEIGHT		2160u
#define GIBS_ATLAS_TILE_SIZE	6u
#define GIBS_ATLAS_TILE_COUNT_U (GIBS_ATLAS_WIDTH / GIBS_ATLAS_TILE_SIZE)
#define GIBS_ATLAS_TILE_COUNT_V (GIBS_ATLAS_HEIGHT / GIBS_ATLAS_TILE_SIZE)
#define GIBS_MAX_SURFEL_COUNT	(GIBS_ATLAS_TILE_COUNT_U * GIBS_ATLAS_TILE_COUNT_V - 1)
#define GIBS_RAY_BUDGET			(GIBS_MAX_SURFEL_COUNT * 8u)
#define GIBS_MIN_RAY_PER_SURFEL 4	  // ideal
#define GIBS_MAX_RAY_PER_SURFEL 64	  // ideal

#define GIBS_CELL_SURFEL_COUNT_PREFIX_TPG 32u
#define GIBS_CELL_SURFEL_COUNT_PREFIX_EPT 32u
#define GIBS_CELL_SURFEL_COUNT_PREFIX_EPG (GIBS_CELL_SURFEL_COUNT_PREFIX_TPG * GIBS_CELL_SURFEL_COUNT_PREFIX_EPT)

#define GIBS_RAY_REDUCE_TPG 32u
#define GIBS_RAY_REDUCE_EPT 32u
#define GIBS_RAY_REDUCE_EPG (GIBS_RAY_REDUCE_TPG * GIBS_RAY_REDUCE_EPT)

#define GIBS_RAY_COUNT_REDUCE_TPG 32u
#define GIBS_RAY_COUNT_REDUCE_EPT 32u
#define GIBS_RAY_COUNT_REDUCE_EPG (GIBS_RAY_COUNT_REDUCE_TPG * GIBS_RAY_COUNT_REDUCE_EPT)

#define GIBS_MAX_OUTER_LAYER_COUNT	  16u
#define GIBS_SCREEN_TILE_SIZE		  16u
#define GIBS_SCREEN_GROUP_SHARED_SIZE 8u	// (tile_size * tile_size / wave_size)

#define GIBS_SPAWN_COVERAGE	   2.f
#define GIBS_KILL_COVERAGE	   4.f
#define GIBS_SPAWN_PROB_FACTOR 0.3f
#define GIBS_KILL_PROB_FACTOR  0.2f

#define GIBS_RADIANCE_CACHE_DELAY 10

#define GIBS_MSME_SHORT_WINDOW_BLEND 0.08f

#define GIBS_MAX_LUMINANCE_FOR_FIREFLY 50.f

#define GIBS_MIN_LUMINANCE					0.01f
#define GIBS_MIN_LUMINANCE_FOR_RAY_GUIDANCE (GIBS_MIN_LUMINANCE * GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE)


#define GIBS_DEBUG_FLAGS_RENDER_RADIANCE			  (1u << 1u)
#define GIBS_DEBUG_FLAGS_RENDER_IRRADIANCE			  (1u << 2u)
#define GIBS_DEBUG_FLAGS_RENDER_VISIBILITY			  (1u << 3u)
#define GIBS_DEBUG_FLAGS_RENDER_INSTABILITY			  (1u << 4u)
#define GIBS_DEBUG_FLAGS_RENDER_RAY_COUNT			  (1u << 5u)
#define GIBS_DEBUG_FLAGS_RENDER_MSME				  (1u << 6u)
#define GIBS_DEBUG_FLAGS_RENDER_ID_HASH				  (1u << 7u)
#define GIBS_DEBUG_FLAGS_RENDER_NORMAL				  (1u << 8u)
#define GIBS_DEBUG_FLAGS_RENDER_COVERAGE			  (1u << 9u)
#define GIBS_DEBUG_FLAGS_RENDER_SHOW_IRRADIANCE_ATLAS (1u << 10u)
#define GIBS_DEBUG_FLAGS_RENDER_SHOW_VISIBILITY_ATLAS (1u << 11u)
#define GIBS_DEBUG_FLAGS_FREEZE_SPAWN				  (1u << 12u)
#define GIBS_DEBUG_FLAGS_RENDER_AGE					  (1u << 13u)
#define GIBS_DEBUG_FLAGS_RENDER_CELL				  (1u << 14u)


#if !defined(AGE_SHADER)

	inline constexpr auto gibs_atlas_extent		  = extent_2d<uint32>{ GIBS_ATLAS_WIDTH, GIBS_ATLAS_HEIGHT };
	inline constexpr auto gibs_atlas_tile_size	  = GIBS_ATLAS_TILE_SIZE;
	inline constexpr auto gibs_atlas_tile_count_u = GIBS_ATLAS_TILE_COUNT_U;
	inline constexpr auto gibs_atlas_tile_count_v = GIBS_ATLAS_TILE_COUNT_V;
	inline constexpr auto gibs_max_surfel_count	  = GIBS_MAX_SURFEL_COUNT;
	inline constexpr auto gibs_ray_budget		  = GIBS_RAY_BUDGET;

	inline constexpr auto gibs_cell_surfel_count_reduce_epg = GIBS_CELL_SURFEL_COUNT_PREFIX_EPG;
	inline constexpr auto gibs_ray_count_reduce_epg			= GIBS_RAY_COUNT_REDUCE_EPG;
	inline constexpr auto gibs_max_outer_layer_count		= GIBS_MAX_OUTER_LAYER_COUNT;

	inline constexpr auto gibs_ray_reduce_epg = GIBS_RAY_REDUCE_EPG;

	inline constexpr auto gibs_surfel_screen_ratio = 0.01f;

	inline constexpr auto gibs_screen_tile_size = GIBS_SCREEN_TILE_SIZE;

	static_assert(GIBS_ATLAS_WIDTH % gibs_atlas_tile_size == 0);
	static_assert(GIBS_ATLAS_HEIGHT % gibs_atlas_tile_size == 0);
	static_assert(gibs_max_surfel_count <= (1u << 24u));

	static_assert(sizeof(shared_type::gibs_lut_data::cell_size_arr) == sizeof(float) * (GIBS_MAX_OUTER_LAYER_COUNT + 1));
	static_assert(sizeof(shared_type::gibs_lut_data::layer_boundary_arr) == sizeof(float) * (GIBS_MAX_OUTER_LAYER_COUNT + 1));
	static_assert(GIBS_SCREEN_TILE_SIZE * GIBS_SCREEN_TILE_SIZE == 32 * GIBS_SCREEN_GROUP_SHARED_SIZE);

	static_assert(sizeof(shared_type::gibs_ray_entry) <= sizeof(shared_type::gibs_ray_result));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_RADIANCE == to_idx(graphics::e::gibs_debug_flags::render_radiance));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_IRRADIANCE == to_idx(graphics::e::gibs_debug_flags::render_irradiance));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_VISIBILITY == to_idx(graphics::e::gibs_debug_flags::render_visibility));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_INSTABILITY == to_idx(graphics::e::gibs_debug_flags::render_instability));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_RAY_COUNT == to_idx(graphics::e::gibs_debug_flags::render_ray_count));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_MSME == to_idx(graphics::e::gibs_debug_flags::render_msme));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_ID_HASH == to_idx(graphics::e::gibs_debug_flags::render_id_hash));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_NORMAL == to_idx(graphics::e::gibs_debug_flags::render_normal));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_COVERAGE == to_idx(graphics::e::gibs_debug_flags::render_coverage));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_SHOW_IRRADIANCE_ATLAS == to_idx(graphics::e::gibs_debug_flags::render_show_irradiance_atlas));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_SHOW_VISIBILITY_ATLAS == to_idx(graphics::e::gibs_debug_flags::render_show_visibility_atlas));
	static_assert(GIBS_DEBUG_FLAGS_FREEZE_SPAWN == to_idx(graphics::e::gibs_debug_flags::freeze_spawn));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_AGE == to_idx(graphics::e::gibs_debug_flags::render_age));
	static_assert(GIBS_DEBUG_FLAGS_RENDER_CELL == to_idx(graphics::e::gibs_debug_flags::render_cell));

#endif

	//---[ static buffer offset ]------------------------------------------------------------------------------------------------------
#define OPAQUE_MSHLT_OBJECT_DATA_OFFSET (0)
#define OBJECT_DATA_OFFSET				(OPAQUE_MSHLT_OBJECT_DATA_OFFSET + sizeof(SHARED_TYPE opaque_meshlet_render_data) * MAX_OPAQUE_MESHLET_RENDER_DATA_COUNT)
#define DIRECTIONAL_LIGHT_OFFSET		(OBJECT_DATA_OFFSET + sizeof(SHARED_TYPE object_data) * MAX_OBJECT_DATA_COUNT)
#define UNIFIED_LIGHT_OFFSET			(DIRECTIONAL_LIGHT_OFFSET + sizeof(SHARED_TYPE directional_light) * MAX_DIRECTIONAL_LIGHT_COUNT)
#define BLOOM_OFFSET					(UNIFIED_LIGHT_OFFSET + sizeof(SHARED_TYPE unified_light) * MAX_LIGHT_COUNT)
#define DDGI_DATA_OFFSET				(BLOOM_OFFSET + sizeof(SHARED_TYPE bloom) * 1)
#define GIBS_DATA_OFFSET				(DDGI_DATA_OFFSET + sizeof(SHARED_TYPE ddgi_data) * 1)
#define GIBS_LUT_DATA_OFFSET			(GIBS_DATA_OFFSET + sizeof(SHARED_TYPE gibs_data) * 1)
#define STATIC_BUFFER_SIZE				(GIBS_LUT_DATA_OFFSET + sizeof(SHARED_TYPE gibs_lut_data) * 1)
#if !defined(AGE_SHADER)
	inline constexpr auto opaque_mshlt_object_data_offset = OPAQUE_MSHLT_OBJECT_DATA_OFFSET;
	inline constexpr auto object_data_offset			  = OBJECT_DATA_OFFSET;
	inline constexpr auto directional_light_offset		  = DIRECTIONAL_LIGHT_OFFSET;
	inline constexpr auto unified_light_offset			  = UNIFIED_LIGHT_OFFSET;
	inline constexpr auto bloom_offset					  = BLOOM_OFFSET;
	inline constexpr auto ddgi_data_offset				  = DDGI_DATA_OFFSET;
	inline constexpr auto gibs_data_offset				  = GIBS_DATA_OFFSET;
	inline constexpr auto gibs_lut_data_offset			  = GIBS_LUT_DATA_OFFSET;
	inline constexpr auto static_buffer_size			  = STATIC_BUFFER_SIZE;
#endif

#if !defined(AGE_SHADER)
}
	#include "age_graphics_backend_dx12_render_pipeline_forward_plus_macro_guard.hpp"
#endif
