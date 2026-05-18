#pragma once
#include "age.hpp"

// util
namespace age::graphics
{
	bool
	is_tearing_allowed() noexcept;

	constexpr std::string_view
	to_string(D3D12_RESOURCE_DIMENSION dim) noexcept;

	constexpr std::string_view
	to_string(DXGI_FORMAT _) noexcept;

	constexpr uint32
	format_size(DXGI_FORMAT format) noexcept;

	constexpr DXGI_FORMAT
	dx12_format(e::texture_format _) noexcept;

	constexpr uint32
	format_size(e::texture_format format) noexcept;
}	 // namespace age::graphics

namespace age::graphics::command
{
	queue_context&
	get_queue_ctx(e::queue_kind kind) noexcept;

	void
	cpu_wait(e::queue_kind _ = e::queue_kind::direct) noexcept;

	FORCE_INLINE void
	gpu_wait(e::queue_kind who, e::queue_kind what) noexcept;

	FORCE_INLINE void
	begin(e::queue_kind, auto... thread_idx /*[0, thread_count)*/) noexcept;

	FORCE_INLINE void
	begin(e::queue_kind _ = e::queue_kind::direct /*thread_idx = 0*/) noexcept;

	FORCE_INLINE void
	execute(e::queue_kind, auto... thread_idx) noexcept;

	FORCE_INLINE void
	execute(e::queue_kind _ = e::queue_kind::direct /*thread_idx = 0*/) noexcept;

	FORCE_INLINE void
	execute_and_wait(e::queue_kind, auto... thread_idx) noexcept;

	FORCE_INLINE void
	execute_and_wait(e::queue_kind _ = e::queue_kind::direct /*thread_idx = 0*/) noexcept;

	FORCE_INLINE void
	execute_and_resume(e::queue_kind, auto... thread_idx) noexcept;

	FORCE_INLINE void
	execute_and_resume(e::queue_kind _ = e::queue_kind::direct /*thread_idx = 0*/) noexcept;

	FORCE_INLINE void
	wait_current_frame(e::queue_kind kind) noexcept;

	FORCE_INLINE void
	begin_frame(e::queue_kind, auto... thread_idx /*[0, thread_count)*/) noexcept;

	FORCE_INLINE void
	begin_frame(e::queue_kind _ = e::queue_kind::direct /*thread_idx = 0*/) noexcept;

	uint64
	signal(e::queue_kind _ = e::queue_kind::direct) noexcept;

	FORCE_INLINE void
	end_frame(e::queue_kind, auto... thread_idx) noexcept;

	FORCE_INLINE void
	end_frame(e::queue_kind _ = e::queue_kind::direct /*thread_idx = 0*/) noexcept;

	FORCE_INLINE uint64
	get_frame_fence_value(e::queue_kind _ = e::queue_kind::direct) noexcept;

	FORCE_INLINE uint64
	get_completed_fence_value(e::queue_kind _ = e::queue_kind::direct) noexcept;

#define DEF_CMD(my_name, dx12_name)                                                                \
	FORCE_INLINE void                                                                              \
	my_name(e::queue_kind kind, uint8 thread_idx, auto&&... arg)                                   \
	{                                                                                              \
		get_queue_ctx(kind).p_cmd_list[thread_idx]->dx12_name(FWD(arg)...);                        \
	}                                                                                              \
                                                                                                   \
	FORCE_INLINE void                                                                              \
	my_name(auto&&... arg)                                                                         \
		requires(sizeof...(arg) == 0                                                               \
				 or meta::not_same_as<meta::variadic_front_t<BARE_OF(arg)...>, e::queue_kind>)     \
	{                                                                                              \
		my_name(e::queue_kind::direct, uint8{ 0 }, FWD(arg)...);                                   \
	}                                                                                              \
	namespace compute                                                                              \
	{                                                                                              \
		FORCE_INLINE void                                                                          \
		my_name(auto&&... arg)                                                                     \
			requires(sizeof...(arg) == 0                                                           \
					 or meta::not_same_as<meta::variadic_front_t<BARE_OF(arg)...>, e::queue_kind>) \
		{                                                                                          \
			command::my_name(e::queue_kind::compute, uint8{ 0 }, FWD(arg)...);                     \
		}                                                                                          \
	}                                                                                              \
	namespace copy                                                                                 \
	{                                                                                              \
		FORCE_INLINE void                                                                          \
		my_name(auto&&... arg)                                                                     \
			requires(sizeof...(arg) == 0                                                           \
					 or meta::not_same_as<meta::variadic_front_t<BARE_OF(arg)...>, e::queue_kind>) \
		{                                                                                          \
			command::my_name(e::queue_kind::copy, uint8{ 0 }, FWD(arg)...);                        \
		}                                                                                          \
	}

