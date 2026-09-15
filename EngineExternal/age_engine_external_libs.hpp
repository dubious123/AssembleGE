#pragma once

namespace age::external::mikk::detail
{
	// corner_idx = 0, 1, 2, 3
	bool
	gen_tangent_space(
		int (* const fp_get_face_count)(void* p_data),

		int (* const fp_get_vertex_count_of_face)(void*		p_data,
												  const int face_idx),

		void (* const fp_get_position)(void*	 p_data,
									   float	 pos_out[],
									   const int face_idx,
									   const int corner_idx),

		void (* const fp_get_normal)(void*	   p_data,
									 float	   normal_out[],
									 const int face_idx,
									 const int corner_idx),

		void (* const fp_get_uv)(void*	   p_data,
								 float	   uv_out[],
								 const int face_idx,
								 const int corner_idx),

		void (* const fp_set_tangent)(void*		  p_data,
									  const float tangent_in[],
									  const float tangent_sign,
									  const int	  face_idx,
									  const int	  corner_idx),

		void* const p_data,
		const float angular_threshold = 180.0f) noexcept;
}	 // namespace age::external::mikk::detail

namespace age::external::earcut::detail
{
	unsigned int
	perform(
		void**				pp_boundary_arr,
		const unsigned int* p_vertex_count_arr,
		const unsigned int	boundary_count,
		unsigned int*&		pp_idx_out) noexcept;
}

namespace age::external::meshopt::detail
{
	// Vertex remap

	// out : p_remap (size == index_count)
	// ret : unique vertex count
	// note: 1. vertex_stride is in bytes
	//		 2. binary equivalence considers all vertex_size bytes,
	//		    including padding which should be zero-initialized.
	//       3. i | index_buffer | p_remap | new_vertex_vec == i | index_buffer | old_vertex_vec
	//       4. fp_cmp_vertex_attribute returns 1 if vertices are equivalent and 0 if they are not
	//	     5. remap_vertex_buffer needs to be called once for each vertex stream
	unsigned long long
	gen_vertex_remap(
		unsigned int*		p_remap_idx_arr_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const void*			p_vertex_buffer,
		unsigned long long	vertex_count,
		unsigned long long	vertex_size_and_stride) noexcept;

	unsigned long long
	gen_vertex_remap(
		unsigned int*		p_remap_idx_arr_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count,
		void**				pp_vertex_attribute_ptr_arr,
		unsigned long long* p_vertex_attribute_size_arr,
		unsigned long long* p_vertex_attribute_stride_arr,
		unsigned long long	stream_count) noexcept;

	// vertex_positions should have float3 position in the first 12 bytes of each vertex
	unsigned long long
	gen_vertex_remap(
		unsigned int*		p_remap_idx_arr_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const float*		p_vertex_position_arr,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride,
		int					(*fp_cmp_vertex_attribute)(
			void*		 p_vertex_attribute_cmp_ctx,
			unsigned int vertex_idx_l,
			unsigned int vertex_idx_r),
		void* p_vertex_attribute_cmp_ctx) noexcept;

	// vertex_positions should have float3 position in the first 12 bytes of each vertex
	// p_remap_idx_arr_out size <= vertex count
	// for v0, v1 : if remap[v0] == remap[v1] : position is same
	void
	gen_position_remap(
		unsigned int*	   p_remap_idx_arr_out,
		const float*	   p_vertex_position_arr,
		unsigned long long vertex_count,
		unsigned long long vertex_positions_stride) noexcept;

	// Reorder buffers
	void
	gen_remapped_vertex_buffer(
		void*				p_remapped_vertex_buffer_out,
		const void*			p_old_vertex_buffer,
		unsigned long long	vertex_count,
		unsigned long long	vertex_size_and_stride,
		const unsigned int* p_remap_idx_arr) noexcept;

	void
	gen_remapped_index_buffer(
		unsigned int*		p_remapped_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const unsigned int* p_remap_idx_arr) noexcept;

	// Shadow index buffers
	void
	gen_shadow_index_buffer(
		unsigned int*		p_shadow_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const void*			p_vertex_buffer,
		unsigned long long	vertex_count,
		unsigned long long	vertex_size,
		unsigned long long	vertex_stride) noexcept;

	void
	gen_shadow_index_buffer_multi(
		unsigned int*		p_shadow_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count,
		void**				pp_vertex_attribute_ptr_arr,
		unsigned long long* p_vertex_attribute_size_arr,
		unsigned long long* p_vertex_attribute_stride_arr,
		unsigned long long	stream_count) noexcept;

	void
	gen_adjacency_index_buffer(
		unsigned int*		p_adjacency_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const float*		vertex_positions,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride) noexcept;

	void
	gen_tessellation_index_buffer(
		unsigned int*		p_tessellation_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const float*		vertex_positions,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride) noexcept;

	// Provoking vertex
	unsigned long long
	gen_provoking_index_buffer(
		unsigned int*		p_provoke_index_buffer_out,
		unsigned int*		p_reorder_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count) noexcept;

	// Vertex cache optimization

	// supports inplace (p_index_buffer_out == p_index_buffer)
	// optimize for post-transform cache to avoid redundant shader invocations
	void
	opt_index_buffer_for_vertex_cache(
		unsigned int*		p_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count) noexcept;

	void
	opt_index_buffer_for_vertex_cache_strip(
		unsigned int*		p_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count) noexcept;

	// faster, but results on less performant results
	void
	opt_index_buffer_for_vertex_cache_fast(
		unsigned int*		p_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count,
		unsigned int		cache_size = 16) noexcept;

