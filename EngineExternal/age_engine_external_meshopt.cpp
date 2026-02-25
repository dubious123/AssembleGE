#include "meshoptimizer/meshoptimizer.h"
#include "meshoptimizer/allocator.cpp"
#include "meshoptimizer/clusterizer.cpp"
#include "meshoptimizer/indexanalyzer.cpp"
#include "meshoptimizer/indexcodec.cpp"
#include "meshoptimizer/indexgenerator.cpp"
#include "meshoptimizer/meshletcodec.cpp"
#include "meshoptimizer/overdrawoptimizer.cpp"
#include "meshoptimizer/partition.cpp"
#include "meshoptimizer/quantization.cpp"
#include "meshoptimizer/rasterizer.cpp"
#include "meshoptimizer/simplifier.cpp"
#include "meshoptimizer/spatialorder.cpp"
#include "meshoptimizer/stripifier.cpp"
#include "meshoptimizer/vcacheoptimizer.cpp"
#include "meshoptimizer/vertexcodec.cpp"	
#include "meshoptimizer/vfetchoptimizer.cpp"
#include "meshoptimizer/vertexfilter.cpp"

namespace age::external::meshopt::detail
{
	unsigned long long
	gen_vertex_remap(
		unsigned int*		p_remap_idx_arr_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const void*			p_vertex_buffer,
		unsigned long long	vertex_count,
		unsigned long long	vertex_size_and_stride) noexcept
	{
		return meshopt_generateVertexRemap(
			p_remap_idx_arr_out,
			p_index_buffer,
			index_count,
			p_vertex_buffer,
			vertex_count,
			vertex_size_and_stride);
	}

