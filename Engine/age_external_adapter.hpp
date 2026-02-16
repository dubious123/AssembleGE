#pragma once
#include "age.hpp"

namespace age::external::mikk
{
	FORCE_INLINE bool
	gen_tangent(asset::mesh_editable&, const float angular_threshold = 180.0f) noexcept;
}	 // namespace age::external::mikk

namespace age::external::earcut
{
	age::vector<uint32>
	perform(const asset::mesh_editable&) noexcept;
}

namespace age::external::meshopt
{
	struct data_stream
	{
		const void* data;
		size_t		size;
		size_t		stride;
	};

	struct vertex_cache_staticstics
	{
		uint32 vertices_transformed;
		uint32 warps_executed;
		float  acmr; /* transformed vertices / triangle count; best case 0.5, worst case 3.0, optimum depends on topology */
		float  atvr; /* transformed vertices / vertex count; best case 1.0, worst case 6.0, optimum is 1.0 (each vertex is transformed once) */
	};

	struct vertex_fetch_statistics
	{
		uint32 bytes_fetched;
		float  overfetch; /* fetched bytes / vertex buffer size; best case 1.0 (each byte is fetched once) */
	};

	struct overdraw_statistics
	{
		uint32 pixels_covered;
		uint32 pixels_shaded;
		float  overdraw; /* shaded pixels / covered pixels; best case 1.0 */
	};

	struct coverage_statistics
	{
		float coverage[3];
		float extent; /* viewport size in mesh coordinates */
	};

	struct bounds
	{
		/* bounding sphere, useful for frustum and occlusion culling */
		float center[3];
		float radius;

		/* normal cone, useful for backface culling */
		float cone_apex[3];
		float cone_axis[3];
		float cone_cutoff; /* = cos(angle/2) */

		/* normal cone axis and cutoff, stored in 8-bit SNORM format; decode using x/127.0 */
		signed char cone_axis_s8[3];
		signed char cone_cutoff_s8;
	};
}	 // namespace age::external::meshopt