	// vertex_positions should have float3 position in the first 12 bytes of each vertex
	// threshold determines how much the algorithm can compromise the vertex cache hit ratio
	// with 1.05 meaning that resulting ratio should be at most 5% worse than before the optimization.
	// the optimization may or may not be beneficial
	void
	opt_index_buffer_for_less_overdraw(
		unsigned int*		p_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const float*		vertex_positions,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride,
		float				threshold = 1.05f) noexcept;

	// Vertex fetch optimization
	unsigned long long
	opt_vertex_buffer_for_vertex_fetch(
		void*			   p_vertex_buffer_out,
		unsigned int*	   p_index_buffer,
		unsigned long long index_count,
		const void*		   p_vertex_buffer,
		unsigned long long vertex_count,
		unsigned long long vertex_size) noexcept;

	// use this if vertex is stored using multiple streams
	unsigned long long
	gen_vertex_remap_for_vertex_fetch(
		unsigned int*		p_remap_idx_arr_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count) noexcept;

	// Meshlets
	unsigned long long
	calc_meshlet_max_count(
		unsigned long long index_count,
		unsigned long long vertex_count_per_meshlet,
		unsigned long long triangle_count_per_meshlet) noexcept;

	// balance topological efficiency and culling efficiency
	// returns meshlet_counts -> need to resize meshlet_buffer
	// p_meshlet_buffer_out size == max_meshlet_count
	// meshlet_vertices, meshlet_triangles size == index_count
	// should resize each buffer after calling this function
	// expects float3 position
	unsigned long long
	gen_meshlet_buffer_balanced(
		void*				p_meshlet_buffer_out,
		unsigned int*		p_meshlet_global_index_buffer_out,
		unsigned char*		p_meshlet_local_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const float*		p_vertex_position_arr,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride,
		unsigned long long	max_vertex_count_per_meshlet,
		unsigned long long	max_triangle_count_per_meshlet,
		float				cone_culling_weight = 0.25) noexcept;

	unsigned long long
	gen_meshlet_buffer_for_spacial_locality(
		void*				p_meshlet_buffer_out,
		unsigned int*		p_meshlet_global_index_buffer_out,
		unsigned char*		p_meshlet_local_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const float*		p_vertex_position_arr,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride,
		unsigned long long	max_vertex_count_per_meshlet,
		unsigned long long	min_triangle_count_per_meshlet,
		unsigned long long	max_triangle_count_per_meshlet,
		float				cone_culling_weight = 0.25,
		float				split_factor		= 2.0f) noexcept;

	// need opt_index_buffer_for_vertex_cache to be done
	unsigned long long
	gen_meshlet_buffer_fast(
		void*				p_meshlet_buffer_out,
		unsigned int*		p_meshlet_global_index_buffer_out,
		unsigned char*		p_meshlet_local_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count,
		unsigned long long	max_vertex_count_per_meshlet,
		unsigned long long	max_triangle_count_per_meshlet) noexcept;

	// use this for raytracing
	// min_triangle_count_per_meshlet should be about max_triangle_count_per_meshlet * 0.5 ~ 0.25(ideally)
	// fill rate 0 : purely for SAH, values between 0.25 and 0.75 typically provide a good balance of SAH quality vs triangle count.
	unsigned long long
	gen_meshlet_buffer_for_surface_area_heuristic(
		void*				p_meshlet_buffer_out,
		unsigned int*		p_meshlet_global_index_buffer_out,
		unsigned char*		p_meshlet_local_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const float*		p_vertex_position_arr,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride,
		unsigned long long	max_vertices,
		unsigned long long	min_triangle_count_per_meshlet,
		unsigned long long	max_triangle_count_per_meshlet,
		float				fill_weight = 0.25f) noexcept;

	// run for_each meshlet
	void
	opt_meshlet(
		unsigned int*	   p_meshlet_global_index_buffer,
		unsigned char*	   p_meshlet_local_index_buffer,
		unsigned long long meshlet_triangle_count,
		unsigned long long meshlet_vertex_count) noexcept;

	void
	calc_triangles_bounds(
		void*				p_bounds_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const float*		p_vertex_position_arr,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride) noexcept;

	void
	calc_meshlet_bounds(
		void*				 p_bounds_out,
		const unsigned int*	 p_meshlet_global_index_buffer,
		const unsigned char* p_meshlet_local_index_buffer,
		unsigned long long	 triangle_count,
		const float*		 p_vertex_position_arr,
		unsigned long long	 vertex_count,
		unsigned long long	 vertex_positions_stride) noexcept;

	void
	calc_spheres_bounds(
		void*			   p_bounds_out,
		const float*	   p_position_arr,
		unsigned long long count,
		unsigned long long positions_stride,
		const float*	   p_radius_arr,
		unsigned long long radius_stride) noexcept;

	// Spatial clustering/sorting

	// return : partition count
	unsigned long long
	partition_clusters(
		unsigned int*		p_partition_index_buffer_out,
		const unsigned int* p_cluster_global_index_buffer,
		unsigned long long	cluster_global_index_buffer_count,
		const unsigned int* p_cluster_index_count_arr,
		unsigned long long	cluster_count,
		const float*		p_vertex_positions,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride,
		unsigned long long	target_partition_size = 32) noexcept;

	void
	spatialSortRemap(
		unsigned int*	   destination,
		const float*	   vertex_positions,
		unsigned long long vertex_count,
		unsigned long long vertex_positions_stride) noexcept;