	DEF_CMD(set_descriptor_heaps, SetDescriptorHeaps);
	DEF_CMD(set_graphics_root_sig, SetGraphicsRootSignature);
	DEF_CMD(set_compute_root_sig, SetComputeRootSignature);

	DEF_CMD(set_graphics_root_constants, SetGraphicsRoot32BitConstants);
	DEF_CMD(set_compute_root_constants, SetComputeRoot32BitConstants);

	DEF_CMD(set_graphics_root_cbv, SetGraphicsRootConstantBufferView);
	DEF_CMD(set_graphics_root_srv, SetGraphicsRootShaderResourceView);
	DEF_CMD(set_graphics_root_uav, SetGraphicsRootUnorderedAccessView);

	DEF_CMD(set_compute_root_cbv, SetComputeRootConstantBufferView);
	DEF_CMD(set_compute_root_srv, SetComputeRootShaderResourceView);
	DEF_CMD(set_compute_root_uav, SetComputeRootUnorderedAccessView);

	DEF_CMD(set_barrier, Barrier);

	DEF_CMD(set_pso, SetPipelineState)
	DEF_CMD(dispatch, Dispatch)
	DEF_CMD(dispatch_mesh, DispatchMesh)
	DEF_CMD(execute_indirect, ExecuteIndirect)

	DEF_CMD(begin_render_pass, BeginRenderPass)
	DEF_CMD(end_render_pass, EndRenderPass)

	DEF_CMD(set_view_ports, RSSetViewports)
	DEF_CMD(set_scissor_rects, RSSetScissorRects)

	// rt
	DEF_CMD(copy_rt_acceleration_structure, CopyRaytracingAccelerationStructure)
	DEF_CMD(build_rt_acceleration_structure, BuildRaytracingAccelerationStructure)

	// copy
	DEF_CMD(copy_buffer, CopyBufferRegion)
	DEF_CMD(copy_texture, CopyTextureRegion)

	// clear
	DEF_CMD(clear_dsv, ClearDepthStencilView)
	DEF_CMD(clear_uav_float, ClearUnorderedAccessViewFloat)

	FORCE_INLINE void
	apply_barriers(auto&&...) noexcept;

#undef DEF_CMD

	uint64
	current_fence_value(e::queue_kind _ = e::queue_kind::direct) noexcept;

	bool
	is_complete(e::queue_kind, uint64 fence_value) noexcept;

	bool
	is_idle(e::queue_kind _) noexcept;

	void
	init() noexcept;

	void
	deinit() noexcept;
}	 // namespace age::graphics::command

// descriptor_pool fwd
namespace age::graphics
{
	void
	pop_descriptor(auto& h_descriptor_out) noexcept;

	void
	push_descriptor(const auto&) noexcept;

	uint32
	calc_desc_idx(auto handle) noexcept;
}	 // namespace age::graphics

// resource
namespace age::graphics::resource
{
	void
	init() noexcept;

	void
	deinit() noexcept;

	resource_handle
	create_committed(const resource_create_desc& desc) noexcept;

	template <auto n>
	std::array<resource_handle, n>
	create_committed(const resource_create_desc& desc) noexcept;

	resource_handle
	create_placed(const resource_create_desc& desc, ID3D12Heap& heap, uint64 offset) noexcept;

	mapping_handle
	create_buffer_committed(uint32				 buffer_byte_size,
							const void*			 p_data			= nullptr,
							e::memory_kind		 kind			= e::memory_kind::cpu_to_gpu_direct,
							D3D12_BARRIER_LAYOUT initial_layout = D3D12_BARRIER_LAYOUT_UNDEFINED,
							D3D12_RESOURCE_FLAGS flags			= D3D12_RESOURCE_FLAG_NONE) noexcept;

	template <auto n>
	std::array<mapping_handle, n>
	create_buffer_committed(uint32				 buffer_byte_size,
							const void*			 p_data			= nullptr,
							e::memory_kind		 kind			= e::memory_kind::cpu_to_gpu_direct,
							D3D12_BARRIER_LAYOUT initial_layout = D3D12_BARRIER_LAYOUT_UNDEFINED,
							D3D12_RESOURCE_FLAGS flags			= D3D12_RESOURCE_FLAG_NONE) noexcept;

	std::array<mapping_handle, global::frame_buffer_count>
	create_buffer_committed_arr(uint32				 buffer_byte_size,
								const void*			 p_data			= nullptr,
								e::memory_kind		 kind			= e::memory_kind::cpu_to_gpu_direct,
								D3D12_BARRIER_LAYOUT initial_layout = D3D12_BARRIER_LAYOUT_UNDEFINED,
								D3D12_RESOURCE_FLAGS flags			= D3D12_RESOURCE_FLAG_NONE) noexcept;

