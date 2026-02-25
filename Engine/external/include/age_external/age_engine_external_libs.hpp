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
		unsigned int*		destination,
		const unsigned int* indices,
		unsigned long long	index_count,
		const float*		vertex_positions,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride) noexcept;

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
		unsigned char*		buffer,
		unsigned long long	buffer_size,
		const unsigned int* indices,
		unsigned long long	index_count) noexcept;

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
		unsigned char*		buffer,
		unsigned long long	buffer_size,
		const unsigned int* indices,
		unsigned long long	index_count) noexcept;

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
		const void*		   vertices,
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
		const void*		   vertices,
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
		unsigned char*		 buffer,
		unsigned long long	 buffer_size,
		const unsigned int*	 vertices,
		unsigned long long	 vertex_count,
		const unsigned char* triangles,
		unsigned long long	 triangle_count);
	unsigned long long
	encodeMeshletBound(
		unsigned long long max_vertices,
		unsigned long long max_triangles);

	int
	decodeMeshlet(
		void*				 vertices,
		unsigned long long	 vertex_count,
		unsigned long long	 vertex_size,
		void*				 triangles,
		unsigned long long	 triangle_count,
		unsigned long long	 triangle_size,
		const unsigned char* buffer,
		unsigned long long	 buffer_size);
	int
	decodeMeshletRaw(
		unsigned int*		 vertices,
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
		unsigned int*		destination,
		const unsigned int* indices,
		unsigned long long	index_count,
		const float*		vertex_positions,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride,
		unsigned long long	target_index_count,
		float				target_error,
		unsigned int		options,
		float*				result_error) noexcept;

	unsigned long long
	simplifyWithAttributes(
		unsigned int*		 destination,
		const unsigned int*	 indices,
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
		unsigned int*		 indices,
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
		unsigned int*		 destination,
		const unsigned int*	 indices,
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
		unsigned int*		destination,
		const unsigned int* indices,
		unsigned long long	index_count,
		const float*		vertex_positions,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride,
		float				target_error) noexcept;

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