	void
	spatialSortTriangles(
		unsigned int* destination,
		const unsigned int* indices,
		unsigned long long index_count,
		const float*	   vertex_positions,
		unsigned long long vertex_count,
		unsigned long long vertex_positions_stride) noexcept;

	void
	spatialClusterPoints(
		unsigned int*	   destination,
		const float*	   vertex_positions,
		unsigned long long vertex_count,
		unsigned long long vertex_positions_stride,
		unsigned long long cluster_size) noexcept;

	// Quantization
	unsigned short
	quantizeHalf(float v) noexcept;
	float
	quantizeFloat(float v, int N) noexcept;
	float
	dequantizeHalf(unsigned short h) noexcept;

	// Index encoding/decoding
	unsigned long long
	encodeIndexBuffer(
		unsigned char*	   buffer,
		unsigned long long buffer_size,
		const unsigned int* indices,
		unsigned long long index_count) noexcept;

	unsigned long long
	encodeIndexBufferBound(
		unsigned long long index_count,
		unsigned long long vertex_count) noexcept;

	void
	encodeIndexVersion(int version) noexcept;

	int
	decodeIndexBuffer(
		void*				 destination,
		unsigned long long	 index_count,
		unsigned long long	 index_size,
		const unsigned char* buffer,
		unsigned long long	 buffer_size) noexcept;

	int
	decodeIndexVersion(
		const unsigned char* buffer,
		unsigned long long	 buffer_size) noexcept;

	// Index sequence encoding
	unsigned long long
	encodeIndexSequence(
		unsigned char*	   buffer,
		unsigned long long buffer_size,
		const unsigned int* indices,
		unsigned long long index_count) noexcept;

	unsigned long long
	encodeIndexSequenceBound(
		unsigned long long index_count,
		unsigned long long vertex_count) noexcept;

	int
	decodeIndexSequence(
		void*				 destination,
		unsigned long long	 index_count,
		unsigned long long	 index_size,
		const unsigned char* buffer,
		unsigned long long	 buffer_size) noexcept;

	// Vertex encoding/decoding
	unsigned long long
	encodeVertexBuffer(
		unsigned char*	   buffer,
		unsigned long long buffer_size,
		const void* vertices,
		unsigned long long vertex_count,
		unsigned long long vertex_size) noexcept;

	unsigned long long
	encodeVertexBufferBound(
		unsigned long long vertex_count,
		unsigned long long vertex_size) noexcept;

	unsigned long long
	encodeVertexBufferLevel(
		unsigned char*	   buffer,
		unsigned long long buffer_size,
		const void* vertices,
		unsigned long long vertex_count,
		unsigned long long vertex_size,
		int				   level,
		int				   version) noexcept;

	void
	encodeVertexVersion(int version) noexcept;

	int
	decodeVertexBuffer(
		void*				 destination,
		unsigned long long	 vertex_count,
		unsigned long long	 vertex_size,
		const unsigned char* buffer,
		unsigned long long	 buffer_size) noexcept;

	int
	decodeVertexVersion(
		const unsigned char* buffer,
		unsigned long long	 buffer_size) noexcept;

	unsigned long long
	encodeMeshlet(
		unsigned char*	   buffer,
		unsigned long long buffer_size,
		const unsigned int* vertices,
		unsigned long long	 vertex_count,
		const unsigned char* triangles,
		unsigned long long	 triangle_count);
	unsigned long long
	encodeMeshletBound(
		unsigned long long max_vertices,
		unsigned long long max_triangles);

	int
	decodeMeshlet(
		void* vertices,
		unsigned long long	 vertex_count,
		unsigned long long	 vertex_size,
		void*				 triangles,
		unsigned long long	 triangle_count,
		unsigned long long	 triangle_size,
		const unsigned char* buffer,
		unsigned long long	 buffer_size);
	int
	decodeMeshletRaw(
		unsigned int* vertices,
		unsigned long long	 vertex_count,
		unsigned int*		 triangles,
		unsigned long long	 triangle_count,
		const unsigned char* buffer,
		unsigned long long	 buffer_size);

	// Decode filters
	void
	decodeFilterOct(void* buffer, unsigned long long count, unsigned long long stride) noexcept;
	void
	decodeFilterQuat(void* buffer, unsigned long long count, unsigned long long stride) noexcept;
	void
	decodeFilterExp(void* buffer, unsigned long long count, unsigned long long stride) noexcept;
	void
	decodeFilterColor(void* buffer, unsigned long long count, unsigned long long stride) noexcept;

	// Encode filters
	void
	encodeFilterOct(
		void*			   destination,
		unsigned long long count,
		unsigned long long stride,
		int				   bits,
		const float*	   data) noexcept;

	void
	encodeFilterQuat(
		void*			   destination,
		unsigned long long count,
		unsigned long long stride,
		int				   bits,
		const float*	   data) noexcept;

	void
	encodeFilterExp(
		void*			   destination,
		unsigned long long count,
		unsigned long long stride,
		int				   bits,
		const float*	   data,
		enum EncodeExpMode mode) noexcept;

	void
	encodeFilterColor(
		void*			   destination,
		unsigned long long count,
		unsigned long long stride,
		int				   bits,
		const float*	   data) noexcept;

	// Simplification
	unsigned long long
	simplify(
		unsigned int* destination,
		const unsigned int* indices,
		unsigned long long index_count,
		const float*	   vertex_positions,
		unsigned long long vertex_count,
		unsigned long long vertex_positions_stride,
		unsigned long long target_index_count,
		float			   target_error,
		unsigned int	   options,
		float*			   result_error) noexcept;

