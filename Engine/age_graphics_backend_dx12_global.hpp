#pragma once
#include "age.hpp"

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

	inline auto deferred_release_data_vec = age::vector<deferred_release_data>::gen_reserved(2);

	inline auto h_upload_buffer	  = mapping_handle{};
	inline auto h_readback_buffer = mapping_handle{};

	inline auto upload_footprint_vec	  = age::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>::gen_reserved(1);
	inline auto upload_num_rows_vec		  = age::vector<uint32>::gen_reserved(1);
	inline auto upload_row_size_bytes_vec = age::vector<uint64>::gen_reserved(1);

	inline auto readback_footprint_vec		= age::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>::gen_reserved(1);
	inline auto readback_num_rows_vec		= age::vector<uint32>::gen_reserved(1);
	inline auto readback_row_size_bytes_vec = age::vector<uint64>::gen_reserved(1);

	inline auto calc_readback_size_footprint_vec	  = age::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>::gen_reserved(1);
	inline auto calc_readback_size_num_rows_vec		  = age::vector<uint32>::gen_reserved(1);
	inline auto calc_readback_size_row_size_bytes_vec = age::vector<uint64>::gen_reserved(1);


	//---[ rt ]---------------------------------------------------------------------

	inline auto h_rt_blas_scratch_buffer = age::graphics::resource_handle{};

	//---[ bake ]------------------------------------------------------------------------
	inline auto bake_pipeline = bake::pipeline{};

	inline auto h_brdf_lut = resource_handle{};
}	 // namespace age::graphics::g