// namespace age::external::meshopt
//{
//
//
//	// Vertex remap
//	std::size_t
//	generateVertexRemap(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		const void*	  vertices,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_size) noexcept;
//
//	template <typename t_vertex>
//	age::vector<uint32>
//	gen_vertex_remap(
//		const age::vector<uint32>&	 index_buffer,
//		const age::vector<t_vertex>& vertex_buffer,
//		std::size_t					 vertex_size_and_stride = sizeof(t_vertex)) noexcept
//	{
//		auto remap_index_buffer = age::vector<uint32>::gen_sized(index_buffer.size());
//
//		// age::vector<uint32>::gen_reserved(10);
//		auto new_vertex_buffer =
//			age::vector<t_vertex>::reserved(
//				detail::gen_vertex_remap(
//					remap_index_buffer.data(),
//					index_buffer.data(),
//					index_buffer.size(),
//					vertex_buffer.data(),
//					vertex_buffer.size(),
//					sizeof(t_vertex)));
//		/*auto new_index_buffer = age::vector<uint32>*/
//
//		// detail::gen_remapped_index_buffer(
//
//		//);
//		// detail::gen_remapped_vertex_buffer();
//	}
//	std::size_t
//	generateVertexRemapMulti(
//		uint32*			   destination,
//		const uint32*	   indices,
//		std::size_t		   index_count,
//		std::size_t		   vertex_count,
//		const data_stream* streams,
//		std::size_t		   stream_count) noexcept;
//
//	std::size_t
//	generateVertexRemapCustom(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		const float*  vertex_positions,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_positions_stride,
//		int			  (*callback)(void*, uint32, uint32),
//		void*		  context) noexcept;
//
//	// Remap buffers
//	void
//	remapVertexBuffer(
//		void*		  destination,
//		const void*	  vertices,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_size,
//		const uint32* remap) noexcept;
//
//	void
//	remapIndexBuffer(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		const uint32* remap) noexcept;
//
//	// Shadow index buffers
//	void
//	generateShadowIndexBuffer(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		const void*	  vertices,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_size,
//		std::size_t	  vertex_stride) noexcept;
//
//	void
//	generateShadowIndexBufferMulti(
//		uint32*			   destination,
//		const uint32*	   indices,
//		std::size_t		   index_count,
//		std::size_t		   vertex_count,
//		const data_stream* streams,
//		std::size_t		   stream_count) noexcept;
//
//	// Position remap & adjacency
//	void
//	generatePositionRemap(
//		uint32*		 destination,
//		const float* vertex_positions,
//		std::size_t	 vertex_count,
//		std::size_t	 vertex_positions_stride) noexcept;
//
//	void
//	generateAdjacencyIndexBuffer(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		const float*  vertex_positions,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_positions_stride) noexcept;
//
//	void
//	generateTessellationIndexBuffer(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		const float*  vertex_positions,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_positions_stride) noexcept;
//
//	// Provoking vertex
//	std::size_t
//	generateProvokingIndexBuffer(
//		uint32*		  destination,
//		uint32*		  reorder,
//		const uint32* indices,
//		std::size_t	  index_count,
//		std::size_t	  vertex_count) noexcept;
//
//	// Vertex cache optimization
//	void
//	optimizeVertexCache(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		std::size_t	  vertex_count) noexcept;
//
//	void
//	optimizeVertexCacheStrip(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		std::size_t	  vertex_count) noexcept;
//
//	void
//	optimizeVertexCacheFifo(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		std::size_t	  vertex_count,
//		uint32		  cache_size) noexcept;
//
//	// Overdraw
//	void
//	optimizeOverdraw(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		const float*  vertex_positions,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_positions_stride,
//		float		  threshold) noexcept;
//
//	// Vertex fetch optimization
//	std::size_t
//	optimizeVertexFetch(
//		void*		destination,
//		uint32*		indices,
//		std::size_t index_count,
//		const void* vertices,
//		std::size_t vertex_count,
//		std::size_t vertex_size) noexcept;
//
//	std::size_t
//	optimizeVertexFetchRemap(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		std::size_t	  vertex_count) noexcept;
//
//	// Index encoding/decoding
//	std::size_t
//	encodeIndexBuffer(
//		unsigned char* buffer,
//		std::size_t	   buffer_size,
//		const uint32*  indices,
//		std::size_t	   index_count) noexcept;
//
//	std::size_t
//	encodeIndexBufferBound(
//		std::size_t index_count,
//		std::size_t vertex_count) noexcept;
//
//	void
//	encodeIndexVersion(int version) noexcept;
//
//	int
//	decodeIndexBuffer(
//		void*				 destination,
//		std::size_t			 index_count,
//		std::size_t			 index_size,
//		const unsigned char* buffer,
//		std::size_t			 buffer_size) noexcept;
//
//	int
//	decodeIndexVersion(
//		const unsigned char* buffer,
//		std::size_t			 buffer_size) noexcept;
//
//	// Index sequence encoding
//	std::size_t
//	encodeIndexSequence(
//		unsigned char* buffer,
//		std::size_t	   buffer_size,
//		const uint32*  indices,
//		std::size_t	   index_count) noexcept;
//
//	std::size_t
//	encodeIndexSequenceBound(
//		std::size_t index_count,
//		std::size_t vertex_count) noexcept;
//
//	int
//	decodeIndexSequence(
//		void*				 destination,
//		std::size_t			 index_count,
//		std::size_t			 index_size,
//		const unsigned char* buffer,
//		std::size_t			 buffer_size) noexcept;
//
//	// Vertex encoding/decoding
//	std::size_t
//	encodeVertexBuffer(
//		unsigned char* buffer,
//		std::size_t	   buffer_size,
//		const void*	   vertices,
//		std::size_t	   vertex_count,
//		std::size_t	   vertex_size) noexcept;
//
//	std::size_t
//	encodeVertexBufferBound(
//		std::size_t vertex_count,
//		std::size_t vertex_size) noexcept;
//
//	std::size_t
//	encodeVertexBufferLevel(
//		unsigned char* buffer,
//		std::size_t	   buffer_size,
//		const void*	   vertices,
//		std::size_t	   vertex_count,
//		std::size_t	   vertex_size,
//		int			   level,
//		int			   version) noexcept;
//
//	void
//	encodeVertexVersion(int version) noexcept;
//
//	int
//	decodeVertexBuffer(
//		void*				 destination,
//		std::size_t			 vertex_count,
//		std::size_t			 vertex_size,
//		const unsigned char* buffer,
//		std::size_t			 buffer_size) noexcept;
//
//	int
//	decodeVertexVersion(
//		const unsigned char* buffer,
//		std::size_t			 buffer_size) noexcept;
//
//	std::size_t
//	encodeMeshlet(
//		unsigned char*		 buffer,
//		std::size_t			 buffer_size,
//		const unsigned int*	 vertices,
//		std::size_t			 vertex_count,
//		const unsigned char* triangles,
//		std::size_t			 triangle_count);
//	std::size_t
//	encodeMeshletBound(
//		std::size_t max_vertices,
//		std::size_t max_triangles);
//
//	int
//	decodeMeshlet(
//		void*				 vertices,
//		std::size_t			 vertex_count,
//		std::size_t			 vertex_size,
//		void*				 triangles,
//		std::size_t			 triangle_count,
//		std::size_t			 triangle_size,
//		const unsigned char* buffer,
//		std::size_t			 buffer_size);
//	int
//	decodeMeshletRaw(
//		unsigned int*		 vertices,
//		std::size_t			 vertex_count,
//		unsigned int*		 triangles,
//		std::size_t			 triangle_count,
//		const unsigned char* buffer,
//		std::size_t			 buffer_size);
//
//
//	// Decode filters
//	void
//	decodeFilterOct(void* buffer, std::size_t count, std::size_t stride) noexcept;
//	void
//	decodeFilterQuat(void* buffer, std::size_t count, std::size_t stride) noexcept;
//	void
//	decodeFilterExp(void* buffer, std::size_t count, std::size_t stride) noexcept;
//	void
//	decodeFilterColor(void* buffer, std::size_t count, std::size_t stride) noexcept;
//
//	// Encode filters
//	void
//	encodeFilterOct(
//		void*		 destination,
//		std::size_t	 count,
//		std::size_t	 stride,
//		int			 bits,
//		const float* data) noexcept;
//
//	void
//	encodeFilterQuat(
//		void*		 destination,
//		std::size_t	 count,
//		std::size_t	 stride,
//		int			 bits,
//		const float* data) noexcept;
//
//	void
//	encodeFilterExp(
//		void*			   destination,
//		std::size_t		   count,
//		std::size_t		   stride,
//		int				   bits,
//		const float*	   data,
//		enum EncodeExpMode mode) noexcept;
//
//	void
//	encodeFilterColor(
//		void*		 destination,
//		std::size_t	 count,
//		std::size_t	 stride,
//		int			 bits,
//		const float* data) noexcept;
//
//	// Simplification
//	std::size_t
//	simplify(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		const float*  vertex_positions,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_positions_stride,
//		std::size_t	  target_index_count,
//		float		  target_error,
//		uint32		  options,
//		float*		  result_error) noexcept;
//
//	std::size_t
//	simplifyWithAttributes(
//		uint32*				 destination,
//		const uint32*		 indices,
//		std::size_t			 index_count,
//		const float*		 vertex_positions,
//		std::size_t			 vertex_count,
//		std::size_t			 vertex_positions_stride,
//		const float*		 vertex_attributes,
//		std::size_t			 vertex_attributes_stride,
//		const float*		 attribute_weights,
//		std::size_t			 attribute_count,
//		const unsigned char* vertex_lock,
//		std::size_t			 target_index_count,
//		float				 target_error,
//		uint32				 options,
//		float*				 result_error) noexcept;
//
//	std::size_t
//	simplifyWithUpdate(
//		uint32*				 indices,
//		std::size_t			 index_count,
//		float*				 vertex_positions,
//		std::size_t			 vertex_count,
//		std::size_t			 vertex_positions_stride,
//		float*				 vertex_attributes,
//		std::size_t			 vertex_attributes_stride,
//		const float*		 attribute_weights,
//		std::size_t			 attribute_count,
//		const unsigned char* vertex_lock,
//		std::size_t			 target_index_count,
//		float				 target_error,
//		uint32				 options,
//		float*				 result_error) noexcept;
//
//	std::size_t
//	simplifySloppy(
//		uint32*				 destination,
//		const uint32*		 indices,
//		std::size_t			 index_count,
//		const float*		 vertex_positions,
//		std::size_t			 vertex_count,
//		std::size_t			 vertex_positions_stride,
//		const unsigned char* vertex_lock,
//		std::size_t			 target_index_count,
//		float				 target_error,
//		float*				 result_error) noexcept;
//
//	std::size_t
//	simplifyPrune(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		const float*  vertex_positions,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_positions_stride,
//		float		  target_error) noexcept;
//
//	std::size_t
//	simplifyPoints(
//		uint32*		 destination,
//		const float* vertex_positions,
//		std::size_t	 vertex_count,
//		std::size_t	 vertex_positions_stride,
//		const float* vertex_colors,
//		std::size_t	 vertex_colors_stride,
//		float		 color_weight,
//		std::size_t	 target_vertex_count) noexcept;
//
//	float
//	simplifyScale(
//		const float* vertex_positions,
//		std::size_t	 vertex_count,
//		std::size_t	 vertex_positions_stride) noexcept;
//
//	// Stripify
//	std::size_t
//	stripify(uint32* destination, const uint32* indices, std::size_t index_count, std::size_t vertex_count, uint32 restart_index) noexcept;
//	std::size_t
//	stripifyBound(std::size_t index_count) noexcept;
//	std::size_t
//	unstripify(uint32* destination, const uint32* indices, std::size_t index_count, uint32 restart_index) noexcept;
//	std::size_t
//	unstripifyBound(std::size_t index_count) noexcept;
//
//	// Analysis
//	vertex_cache_staticstics
//	analyzeVertexCache(
//		const uint32* indices,
//		std::size_t	  index_count,
//		std::size_t	  vertex_count,
//		uint32		  cache_size,
//		uint32		  warp_size,
//		uint32		  primgroup_size) noexcept;
//
//	vertex_fetch_statistics
//	analyzeVertexFetch(
//		const uint32* indices,
//		std::size_t	  index_count,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_size) noexcept;
//
//	overdraw_statistics
//	analyzeOverdraw(
//		const uint32* indices,
//		std::size_t	  index_count,
//		const float*  vertex_positions,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_positions_stride) noexcept;
//
//	coverage_statistics
//	analyzeCoverage(
//		const uint32* indices,
//		std::size_t	  index_count,
//		const float*  vertex_positions,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_positions_stride) noexcept;
//
//	// Meshlets
//	std::size_t
//	buildMeshlets(
//		meshlet*	   meshlets,
//		uint32*		   meshlet_vertices,
//		unsigned char* meshlet_triangles,
//		const uint32*  indices,
//		std::size_t	   index_count,
//		const float*   vertex_positions,
//		std::size_t	   vertex_count,
//		std::size_t	   vertex_positions_stride,
//		std::size_t	   max_vertices,
//		std::size_t	   max_triangles,
//		float		   cone_weight) noexcept;
//
//	std::size_t
//	buildMeshletsScan(
//		meshlet*	   meshlets,
//		uint32*		   meshlet_vertices,
//		unsigned char* meshlet_triangles,
//		const uint32*  indices,
//		std::size_t	   index_count,
//		std::size_t	   vertex_count,
//		std::size_t	   max_vertices,
//		std::size_t	   max_triangles) noexcept;
//
//	std::size_t
//	buildMeshletsBound(
//		std::size_t index_count,
//		std::size_t max_vertices,
//		std::size_t max_triangles) noexcept;
//
//	std::size_t
//	buildMeshletsFlex(
//		meshlet*	   meshlets,
//		uint32*		   meshlet_vertices,
//		unsigned char* meshlet_triangles,
//		const uint32*  indices,
//		std::size_t	   index_count,
//		const float*   vertex_positions,
//		std::size_t	   vertex_count,
//		std::size_t	   vertex_positions_stride,
//		std::size_t	   max_vertices,
//		std::size_t	   min_triangles,
//		std::size_t	   max_triangles,
//		float		   cone_weight,
//		float		   split_factor) noexcept;
//
//	std::size_t
//	buildMeshletsSpatial(
//		meshlet*	   meshlets,
//		uint32*		   meshlet_vertices,
//		unsigned char* meshlet_triangles,
//		const uint32*  indices,
//		std::size_t	   index_count,
//		const float*   vertex_positions,
//		std::size_t	   vertex_count,
//		std::size_t	   vertex_positions_stride,
//		std::size_t	   max_vertices,
//		std::size_t	   min_triangles,
//		std::size_t	   max_triangles,
//		float		   fill_weight) noexcept;
//
//	void
//	optimizeMeshlet(
//		uint32*		   meshlet_vertices,
//		unsigned char* meshlet_triangles,
//		std::size_t	   triangle_count,
//		std::size_t	   vertex_count) noexcept;
//
//	// Bounds
//	bounds
//	computeClusterBounds(
//		const uint32* indices,
//		std::size_t	  index_count,
//		const float*  vertex_positions,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_positions_stride) noexcept;
//
//	bounds
//	computeMeshletBounds(
//		const uint32*		 meshlet_vertices,
//		const unsigned char* meshlet_triangles,
//		std::size_t			 triangle_count,
//		const float*		 vertex_positions,
//		std::size_t			 vertex_count,
//		std::size_t			 vertex_positions_stride) noexcept;
//
//	bounds
//	computeSphereBounds(
//		const float* positions,
//		std::size_t	 count,
//		std::size_t	 positions_stride,
//		const float* radii,
//		std::size_t	 radii_stride) noexcept;
//
//	// Spatial clustering/sorting
//	std::size_t
//	partitionClusters(
//		uint32*		  destination,
//		const uint32* cluster_indices,
//		std::size_t	  total_index_count,
//		const uint32* cluster_index_counts,
//		std::size_t	  cluster_count,
//		const float*  vertex_positions,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_positions_stride,
//		std::size_t	  target_partition_size) noexcept;
//
//	void
//	spatialSortRemap(
//		uint32*		 destination,
//		const float* vertex_positions,
//		std::size_t	 vertex_count,
//		std::size_t	 vertex_positions_stride) noexcept;
//
//	void
//	spatialSortTriangles(
//		uint32*		  destination,
//		const uint32* indices,
//		std::size_t	  index_count,
//		const float*  vertex_positions,
//		std::size_t	  vertex_count,
//		std::size_t	  vertex_positions_stride) noexcept;
//
//	void
//	spatialClusterPoints(
//		uint32*		 destination,
//		const float* vertex_positions,
//		std::size_t	 vertex_count,
//		std::size_t	 vertex_positions_stride,
//		std::size_t	 cluster_size) noexcept;
//
//	// Quantization
//	unsigned short
//	quantizeHalf(float v) noexcept;
//	float
//	quantizeFloat(float v, int N) noexcept;
//	float
//	dequantizeHalf(unsigned short h) noexcept;
//
//	// Allocator
//	void
//	setAllocator(
//		void*(AGE_ALLOC_CALLCONV* allocate)(std::size_t),
//		void(AGE_ALLOC_CALLCONV* deallocate)(void*)) noexcept;
//
//}	 // namespace age::external::meshopt