	unsigned long long
	simplifyWithAttributes(
		unsigned int* destination,
		const unsigned int* indices,
		unsigned long long	 index_count,
		const float*		 vertex_positions,
		unsigned long long	 vertex_count,
		unsigned long long	 vertex_positions_stride,
		const float*		 vertex_attributes,
		unsigned long long	 vertex_attributes_stride,
		const float*		 attribute_weights,
		unsigned long long	 attribute_count,
		const unsigned char* vertex_lock,
		unsigned long long	 target_index_count,
		float				 target_error,
		unsigned int		 options,
		float*				 result_error) noexcept;

	unsigned long long
	simplifyWithUpdate(
		unsigned int* indices,
		unsigned long long	 index_count,
		float*				 vertex_positions,
		unsigned long long	 vertex_count,
		unsigned long long	 vertex_positions_stride,
		float*				 vertex_attributes,
		unsigned long long	 vertex_attributes_stride,
		const float*		 attribute_weights,
		unsigned long long	 attribute_count,
		const unsigned char* vertex_lock,
		unsigned long long	 target_index_count,
		float				 target_error,
		unsigned int		 options,
		float*				 result_error) noexcept;

	unsigned long long
	simplifySloppy(
		unsigned int* destination,
		const unsigned int* indices,
		unsigned long long	 index_count,
		const float*		 vertex_positions,
		unsigned long long	 vertex_count,
		unsigned long long	 vertex_positions_stride,
		const unsigned char* vertex_lock,
		unsigned long long	 target_index_count,
		float				 target_error,
		float*				 result_error) noexcept;

	unsigned long long
	simplifyPrune(
		unsigned int* destination,
		const unsigned int* indices,
		unsigned long long index_count,
		const float*	   vertex_positions,
		unsigned long long vertex_count,
		unsigned long long vertex_positions_stride,
		float			   target_error) noexcept;

	unsigned long long
	simplifyPoints(
		unsigned int*	   destination,
		const float*	   vertex_positions,
		unsigned long long vertex_count,
		unsigned long long vertex_positions_stride,
		const float*	   vertex_colors,
		unsigned long long vertex_colors_stride,
		float			   color_weight,
		unsigned long long target_vertex_count) noexcept;

	float
	simplifyScale(
		const float*	   vertex_positions,
		unsigned long long vertex_count,
		unsigned long long vertex_positions_stride) noexcept;

	// Stripify
	unsigned long long
	stripify(unsigned int* destination, const unsigned int* indices, unsigned long long index_count, unsigned long long vertex_count, unsigned int restart_index) noexcept;
	unsigned long long
	stripifyBound(unsigned long long index_count) noexcept;
	unsigned long long
	unstripify(unsigned int* destination, const unsigned int* indices, unsigned long long index_count, unsigned int restart_index) noexcept;
	unsigned long long
	unstripifyBound(unsigned long long index_count) noexcept;

	// Analysis
	// vertex_cache_staticstics
	// analyzeVertexCache(
	//	const unsigned int* indices,
	//	unsigned long long	index_count,
	//	unsigned long long	vertex_count,
	//	unsigned int		cache_size,
	//	unsigned int		warp_size,
	//	unsigned int		primgroup_size) noexcept;

	// vertex_fetch_statistics
	// analyzeVertexFetch(
	//	const unsigned int* indices,
	//	unsigned long long	index_count,
	//	unsigned long long	vertex_count,
	//	unsigned long long	vertex_size) noexcept;

	// overdraw_statistics
	// analyzeOverdraw(
	//	const unsigned int* indices,
	//	unsigned long long	index_count,
	//	const float*		vertex_positions,
	//	unsigned long long	vertex_count,
	//	unsigned long long	vertex_positions_stride) noexcept;

	// coverage_statistics
	// analyzeCoverage(
	//	const unsigned int* indices,
	//	unsigned long long	index_count,
	//	const float*		vertex_positions,
	//	unsigned long long	vertex_count,
	//	unsigned long long	vertex_positions_stride) noexcept;

	// Allocator
	void
	setAllocator(
		void* (*allocate)(unsigned long long),
		void  (*deallocate)(void*)) noexcept;

}	 // namespace age::external::meshopt::detail

namespace age::external::msdfgen
{
	bool
	bake_font(const char*		 font_path,
			  const char*		 output_image_path,
			  const char*		 output_csv_path,
			  const char*		 output_json_path,
			  unsigned long long font_charset_flag,
			  unsigned short*	 extra_unicode_arr,
			  unsigned short	 extra_unicode_arr_count,
			  unsigned int		 em_size  = 48,
			  unsigned int		 px_range = 2) noexcept;
}

namespace age::external::texconv
{
	namespace detail
	{
		struct bake_options
		{
			const char* dxgi_format_name = "BC7_UNORM_SRGB";	// final -f
			const char* output_dir		 = nullptr;

			const char* assemble_kind			 = "";
			const char* assemble_output_filename = nullptr;
			bool		assemble_gif_bg_color	 = false;
			const char* assemble_swizzle		 = nullptr;	   // merge mode

			const char* intermediate_format = nullptr;

			// resize, mip
			unsigned int width	   = 0;
			unsigned int height	   = 0;
			unsigned int mip_count = 0;	   // 0 = full, 1 = no mip
			bool		 fit_pow2  = false;

			// filtering
			const char* image_filter = nullptr;
			bool		wrap		 = false;
			bool		mirror		 = false;