	unsigned long long
	gen_vertex_remap(
		unsigned int*		p_remap_idx_arr_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count,
		void**				pp_vertex_attribute_ptr_arr,
		unsigned long long* p_vertex_attribute_size_arr,
		unsigned long long* p_vertex_attribute_stride_arr,
		unsigned long long	stream_count) noexcept
	{
		auto* p_stream_arr = new meshopt_Stream[stream_count];
		for (auto i = 0; i < stream_count; ++i)
		{
			p_stream_arr[i] = {
				pp_vertex_attribute_ptr_arr[i],
				p_vertex_attribute_size_arr[i],
				p_vertex_attribute_stride_arr[i]
			};
		}

		auto res = meshopt_generateVertexRemapMulti(
			p_remap_idx_arr_out,
			p_index_buffer,
			index_count,
			vertex_count,
			p_stream_arr,
			stream_count);

		delete[] p_stream_arr;

		return res;
	}

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
		void* p_vertex_attribute_cmp_ctx) noexcept
	{
		return meshopt_generateVertexRemapCustom(
			p_remap_idx_arr_out,
			p_index_buffer,
			index_count,
			p_vertex_position_arr,
			vertex_count,
			vertex_positions_stride,
			fp_cmp_vertex_attribute,
			p_vertex_attribute_cmp_ctx);
	}

	void
	gen_remapped_vertex_buffer(
		void*				p_reordered_vertex_buffer_out,
		const void*			p_old_vertex_buffer,
		unsigned long long	vertex_count,
		unsigned long long	vertex_size_and_stride,
		const unsigned int* p_remap_idx_arr) noexcept
	{
		meshopt_remapVertexBuffer(p_reordered_vertex_buffer_out,
								  p_old_vertex_buffer,
								  vertex_count,
								  vertex_size_and_stride,
								  p_remap_idx_arr);
	}

	void
	gen_remapped_index_buffer(
		unsigned int*		p_reordered_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const unsigned int* p_remap_idx_arr) noexcept
	{
		meshopt_remapIndexBuffer(
			p_reordered_index_buffer_out,
			p_index_buffer,
			index_count,
			p_remap_idx_arr);
	}

	void
	gen_shadow_index_buffer(
		unsigned int*		p_shadow_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const void*			p_vertex_buffer,
		unsigned long long	vertex_count,
		unsigned long long	vertex_size,
		unsigned long long	vertex_stride) noexcept
	{
		return meshopt_generateShadowIndexBuffer(
			p_shadow_index_buffer_out,
			p_index_buffer,
			index_count,
			p_vertex_buffer,
			vertex_count,
			vertex_size,
			vertex_stride);
	}

	void
	gen_shadow_index_buffer_multi(
		unsigned int*		p_shadow_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count,
		void**				pp_vertex_attribute_ptr_arr,
		unsigned long long* p_vertex_attribute_size_arr,
		unsigned long long* p_vertex_attribute_stride_arr,
		unsigned long long	stream_count) noexcept
	{
		auto* p_stream_arr = new meshopt_Stream[stream_count];
		for (auto i = 0; i < stream_count; ++i)
		{
			p_stream_arr[i] = {
				pp_vertex_attribute_ptr_arr[i],
				p_vertex_attribute_size_arr[i],
				p_vertex_attribute_stride_arr[i]
			};
		}

		meshopt_generateShadowIndexBufferMulti(
			p_shadow_index_buffer_out,
			p_index_buffer,
			index_count,
			vertex_count,
			p_stream_arr,
			stream_count);

		delete[] p_stream_arr;
	}

	void
	gen_position_remap(
		unsigned int*	   p_remap_idx_arr_out,
		const float*	   p_vertex_position_arr,
		unsigned long long vertex_count,
		unsigned long long vertex_positions_stride) noexcept
	{
		return meshopt_generatePositionRemap(p_remap_idx_arr_out,
											 p_vertex_position_arr,
											 vertex_count,
											 vertex_positions_stride);
	}

	void
	opt_index_buffer_for_vertex_cache(
		unsigned int*		p_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count) noexcept
	{
		meshopt_optimizeVertexCache(
			p_index_buffer_out,
			p_index_buffer,
			index_count,
			vertex_count);
	}

	void
	opt_index_buffer_for_vertex_cache_strip(
		unsigned int*		p_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count) noexcept
	{
		meshopt_optimizeVertexCacheStrip(
			p_index_buffer_out,
			p_index_buffer,
			index_count,
			vertex_count);
	}

	// faster, but results on less performant results
	void
	opt_index_buffer_for_vertex_cache_fast(
		unsigned int*		p_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count,
		unsigned int		cache_size) noexcept
	{
		meshopt_optimizeVertexCacheFifo(
			p_index_buffer_out,
			p_index_buffer,
			index_count,
			vertex_count,
			cache_size);
	}

	void
	opt_index_buffer_for_less_overdraw(
		unsigned int*		p_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const float*		vertex_positions,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride,
		float				threshold) noexcept
	{
		meshopt_optimizeOverdraw(
			p_index_buffer_out,
			p_index_buffer,
			index_count,
			vertex_positions,
			vertex_count,
			vertex_positions_stride,
			threshold);
	}

	unsigned long long
	opt_vertex_buffer_for_vertex_fetch(
		void*			   p_vertex_buffer_out,
		unsigned int*	   p_index_buffer,
		unsigned long long index_count,
		const void*		   p_vertex_buffer,
		unsigned long long vertex_count,
		unsigned long long vertex_size) noexcept
	{
		return meshopt_optimizeVertexFetch(
			p_vertex_buffer_out,
			p_index_buffer,
			index_count,
			p_vertex_buffer,
			vertex_count,
			vertex_size);
	}

	unsigned long long
	gen_vertex_remap_for_vertex_fetch(
		unsigned int*		p_remap_idx_arr_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count) noexcept
	{
		return meshopt_optimizeVertexFetchRemap(
			p_remap_idx_arr_out,
			p_index_buffer,
			index_count,
			vertex_count);
	}

	unsigned long long
	calc_meshlet_max_count(
		unsigned long long index_count,
		unsigned long long max_vertex_count_per_meshlet,
		unsigned long long max_triangle_count_per_meshlet) noexcept
	{
		return meshopt_buildMeshletsBound(
			index_count,
			max_vertex_count_per_meshlet,
			max_triangle_count_per_meshlet);
	}

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
		float				cone_culling_weight) noexcept
	{
		return meshopt_buildMeshlets(
			reinterpret_cast<meshopt_Meshlet*>(p_meshlet_buffer_out),
			p_meshlet_global_index_buffer_out,
			p_meshlet_local_index_buffer_out,
			p_index_buffer,
			index_count,
			p_vertex_position_arr,
			vertex_count,
			vertex_positions_stride,
			max_vertex_count_per_meshlet,
			max_triangle_count_per_meshlet,
			cone_culling_weight);
	}

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
		float				cone_culling_weight,
		float				split_factor) noexcept
	{
		return meshopt_buildMeshletsFlex(
			reinterpret_cast<meshopt_Meshlet*>(p_meshlet_buffer_out),
			p_meshlet_global_index_buffer_out,
			p_meshlet_local_index_buffer_out,
			p_index_buffer,
			index_count,
			p_vertex_position_arr,
			vertex_count,
			vertex_positions_stride,
			max_vertex_count_per_meshlet,
			min_triangle_count_per_meshlet,
			max_triangle_count_per_meshlet,
			cone_culling_weight,
			split_factor);
	}

	void
	opt_meshlet(
		unsigned int*	   p_meshlet_global_index_buffer,
		unsigned char*	   p_meshlet_local_index_buffer,
		unsigned long long meshlet_triangle_count,
		unsigned long long meshlet_vertex_count) noexcept
	{
		meshopt_optimizeMeshlet(
			p_meshlet_global_index_buffer,
			p_meshlet_local_index_buffer,
			meshlet_triangle_count,
			meshlet_vertex_count);
	}

	unsigned long long
	gen_meshlet_buffer_fast(
		void*				p_meshlet_buffer_out,
		unsigned int*		p_meshlet_global_index_buffer_out,
		unsigned char*		p_meshlet_local_index_buffer_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		unsigned long long	vertex_count,
		unsigned long long	max_vertex_count_per_meshlet,
		unsigned long long	max_triangle_count_per_meshlet) noexcept
	{
		return meshopt_buildMeshletsScan(
			reinterpret_cast<meshopt_Meshlet*>(p_meshlet_buffer_out),
			p_meshlet_global_index_buffer_out,
			p_meshlet_local_index_buffer_out,
			p_index_buffer,
			index_count,
			vertex_count,
			max_vertex_count_per_meshlet,
			max_triangle_count_per_meshlet);
	}

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
		float				fill_weight) noexcept
	{
		return meshopt_buildMeshletsSpatial(
			reinterpret_cast<meshopt_Meshlet*>(p_meshlet_buffer_out),
			p_meshlet_global_index_buffer_out,
			p_meshlet_local_index_buffer_out,
			p_index_buffer,
			index_count,
			p_vertex_position_arr,
			vertex_count,
			vertex_positions_stride,
			max_vertices,
			min_triangle_count_per_meshlet,
			max_triangle_count_per_meshlet,
			fill_weight);
	}

	void
	calc_triangles_bounds(
		void*				p_bounds_out,
		const unsigned int* p_index_buffer,
		unsigned long long	index_count,
		const float*		p_vertex_position_arr,
		unsigned long long	vertex_count,
		unsigned long long	vertex_positions_stride) noexcept
	{
		auto bounds = meshopt_computeClusterBounds(
			p_index_buffer,
			index_count,
			p_vertex_position_arr,
			vertex_count,
			vertex_positions_stride);

		memcpy(p_bounds_out, &bounds, sizeof(bounds));
	}

	void
	calc_meshlet_bounds(
		void*				 p_bounds_out,
		const unsigned int*	 p_meshlet_global_index_buffer,
		const unsigned char* p_meshlet_local_index_buffer,
		unsigned long long	 triangle_count,
		const float*		 p_vertex_position_arr,
		unsigned long long	 vertex_count,
		unsigned long long	 vertex_positions_stride) noexcept
	{
		auto bounds = meshopt_computeMeshletBounds(
			p_meshlet_global_index_buffer,
			p_meshlet_local_index_buffer,
			triangle_count,
			p_vertex_position_arr,
			vertex_count,
			vertex_positions_stride);

		memcpy(p_bounds_out, &bounds, sizeof(bounds));
	}

	void
	calc_spheres_bounds(
		void*			   p_bounds_out,
		const float*	   p_position_arr,
		unsigned long long count,
		unsigned long long positions_stride,
		const float*	   p_radius_arr,
		unsigned long long radius_stride) noexcept
	{
		auto bounds = meshopt_computeSphereBounds(
			p_position_arr,
			count,
			positions_stride,
			p_radius_arr,
			radius_stride);

		memcpy(p_bounds_out, &bounds, sizeof(bounds));
	}

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
		unsigned long long	target_partition_size) noexcept
	{
		return meshopt_partitionClusters(
			p_partition_index_buffer_out,
			p_cluster_global_index_buffer,
			cluster_global_index_buffer_count,
			p_cluster_index_count_arr,
			cluster_count,
			p_vertex_positions,
			vertex_count,
			vertex_positions_stride,
			target_partition_size);
	}
}	 // namespace age::external::meshopt::detail