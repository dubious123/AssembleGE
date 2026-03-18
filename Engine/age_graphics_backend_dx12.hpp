#pragma once
#include "age.hpp"

// internal globals
namespace age::graphics::g
{
	inline auto frame_buffer_idx = uint8{ 0 };	  // [0, 1 ... ,frame_buffer_count - 1]

	inline auto* p_dxgi_factory = (IDXGIFactory7*)nullptr;
	inline auto* p_main_adapter = (IDXGIAdapter4*)nullptr;
	inline auto* p_main_device	= (ID3D12Device11*)nullptr;

	//---[ command ]------------------------------------------------------------
	inline queue_context queue_ctx[e::size<e::queue_kind>()];

	//---[ shader ]------------------------------------------------------------
	inline auto* p_dxc_compiler		   = (IDxcCompiler3*)nullptr;
	inline auto* p_dxc_utils		   = (IDxcUtils*)nullptr;
	inline auto* p_dxc_include_handler = (IDxcIncludeHandler*)nullptr;

	//------------------------------------------------------------------------------
	inline auto rtv_desc_pool		  = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 128>{};
	inline auto dsv_desc_pool		  = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 128>{};
	inline auto cbv_srv_uav_desc_pool = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024>{};
	inline auto sampler_desc_pool	  = descriptor_pool<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 128>{};

	inline auto render_surface_vec = age::stable_dense_vector<render_surface>::gen_reserved(2);

	inline auto root_signature_ptr_vec = age::stable_dense_vector<ID3D12RootSignature*>::gen_reserved(2);

	inline auto pso_ptr_vec = age::stable_dense_vector<ID3D12PipelineState*>::gen_reserved(2);

	inline auto shader_blob_vec = age::stable_dense_vector<shader::shader_blob>::gen_reserved(16);

	inline auto command_signature_ptr_vec = age::stable_dense_vector<ID3D12CommandSignature*>::gen_reserved(2);

	//---[ resource ]--------------------------------------------------------------
	inline auto resource_vec = age::stable_dense_vector<d3d12_resource>::gen_reserved(2);

	inline auto resource_mapping_vec = age::stable_dense_vector<resource_mapping>::gen_reserved(2);

	//---[ rt ]---------------------------------------------------------------------

	inline auto h_rt_scratch_buffer = age::graphics::resource_handle{};

	inline auto blas_buffer_data_vec = age::stable_dense_vector<rt::blas_buffer_data>::gen_reserved(2);
	inline auto blas_data_vec		 = age::stable_dense_vector<rt::blas_data>::gen_reserved(2);

	//------------------------------------------------------------------------------
}	 // namespace age::graphics::g

// util
namespace age::graphics
{
	bool
	is_tearing_allowed() noexcept;

	constexpr std::string_view
	to_string(D3D12_RESOURCE_DIMENSION dim) noexcept;

	constexpr std::string_view
	to_string(DXGI_FORMAT _) noexcept;
}	 // namespace age::graphics

namespace age::graphics::command
{
	FORCE_INLINE void
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
	begin_frame(e::queue_kind, auto... thread_idx /*[0, thread_count)*/) noexcept;

	FORCE_INLINE void
	begin_frame(e::queue_kind _ = e::queue_kind::direct /*thread_idx = 0*/) noexcept;

	FORCE_INLINE uint64
	signal(e::queue_kind _ = e::queue_kind::direct) noexcept;

	FORCE_INLINE void
	end_frame(e::queue_kind, auto... thread_idx) noexcept;

	FORCE_INLINE void
	end_frame(e::queue_kind _ = e::queue_kind::direct /*thread_idx = 0*/) noexcept;

#define DEF_CMD(my_name, dx12_name)                                                            \
	FORCE_INLINE void                                                                          \
	my_name(e::queue_kind kind, uint8 thread_idx, auto&&... arg)                               \
	{                                                                                          \
		auto& cmd_list = *g::queue_ctx[std::to_underlying(kind)].p_cmd_list[thread_idx];       \
                                                                                               \
		cmd_list.dx12_name(FWD(arg)...);                                                       \
	}                                                                                          \
                                                                                               \
	FORCE_INLINE void                                                                          \
	my_name(auto&&... arg)                                                                     \
		requires(sizeof...(arg) == 0                                                           \
				 or meta::not_same_as<meta::variadic_front_t<BARE_OF(arg)...>, e::queue_kind>) \
	{                                                                                          \
		my_name(e::queue_kind::direct, uint8{ 0 }, FWD(arg)...);                               \
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

	resource_handle
	create_placed(const resource_create_desc& desc, ID3D12Heap& heap, uint64 offset) noexcept;

	mapping_handle
	create_buffer_committed(uint32				 buffer_byte_size,
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

	void
	release_resource(resource_handle& _) noexcept;


	mapping_handle
	map_all(resource_handle _) noexcept;

	void
	unmap(mapping_handle& _) noexcept;

	void
	unmap_and_release(mapping_handle& _) noexcept;

	FORCE_INLINE void
	create_view(const ID3D12Resource&, const auto& h_desc, const auto& view_desc) noexcept;

	FORCE_INLINE void
	create_view(const graphics::resource_handle& h_resource, const auto& h_desc, const auto& view_desc) noexcept;
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

	void
	unload_shader(shader_handle _) noexcept;
}	 // namespace age::graphics::shader

namespace age::graphics::rt
{
	void
	init() noexcept;

	blas_buffer_handle
	create_blas_buffer(std::size_t initial_size) noexcept;

	void
	release_blas_buffer(blas_buffer_handle&) noexcept;

	blas_handle
	build_blas(blas_buffer_handle, auto&&... rt_geo_desc) noexcept;

	void
	release_blas(blas_handle& h_blas) noexcept;

	void
	deinit() noexcept;
}	 // namespace age::graphics::rt