			// srgb
			bool		srgb_both		 = false;
			bool		srgb_in			 = false;
			bool		srgb_out		 = false;
			bool		ignore_srgb		 = false;
			const char* rotate_color	 = nullptr;
			float		paper_white_nits = 0.0f;
			bool		tonemap			 = false;

			// alpha
			bool		premultiply_alpha = false;
			bool		straight_alpha	  = false;
			bool		separate_alpha	  = false;
			float		alpha_threshold	  = -1.0f;
			float		keep_coverage	  = -1.0f;
			const char* color_key_hex	  = nullptr;

			// bc
			int			gpu_index	 = -1;
			const char* bc_flags	 = nullptr;
			float		alpha_weight = -1.0f;

			// normal
			const char* nmap_flags	   = nullptr;
			float		nmap_amplitude = -1.0f;
			bool		invert_y	   = false;
			bool		reconstruct_z  = false;
			bool		x2_bias		   = false;

			// transform
			bool		hflip	= false;
			bool		vflip	= false;
			const char* swizzle = nullptr;	  // texconv
		};
	}	 // namespace detail

	bool
	bake_texture(const char* const*			 pp_src_arr,
				 unsigned int				 src_count,
				 const detail::bake_options& opt) noexcept;
}	 // namespace age::external::texconv

// age::external::cgltf boundary: C-only POD translation of a glTF 2.0 document.
//  - no age / cgltf types. every pointer is owned by gltf_data, freed by release().
//  - accessor data is unpacked dense (sparse resolved, normalized, unstrided).
//    exception: EXT_meshopt_compression -> attribute_view.meshopt (layout only).
//    precedence: view.p != nullptr -> view; else meshopt.block_idx >= 0; else absent.
//  - KHR_draco_mesh_compression is passed through undecoded, accessor views absent.
//  - matrices: row-major, column-vector (mul(mat, v)), translation at [3],[7],[11].
//    meshopt-backed matrices stay in glTF column-major.
//  - *_idx == -1 means none. this layer decides nothing; the importer judges.

namespace age::external::cgltf::detail
{
	// views

	struct float_view
	{
		const float* p;
		int			 count;
	};	  // count = floats

	struct index_view
	{
		const unsigned int* p;
		int					count;
	};	  // count = elements

	struct name_view
	{
		const char* p;
		int			count;
	};	  // null-terminated, count excludes it. also raw json

	struct extension_data
	{
		name_view name;
		name_view json;
	};	  // unparsed extension entry

	// enums

	enum class load_error
	{
		none,
		invalid_format,
		buffer_not_found,
		internal
	};

	enum class primitive_mode	 // glTF constants
	{
		points		   = 0,
		lines		   = 1,
		line_loop	   = 2,
		line_strip	   = 3,
		triangles	   = 4,
		triangle_strip = 5,
		triangle_fan   = 6,
	};

	enum class animation_path
	{
		translation,
		rotation,
		scale,
		weights
	};
	enum class interpolation_kind
	{
		linear,
		step,
		cubic_spline
	};
	enum class light_kind
	{
		directional,
		point,
		spot
	};
	enum class camera_kind
	{
		perspective,
		orthographic
	};
	enum class alpha_kind
	{
		opaque,
		mask,
		blend
	};
	enum class wrap_kind
	{
		clamp_to_edge,
		mirrored_repeat,
		repeat
	};
	enum class filter_kind
	{
		undefined,
		nearest,
		linear
	};	  // undefined = sampler omitted it
	enum class mipmap_kind
	{
		none,
		nearest,
		linear
	};
	enum class component_kind
	{
		i8,
		u8,
		i16,
		u16,
		u32,
		f32
	};	  // stride cannot tell sign / float
	enum class meshopt_mode
	{
		attributes,
		triangles,
		indices
	};
	enum class meshopt_filter
	{
		none,
		octahedral,
		quaternion,
		exponential
	};

	enum material_feature : unsigned int	// cgltf has_* + unlit
	{
		material_feature_unlit				  = 1u << 0,
		material_feature_specular_glossiness  = 1u << 1,
		material_feature_clearcoat			  = 1u << 2,
		material_feature_transmission		  = 1u << 3,
		material_feature_volume				  = 1u << 4,
		material_feature_ior				  = 1u << 5,
		material_feature_specular			  = 1u << 6,
		material_feature_sheen				  = 1u << 7,
		material_feature_emissive_strength	  = 1u << 8,
		material_feature_iridescence		  = 1u << 9,
		material_feature_diffuse_transmission = 1u << 10,
		material_feature_anisotropy			  = 1u << 11,
		material_feature_dispersion			  = 1u << 12,
	};

	// accessor views

	struct meshopt_block_data			// one compressed buffer view
	{
		const unsigned char* p;
		int					 size;
		int					 stride;	// decoded element stride
		int					 count;		// decoded element count
		meshopt_mode		 mode;
		meshopt_filter		 filter;
	};

	struct meshopt_ref					// one accessor inside a decoded block
	{
		int			   block_idx;		// -1 = none
		int			   byte_offset;
		int			   count;			// accessor elements, may be < block count
		component_kind component;
		int			   components;		// 1..16
		bool		   normalized;
	};

	struct attribute_view
	{
		float_view	view;
		meshopt_ref meshopt;
	};

	struct index_attribute_view
	{
		index_view	view;
		meshopt_ref meshopt;
	};

	// mesh

	struct trs_data
	{
		float translation[3];
		float rotation[4];			// xyzw
		float scale[3];
	};

	struct custom_attribute_data	// e.g. "_MY_ATTR"
	{
		name_view	   name;
		attribute_view data;
		int			   components;
	};

