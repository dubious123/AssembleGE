#pragma once
#include "age.hpp"

namespace age::graphics::pso
{
	using t_pso_id = uint32;

	struct handle
	{
		t_pso_id id;
	};

	template <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE pss_type_v, typename t_subobj>
	struct alignas(void*) pss
	{
		// using type = typename pss<pss_type, t_subobj>;

		const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE pss_type = pss_type_v;
		t_subobj								  subobj{};
	};

	template <typename... t_pss>
	struct alignas(void*) pss_stream
	{
		static constexpr std::size_t size_in_bytes = (age::util::align_up(sizeof(t_pss), alignof(void*)) + ...);
		alignas(void*)
			std::byte storage[size_in_bytes]{ std::byte{ 0 } };

		constexpr pss_stream(t_pss&&... arg) noexcept
		{
			auto offset = std::size_t{ 0 };

			(([&] {
				 std::memcpy(&storage[offset], &arg, sizeof(arg));
				 offset += age::util::align_up(sizeof(t_pss), alignof(void*));
			 }()),
			 ...);

			AGE_ASSERT(offset == size_in_bytes);
		}
	};

	template <typename... t_pss>
	pss_stream(t_pss&&...) -> pss_stream<t_pss...>;

	using pss_root_signature		= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*>;
	using pss_vs					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE>;
	using pss_ps					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE>;
	using pss_ds					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS, D3D12_SHADER_BYTECODE>;
	using pss_hs					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS, D3D12_SHADER_BYTECODE>;
	using pss_gs					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS, D3D12_SHADER_BYTECODE>;
	using pss_cs					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS, D3D12_SHADER_BYTECODE>;
	using pss_stream_output			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT, D3D12_STREAM_OUTPUT_DESC>;
	using pss_blend					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC>;
	using pss_sample_mask			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, UINT>;
	using pss_rasterizer			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC>;
	using pss_depth_stencil			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, D3D12_DEPTH_STENCIL_DESC>;
	using pss_input_layout			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC>;
	using pss_ib_strip_cut_value	= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE>;
	using pss_primitive_topology	= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE>;
	using pss_render_target_formats = pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY>;
	using pss_depth_stencil_format	= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT>;
	using pss_sample_desc			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC>;
	using pss_node_mask				= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK, D3D12_NODE_MASK>;
	using pss_cached_pso			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO, D3D12_CACHED_PIPELINE_STATE>;
	using pss_flags					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS, D3D12_PIPELINE_STATE_FLAGS>;
	using pss_depth_stencil1		= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1, D3D12_DEPTH_STENCIL_DESC1>;
	using pss_view_instancing		= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING, D3D12_VIEW_INSTANCING_DESC>;
	using pss_as					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS, D3D12_SHADER_BYTECODE>;
	using pss_ms					= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE>;
	using pss_depth_stencil2		= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2, D3D12_DEPTH_STENCIL_DESC2>;
	using pss_rasterizer1			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1, D3D12_RASTERIZER_DESC1>;
	using pss_rasterizer2			= pss<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2, D3D12_RASTERIZER_DESC2>;

	namespace detail
	{
		template <typename t>
		concept cx_pss = std::is_same_v<t, pss_root_signature>
					  or std::is_same_v<t, pss_vs>
					  or std::is_same_v<t, pss_ps>
					  or std::is_same_v<t, pss_ds>
					  or std::is_same_v<t, pss_hs>
					  or std::is_same_v<t, pss_gs>
					  or std::is_same_v<t, pss_cs>
					  or std::is_same_v<t, pss_stream_output>
					  or std::is_same_v<t, pss_blend>
					  or std::is_same_v<t, pss_sample_mask>
					  or std::is_same_v<t, pss_rasterizer>
					  or std::is_same_v<t, pss_depth_stencil>
					  or std::is_same_v<t, pss_input_layout>
					  or std::is_same_v<t, pss_ib_strip_cut_value>
					  or std::is_same_v<t, pss_primitive_topology>
					  or std::is_same_v<t, pss_render_target_formats>
					  or std::is_same_v<t, pss_depth_stencil_format>
					  or std::is_same_v<t, pss_sample_desc>
					  or std::is_same_v<t, pss_node_mask>
					  or std::is_same_v<t, pss_cached_pso>
					  or std::is_same_v<t, pss_flags>
					  or std::is_same_v<t, pss_depth_stencil1>
					  or std::is_same_v<t, pss_view_instancing>
					  or std::is_same_v<t, pss_as>
					  or std::is_same_v<t, pss_ms>
					  or std::is_same_v<t, pss_depth_stencil2>
					  or std::is_same_v<t, pss_rasterizer1>
					  or std::is_same_v<t, pss_rasterizer2>;
	}
}	 // namespace age::graphics::pso

namespace age::graphics::pso
{
	inline void
	destroy(handle h_pso) noexcept
	{
		g::pso_ptr_vec[h_pso.id]->Release();
		g::pso_ptr_vec.remove(h_pso.id);
	}

	handle
	create(detail::cx_pss auto&&... arg) noexcept
	{
		auto stream = pss_stream{
			FWD(arg)...
		};

		AGE_ASSERT(stream.storage is_not_nullptr);
		AGE_ASSERT(stream.size_in_bytes > 0);
		auto* p_pso = (ID3D12PipelineState*)nullptr;
		auto  desc	= D3D12_PIPELINE_STATE_STREAM_DESC{
			.SizeInBytes				   = stream.size_in_bytes,
			.pPipelineStateSubobjectStream = stream.storage
		};

		AGE_HR_CHECK(g::p_main_device->CreatePipelineState(&desc, IID_PPV_ARGS(&p_pso)));

		return handle{ .id = g::pso_ptr_vec.emplace_back(p_pso) };
	}
}	 // namespace age::graphics::pso