	mapping_handle
	create_buffer_placed(uint32				  buffer_byte_size,
						 ID3D12Heap&		  heap,
						 uint64				  offset,
						 const void*		  p_data		 = nullptr,
						 e::memory_kind		  kind			 = e::memory_kind::cpu_to_gpu_direct,
						 D3D12_BARRIER_LAYOUT initial_layout = D3D12_BARRIER_LAYOUT_UNDEFINED,
						 D3D12_RESOURCE_FLAGS flags			 = D3D12_RESOURCE_FLAG_NONE) noexcept;


	FORCE_INLINE void
	release_deferred(resource_handle&, e::queue_kind = e::queue_kind::direct) noexcept;

	FORCE_INLINE void
	release_deferred(resource_handle& h_resource, srv_desc_handle h_srv, e::queue_kind kind = e::queue_kind::direct) noexcept;

	void
	process_deferred_releases() noexcept;

	FORCE_INLINE bool
	resize_buffer(resource_handle&, uint64 required_size) noexcept;

	FORCE_INLINE bool
	resize_buffer(mapping_handle& h_mapping, uint64 required_size) noexcept;

	FORCE_INLINE bool
	resize_texture_2d(resource_handle& h_resource, extent_2d<uint32> required_size, DXGI_FORMAT format) noexcept;

	FORCE_INLINE bool
	resize_texture_2d(resource_handle& h_resource, extent_2d<uint32> required_size) noexcept;

	FORCE_INLINE bool
	resize_buffer_preserve(resource_handle&, uint64 required_size) noexcept;

	FORCE_INLINE bool
	resize_buffer_preserve(mapping_handle& h_mapping, uint64 required_size) noexcept;

	mapping_handle
	map_all(resource_handle _) noexcept;

	void
	unmap(mapping_handle& _) noexcept;

	void
	unmap_and_release(mapping_handle& _) noexcept;

	void
	unmap_and_release_deferred(mapping_handle& _) noexcept;

	void
	unmap_and_release(std::span<mapping_handle> _) noexcept;

	FORCE_INLINE void
	create_view(const ID3D12Resource&, const auto& h_desc, const auto& view_desc) noexcept;

	FORCE_INLINE void
	create_view(const graphics::resource_handle& h_resource, const auto& h_desc, const auto& view_desc) noexcept;

	FORCE_INLINE void
	create_view(const auto& h_desc, const auto& view_desc) noexcept;

	inline void
	set_name(std::span<resource_handle>, const wchar_t* fmt) noexcept;

	inline void
	set_name(std::span<mapping_handle>, const wchar_t* fmt) noexcept;

	void
	upload_texture(resource_handle h_dst, const void* p_src_cpu, age::extent_2d<uint32>, DXGI_FORMAT) noexcept;

	void
	upload_texture(resource_handle h_dst, const void* p_src_cpu) noexcept;
}	 // namespace age::graphics::resource

// resource_barrier
namespace age::graphics
{
	FORCE_INLINE void
	apply_barriers(auto&&... barrier) noexcept;
}	 // namespace age::graphics

// root signature
namespace age::graphics::root_signature
{
	inline void destroy(handle) noexcept;

	inline void
	init() noexcept;

	inline void
	deinit() noexcept;
}	 // namespace age::graphics::root_signature

namespace age::graphics::command_signature
{
	template <typename t_indirect_arg>
	FORCE_INLINE handle
	create(root_signature::handle, auto&&... indirect_arg_desc) noexcept;

	void
	destroy(handle& _) noexcept;

	void
	init() noexcept;

	void
	deinit() noexcept;
}	 // namespace age::graphics::command_signature

// shader
namespace age::graphics::shader
{
	void
	init() noexcept;

	void
	deinit() noexcept;

	void
	compile_shader(
		std::wstring_view	  hlsl_path,
		std::wstring_view	  entry_point,
		std::wstring_view	  target,
		std::filesystem::path save_path) noexcept;

	shader_handle
	load_shader(std::filesystem::path shader_path) noexcept;

	D3D12_SHADER_BYTECODE
	get_d3d12_bytecode(shader_handle) noexcept;

	D3D12_SHADER_BYTECODE
	get_d3d12_bytecode(e::engine_shader_kind) noexcept;

	void
	unload_shader(shader_handle _) noexcept;
}	 // namespace age::graphics::shader

namespace age::graphics::rt
{
	void
	init() noexcept;

	resource_handle
	create_blas_buffer(std::size_t initial_size) noexcept;

	resource_handle
	build_blas(auto&&... rt_geo_desc) noexcept;

	FORCE_INLINE
	std::tuple<uint64, uint64>
	query_tlas_size(uint32 instance_count) noexcept;

	FORCE_INLINE void
	build_tlas(resource_handle h_tlas_buffer, resource_handle h_tlas_scratch_buffer, resource_handle h_instance_buffer, uint32 instance_count) noexcept;

	void
	deinit() noexcept;
}	 // namespace age::graphics::rt

namespace age::graphics::bake
{
	void
	init() noexcept;

	void
	deinit() noexcept;
}	 // namespace age::graphics::bake