	struct morph_target_data
	{
		attribute_view		   position_delta;
		attribute_view		   normal_delta;
		attribute_view		   tangent_delta;				// float3
		attribute_view*		   p_uv_delta;
		int					   uv_delta_count;
		attribute_view*		   p_color_delta;
		int*				   p_color_delta_components;	// 3 or 4
		int					   color_delta_count;
		custom_attribute_data* p_custom;
		int					   custom_count;
	};

	struct submesh_data								  // glTF primitive
	{
		int vertex_count;

		attribute_view		 position;				  // float3
		attribute_view		 normal;				  // float3
		attribute_view		 tangent;				  // float4
		index_attribute_view index;					  // absent = non-indexed

		attribute_view*		   p_uv;				  // float2 per set
		int					   uv_count;
		attribute_view*		   p_color;
		int*				   p_color_components;	  // 3 or 4
		int					   color_count;
		index_attribute_view*  p_joints;			  // uint4 per set
		attribute_view*		   p_weights;			  // float4 per set
		int					   joints_count;		  // == weights count
		custom_attribute_data* p_custom;
		int					   custom_count;

		morph_target_data* p_morph_target;
		int				   morph_target_count;

		bool  has_position_bound;
		float position_min[3];
		float position_max[3];

		int			   material_idx;
		primitive_mode mode;

		// KHR_draco_mesh_compression, p_draco == nullptr when unused. ids: -1 = absent
		const unsigned char* p_draco;
		int					 draco_size;
		int					 draco_position_id;
		int					 draco_normal_id;
		int					 draco_tangent_id;
		int*				 p_draco_uv_id;			// [uv_count]
		int*				 p_draco_color_id;		// [color_count]
		int*				 p_draco_joints_id;		// [joints_count]
		int*				 p_draco_weights_id;	// [joints_count]
		int*				 p_draco_custom_id;		// [custom_count]

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	struct material_set_data		  // KHR_materials_variants
	{
		int		   name_idx;		  // -> gltf_data::p_material_set_name
		int*	   p_material;		  // material_idx per submesh
		name_view* p_extras;		  // mapping extras per submesh
		int		   material_count;	  // == submesh_count
	};

	struct mesh_data
	{
		name_view name;

		submesh_data* p_submesh;
		int			  submesh_count;

		float_view morph_weight;		   // one per morph target
		name_view* p_morph_target_name;	   // extras.targetNames
		int		   morph_target_name_count;

		material_set_data* p_material_set;
		int				   material_set_count;

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	// image / texture / material

	struct image_data
	{
		name_view name;
		name_view uri;							   // percent-decoded, empty when embedded
		name_view mime_type;

