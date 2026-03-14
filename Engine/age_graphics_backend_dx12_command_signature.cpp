#include "age_pch.hpp"
#include "age.hpp"

#if defined(AGE_GRAPHICS_BACKEND_DX12)
namespace age::graphics::command_signature
{
	FORCE_INLINE decltype(auto)
	handle::ptr() const noexcept
	{
		return g::command_signature_ptr_vec[id];
	}
}	 // namespace age::graphics::command_signature

namespace age::graphics::command_signature
{
	template <typename t_indirect_arg>
	FORCE_INLINE handle
	create(root_signature::handle h_root_sig, auto&&... arg) noexcept
	{
		auto* p_res = (ID3D12CommandSignature*)(nullptr);

		D3D12_INDIRECT_ARGUMENT_DESC args[]{
			FWD(arg)...
		};

		c_auto desc = D3D12_COMMAND_SIGNATURE_DESC{
			.ByteStride		  = sizeof(t_indirect_arg),
			.NumArgumentDescs = sizeof...(arg),
			.pArgumentDescs	  = args
		};

		AGE_HR_CHECK(
			g::p_main_device->CreateCommandSignature(
				&desc,
				h_root_sig.ptr(),
				IID_PPV_ARGS(&p_res)));

		return handle{ .id = g::command_signature_ptr_vec.emplace_back(p_res) };
	}

	void
	destroy(handle& h) noexcept
	{
		auto& vec = g::command_signature_ptr_vec;
		vec[h]->Release();
		vec.remove(h);

		h.id = age::get_invalid_id<t_command_signature_id>();
	}

	void
	init() noexcept
	{
		auto options21 = D3D12_FEATURE_DATA_D3D12_OPTIONS21{};
		AGE_HR_CHECK(g::p_main_device->CheckFeatureSupport(
			D3D12_FEATURE_D3D12_OPTIONS21,
			&options21,
			sizeof(options21)));

		AGE_ASSERT(options21.ExecuteIndirectTier >= D3D12_EXECUTE_INDIRECT_TIER_1_1);
	}

	void
	deinit() noexcept
	{
		AGE_ASSERT(g::command_signature_ptr_vec.is_empty());

		for (auto* p_cmd_sig : g::command_signature_ptr_vec)
		{
			p_cmd_sig->Release();
		}

		if constexpr (age::config::debug_mode)
		{
			g::command_signature_ptr_vec.debug_validate();
		}

		g::command_signature_ptr_vec.clear();
	}
}	 // namespace age::graphics::command_signature
#endif