namespace age::external::meshopt
{
	template <typename t_vertex>
	std::pair<age::vector<uint32>, age::vector<t_vertex>>
	gen_remap(
		const age::vector<uint32>&	 index_buffer,
		const age::vector<t_vertex>& vertex_buffer,
		const std::size_t			 vertex_size_and_stride = sizeof(t_vertex)) noexcept
	{
		auto remap_index_buffer = age::vector<uint32>::gen_sized(index_buffer.size());
		auto new_index_buffer	= age::vector<uint32>::gen_sized(index_buffer.size());
		auto new_vertex_buffer	= age::vector<t_vertex>::gen_sized(detail::gen_vertex_remap(
			remap_index_buffer.data(),
			index_buffer.data(),
			index_buffer.size(),
			vertex_buffer.data(),
			vertex_buffer.size(),
			vertex_size_and_stride));

		detail::gen_remapped_vertex_buffer(
			new_vertex_buffer.data(),
			vertex_buffer.data(),
			vertex_buffer.size(),
			vertex_size_and_stride,
			remap_index_buffer.data());

		detail::gen_remapped_index_buffer(
			new_index_buffer.data(),
			index_buffer.data(),
			index_buffer.size(),
			remap_index_buffer.data());

		return std::pair{ new_index_buffer, new_vertex_buffer };
	}