		const unsigned char* p_embedded;
		int					 embedded_size;
		bool				 is_embedded_owned;	   // decoded data uri, freed by release()

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	struct texture_data
	{
		name_view name;
		name_view sampler_name;

		int image_idx;			 // source
		int basisu_image_idx;	 // KHR_texture_basisu
		int webp_image_idx;		 // EXT_texture_webp

		wrap_kind	wrap_s;
		wrap_kind	wrap_t;
		filter_kind mag_filter;
		filter_kind min_filter;
		mipmap_kind mipmap;

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	struct texture_transform_data	 // KHR_texture_transform
	{
		bool  has;
		float offset[2];
		float rotation;
		float scale[2];
		int	  texcoord;	   // -1 = use texture_slot::texcoord
	};

	struct texture_slot
	{
		int					   texture_idx;
		int					   texcoord;
		float				   scale;	 // normal scale / occlusion strength, else 1
		texture_transform_data transform;
	};

	struct specular_glossiness_data
	{
		texture_slot diffuse_texture;
		texture_slot specular_glossiness_texture;
		float		 diffuse_factor[4];
		float		 specular_factor[3];
		float		 glossiness_factor;
	};

	struct clearcoat_data
	{
		texture_slot clearcoat_texture;
		texture_slot roughness_texture;
		texture_slot normal_texture;
		float		 factor;
		float		 roughness_factor;
	};

	struct transmission_data
	{
		texture_slot texture;
		float		 factor;
	};

	struct volume_data
	{
		texture_slot thickness_texture;
		float		 thickness_factor;
		float		 attenuation_color[3];
		float		 attenuation_distance;
	};

	struct sheen_data
	{
		texture_slot color_texture;
		float		 color_factor[3];
		texture_slot roughness_texture;
		float		 roughness_factor;
	};

	struct specular_data
	{
		texture_slot specular_texture;
		texture_slot color_texture;
		float		 color_factor[3];
		float		 factor;
	};

	struct iridescence_data
	{
		float		 factor;
		texture_slot texture;
		float		 ior;
		float		 thickness_min;
		float		 thickness_max;
		texture_slot thickness_texture;
	};

	struct diffuse_transmission_data
	{
		texture_slot texture;
		float		 factor;
		texture_slot color_texture;
		float		 color_factor[3];
	};

	struct anisotropy_data
	{
		float		 strength;
		float		 rotation;
		texture_slot texture;
	};

	struct material_data
	{
		name_view name;

		bool  has_pbr_metallic_roughness;
		float base_color_factor[4];
		float metallic_factor;
		float roughness_factor;
		float emissive_factor[3];
		float emissive_strength;	// not folded, 1 when absent

		alpha_kind alpha_mode;
		float	   alpha_cutoff;
		bool	   double_sided;

		texture_slot base_color_texture;
		texture_slot metallic_roughness_texture;
		texture_slot normal_texture;	   // scale = normal scale
		texture_slot occlusion_texture;	   // scale = strength
		texture_slot emissive_texture;

		unsigned int feature_mask;		   // material_feature bits, gate the fields below
		float		 ior;
		float		 dispersion;

		specular_glossiness_data  specular_glossiness;
		clearcoat_data			  clearcoat;
		transmission_data		  transmission;
		volume_data				  volume;
		sheen_data				  sheen;
		specular_data			  specular;
		iridescence_data		  iridescence;
		diffuse_transmission_data diffuse_transmission;
		anisotropy_data			  anisotropy;

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	// skin / animation

	struct skin_data
	{
		name_view name;

		int* p_joint_node_idx;
		int	 joint_count;

		attribute_view inverse_bind_matrix;		  // 16 per joint
		int			   skeleton_root_node_idx;	  // optional

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	struct animation_sampler_data
	{
		attribute_view	   input;	  // key times
		attribute_view	   output;	  // components by path
		interpolation_kind interpolation;

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	struct animation_channel_data
	{
		int			   sampler_idx;
		int			   target_node_idx;
		animation_path path;

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	struct animation_data
	{
		name_view name;

		animation_sampler_data* p_sampler;
		int						sampler_count;
		animation_channel_data* p_channel;
		int						channel_count;

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	// node / scene / light / camera

	struct node_data
	{
		name_view name;

		trs_data local;
		bool	 local_decompose_trs_failed;

		float	 world_matrix[16];
		trs_data world;
		bool	 world_decompose_trs_failed;

		int	 parent_idx;
		int* p_child_node_idx;
		int	 child_count;
		int	 mesh_idx;
		int	 skin_idx;
		int	 camera_idx;
		int	 light_idx;

		float_view morph_weight;	// overrides mesh weights

		// EXT_mesh_gpu_instancing, instance_count == 0 when unused
		int					   instance_count;
		attribute_view		   instance_translation;	// float3
		attribute_view		   instance_rotation;		// float4
		attribute_view		   instance_scale;			// float3
		custom_attribute_data* p_instance_custom;
		int					   instance_custom_count;

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	struct scene_data
	{
		name_view name;

		int* p_root_node_idx;
		int	 root_node_count;

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	struct light_data	 // KHR_lights_punctual, cgltf exposes no extensions here
	{
		name_view  name;
		light_kind kind;
		float	   color[3];
		float	   intensity;
		float	   range;	 // 0 = infinite
		float	   spot_inner_cone_angle;
		float	   spot_outer_cone_angle;
		name_view  extras;
	};

	struct camera_data
	{
		name_view	name;
		camera_kind kind;

		// perspective
		bool  has_aspect_ratio;
		float aspect_ratio;
		float yfov;
		bool  has_zfar;	   // false = infinite
		float zfar;
		float znear;

		// orthographic, zfar / znear shared
		float xmag;
		float ymag;

		name_view		projection_extras;	  // perspective / orthographic object extras
		name_view		extras;
		extension_data* p_extension;
		int				extension_count;
	};

	// document

	struct gltf_data
	{
		void* p_internal;	 // opaque cgltf source

		name_view		generator;
		name_view		copyright;
		name_view		version;
		name_view		min_version;
		name_view		asset_extras;
		extension_data* p_asset_extension;
		int				asset_extension_count;

		mesh_data*		p_mesh;
		int				mesh_count;
		image_data*		p_image;
		int				image_count;
		texture_data*	p_texture;
		int				texture_count;
		material_data*	p_material;
		int				material_count;
		skin_data*		p_skin;
		int				skin_count;
		animation_data* p_animation;
		int				animation_count;
		node_data*		p_node;
		int				node_count;
		scene_data*		p_scene;
		int				scene_count;
		light_data*		p_light;
		int				light_count;
		camera_data*	p_camera;
		int				camera_count;

		name_view*			p_material_set_name;
		int					material_set_name_count;	// variant names
		name_view*			p_material_set_extras;		// variant extras, [material_set_name_count]
		meshopt_block_data* p_meshopt_block;
		int					meshopt_block_count;
		name_view*			p_required_extension;
		int					required_extension_count;
		name_view*			p_used_extension;
		int					used_extension_count;

		name_view		extras;
		extension_data* p_extension;
		int				extension_count;

		int		   default_scene_idx;
		load_error error;	 // != none -> everything else empty
	};
}	 // namespace age::external::cgltf::detail

namespace age::external::cgltf
{
	// legend
	//   +-- x[]     owned array, freed by release()
	//   ---> y      index into another array, -1 when absent
	//   x[]         view, count per the header contract
	//   x{}         attribute_view - a view and/or a meshopt_ref, view wins when both
	//   p[size]     borrowed bytes inside the parsed source
	//
	// gltf_data
	// |
	// +-- p_internal ------------------------------ cgltf source (backs every borrowed pointer)
	// +-- generator, copyright, version, min_version, asset_extras, p_asset_extension[]
	// +-- extras, p_extension[extension_count]
	// |
	// +-- p_scene[scene_count]
	// |     +-- scene_data { name, p_root_node_idx[root_node_count] ------> node, extras, p_extension[] }
	// |
	// +-- p_node[node_count]
	// |     +-- node_data
	// |           +-- name
	// |           +-- local (trs), has_matrix
	// |           +-- world_matrix[16], world (trs), world_has_shear
	// |           +-- parent_idx ---------------------> node
	// |           +-- p_child_node_idx[child_count] --> node
	// |           +-- mesh_idx -----------------------> mesh
	// |           +-- light_idx ----------------------> light
	// |           +-- camera_idx ---------------------> camera
	// |           +-- skin_idx -----------------------> skin
	// |           +-- morph_weight[]                      (overrides mesh.morph_weight)
	// |           +-- instance_translation{} instance_rotation{} instance_scale{}
	// |           +-- p_instance_custom[instance_custom_count] { name, data{}, components }
	// |           +-- extras, p_extension[]
	// |
	// +-- p_mesh[mesh_count]
	// |     +-- mesh_data
	// |           +-- name
	// |           +-- morph_weight[]                      (one per morph target)
	// |           +-- p_morph_target_name[morph_target_name_count]
	// |           +-- p_submesh[submesh_count]
	// |           |     +-- submesh_data
	// |           |           +-- position{} normal{} tangent{} index{}
	// |           |           +-- p_uv[uv_count]{}  p_color[color_count]{}  p_color_components[]
	// |           |           +-- p_joints[joints_count]{}  p_weights[weights_count]{}
	// |           |           |                                   (joint slot --> skin.p_joint_node_idx)
	// |           |           +-- p_custom[custom_count] { name, data{}, components }
	// |           |           +-- p_morph_target[morph_target_count]
	// |           |           |     +-- morph_target_data
	// |           |           |           +-- position_delta{} normal_delta{} tangent_delta{}
	// |           |           |           +-- p_uv_delta[uv_delta_count]{}  p_color_delta[color_delta_count]{}
	// |           |           |           +-- p_custom[custom_count] { name, data{}, components }
	// |           |           +-- has_position_bound, position_min[3], position_max[3]
	// |           |           +-- material_idx ---------> material
	// |           |           +-- mode
	// |           |           +-- p_draco[draco_size], draco_*_id, p_draco_*_id[]
	// |           |           +-- extras, p_extension[]
	// |           +-- p_material_set[material_set_count]
	// |           |     +-- material_set_data
	// |           |           +-- name_idx -------------> gltf_data.p_material_set_name
	// |           |           +-- p_material[submesh_count] --> material   (-1 = submesh default)
	// |           |           +-- p_extras[submesh_count]
	// |           +-- extras, p_extension[]
	// |
	// +-- p_material[material_count]
	// |     +-- material_data
	// |           +-- name, has_pbr_metallic_roughness
	// |           +-- factors, emissive_strength, alpha_mode, double_sided, feature_mask
	// |           +-- base_color / metallic_roughness / normal / occlusion / emissive
	// |           |     +-- texture_slot { texture_idx ---> texture, texcoord, scale, transform }
	// |           +-- ior, dispersion
	// |           +-- specular_glossiness, clearcoat, transmission, volume, sheen,
	// |           |   specular, iridescence, diffuse_transmission, anisotropy
	// |           |     +-- factors + texture_slot(s) ---> texture
	// |           +-- extras, p_extension[]
	// |
	// +-- p_texture[texture_count]
	// |     +-- texture_data { name, sampler_name, image_idx / basisu_image_idx / webp_image_idx ---> image,
	// |                        wrap / filter / mipmap, extras, p_extension[] }
	// |
	// +-- p_image[image_count]
	// |     +-- image_data { name, uri, mime_type, p_embedded[embedded_size], extras, p_extension[] }
	// |
	// +-- p_skin[skin_count]
	// |     +-- skin_data
	// |           +-- name
	// |           +-- p_joint_node_idx[joint_count] --> node        (slot k = k-th bone)
	// |           +-- inverse_bind_matrix{}               (joint_count x 16)
	// |           +-- skeleton_root_idx --------------> node
	// |           +-- extras, p_extension[]
	// |
	// +-- p_animation[animation_count]
	// |     +-- animation_data
	// |           +-- name
	// |           +-- p_sampler[sampler_count]
	// |           |     +-- animation_sampler_data { input{}, output{}, interpolation, extras, p_extension[] }
	// |           +-- p_channel[channel_count]
	// |           |     +-- animation_channel_data
	// |           |           +-- sampler_idx ---------> this animation's p_sampler
	// |           |           +-- target_node_idx -----> node
	// |           |           +-- path                    (translation / rotation / scale / weights)
	// |           |           +-- extras, p_extension[]
	// |           +-- extras, p_extension[]
	// |
	// +-- p_light[light_count]     +-- light_data  { name, kind, color, intensity, range, cone, extras }
	// +-- p_camera[camera_count]   +-- camera_data { name, kind, has_aspect_ratio, aspect_ratio, yfov,
	// |                                              has_zfar, zfar, znear, xmag, ymag,
	// |                                              projection_extras, extras, p_extension[] }
	// |
	// +-- p_material_set_name[material_set_name_count]     (variant names, referenced by name_idx)
	// +-- p_material_set_extras[material_set_name_count]
	// +-- p_meshopt_block[meshopt_block_count]             (compressed blocks, referenced by every {})
	// |     +-- meshopt_block_data { p[size], stride, count, mode, filter }
	// +-- p_required_extension[required_extension_count]
	// +-- p_used_extension[used_extension_count]
	// |
	// +-- default_scene_idx --------------------------> scene
	// +-- error                                            (load_error)
	//
	// reference flow
	//
	//   scene --> node --> mesh --> material --> texture --> image
	//                  +-> skin --> node (bones)
	//                  +-> light
	//                  +-> camera
	//
	//   animation --> node
	//   any {}    --> meshopt block                        (compressed accessors only)

	detail::gltf_data
	load_gltf(const char* full_path);

	void
	release(detail::gltf_data& data);

	void
	print_gltf(const detail::gltf_data& data);
}	 // namespace age::external::cgltf