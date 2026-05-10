#pragma once
#include "age.hpp"

namespace age::graphics::root_signature
{
	FORCE_INLINE decltype(auto)
	handle::operator->() noexcept
	{
		return g::root_signature_ptr_vec[id];
	}

	FORCE_INLINE decltype(auto)
	handle::ptr() const noexcept
	{
		return g::root_signature_ptr_vec[id];
	}

	template <auto root_param_size, auto sampler_param_size>
	handle
	create(D3D12_ROOT_SIGNATURE_FLAGS										 flags,
		   const std::array<D3D12_ROOT_PARAMETER1, root_param_size>&		 root_param_arr,
		   const std::array<D3D12_STATIC_SAMPLER_DESC1, sampler_param_size>& sampler_arr) noexcept
	{
		auto root_signature_desc = D3D12_VERSIONED_ROOT_SIGNATURE_DESC{
			.Version  = D3D_ROOT_SIGNATURE_VERSION_1_2,
			.Desc_1_2 = {
				.NumParameters	   = static_cast<UINT>(root_param_arr.size()),
				.pParameters	   = root_param_arr.data(),
				.NumStaticSamplers = static_cast<UINT>(sampler_arr.size()),
				.pStaticSamplers   = sampler_arr.data(),
				.Flags			   = flags }
		};

		auto* p_serialized_root_signature = (ID3DBlob*)nullptr;
		{
			auto* p_error_blob = (ID3DBlob*)nullptr;

			auto hr = ::D3D12SerializeVersionedRootSignature(
				&root_signature_desc,
				&p_serialized_root_signature,
				&p_error_blob);

			if (p_error_blob is_not_nullptr)
			{
				std::println("{}", static_cast<const char*>(p_error_blob->GetBufferPointer()));
				p_error_blob->Release();
			}

			AGE_HR_CHECK(hr);
		}

		auto* p_signature = (ID3D12RootSignature*)nullptr;

		AGE_HR_CHECK(
			g::p_main_device->CreateRootSignature(
				0,
				p_serialized_root_signature->GetBufferPointer(),
				p_serialized_root_signature->GetBufferSize(),
				IID_PPV_ARGS(&p_signature)));

		p_serialized_root_signature->Release();

		return handle{ .id = g::root_signature_ptr_vec.emplace_back(p_signature) };
	}

	handle
	create(D3D12_ROOT_SIGNATURE_FLAGS flags, auto&&... root_parameter) noexcept
		requires(detail::cx_root_parameter<BARE_OF(root_parameter)> and ...)
	{
		static_assert((detail::cx_root_parameter<decltype(root_parameter)> and ...),
					  "invalid root parameter: expected constants/descriptor/descriptor_table");

		return root_signature::create(flags,
									  std::array<D3D12_ROOT_PARAMETER1, sizeof...(root_parameter)>{ root_parameter.build_d3d12_root_parameter()... },
									  std::array<D3D12_STATIC_SAMPLER_DESC1, 0>{});
	}
}	 // namespace age::graphics::root_signature