	template <typename t_vertex>
	void
	opt_reorder_buffers(
		age::vector<uint32>&   index_buffer,
		age::vector<t_vertex>& vertex_buffer,
		const std::size_t	   vertex_size_and_stride	   = sizeof(t_vertex),
		const float			   threshold_for_less_overdarw = 1.05f) noexcept
	{
		static_assert(sizeof(t_vertex) >= sizeof(float) * 3);
		detail::opt_index_buffer_for_vertex_cache(
			index_buffer.data(),
			index_buffer.data(),
			index_buffer.size(),
			vertex_buffer.size());

		detail::opt_index_buffer_for_less_overdraw(
			index_buffer.data(),
			index_buffer.data(),
			index_buffer.size(),
			reinterpret_cast<float*>(vertex_buffer.data()),
			vertex_buffer.size(),
			vertex_size_and_stride,
			threshold_for_less_overdarw);

		detail::opt_vertex_buffer_for_vertex_fetch(
			vertex_buffer.data(),
			index_buffer.data(),
			index_buffer.size(),
			vertex_buffer.data(),
			vertex_buffer.size(),
			vertex_size_and_stride);
	}

	struct meshlet
	{
		/* offsets within meshlet_vertices and meshlet_triangles arrays with meshlet data */
		uint32 vertex_offset;
		uint32 triangle_offset;

