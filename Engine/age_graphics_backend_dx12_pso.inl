#pragma once
#include "age.hpp"

namespace age::graphics::pso
{
	inline void
	handle::set_name(auto* p_name) const noexcept
	{
		g::pso_ptr_vec[id]->SetName(p_name);
	}

	inline ID3D12PipelineState*
	handle::ptr() const noexcept
	{
		return g::pso_ptr_vec[id];
	}

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