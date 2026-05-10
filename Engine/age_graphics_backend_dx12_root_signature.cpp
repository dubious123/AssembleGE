#include "age_pch.hpp"
#include "age.hpp"

namespace age::graphics::root_signature
{

	void
	init() noexcept
	{
		auto feature = D3D12_FEATURE_DATA_ROOT_SIGNATURE{
			.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_2
		};

		AGE_HR_CHECK(g::p_main_device->CheckFeatureSupport(
			D3D12_FEATURE_ROOT_SIGNATURE,
			&feature,
			sizeof(feature)));
	}

	void
	deinit() noexcept
	{
		AGE_ASSERT(g::root_signature_ptr_vec.is_empty());
		for (auto* p_root_signature : g::root_signature_ptr_vec)
		{
			p_root_signature->Release();
		}

		if constexpr (age::config::debug_mode)
		{
			g::root_signature_ptr_vec.debug_validate();
		}

		g::root_signature_ptr_vec.clear();
	}

	void
	destroy(handle h) noexcept
	{
		g::root_signature_ptr_vec[h.id]->Release();
		g::root_signature_ptr_vec.remove(h.id);
	}
}	 // namespace age::graphics::root_signature