		/* number of vertices and triangles used in the meshlet; data is stored in consecutive range [offset..offset+count) for vertices and [offset..offset+count*3) for triangles */
		uint32 vertex_count;
		uint32 triangle_count;
	};

	template <typename t_vertex>
	decltype(auto)
	gen_meshlets(
		const age::vector<uint32>&	 index_buffer,
		const age::vector<t_vertex>& vertex_buffer,
		const std::size_t			 vertex_position_stride			= sizeof(t_vertex),
		const std::size_t			 max_vertex_count_per_meshlet	= 64,
		const std::size_t			 max_triangle_count_per_meshlet = 126,
		const float					 cone_culling_weight			= 0.25f) noexcept
	{
		auto max_meshlet_count = detail::calc_meshlet_max_count(
			index_buffer.size(),
			max_vertex_count_per_meshlet,
			max_triangle_count_per_meshlet);

		auto meshlet_vec				 = age::vector<meshlet>::gen_sized(max_meshlet_count);
		auto meshlet_global_index_buffer = age::vector<uint32>::gen_sized(index_buffer.size());
		auto meshlet_local_index_buffer	 = age::vector<uint8>::gen_sized(index_buffer.size());

		auto meshlet_count = detail::gen_meshlet_buffer_balanced(
			meshlet_vec.data(),
			meshlet_global_index_buffer.data(),
			meshlet_local_index_buffer.data(),
			index_buffer.data(),
			index_buffer.size(),
			reinterpret_cast<const float*>(vertex_buffer.data()),
			vertex_buffer.size(),
			vertex_position_stride,
			max_vertex_count_per_meshlet,
			max_triangle_count_per_meshlet,
			cone_culling_weight);

		{
			auto& meshlet_last = meshlet_vec[meshlet_count - 1];
			meshlet_global_index_buffer.resize(meshlet_last.vertex_offset + meshlet_last.vertex_count);
			meshlet_local_index_buffer.resize(meshlet_last.triangle_offset + meshlet_last.triangle_count * 3);
			meshlet_vec.resize(meshlet_count);
		}

		auto meshlet_bound_vec = age::vector<bounds>::gen_sized(meshlet_vec.size());

		for (auto&& [m, b] : std::views::zip(meshlet_vec, meshlet_bound_vec))
		{
			detail::opt_meshlet(
				&meshlet_global_index_buffer[m.vertex_offset],
				&meshlet_local_index_buffer[m.triangle_offset],
				m.vertex_count,
				m.triangle_count);

			detail::calc_meshlet_bounds(
				&b,
				&meshlet_global_index_buffer[m.vertex_offset],
				&meshlet_local_index_buffer[m.triangle_offset],
				m.triangle_count,
				reinterpret_cast<const float*>(vertex_buffer.data()),
				m.vertex_count,
				vertex_position_stride);
		}

		return std::tuple{ meshlet_global_index_buffer, meshlet_local_index_buffer, meshlet_vec, meshlet_bound_vec };
	}
}	 // namespace age::external::meshopt